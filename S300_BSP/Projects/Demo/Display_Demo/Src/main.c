#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "s300.h"
#include "rcc.h"
#include "board.h"
#include "video.h"
#include "lvgl.h"
#include "uart.h"
#include "uart_s300.h"
/* Camera deps */
#include "gpio.h"
#include "i2c_soft.h"
#include "ov5640.h"
extern const lv_image_dsc_t img_demo; // demo image asset
extern const lv_image_dsc_t img_icons8_eye_16; // 16x16 eye icon used for both eyes

/* Animation: set y coordinate */
static void anim_set_y(void * obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t*)obj, v);
}

/* legacy oscillating animation (kept for reference) */
static __attribute__((unused)) void start_ud_anim(lv_obj_t * obj, int32_t base_y, int32_t amp, uint32_t t_ms, uint32_t delay_ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, base_y - amp, base_y + amp);
    lv_anim_set_time(&a, t_ms);
    lv_anim_set_playback_time(&a, t_ms);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_exec_cb(&a, anim_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* One-shot vertical animation: from -> to, no playback, no repeat */
static void start_ud_anim_one_shot(lv_obj_t * obj, int32_t from_y, int32_t to_y, uint32_t t_ms, uint32_t delay_ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from_y, to_y);
    lv_anim_set_time(&a, t_ms);
    lv_anim_set_playback_time(&a, 0);
    lv_anim_set_repeat_count(&a, 0);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_exec_cb(&a, anim_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* Retarget in-flight one-shot animation: cancel current and start towards new target from current Y */
static void retarget_ud_anim_one_shot(lv_obj_t * obj, int32_t to_y, uint32_t t_ms, uint32_t delay_ms)
{
    /* Delete any existing animation on Y for this object */
    lv_anim_delete(obj, anim_set_y);
    /* Start from the current displayed Y to ensure seamless retarget */
    int32_t from_y = lv_obj_get_y(obj);
    start_ud_anim_one_shot(obj, from_y, to_y, t_ms, delay_ms);
}

/* ---------------- Eyes globals and helpers ---------------- */
static lv_obj_t * g_eye_top = NULL;
static lv_obj_t * g_eye_bot = NULL;
static int32_t    g_eye_h = 16;
static int32_t    g_eye_spacing = 24;         /* distance between eye centers (vertical) */
static uint32_t   g_anim_time_ms = 2000;      /* default single-trip duration */

static inline void eyes_mid_limits(int32_t *min_out, int32_t *max_out)
{
    const int32_t screen_h = DISP_IMAGE_HEIGHT;
    const int32_t half = g_eye_h / 2;
    int32_t mid_min = g_eye_spacing + half;
    int32_t mid_max = screen_h - (g_eye_spacing + half);
    if (mid_min > mid_max) { mid_min = mid_max = screen_h / 2; }
    if (min_out) *min_out = mid_min;
    if (max_out) *max_out = mid_max;
}

static int32_t eyes_goto_mid_y(int32_t mid_y)
{
    if (!g_eye_top || !g_eye_bot) return mid_y;
    /* Clamp midpoint so that both eyes remain fully visible */
    const int32_t half = g_eye_h / 2;
    int32_t mid_min, mid_max;
    eyes_mid_limits(&mid_min, &mid_max);
    if (mid_y < mid_min) mid_y = mid_min;
    if (mid_y > mid_max) mid_y = mid_max;

    int32_t top_center_y = mid_y - g_eye_spacing;
    int32_t bot_center_y = mid_y + g_eye_spacing;
    int32_t to_y_top = top_center_y - half;  /* object top-left y */
    int32_t to_y_bot = bot_center_y - half;

    /* 动画时长：按像素位移计，每像素150ms */
    const uint32_t per_px_ms = 150u;
    int32_t cur_top_y = lv_obj_get_y(g_eye_top);
    int32_t cur_bot_y = lv_obj_get_y(g_eye_bot);
    uint32_t dy_top = (cur_top_y > to_y_top) ? (uint32_t)(cur_top_y - to_y_top) : (uint32_t)(to_y_top - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > to_y_bot) ? (uint32_t)(cur_bot_y - to_y_bot) : (uint32_t)(to_y_bot - cur_bot_y);
    uint32_t t_top_ms = dy_top * per_px_ms;
    uint32_t t_bot_ms = dy_bot * per_px_ms;

    if (t_top_ms == 0u) {
        /* 无位移，直接设置Y */
        lv_obj_set_y(g_eye_top, to_y_top);
    } else {
        retarget_ud_anim_one_shot(g_eye_top, to_y_top, t_top_ms, 0);
    }
    if (t_bot_ms == 0u) {
        lv_obj_set_y(g_eye_bot, to_y_bot);
    } else {
        retarget_ud_anim_one_shot(g_eye_bot, to_y_bot, t_bot_ms, 0);
    }

    return mid_y;
}

// 1ms 节拍计时
static volatile uint32_t g_tick_ms = 0;

void SysTick_Handler(void)
{
    g_tick_ms++;
    /* feed LVGL tick (1ms) */
    lv_tick_inc(1);
}

static inline uint32_t millis(void)
{
    return g_tick_ms;
}

/* ---------------- UART echo (RX polling on debug UART) ---------------- */
#ifndef UART_DEBUG_IDX
#define UART_DEBUG_IDX BOARD_UART_DEBUG_IDX
#endif

static inline S300_UART_TypeDef * dbg_uart_dev(void)
{
    switch (UART_DEBUG_IDX)
    {
    case 0: return UART0;
    case 1: return UART1;
    case 2: return UART2;
    default: return UART3;
    }
}

static void uart_echo_poll(void)
{
    S300_UART_TypeDef *U = dbg_uart_dev();
    /* Simple line buffer for commands */
    static char     s_buf[64];
    static uint8_t  s_len = 0;
    /* USR bit3: RFNE (RX FIFO Not Empty). Drain all pending bytes and echo back. */
    while (U->USR & 0x8u)
    {
        uint8_t ch = (uint8_t)U->RBR_THR_DLL; /* read one byte */
        /* echo */
        write_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, ch);
        /* accumulate */
        if (ch == '\r' || ch == '\n')
        {
            if (s_len > 0)
            {
                s_buf[s_len] = '\0';
                /* parse command */
                const char *p = s_buf;
                /* skip leading spaces */
                while (*p == ' ' || *p == '\t') p++;
                if ((p[0]=='g'||p[0]=='G') && (p[1]=='o'||p[1]=='O') && (p[2]=='t'||p[2]=='T') && (p[3]=='o'||p[3]=='O'))
                {
                    p += 4;
                    while (*p == ' ' || *p == '\t') p++;
                    int32_t neg = 0, val = 0, got = 0;
                    if (*p == '+') { p++; }
                    else if (*p == '-') { neg = 1; p++; }
                    while (*p >= '0' && *p <= '9') { val = val*10 + (*p - '0'); p++; got = 1; }
                    if (got)
                    {
                        if (neg) val = -val;
                        int32_t min_mid, max_mid; eyes_mid_limits(&min_mid, &max_mid);
                        int32_t eff = eyes_goto_mid_y(val);
               int32_t half = g_eye_h / 2;
               int32_t top_y = (eff - g_eye_spacing) - half;
               int32_t bot_y = (eff + g_eye_spacing) - half;
               /* 计算持续时间用于打印（与eyes_goto_mid_y逻辑一致）：每像素150ms */
               const uint32_t per_px_ms = 150u;
               int32_t cur_top_y = lv_obj_get_y(g_eye_top);
               int32_t cur_bot_y = lv_obj_get_y(g_eye_bot);
               uint32_t dy_top = (cur_top_y > top_y) ? (uint32_t)(cur_top_y - top_y) : (uint32_t)(top_y - cur_top_y);
               uint32_t dy_bot = (cur_bot_y > bot_y) ? (uint32_t)(cur_bot_y - bot_y) : (uint32_t)(bot_y - cur_bot_y);
               uint32_t t_top_ms = dy_top * per_px_ms;
               uint32_t t_bot_ms = dy_bot * per_px_ms;
               printf("[S300][CMD] goto %ld => mid=%ld (range %ld..%ld), top_y=%ld (%lums), bot_y=%ld (%lums)\r\n",
                   (long)val, (long)eff, (long)min_mid, (long)max_mid, (long)top_y, (unsigned long)t_top_ms, (long)bot_y, (unsigned long)t_bot_ms);
                    }
                    else
                    {
                        printf("[S300][CMD] usage: goto <y_mid>\r\n");
                    }
                }
                s_len = 0; /* reset buffer */
            }
            /* CR->LF for nicer terminals */
            if (ch == '\r') write_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, '\n');
        }
        else if (ch == 0x08 || ch == 0x7F)
        {
            /* handle backspace */
            if (s_len > 0) s_len--;
        }
            else if ((size_t)s_len + 1U < sizeof(s_buf))
        {
            s_buf[s_len++] = (char)ch;
        }
        else
        {
            /* overflow: reset */
            s_len = 0;
        }
    }
}

