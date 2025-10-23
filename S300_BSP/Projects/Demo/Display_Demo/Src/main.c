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
#include "mailbox.h"
extern const lv_image_dsc_t img_demo; // demo image asset
extern const lv_image_dsc_t img_yanqiu; // 30x38 eye image asset
extern const lv_image_dsc_t img_yankuang; // 74x85 eye socket background

/* --- Tuning knobs --- */
#ifndef EYE_PER_PX_MS
#define EYE_PER_PX_MS 30u   /* per-pixel animation time (ms) for both X/Y axes */
#endif
#ifndef EYE_SMOOTH_NUM
#define EYE_SMOOTH_NUM 1    /* EMA numerator (alpha = NUM/DEN) */
#endif
#ifndef EYE_SMOOTH_DEN
#define EYE_SMOOTH_DEN 2    /* EMA denominator; 1/2 => 0.5 smoothing */
#endif
/* Minimal movement to trigger a new animation (in mid coords, pixels) */
#ifndef EYE_MIN_MOVE_PX
#define EYE_MIN_MOVE_PX 2
#endif
/* Throttle how often we print movement logs */
#ifndef MOVE_LOG_MIN_INTERVAL_MS
#define MOVE_LOG_MIN_INTERVAL_MS 120u
#endif
/* Throttle how often we print invalid-face logs */
#ifndef INVALID_LOG_INTERVAL_MS
#define INVALID_LOG_INTERVAL_MS 300u
#endif
/* Timeout to return to neutral when no valid face is detected */
#ifndef FACE_LOST_TIMEOUT_MS
#define FACE_LOST_TIMEOUT_MS 2000u
#endif
/* Throttle IDLE return logs */
#ifndef IDLE_RETURN_LOG_MIN_INTERVAL_MS
#define IDLE_RETURN_LOG_MIN_INTERVAL_MS 1000u
#endif

/* Animation: set y coordinate */
static void anim_set_y(void * obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t*)obj, v);
}

