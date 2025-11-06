/*
 * 文件：ui_display.c
 * 说明：显示适配（单硬件buffer）版：单帧显示 + LVGL局部刷新 + DMA中断
 * 特性：
 *  - 继续使用 DSP 侧双“读帧”缓冲（DISP_RFRAME0/1），作“前台显示/后台渲染”轮换；
 *  - LVGL 改为 PARTIAL 渲染模式，提供两块小型draw buffer，仅搬运脏矩形；
 *  - flush 回调里用 DMA 将 px_map 的矩形搬运到“后台硬件buffer”的对应偏移；
 *  - 以 DMA 传输完成中断为 flush 完成信号；若为本帧最后一次 flush，再“寄存器敲门”切换显示buffer；
 *  - 上电初始化仍填充白色画布，避免噪点/花屏。
 */
#include <string.h>
#include <stdio.h>
#include "s300.h"
#include "video.h"
#include "lvgl.h"
#include "ui_display.h"
#include "dma.h"
#include "rcc.h"

/* ---- Debug logging ---- */
/* 开关：UI_DEBUG=0 全关；UI_DEBUG=1 使用分级日志。
 * 等级：0 ERROR, 1 WARN, 2 INFO, 3 DEBUG, 4 VERBOSE
 * 通过编译选项 -DUI_LOG_LEVEL=3 配置；默认 INFO(2)。
 */
#ifndef UI_DEBUG
#define UI_DEBUG 1
#endif
#ifndef UI_LOG_LEVEL
#define UI_LOG_LEVEL 2
#endif

#if UI_DEBUG
typedef enum {
    UI_LOG_LEVEL_ERROR = 0,
    UI_LOG_LEVEL_WARN  = 1,
    UI_LOG_LEVEL_INFO  = 2,
    UI_LOG_LEVEL_DEBUG = 3,
    UI_LOG_LEVEL_VERBOSE = 4,
} ui_log_level_t;

#define UI_LOG_IMPL(lv, tag, fmt, ...) do { \
    if ((lv) <= UI_LOG_LEVEL) { \
        printf("[UI][%s] " fmt "\r\n", tag, ##__VA_ARGS__); \
    } \
} while (0)

#define UI_LOGE(tag, fmt, ...) UI_LOG_IMPL(UI_LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#define UI_LOGW(tag, fmt, ...) UI_LOG_IMPL(UI_LOG_LEVEL_WARN,  tag, fmt, ##__VA_ARGS__)
#define UI_LOGI(tag, fmt, ...) UI_LOG_IMPL(UI_LOG_LEVEL_INFO,  tag, fmt, ##__VA_ARGS__)
#define UI_LOGD(tag, fmt, ...) UI_LOG_IMPL(UI_LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define UI_LOGV(tag, fmt, ...) UI_LOG_IMPL(UI_LOG_LEVEL_VERBOSE, tag, fmt, ##__VA_ARGS__)
/* 兼容旧宏：按 INFO 打印 */
#define UI_LOG(tag, fmt, ...) UI_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define UI_LOGE(tag, fmt, ...) ((void)0)
#define UI_LOGW(tag, fmt, ...) ((void)0)
#define UI_LOGI(tag, fmt, ...) ((void)0)
#define UI_LOGD(tag, fmt, ...) ((void)0)
#define UI_LOGV(tag, fmt, ...) ((void)0)
#define UI_LOG(tag, fmt, ...)  ((void)0)
#endif

static volatile uint16_t* s_f0;
static volatile uint16_t* s_f1;
static volatile uint8_t*  s_a0;
static volatile uint8_t*  s_a1;
static const uint32_t REG_F0 = (DSP_VIDEO_SS_BASE + 0x50u);
static const uint32_t REG_F1 = (DSP_VIDEO_SS_BASE + 0x54u);