static void fill_buffer(volatile uint16_t *frame,
                        volatile uint8_t *alpha,
                        size_t pixel_count,
                        uint16_t color,
                        uint8_t alpha_value)
{
    for (size_t i = 0; i < pixel_count; ++i)
    {
        frame[i] = color;
        alpha[i] = alpha_value;
    }
}

/* ---------------- LVGL display binding ---------------- */
static volatile uint16_t* s_f0;
static volatile uint16_t* s_f1;
static volatile uint8_t*  s_a0;
static volatile uint8_t*  s_a1;
static const uint32_t REG_F0 = (DSP_VIDEO_SS_BASE + 0x50u);
static const uint32_t REG_F1 = (DSP_VIDEO_SS_BASE + 0x54u);

static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    /* FULL frame double-buffering: px_map points to the active draw buffer (s_f0 or s_f1) */
    (void)area;

    /* Present exactly one buffer as ready */
    REG32(REG_F0) = 0u;
    REG32(REG_F1) = 0u;

    uintptr_t p = (uintptr_t)px_map;
    if (p == (uintptr_t)s_f0)
    {
        REG32(REG_F0) = 1u;
    }
    else if (p == (uintptr_t)s_f1)
    {
        REG32(REG_F1) = 1u;
    }
    else
    {
        /* Unexpected pointer: as a fallback copy to f0 and present */
        const int32_t w = area->x2 - area->x1 + 1;
        const int32_t h = area->y2 - area->y1 + 1;
        for (int32_t y = 0; y < h; ++y)
        {
            memcpy((void*)&s_f0[(area->y1 + y) * DISP_IMAGE_WIDTH + area->x1],
                   (const void*)&((const uint16_t*)px_map)[y * w],
                   (size_t)w * sizeof(uint16_t));
        }
        REG32(REG_F0) = 1u;
    }

    lv_display_flush_ready(disp);
}