/* Animation: set x coordinate */
static void anim_set_x(void * obj, int32_t v)
{
    lv_obj_set_x((lv_obj_t*)obj, v);
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

/* Retarget one-shot animation on X axis */
static void retarget_lr_anim_one_shot(lv_obj_t * obj, int32_t to_x, uint32_t t_ms, uint32_t delay_ms)
{
    lv_anim_delete(obj, anim_set_x);
    int32_t from_x = lv_obj_get_x(obj);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from_x, to_x);
    lv_anim_set_time(&a, t_ms);
    lv_anim_set_playback_time(&a, 0);
    lv_anim_set_repeat_count(&a, 0);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_exec_cb(&a, anim_set_x);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* ---------------- Eyes globals and helpers ---------------- */
static lv_obj_t * g_eye_top = NULL;
static lv_obj_t * g_eye_bot = NULL;
static int32_t    g_eye_h = 16;
static int32_t    g_eye_spacing = 24;         /* distance between eye centers (vertical) */
static uint32_t   g_anim_time_ms = 2000;      /* default single-trip duration */
/* Visible heights after 90° rotation: eye -> eye_w, socket -> bg_w */
static int32_t    g_eye_vis_h = 16;           /* rotated bounding-box height of eye */
static int32_t    g_socket_vis_h = 16;        /* rotated bounding-box height of socket */
/* Visible widths after 90° rotation: eye -> eye_h, socket -> bg_h */
static int32_t    g_eye_vis_w = 16;           /* rotated bounding-box width of eye  */
static int32_t    g_socket_vis_w = 16;        /* rotated bounding-box width of socket */

static inline void eyes_mid_limits(int32_t *min_out, int32_t *max_out)
{
        /* Constrain mid_y so that each eye stays inside its socket (rotated 90°).
             Socket centers are fixed at center_y ± g_eye_spacing. If socket visible height is S
             and eye visible height is E, then allowed mid_y range is:
                 mid ∈ [center_y - (S-E)/2, center_y + (S-E)/2]
             Additionally clamp to screen to be safe. */
        const int32_t screen_h = DISP_IMAGE_HEIGHT;
        const int32_t center_y = screen_h / 2;
        int32_t S = g_socket_vis_h; /* socket visible height after rotation */
        int32_t E = g_eye_vis_h;    /* eye visible height after rotation    */
        if (S < E) S = E; /* if socket smaller than eye, fall back to eye height */
        int32_t margin = (S - E) / 2; /* how far mid can deviate from center */
        int32_t mid_min = center_y - margin;
        int32_t mid_max = center_y + margin;
        /* Screen safety clamp (eye must remain fully on-screen as well) */
        int32_t half_eye = g_eye_h / 2; /* use object pivot-based placement */
        int32_t scr_min = g_eye_spacing + half_eye;
        int32_t scr_max = screen_h - (g_eye_spacing + half_eye);
        if (mid_min < scr_min) mid_min = scr_min;
        if (mid_max > scr_max) mid_max = scr_max;
        if (mid_min > mid_max) { mid_min = mid_max = center_y; }
        if (min_out) *min_out = mid_min;
        if (max_out) *max_out = mid_max;
}

/* Horizontal limits for eye mid X (shared by both eyes, socket centers at center_x) */
static inline void eyes_mid_limits_x(int32_t *min_out, int32_t *max_out)
{
    const int32_t screen_w = DISP_IMAGE_WIDTH;
    const int32_t center_x = screen_w / 2;
    int32_t S = g_socket_vis_w; /* socket visible width after rotation */
    int32_t E = g_eye_vis_w;    /* eye visible width after rotation    */
    if (S < E) S = E;
    int32_t margin = (S - E) / 2; /* how far mid can deviate from center */
    int32_t mid_min = center_x - margin;
    int32_t mid_max = center_x + margin;
    /* Screen safety clamp */
    int32_t half_eye_w = g_eye_vis_w / 2;
    int32_t scr_min = half_eye_w;
    int32_t scr_max = screen_w - half_eye_w;
    if (mid_min < scr_min) mid_min = scr_min;
    if (mid_max > scr_max) mid_max = scr_max;
    if (mid_min > mid_max) { mid_min = mid_max = center_x; }
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

    /* 动画时长：按像素位移计，可调 */
    const uint32_t per_px_ms = EYE_PER_PX_MS;
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

/* Move both eyes to target mid point (mid_x, mid_y) with clamping and XY animation */
static void eyes_goto_mid_xy(int32_t mid_x, int32_t mid_y)
{
    if (!g_eye_top || !g_eye_bot) return;

    /* Clamp mid points */
    int32_t mid_min_y, mid_max_y; eyes_mid_limits(&mid_min_y, &mid_max_y);
    if (mid_y < mid_min_y) { mid_y = mid_min_y; }
    if (mid_y > mid_max_y) { mid_y = mid_max_y; }
    int32_t mid_min_x, mid_max_x; eyes_mid_limits_x(&mid_min_x, &mid_max_x);
    if (mid_x < mid_min_x) { mid_x = mid_min_x; }
    if (mid_x > mid_max_x) { mid_x = mid_max_x; }

    const int32_t half_h = g_eye_h / 2;
    const int32_t half_w = g_eye_vis_w / 2; /* object width after rotation */

    int32_t top_center_y = mid_y - g_eye_spacing;
    int32_t bot_center_y = mid_y + g_eye_spacing;
    int32_t to_y_top = top_center_y - half_h;  /* object top-left y */
    int32_t to_y_bot = bot_center_y - half_h;
    int32_t to_x = mid_x - half_w;             /* object top-left x (both eyes share the same x) */

    const uint32_t per_px_ms = EYE_PER_PX_MS;
    /* current positions */
    int32_t cur_top_y = lv_obj_get_y(g_eye_top);
    int32_t cur_bot_y = lv_obj_get_y(g_eye_bot);
    int32_t cur_top_x = lv_obj_get_x(g_eye_top);
    int32_t cur_bot_x = lv_obj_get_x(g_eye_bot);

    uint32_t dy_top = (cur_top_y > to_y_top) ? (uint32_t)(cur_top_y - to_y_top) : (uint32_t)(to_y_top - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > to_y_bot) ? (uint32_t)(cur_bot_y - to_y_bot) : (uint32_t)(to_y_bot - cur_bot_y);
    uint32_t dx_top = (cur_top_x > to_x) ? (uint32_t)(cur_top_x - to_x) : (uint32_t)(to_x - cur_top_x);
    uint32_t dx_bot = (cur_bot_x > to_x) ? (uint32_t)(cur_bot_x - to_x) : (uint32_t)(to_x - cur_bot_x);

    uint32_t t_top_y = dy_top * per_px_ms;
    uint32_t t_bot_y = dy_bot * per_px_ms;
    uint32_t t_top_x = dx_top * per_px_ms;
    uint32_t t_bot_x = dx_bot * per_px_ms;

    /* Animate Y */
    if (t_top_y == 0u) lv_obj_set_y(g_eye_top, to_y_top); else retarget_ud_anim_one_shot(g_eye_top, to_y_top, t_top_y, 0);
    if (t_bot_y == 0u) lv_obj_set_y(g_eye_bot, to_y_bot); else retarget_ud_anim_one_shot(g_eye_bot, to_y_bot, t_bot_y, 0);
    /* Animate X */
    if (t_top_x == 0u) lv_obj_set_x(g_eye_top, to_x); else retarget_lr_anim_one_shot(g_eye_top, to_x, t_top_x, 0);
    if (t_bot_x == 0u) lv_obj_set_x(g_eye_bot, to_x); else retarget_lr_anim_one_shot(g_eye_bot, to_x, t_bot_x, 0);
}

// 1ms 节拍计时
static volatile uint32_t g_tick_ms = 0;

/* 将“移动到指定中点并打印详情”的逻辑封装为接口，供串口与邮箱两处共用 */
static void eyes_move_to_mid_and_log(int32_t target_mid_y, const char *src_tag)
{
    int32_t min_mid, max_mid;
    eyes_mid_limits(&min_mid, &max_mid);

    /* 执行动画重定向，返回实际生效的中点（被范围裁剪后） */
    int32_t eff = eyes_goto_mid_y(target_mid_y);

    /* 计算目标 top/bot Y 与动画估计时长（与 eyes_goto_mid_y 逻辑保持一致） */
    int32_t half = g_eye_h / 2;
    int32_t top_y = (eff - g_eye_spacing) - half;
    int32_t bot_y = (eff + g_eye_spacing) - half;
    const uint32_t per_px_ms = EYE_PER_PX_MS;
    int32_t cur_top_y = g_eye_top ? lv_obj_get_y(g_eye_top) : top_y;
    int32_t cur_bot_y = g_eye_bot ? lv_obj_get_y(g_eye_bot) : bot_y;
    uint32_t dy_top = (cur_top_y > top_y) ? (uint32_t)(cur_top_y - top_y) : (uint32_t)(top_y - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > bot_y) ? (uint32_t)(cur_bot_y - bot_y) : (uint32_t)(bot_y - cur_bot_y);
    uint32_t t_top_ms = dy_top * per_px_ms;
    uint32_t t_bot_ms = dy_bot * per_px_ms;

    printf("[S300][MOVE:%s] goto %ld => mid=%ld (range %ld..%ld), top_y=%ld (%lums), bot_y=%ld (%lums)\r\n",
           (src_tag ? src_tag : "?"),
           (long)target_mid_y, (long)eff, (long)min_mid, (long)max_mid,
           (long)top_y, (unsigned long)t_top_ms, (long)bot_y, (unsigned long)t_bot_ms);
}

/* New helper: move to (mid_x, mid_y) and log */
static void eyes_move_to_xy_and_log(int32_t target_mid_x, int32_t target_mid_y, const char *src_tag)
{
    int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
    int32_t min_y, max_y; eyes_mid_limits(&min_y, &max_y);

    /* Before move, fetch current for time estimation */
    int32_t half_h = g_eye_h / 2;
    int32_t half_w = g_eye_vis_w / 2;
    int32_t top_to_y = (target_mid_y - g_eye_spacing) - half_h;
    int32_t bot_to_y = (target_mid_y + g_eye_spacing) - half_h;
    int32_t to_x = target_mid_x - half_w;
    const uint32_t per_px_ms = EYE_PER_PX_MS;
    int32_t cur_top_y = g_eye_top ? lv_obj_get_y(g_eye_top) : top_to_y;
    int32_t cur_bot_y = g_eye_bot ? lv_obj_get_y(g_eye_bot) : bot_to_y;
    int32_t cur_top_x = g_eye_top ? lv_obj_get_x(g_eye_top) : to_x;
    int32_t cur_bot_x = g_eye_bot ? lv_obj_get_x(g_eye_bot) : to_x;
    uint32_t dy_top = (cur_top_y > top_to_y) ? (uint32_t)(cur_top_y - top_to_y) : (uint32_t)(top_to_y - cur_top_y);
    uint32_t dy_bot = (cur_bot_y > bot_to_y) ? (uint32_t)(cur_bot_y - bot_to_y) : (uint32_t)(bot_to_y - cur_bot_y);
    uint32_t dx_top = (cur_top_x > to_x) ? (uint32_t)(cur_top_x - to_x) : (uint32_t)(to_x - cur_top_x);
    uint32_t dx_bot = (cur_bot_x > to_x) ? (uint32_t)(cur_bot_x - to_x) : (uint32_t)(to_x - cur_bot_x);
    /* Independent axis animations; estimate per-eye by taking max of axis durations */
    uint32_t t_top_y = dy_top * per_px_ms;
    uint32_t t_top_x = dx_top * per_px_ms;
    uint32_t t_bot_y = dy_bot * per_px_ms;
    uint32_t t_bot_x = dx_bot * per_px_ms;
    uint32_t t_top_ms = (t_top_y > t_top_x) ? t_top_y : t_top_x;
    uint32_t t_bot_ms = (t_bot_y > t_bot_x) ? t_bot_y : t_bot_x;

    eyes_goto_mid_xy(target_mid_x, target_mid_y);

    printf("[S300][MOVE:%s] goto (%ld,%ld) midX[%ld..%ld] midY[%ld..%ld], est top=%lums bot=%lums\r\n",
           (src_tag ? src_tag : "?"), (long)target_mid_x, (long)target_mid_y,
           (long)min_x, (long)max_x, (long)min_y, (long)max_y,
           (unsigned long)t_top_ms, (unsigned long)t_bot_ms);
}

/* FaceRect from DSP shared memory (base + offset) */
typedef struct FaceRect_ {
    float score;
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;
    float lm[10];
} FaceRect;

#ifndef DSP_FACE_BASE_ADDR
#define DSP_FACE_BASE_ADDR 0x44800000u
#endif

/* Face coordinates space (can be overridden if DSP uses different space, e.g., 1280x720) */
#ifndef FACE_COORD_SPACE_W
#define FACE_COORD_SPACE_W DISP_IMAGE_WIDTH
#endif
#ifndef FACE_COORD_SPACE_H
#define FACE_COORD_SPACE_H DISP_IMAGE_HEIGHT
#endif

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

/* Count display flushes for FPS measurement */
static volatile uint32_t g_flush_cnt = 0;

/* FPS timer callback: update label text based on flush count */
static void fps_timer_cb(lv_timer_t *t)
{
    lv_obj_t *label = (lv_obj_t*)lv_timer_get_user_data(t);
    static uint32_t last_ms = 0;
    static uint32_t last_cnt = 0;
    uint32_t now = millis();
    if (last_ms == 0) { last_ms = now; last_cnt = g_flush_cnt; return; }
    uint32_t dt = now - last_ms;
    if (dt < 250) return; /* update not too often */
    uint32_t dc = g_flush_cnt - last_cnt;
    /* Compute FPS with one decimal using integer math */
    uint32_t fps10 = (dt > 0) ? (uint32_t)((dc * 10000u + (dt/2)) / dt) : 0u; /* x10 */
    uint32_t ip = fps10 / 10u;
    uint32_t fp = fps10 % 10u;
    char buf[24];
    snprintf(buf, sizeof(buf), "FPS: %lu.%lu", (unsigned long)ip, (unsigned long)fp);
    lv_label_set_text(label, buf);
    last_ms = now;
    last_cnt = g_flush_cnt;
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
                        eyes_move_to_mid_and_log(val, "UART");
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

    // /* Present exactly one buffer as ready */
    // REG32(REG_F0) = 0u;
    // REG32(REG_F1) = 0u;

    uintptr_t p = (uintptr_t)px_map;
    if (p == (uintptr_t)s_f0)
    {
        REG32(REG_F0) = 1u;
        while ((REG32(REG_F0) & 0x1u) != 0u) { /* wait until accepted */ }
    }
    else if (p == (uintptr_t)s_f1)
    {
        REG32(REG_F1) = 1u;
        while ((REG32(REG_F1) & 0x1u) != 0u) { /* wait until accepted */ }
    }
    // else
    // {
    //     /* Unexpected pointer: as a fallback copy to f0 and present */
    //     const int32_t w = area->x2 - area->x1 + 1;
    //     const int32_t h = area->y2 - area->y1 + 1;
    //     for (int32_t y = 0; y < h; ++y)
    //     {
    //         memcpy((void*)&s_f0[(area->y1 + y) * DISP_IMAGE_WIDTH + area->x1],
    //                (const void*)&((const uint16_t*)px_map)[y * w],
    //                (size_t)w * sizeof(uint16_t));
    //     }
    //     REG32(REG_F0) = 1u;
    // }

    /* one frame flushed */
    g_flush_cnt++;
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


static void monitor_mailbox_rx(void)
{
    /* 若 DSP->M4 有数据（即 CM4_MAILBOX_BASE 非空），读出并打印 */
    static uint32_t s_last_valid_face_ms = 0;  /* 上次收到有效人脸的时间戳 */
    static uint32_t s_last_idle_log_ms   = 0;  /* 空闲回中打印的节流 */
    static int      s_idle_active        = 0;  /* 已处于“空闲回中”状态 */
    while (mailbox_sta_empty_flag_is(MAILBOX_BASE, 0) == 0)
    {
        uint32_t offset = read_mailbox(MAILBOX_BASE);
        uintptr_t addr = (uintptr_t)DSP_FACE_BASE_ADDR + (uintptr_t)offset;
        const FaceRect *fr = (const FaceRect*)addr;

        /* 读取人脸矩形中心，做边界检查与过滤无效数据 */
        int32_t x1 = fr->x1, y1 = fr->y1, x2 = fr->x2, y2 = fr->y2;
        if (x2 < x1) { int32_t t = x1; x1 = x2; x2 = t; }
        if (y2 < y1) { int32_t t = y1; y1 = y2; y2 = t; }

        bool valid = true;
        if (x1 < 0 || y1 < 0 || x2 > FACE_COORD_SPACE_W || y2 > FACE_COORD_SPACE_H) valid = false;
        if ((x2 - x1) <= 2 || (y2 - y1) <= 2) valid = false; /* 太小或无面积的框忽略 */
        /* 可选：score 阈值过滤
         * if (!(fr->score >= 0.0f && fr->score <= 1.0f) || fr->score < 0.10f) valid = false; */

        static uint32_t s_last_invalid_log_ms = 0;
        uint32_t now_ms = millis();
        if (!valid)
        {
            if ((uint32_t)(now_ms - s_last_invalid_log_ms) >= INVALID_LOG_INTERVAL_MS)
            {
                printf("RX[M4]: off=0x%08lx addr=%p invalid face=(%ld,%ld)-(%ld,%ld) skip\r\n",
                       (unsigned long)offset, (void*)addr,
                       (long)x1, (long)y1, (long)x2, (long)y2);
                s_last_invalid_log_ms = now_ms;
            }
            continue;
        }

        int32_t cx_raw = x1 + (x2 - x1) / 2;
        int32_t cy_raw = y1 + (y2 - y1) / 2;

        /* 将原始坐标空间 (FACE_COORD_SPACE_W/H) 等比例映射到眼球中点允许范围 */
        int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
        int32_t min_y, max_y; eyes_mid_limits(&min_y, &max_y);
        if (cx_raw < 0) { cx_raw = 0; }
        if (cx_raw > FACE_COORD_SPACE_W) { cx_raw = FACE_COORD_SPACE_W; }
        if (cy_raw < 0) { cy_raw = 0; }
        if (cy_raw > FACE_COORD_SPACE_H) { cy_raw = FACE_COORD_SPACE_H; }
        int32_t span_x = (max_x >= min_x) ? (max_x - min_x) : 0;
        int32_t span_y = (max_y >= min_y) ? (max_y - min_y) : 0;
        int32_t mid_x = min_x;
        int32_t mid_y = min_y;
        if (FACE_COORD_SPACE_W > 0 && span_x > 0)
            mid_x = min_x + (int32_t)(((int64_t)cx_raw * (int64_t)span_x) / (int64_t)FACE_COORD_SPACE_W);
        if (FACE_COORD_SPACE_H > 0 && span_y > 0)
            mid_y = min_y + (int32_t)(((int64_t)cy_raw * (int64_t)span_y) / (int64_t)FACE_COORD_SPACE_H);

        /* 简单EMA平滑，减少抖动 */
        static int s_ema_inited = 0;
        static int32_t s_mid_x = 0, s_mid_y = 0;
        if (!s_ema_inited)
        {
            s_mid_x = mid_x; s_mid_y = mid_y; s_ema_inited = 1;
        }
        else
        {
            s_mid_x = (int32_t)(((int64_t)s_mid_x * (EYE_SMOOTH_DEN - EYE_SMOOTH_NUM) + (int64_t)mid_x * EYE_SMOOTH_NUM) / EYE_SMOOTH_DEN);
            s_mid_y = (int32_t)(((int64_t)s_mid_y * (EYE_SMOOTH_DEN - EYE_SMOOTH_NUM) + (int64_t)mid_y * EYE_SMOOTH_NUM) / EYE_SMOOTH_DEN);
        }

        /* 仅在必要时移动与打印：小于阈值的微小变化不触发 */
        static int      s_have_last_cmd = 0;
        static int32_t  s_last_cmd_mid_x = 0;
        static int32_t  s_last_cmd_mid_y = 0;
        static uint32_t s_last_move_log_ms = 0;

        int32_t dx = s_have_last_cmd ? (s_mid_x - s_last_cmd_mid_x) : EYE_MIN_MOVE_PX;
        int32_t dy = s_have_last_cmd ? (s_mid_y - s_last_cmd_mid_y) : EYE_MIN_MOVE_PX;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;

        if (dx < EYE_MIN_MOVE_PX && dy < EYE_MIN_MOVE_PX)
        {
            /* 抑制抖动：不移动、不打印 */
            continue;
        }

        bool do_log = ((uint32_t)(now_ms - s_last_move_log_ms) >= MOVE_LOG_MIN_INTERVAL_MS);
        if (do_log)
        {
            printf("RX[M4]: off=0x%08lx addr=%p face=(%ld,%ld)-(%ld,%ld) center=(%ld,%ld) -> mid=(%ld,%ld) smoothed=(%ld,%ld)\r\n",
                   (unsigned long)offset, (void*)addr,
                   (long)x1, (long)y1, (long)x2, (long)y2, (long)cx_raw, (long)cy_raw, (long)mid_x, (long)mid_y, (long)s_mid_x, (long)s_mid_y);
            eyes_move_to_xy_and_log(s_mid_x, s_mid_y, "MBX");
            s_last_move_log_ms = now_ms;
        }
        else
        {
            /* 不打印，仅执行移动，避免日志刷屏影响性能 */
            eyes_goto_mid_xy(s_mid_x, s_mid_y);
        }
        /* 标记为“有有效人脸”，清除空闲状态 */
        s_last_valid_face_ms = now_ms;
        s_idle_active = 0;
        s_last_cmd_mid_x = s_mid_x;
        s_last_cmd_mid_y = s_mid_y;
        s_have_last_cmd = 1;
    }

    /* 邮箱拉取结束后，若长时间无有效人脸，则回到居中（初始）状态 */
    uint32_t now2 = millis();
    if (s_last_valid_face_ms != 0u)
    {
        uint32_t dt = (uint32_t)(now2 - s_last_valid_face_ms);
        if (dt >= FACE_LOST_TIMEOUT_MS && !s_idle_active)
        {
            int32_t min_x, max_x; eyes_mid_limits_x(&min_x, &max_x);
            int32_t min_y, max_y; eyes_mid_limits(&min_y, &max_y);
            int32_t mid_x = (min_x + max_x) / 2;
            int32_t mid_y = (min_y + max_y) / 2;
            if ((uint32_t)(now2 - s_last_idle_log_ms) >= IDLE_RETURN_LOG_MIN_INTERVAL_MS)
            {
                printf("[S300][MOVE:IDLE] no face %lums -> return to center (%ld,%ld) midX[%ld..%ld] midY[%ld..%ld)\r\n",
                       (unsigned long)dt, (long)mid_x, (long)mid_y,
                       (long)min_x, (long)max_x, (long)min_y, (long)max_y);
                eyes_move_to_xy_and_log(mid_x, mid_y, "IDLE");
                s_last_idle_log_ms = now2;
            }
            else
            {
                eyes_goto_mid_xy(mid_x, mid_y);
            }
            s_idle_active = 1; /* 避免重复回中 */
        }
    }
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

    rcc_init_mm_pll(8, 400, 0, 3, 2); /* 100MHz */
    rcc_init_dsp_pll(8, 400, 0, 2, 1); /* 300MHz */

    // 在初始化视频前先初始化 OV5640（DVP 摄像头经软 I2C 配置到 YUYV）
    int cam_ret = ov5640_preinit();
    if (cam_ret != 0) {
        printf("[S300][DisplayDemo][WARN] OV5640 init failed (%d), continue to init video for display path only.\r\n", cam_ret);
    }

    // 初始化视频子系统（包含 ST77 SPI LCD 序列）
    printf("[S300][DisplayDemo] init video...\r\n");
    init_video(EM_DVP, CAMREA_YUV422, C1080X720P);
    // while (1)
    // {
    //     /* code */;
    // }
    
 
    init_mailbox(MAILBOX_BASE, 4, MAILBOX_IRQ_NONE);
    set_dsp_warm_reset(true);

    write_mailbox(MAILBOX_BASE, 0x5A5A5A5A);

    // /* 测试与DSP通信 */
    // while (1)
    // {
    //     /* UART echo (non-blocking) */
    //     uart_echo_poll();
    //     monitor_mailbox_rx();
    //     /* tiny sleep ~5ms to reduce busy loop */
    //     uint32_t t0 = millis();
    //     while ((uint32_t)(millis() - t0) < 5u) { /* spin */ }
    // }

    volatile uint16_t* f0 = (volatile uint16_t*)DISP_RFRAME0_ADDR;
    volatile uint16_t* f1 = (volatile uint16_t*)DISP_RFRAME1_ADDR;
    volatile uint8_t*  a0 = (volatile uint8_t*)DISP_RALPHA0_ADDR;
    volatile uint8_t*  a1 = (volatile uint8_t*)DISP_RALPHA1_ADDR;

    /* Cache to globals for flush callback */
    s_f0 = f0; s_f1 = f1; s_a0 = a0; s_a1 = a1;

    printf("[S300][DisplayDemo] frame0=%p frame1=%p alpha0=%p alpha1=%p\r\n", (void*)f0, (void*)f1, (void*)a0, (void*)a1);

    const size_t pixels = (size_t)DISP_IMAGE_WIDTH * (size_t)DISP_IMAGE_HEIGHT;

    /* Prepare initial frame buffers: white canvas so transparent parts show white, not video */
    fill_buffer(f0, a0, pixels, 0xFFFFu, 0x88u); // white
    fill_buffer(f1, a1, pixels, 0xFFFFu, 0x88u); // white

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

    /* Make screen background opaque white to ensure alpha areas reveal white canvas */
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* FPS label: red, rotated 90°, center pivot, 12pt font */
    lv_obj_t * fps_label = lv_label_create(lv_screen_active());
    lv_label_set_text(fps_label, "FPS: --.-");
    extern const lv_font_t lv_font_montserrat_12;
    lv_obj_set_style_text_font(fps_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(fps_label, lv_color_hex(0xFF0000), 0);
    /* Reserve transform box to avoid clipping after rotation (fixed safe size) */
    // lv_obj_set_style_transform_width(fps_label, 80, 0);
    // lv_obj_set_style_transform_height(fps_label, 20, 0);
    // lv_obj_set_style_transform_pivot_x(fps_label, 40, 0);
    // lv_obj_set_style_transform_pivot_y(fps_label, 10, 0);
    // lv_obj_set_style_transform_angle(fps_label, 900, 0); /* 90deg */
    lv_obj_align(fps_label, LV_ALIGN_TOP_LEFT, 0, 0);

    /* FPS timer: update text based on flush count every 500ms */
    (void)lv_timer_create(fps_timer_cb, 500, fps_label);

    /* Two eyes after 90° rotation: symmetric around horizontal center line, vertical movement */
    const int32_t screen_w = DISP_IMAGE_WIDTH;
    const int32_t screen_h = DISP_IMAGE_HEIGHT;
    const int32_t eye_w = 30;  /* updated to match img_yanqiu width */
    const int32_t eye_h = 38;  /* updated to match img_yanqiu height */
    const int32_t bg_w  = 74;  /* eye socket background width */
    const int32_t bg_h  = 85;  /* eye socket background height */
    const int32_t center_x = screen_w / 2;
    const int32_t center_y = screen_h / 2;
    const int32_t spacing = 37;      /* vertical distance between eye centers */

    /* Common base X: center horizontally */
    const int32_t base_x = center_x - eye_w / 2;
    const int32_t base_x_bg = center_x - bg_w / 2;

    /* Eye socket backgrounds: create first to keep them behind eyes */
    lv_obj_t * bg_top = lv_image_create(lv_screen_active());
    lv_image_set_src(bg_top, &img_yankuang);
    lv_image_set_pivot(bg_top, bg_w / 2, bg_h / 2);
    lv_image_set_rotation(bg_top, 900);  /* 90 deg clockwise */
    int32_t base_y_bg_top = (center_y - spacing) - bg_h / 2;
    lv_obj_set_pos(bg_top, base_x_bg, base_y_bg_top);

    lv_obj_t * bg_bot = lv_image_create(lv_screen_active());
    lv_image_set_src(bg_bot, &img_yankuang);
    lv_image_set_pivot(bg_bot, bg_w / 2, bg_h / 2);
    lv_image_set_rotation(bg_bot, 900);  /* 90 deg clockwise */
    int32_t base_y_bg_bot = (center_y + spacing) - bg_h / 2;
    lv_obj_set_pos(bg_bot, base_x_bg, base_y_bg_bot);

    /* Top eye */
    lv_obj_t * eye_top = lv_image_create(lv_screen_active());
    lv_image_set_src(eye_top, &img_yanqiu);
    /* rotate clockwise 90 deg around center */
    lv_image_set_pivot(eye_top, eye_w / 2, eye_h / 2);
    lv_image_set_rotation(eye_top, 900);
    int32_t base_y_top = center_y - spacing - eye_h / 2;
    lv_obj_set_pos(eye_top, base_x, base_y_top);

    /* Bottom eye */
    lv_obj_t * eye_bot = lv_image_create(lv_screen_active());
    lv_image_set_src(eye_bot, &img_yanqiu);
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
    /* after 90° rotation, visible heights equal original widths */
    g_eye_vis_h = eye_w;
    g_socket_vis_h = bg_w;
    /* after 90° rotation, visible widths equal original heights */
    g_eye_vis_w = eye_h;
    g_socket_vis_w = bg_h;

    printf("[S300][DisplayDemo] LVGL started.\r\n");
    printf("[S300][DisplayDemo] UART echo enabled on debug UART (CR->CRLF).\r\n");
    printf("[S300][DisplayDemo] Command: goto <y_mid>  (move eyes midpoint vertically)\r\n");

    /* Main loop: run LVGL timers */
    while (1)
    {
        /* UART echo (non-blocking) */
        // uart_echo_poll();
        monitor_mailbox_rx();
        lv_timer_handler();
        /* tiny sleep ~5ms to reduce busy loop */
        uint32_t t0 = millis();
        while ((uint32_t)(millis() - t0) < 5u) { /* spin */ }
    }
}