/* LVGL 局部渲染用的双draw buffer（约屏幕 1/5 高度，按需调整） */
#define DRAWBUF_LINES     30u  /* 兼顾帧率与限制：128x30=3840 <= 4095，减少每帧 flush 次数 */
#define BYTES_PER_PIXEL   2u  /* RGB565 */
/* LVGL 要求 buffer 指针满足特定对齐（通常>=8B），使用 LV_ATTRIBUTE_MEM_ALIGN 保证 */
LV_ATTRIBUTE_MEM_ALIGN static uint16_t s_drawbuf1[DISP_IMAGE_WIDTH * DRAWBUF_LINES] __attribute__((aligned(8)));
LV_ATTRIBUTE_MEM_ALIGN static uint16_t s_drawbuf2[DISP_IMAGE_WIDTH * DRAWBUF_LINES] __attribute__((aligned(8)));

/* 后台/前台状态与 DMA 传输上下文 */
static volatile uint8_t  s_front_idx = 0u;   /* 当前正在显示的硬件buffer：0->s_f0，1->s_f1 */
static volatile uint8_t  s_dma_busy = 0u;    /* DMA 正在搬运一个 flush 区域 */
static volatile uint8_t  s_dma_last = 0u;    /* 本次 DMA 是否对应本帧最后一次 flush */
static lv_display_t *    s_dma_disp = NULL;  /* 保存 flush 的 disp，用于中断里回调 ready */

/* 选用 DMA0 的固定通道（与其它Demo/外设错开，避免冲突） */
#define UI_DMA_IDX   DMA_IDX0
#define UI_DMA_CH    2u

static inline volatile uint16_t * get_draw_fb(void)
{
    /* 单硬件buffer：固定向 F0 写，并且由 DSP 一直显示 F0 */
    return s_f0;
}

static inline void switch_present_to(uint8_t fb_idx)
{
    /* 写入对应寄存器触发DSP显示该buffer，并等待硬件受理（寄存器清零） */
    if (fb_idx == 0u) {
        REG32(REG_F0) = 1u;
        while ((REG32(REG_F0) & 0x1u) != 0u) { }
        s_front_idx = 0u;
    UI_LOGI("SWAP", "Present->F0 (addr=%p)", s_f0);
    } else {
        REG32(REG_F1) = 1u;
        while ((REG32(REG_F1) & 0x1u) != 0u) { }
        s_front_idx = 1u;
    UI_LOGI("SWAP", "Present->F1 (addr=%p)", s_f1);
    }
}

static inline void cpu_copy_rect_to_backfb(const lv_area_t *area, const uint16_t *src)
{
    /* 行拷贝到“后台硬件buffer”的矩形偏移处 */
    volatile uint16_t *dst_base = get_draw_fb();
    const int32_t x1 = area->x1;
    const int32_t y1 = area->y1;
    const int32_t w  = area->x2 - area->x1 + 1;
    const int32_t h  = area->y2 - area->y1 + 1;
    for (int32_t r = 0; r < h; ++r) {
        volatile uint16_t *dst = dst_base + (y1 + r) * DISP_IMAGE_WIDTH + x1;
        memcpy((void *)dst, (const void *)(src + r * w), (size_t)w * BYTES_PER_PIXEL);
    }
}