/* ---------------- OV5640 camera bring-up (before video init) ---------------- */
/* Assume board wiring: OV5640 reset/powerdown pins on GPIOA15 and GPIOA6 respectively. */
#ifndef CAM_RST_PIN
#define CAM_RST_PIN  15u  /* GPIOA15 */
#endif
#ifndef CAM_PWDN_PIN
#define CAM_PWDN_PIN 6u   /* GPIOA6  */
#endif

static void cam_gpio_init(void)
{
    /* Enable GPIO clock and configure pins as output, pull-up */
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    gpio_set_function(GPIOA, CAM_RST_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_RST_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_RST_PIN, 1);
    gpio_set_function(GPIOA, CAM_PWDN_PIN, FUNCTION_2);
    gpio_set_mode(GPIOA, CAM_PWDN_PIN, GPIO_UP);
    gpio_set_direction(GPIOA, CAM_PWDN_PIN, 1);
}

static void cam_power_on_sequence(void)
{
    /* PWDN high, RST low -> delay -> PWDN low -> delay -> RST high -> delay */
    gpio_set_data(GPIOA, CAM_RST_PIN, 0);
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 1);
    for (volatile uint32_t i = 0; i < 800000u; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_PWDN_PIN, 0);
    for (volatile uint32_t i = 0; i < 800000u; i++) __asm volatile("nop");
    gpio_set_data(GPIOA, CAM_RST_PIN, 1);
    for (volatile uint32_t i = 0; i < 2400000u; i++) __asm volatile("nop");
}

