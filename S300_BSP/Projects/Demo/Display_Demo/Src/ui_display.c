/*
 * 文件：ui_display.c
 * 说明：LVGL 与硬件显示的适配层，实现双缓冲整帧渲染与“寄存器敲门”式刷帧同步。
 * 关键点：
 *  - s_f0/s_f1 映射到 DSP 侧可读的帧缓冲地址（DISP_RFRAME*）作为 LVGL 渲染缓冲；
 *  - lvgl_flush_cb 中，根据回调传入的像素指针判断当前缓冲，并写 REG_F0/REG_F1 发起刷新；
 *  - 简单等待寄存器清零作为完成信号，再调用 lv_display_flush_ready 结束本次刷帧；
 *  - 初始化时先填充白色画布，避免上电噪点/花屏。
 */
#include <string.h>
#include <stdio.h>
#include "s300.h"
#include "video.h"
#include "lvgl.h"
#include "ui_display.h"

static volatile uint16_t* s_f0;
static volatile uint16_t* s_f1;
static volatile uint8_t*  s_a0;
static volatile uint8_t*  s_a1;
static const uint32_t REG_F0 = (DSP_VIDEO_SS_BASE + 0x50u);
static const uint32_t REG_F1 = (DSP_VIDEO_SS_BASE + 0x54u);

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
    (void)area;
    uintptr_t p = (uintptr_t)px_map;
    if (p == (uintptr_t)s_f0) {
        REG32(REG_F0) = 1u;
        while ((REG32(REG_F0) & 0x1u) != 0u) { }
    } else if (p == (uintptr_t)s_f1) {
        REG32(REG_F1) = 1u;
        while ((REG32(REG_F1) & 0x1u) != 0u) { }
    }
    lv_display_flush_ready(disp);
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

    lv_display_t * disp = lv_display_create(DISP_IMAGE_WIDTH, DISP_IMAGE_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp,
                           (void*)f0,
                           (void*)f1,
                           (uint32_t)(DISP_IMAGE_WIDTH * DISP_IMAGE_HEIGHT * sizeof(uint16_t)),
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    /* Prepare initial frame buffers: white canvas */
    fill_buffer(f0, a0, pixels, 0xFFFFu, 0xAAu);
    fill_buffer(f1, a1, pixels, 0xFFFFu, 0xAAu);

    return disp;
}

void ui_display_set_bg_color(uint32_t rgb24)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(rgb24), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}