static void start_dma_rect_copy(lv_display_t * disp, const lv_area_t * area, const uint16_t * src, bool is_last)
{
    /* 使用 M2M + 16bit + 目的散射 实现 2D 矩形搬运（逐行，带行间隙） */
    const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    const uint32_t total_bytes = w * h * BYTES_PER_PIXEL;

    volatile uint16_t *dst_base = get_draw_fb();
    uintptr_t dst_start = (uintptr_t)(dst_base + area->y1 * DISP_IMAGE_WIDTH + area->x1);

    UI_LOGD("FLUSH", "area=(%d,%d)-(%d,%d) w=%lu h=%lu last=%u src=%p dst=%p bf=%u",
           (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2,
           (unsigned long)w, (unsigned long)h, (unsigned)is_last, src, (void*)dst_start, (unsigned)s_front_idx);

    /* 小块或 DMA 正忙：退化为 CPU 行拷贝，减少 DMA 启停抖动 */
    const uint32_t pix = w * h;
    if (s_dma_busy || pix < 256u) {
    UI_LOGD("FALLBACK", "DMA busy->CPU copy");
        cpu_copy_rect_to_backfb(area, src);
        lv_display_flush_ready(disp);
        return;
    }

    /* 选择传输位宽：AHB 32bit 时优先用 32bit，要求 x1 与 w 为偶数，确保起始/每行均 4B 对齐且无尾半字 */
    const bool use32 = (((area->x1 & 1) == 0) && ((w & 1) == 0));
    emDMATRWIDTH tw = use32 ? EM_TR_WIDTH_32_BIT : EM_TR_WIDTH_16_BIT;

    /* 编程 DMA：一次传输长度为 total_bytes，位宽按 tw；目的散射参数单位=位宽transfer */
    set_dma_std(EM_DMA0, UI_DMA_CH, (uint32_t)(uintptr_t)src, (uint32_t)dst_start, total_bytes, tw);
    set_dma_std_increment(EM_DMA0, UI_DMA_CH, EM_ADDRESS_INC, EM_ADDRESS_INC);
    set_dma_std_transfer_bitwidth(EM_DMA0, UI_DMA_CH, tw, tw);
    /* 提升带宽：提高突发深度（对 M2M 有效），保持 8 作为折中 */
    set_dma_burst_size(EM_DMA0, UI_DMA_CH, EM_MSIZE_8B, EM_MSIZE_8B);
    /* 目的散射设置：单位为 transfer；
     * - 16bit: dsc=w, dsi=(stride-w)
     * - 32bit: 一次传2像素，因此 dsc=w/2, dsi=(stride-w)/2
     */
    uint32_t dsc, dsi;
    if (use32) {
        dsc = (w >> 1);
        dsi = ((uint32_t)DISP_IMAGE_WIDTH - w) >> 1;
    } else {
        dsc = w;
        dsi = (uint32_t)DISP_IMAGE_WIDTH - w;
    }
    set_dma_dst_scatter(EM_DMA0, UI_DMA_CH, dsc, dsi);
    /* 源收集关闭（源是紧凑矩形） */
    set_dma_src_gather(EM_DMA0, UI_DMA_CH, 0, 0);
    /* 只用传输完成中断（保持与 Demo/I2S 用法一致） */
    set_dma_interrupt(EM_DMA0, UI_DMA_CH, EM_DMA_INT_TFR, 1);

    s_dma_busy = 1u;
    s_dma_last = 0u; /* 单buffer：不做帧尾切换 */
    s_dma_disp = disp;

    /* NVIC 在初始化时已开启，这里不重复设置 */

    set_dma_start(EM_DMA0, UI_DMA_CH);

    /* 打点当前通道寄存器与全局状态，便于确认是否启动 */
    S300_DMA_TypeDef *D = DMAC0;
    UI_LOGD("DMA", "Ch=%u start", (unsigned)UI_DMA_CH);
}

static void fill_buffer(volatile uint16_t *frame,
                        volatile uint8_t  *alpha,
                        size_t pixel_count,
                        uint16_t color,
                        uint8_t alpha_value)
{
    for (size_t i = 0; i < pixel_count; ++i) { frame[i] = color; alpha[i] = alpha_value; }
}

static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    /* PARTIAL 模式：px_map 指向紧凑矩形，按区域搬运到“后台硬件buffer” */
    // UI_LOG("CALL", "flush_cb px=%p", px_map);
    start_dma_rect_copy(disp, area, (const uint16_t *)px_map, false);
}