/* Initialize OV5640 via software I2C (I2C1: GPIOA0/A1) to YUYV 720p */
static int ov5640_preinit(void)
{
    i2c_soft_t i2c1;
    /* Moderate bus speed for robustness */
    int ret = i2c_soft_init_default_idx(&i2c1, 1, 50000);
    if (ret) {
        printf("[S300][DisplayDemo][CAM] i2c init fail %d\r\n", ret);
        return ret;
    }
    cam_gpio_init();
    cam_power_on_sequence();
    (void)i2c_soft_bus_recover(&i2c1);

    /* Probe slave address 0x3C/0x3D */
    uint8_t saddr = 0x3C;
    int p3c = i2c_soft_probe(&i2c1, 0x3C);
    int p3d = i2c_soft_probe(&i2c1, 0x3D);
    if (p3c != 0 && p3d == 0) saddr = 0x3D;

    /* Read chip ID for log */
    uint8_t idh = 0, idl = 0;
    (void)i2c_soft_mem_read(&i2c1, saddr, 0x300Au, true, &idh, 1);
    (void)i2c_soft_mem_read(&i2c1, saddr, 0x300Bu, true, &idl, 1);
    printf("[S300][DisplayDemo][CAM] OV5640 ID: 0x%02X 0x%02X (addr=0x%02X)\r\n", idh, idl, saddr);
    int lr = ov5640_set_light(&i2c1, saddr, true);
    printf("Enable light: %s\n", lr == 0 ? "OK" : "FAIL");
    /* 简短预览一段时间后自动关闭，避免常亮 */
    for (volatile uint32_t i = 0; i < 4800000u; ++i) __asm volatile("nop");
    int lf = ov5640_set_light(&i2c1, saddr, false);
    printf("Disable light: %s\n", lf == 0 ? "OK" : "FAIL");
    ret = ov5640_init(&i2c1, saddr, OV5640_FMT_YUV422_YUYV);
    printf("[S300][DisplayDemo][CAM] ov5640_init ret=%d\r\n", ret);
    return ret;
}

int main(void)
{
    // 板级初始化：时钟 + UART3，printf 可用
    board_init();
    printf("\r\n[S300][DisplayDemo] Booting...\r\n");

    // 启动 SysTick 为 1ms 节拍
    SystemCoreClockUpdate();
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
    {
        printf("[S300][DisplayDemo][ERR] SysTick_Config failed!\r\n");
    }

    rcc_init_mm_pll(8, 400, 0, 3, 2);

    // 在初始化视频前先初始化 OV5640（DVP 摄像头经软 I2C 配置到 YUYV）
    int cam_ret = ov5640_preinit();
    if (cam_ret != 0) {
        printf("[S300][DisplayDemo][WARN] OV5640 init failed (%d), continue to init video for display path only.\r\n", cam_ret);
    }

    // 初始化视频子系统（包含 ST77 SPI LCD 序列）
    printf("[S300][DisplayDemo] init video...\r\n");
    init_video(EM_DVP, CAMREA_YUV422, C1080X720P);

    volatile uint16_t* f0 = (volatile uint16_t*)DISP_RFRAME0_ADDR;
    volatile uint16_t* f1 = (volatile uint16_t*)DISP_RFRAME1_ADDR;
    volatile uint8_t*  a0 = (volatile uint8_t*)DISP_RALPHA0_ADDR;
    volatile uint8_t*  a1 = (volatile uint8_t*)DISP_RALPHA1_ADDR;

    /* Cache to globals for flush callback */
    s_f0 = f0; s_f1 = f1; s_a0 = a0; s_a1 = a1;

    printf("[S300][DisplayDemo] frame0=%p frame1=%p alpha0=%p alpha1=%p\r\n", (void*)f0, (void*)f1, (void*)a0, (void*)a1);

    const size_t pixels = (size_t)DISP_IMAGE_WIDTH * (size_t)DISP_IMAGE_HEIGHT;

    /* Prepare initial frame buffers */
    fill_buffer(f0, a0, pixels, 0x0000u, 0xFFu); // black
    fill_buffer(f1, a1, pixels, 0x0000u, 0xFFu); // black

    /* Stop presenting during init */
    REG32(REG_F0) = 0u;
    REG32(REG_F1) = 0u;

    /* ---------------- LVGL init ---------------- */
    lv_init();

    lv_display_t * disp = lv_display_create(DISP_IMAGE_WIDTH, DISP_IMAGE_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    /* Provide full-frame double buffers mapped to HW frame buffers */
    lv_display_set_buffers(disp,
                           (void*)f0,
                           (void*)f1,
                           (uint32_t)(DISP_IMAGE_WIDTH * DISP_IMAGE_HEIGHT * sizeof(uint16_t)),
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    // /* Simple UI: Title */
    // lv_obj_t * label = lv_label_create(lv_screen_active());
    // lv_label_set_text(label, "LVGL Image Demo");
    // lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 4);

    /* Two eyes after 90° rotation: symmetric around horizontal center line, vertical movement */
    const int32_t screen_w = DISP_IMAGE_WIDTH;
    const int32_t screen_h = DISP_IMAGE_HEIGHT;
    const int32_t eye_w = 16;
    const int32_t eye_h = 16;
    const int32_t center_x = screen_w / 2;
    const int32_t center_y = screen_h / 2;
    const int32_t spacing = 24;      /* vertical distance between eye centers */

    /* Common base X: center horizontally */
    const int32_t base_x = center_x - eye_w / 2;

    /* Top eye */
    lv_obj_t * eye_top = lv_image_create(lv_screen_active());
    lv_image_set_src(eye_top, &img_icons8_eye_16);
    /* rotate clockwise 90 deg around center */
    lv_image_set_pivot(eye_top, eye_w / 2, eye_h / 2);
    lv_image_set_rotation(eye_top, 900);
    int32_t base_y_top = center_y - spacing - eye_h / 2;
    lv_obj_set_pos(eye_top, base_x, base_y_top);

    /* Bottom eye */
    lv_obj_t * eye_bot = lv_image_create(lv_screen_active());
    lv_image_set_src(eye_bot, &img_icons8_eye_16);
    /* rotate clockwise 90 deg around center */
    lv_image_set_pivot(eye_bot, eye_w / 2, eye_h / 2);
    lv_image_set_rotation(eye_bot, 900);
    int32_t base_y_bot = center_y + spacing - eye_h / 2;
    lv_obj_set_pos(eye_bot, base_x, base_y_bot);

    /* Keep initial positions symmetric; avoid overlapping at center line. */
    const uint32_t t_ms = 2000;      /* default single trip duration for future retargets */

    /* export eyes and parameters for runtime control */
    g_eye_top = eye_top;
    g_eye_bot = eye_bot;
    g_eye_h = eye_h;
    g_eye_spacing = spacing;
    g_anim_time_ms = t_ms;

    printf("[S300][DisplayDemo] LVGL started.\r\n");
    printf("[S300][DisplayDemo] UART echo enabled on debug UART (CR->CRLF).\r\n");
    printf("[S300][DisplayDemo] Command: goto <y_mid>  (move eyes midpoint vertically)\r\n");

    /* Main loop: run LVGL timers */
    while (1)
    {
        /* UART echo (non-blocking) */
        uart_echo_poll();
        lv_timer_handler();
        /* tiny sleep ~5ms to reduce busy loop */
        uint32_t t0 = millis();
        while ((uint32_t)(millis() - t0) < 5u) { /* spin */ }
    }
}