lv_display_t * ui_display_init(void)
{
    volatile uint16_t* f0 = (volatile uint16_t*)DISP_RFRAME0_ADDR;
    volatile uint16_t* f1 = (volatile uint16_t*)DISP_RFRAME1_ADDR;
    volatile uint8_t*  a0 = (volatile uint8_t*)DISP_RALPHA0_ADDR;
    volatile uint8_t*  a1 = (volatile uint8_t*)DISP_RALPHA1_ADDR;

    s_f0 = f0; s_f1 = f1; s_a0 = a0; s_a1 = a1;

    const size_t pixels = (size_t)DISP_IMAGE_WIDTH * (size_t)DISP_IMAGE_HEIGHT;

    /* Stop presenting during init */
    REG32(REG_F0) = 0u; REG32(REG_F1) = 0u;

    lv_init();

    /* 硬件前置：打开 DMA0 时钟并完成一次性初始化与 NVIC 配置 */
    rcc_set_cortex_m4_sys_clock(0, 0, 1, true); /* AON=0, DMA1=0, DMA0=1, enable */
    dma_init(UI_DMA_IDX);
    NVIC_ClearPendingIRQ(DMA0_IRQn);
    NVIC_SetPriority(DMA0_IRQn, 5);
    NVIC_EnableIRQ(DMA0_IRQn);

    lv_display_t * disp = lv_display_create(DISP_IMAGE_WIDTH, DISP_IMAGE_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    /* 使用“小块双缓冲”进行局部刷新，加速与CPU渲染并行 */
    lv_display_set_buffers(disp,
                           (void*)s_drawbuf1,
                           (void*)s_drawbuf2,
                           (uint32_t)(sizeof(s_drawbuf1)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    UI_LOGI("INIT", "drawbuf1=%p drawbuf2=%p align=%u bytes=%lu", s_drawbuf1, s_drawbuf2, (unsigned)8, (unsigned long)sizeof(s_drawbuf1));
    UI_LOGI("INIT", "fb0=%p fb1=%p alpha0=%p alpha1=%p", s_f0, s_f1, s_a0, s_a1);

    /* Prepare initial frame buffers: white canvas */
    fill_buffer(f0, a0, pixels, 0xFFFFu, 0xAAu);
    fill_buffer(f1, a1, pixels, 0xFFFFu, 0xAAu);

    /* 单硬件buffer：固定显示 F0 */
    REG32(REG_F0) = 1u;

    /* 默认先认为前台显示 f0，后台渲染 f1（实际切换发生在首帧完成后）*/
    s_front_idx = 0u;
    s_dma_busy = 0u;
    s_dma_last = 0u;

    UI_LOGI("INIT", "DMA clock+NVIC ready, front=%u", (unsigned)s_front_idx);

    return disp;
}

void ui_display_set_bg_color(uint32_t rgb24)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(rgb24), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}

    /* DMA0 IRQ：结束一次矩形搬运；单buffer：仅 flush_ready，不做切换 */
void DMA0_IRQHandler(void)
{
    S300_DMA_TypeDef *D = DMAC0;
    uint32_t st = D->StatusTfr;
    /* 只关心我们使用的通道 UI_DMA_CH */
    if (st & (1u << UI_DMA_CH)) {
        /* 采样打印，防止中断频繁刷屏 */
        #if UI_DEBUG
        if (UI_LOG_LEVEL >= UI_LOG_LEVEL_DEBUG) {
            static uint32_t s_irq_log_cnt = 0;
            if ((s_irq_log_cnt++ & 0xFFu) == 0u) { /* 每 256 次打印一次 */
                UI_LOGD("IRQ", "StatusTfr=0x%08lX ch%u", (unsigned long)st, (unsigned)UI_DMA_CH);
            }
        }
        #endif
        /* 清除传输完成中断标志 */
        D->ClearTfr = (1u << UI_DMA_CH);

        /* 关闭目的散射，避免影响后续配置（安全起见） */
        set_dma_dst_scatter(EM_DMA0, UI_DMA_CH, 0, 0);

        s_dma_busy = 0u;
        if (s_dma_disp) {
            lv_display_t *disp = s_dma_disp;
            s_dma_disp = NULL;
            /* 通知 LVGL：本次 flush 完成 */
            UI_LOGD("IRQ", "flush_ready()");
            lv_display_flush_ready(disp);
        }

        /* 单buffer：不进行帧切换 */
    } else {
        /* 如果进中断却非我们通道，记录一次 */
    if (st) UI_LOGD("IRQ", "StatusTfr=0x%08lX (not our ch%u)", (unsigned long)st, (unsigned)UI_DMA_CH);
    }
}
