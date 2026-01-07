/**
 * @file direct_display.c
 * @brief 简单显示Demo实现 - 不使用LVGL，直接操作显存
 * 
 * 实现说明：
 *   - 直接操作PSRAM中的显存和Alpha通道
 *   - 由于PSRAM不支持8bit读写，所有操作使用16bit
 *   - 图层初始化为全绿色，透明度为0
 *   - 通过改变透明度来实现画框效果
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "direct_display.h"
#include "s300.h"
#include "video.h"

/* 寄存器访问宏 */
#ifndef REG32
#define REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#endif

/* 显存地址（从video.h获取） 
 * 注意：PSRAM不支持8bit读写，只能16bit读写
 * - Frame缓冲区: RGB565格式，每像素16bit，直接使用uint16_t指针
 * - Alpha缓冲区: 每像素8bit，但必须以16bit访问（2像素/次）
 */
static volatile uint16_t *s_frame0 = (volatile uint16_t *)DISP_RFRAME0_ADDR;
static volatile uint16_t *s_frame1 = (volatile uint16_t *)DISP_RFRAME1_ADDR;
/* Alpha缓冲区使用uint16_t指针，每次访问2个像素的alpha */
static volatile uint16_t *s_alpha0 = (volatile uint16_t *)DISP_RALPHA0_ADDR;
static volatile uint16_t *s_alpha1 = (volatile uint16_t *)DISP_RALPHA1_ADDR;

/* 显示控制寄存器 */
static const uint32_t REG_FRAME0 = (DSP_VIDEO_SS_BASE + 0x50u);
static const uint32_t REG_FRAME1 = (DSP_VIDEO_SS_BASE + 0x54u);

/* 当前前台缓冲区索引 */
static volatile uint8_t s_front_idx = 0u;

/* 屏幕尺寸 */
#define SCREEN_WIDTH    DISP_IMAGE_WIDTH
#define SCREEN_HEIGHT   DISP_IMAGE_HEIGHT
#define PIXEL_COUNT     (SCREEN_WIDTH * SCREEN_HEIGHT)

/**
 * @brief 使用16bit填充内存
 *   由于PSRAM不支持8bit读写，使用16bit填充
 */
static void memset16(volatile uint16_t *dest, uint16_t value, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        dest[i] = value;
    }
}

/**
 * @brief 将两个8bit alpha值打包成16bit
 *   低字节为偶数像素，高字节为奇数像素
 */
static inline uint16_t pack_alpha16(uint8_t alpha0, uint8_t alpha1)
{
    return (uint16_t)((alpha1 << 8) | alpha0);
}

/**
 * @brief 使用16bit方式设置alpha缓冲区
 *   由于PSRAM不支持8bit读写，将两个相邻像素的alpha打包为16bit
 *   Alpha缓冲区大小为 pixel_count 字节，以16bit访问则为 pixel_count/2 次
 */
static void fill_alpha16(volatile uint16_t *alpha_buf, uint8_t alpha_value, uint32_t pixel_count)
{
    /* 每次写入2个像素的alpha（16bit = 2 * 8bit alpha） */
    uint16_t packed_alpha = pack_alpha16(alpha_value, alpha_value);
    /* Alpha缓冲区总共 pixel_count 字节，16bit访问需要 pixel_count/2 次 */
    uint32_t word_count = pixel_count / 2;
    
    for (uint32_t i = 0; i < word_count; i++) {
        alpha_buf[i] = packed_alpha;
    }
}

/**
 * @brief 获取当前后台帧缓冲区
 */
static inline volatile uint16_t *get_back_frame(void)
{
    return (s_front_idx == 0u) ? s_frame1 : s_frame0;
}

/**
 * @brief 获取当前后台alpha缓冲区
 */
static inline volatile uint16_t *get_back_alpha(void)
{
    return (s_front_idx == 0u) ? s_alpha1 : s_alpha0;
}

/**
 * @brief 获取当前前台帧缓冲区
 */
static inline volatile uint16_t *get_front_frame(void)
{
    return (s_front_idx == 0u) ? s_frame0 : s_frame1;
}

/**
 * @brief 获取当前前台alpha缓冲区
 */
static inline volatile uint16_t *get_front_alpha(void)
{
    return (s_front_idx == 0u) ? s_alpha0 : s_alpha1;
}

int direct_display_init(void)
{
    printf("[DirectDisplay] Initializing...\r\n");
    printf("[DirectDisplay] Screen: %dx%d\r\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("[DirectDisplay] Frame0: 0x%08X, Frame1: 0x%08X\r\n", 
           (unsigned int)DISP_RFRAME0_ADDR, (unsigned int)DISP_RFRAME1_ADDR);
    printf("[DirectDisplay] Alpha0: 0x%08X, Alpha1: 0x%08X\r\n", 
           (unsigned int)DISP_RALPHA0_ADDR, (unsigned int)DISP_RALPHA1_ADDR);
    
    /* 初始化两个帧缓冲区为全绿色 */
    printf("[DirectDisplay] Filling frames with GREEN color...\r\n");
    memset16(s_frame0, COLOR_GREEN, PIXEL_COUNT);
    memset16(s_frame1, COLOR_GREEN, PIXEL_COUNT);
    
    /* 初始化两个alpha缓冲区为0（完全透明） */
    printf("[DirectDisplay] Setting alpha to 0 (transparent)...\r\n");
    fill_alpha16(s_alpha0, ALPHA_TRANSPARENT, PIXEL_COUNT);
    fill_alpha16(s_alpha1, ALPHA_TRANSPARENT, PIXEL_COUNT);
    
    /* 默认显示缓冲区0 */
    s_front_idx = 0u;
    direct_display_switch_buffer(0);
    
    printf("[DirectDisplay] Initialization complete.\r\n");
    return 0;
}

void direct_display_fill_color(uint16_t color)
{
    /* 填充后台缓冲区 */
    volatile uint16_t *back_frame = get_back_frame();
    memset16(back_frame, color, PIXEL_COUNT);
}

void direct_display_set_alpha(uint8_t alpha)
{
    /* 设置后台alpha缓冲区 */
    volatile uint16_t *back_alpha = get_back_alpha();
    fill_alpha16(back_alpha, alpha, PIXEL_COUNT);
}

void direct_display_clear_boxes(void)
{
    /* 将后台alpha缓冲区全部设为0（透明） */
    volatile uint16_t *back_alpha = get_back_alpha();
    fill_alpha16(back_alpha, ALPHA_TRANSPARENT, PIXEL_COUNT);
}

void direct_display_set_region_alpha(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t alpha)
{
    /* 边界检查 */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    
    volatile uint16_t *alpha_buf = get_back_alpha();
    
    /* 
     * 由于PSRAM不支持8bit读写，需要使用16bit读改写
     * alpha缓冲区每16bit存储2个像素的alpha值
     * 内存布局: [pixel0_alpha(低字节) | pixel1_alpha(高字节)] 为16bit
     * 
     * 优化策略：
     * 1. 如果起始x为偶数且宽度为偶数，可以直接写入完整的16bit值
     * 2. 对于边界的奇数像素，使用读改写
     */
    uint16_t packed_alpha = pack_alpha16(alpha, alpha);
    
    for (int16_t row = 0; row < h; row++) {
        int16_t curr_y = y + row;
        int16_t start_x = x;
        int16_t end_x = x + w;
        int16_t col = start_x;
        
        /* 处理起始奇数像素（如果有） */
        if (col & 1) {
            uint32_t pixel_idx = curr_y * SCREEN_WIDTH + col;
            uint32_t word_idx = pixel_idx / 2;
            uint16_t word_val = alpha_buf[word_idx];
            /* col为奇数，所以是高字节 */
            word_val = (word_val & 0x00FF) | ((uint16_t)alpha << 8);
            alpha_buf[word_idx] = word_val;
            col++;
        }
        
        /* 处理中间成对的像素（直接写入16bit，无需读改写） */
        while (col + 1 < end_x) {
            uint32_t pixel_idx = curr_y * SCREEN_WIDTH + col;
            uint32_t word_idx = pixel_idx / 2;
            alpha_buf[word_idx] = packed_alpha;
            col += 2;
        }
        
        /* 处理结尾的奇数像素（如果有） */
        if (col < end_x) {
            uint32_t pixel_idx = curr_y * SCREEN_WIDTH + col;
            uint32_t word_idx = pixel_idx / 2;
            uint16_t word_val = alpha_buf[word_idx];
            /* col为偶数，所以是低字节 */
            word_val = (word_val & 0xFF00) | alpha;
            alpha_buf[word_idx] = word_val;
        }
    }
}

void direct_display_draw_box(const rect_box_t *box)
{
    if (box == NULL || box->width <= 0 || box->height <= 0) return;
    
    int16_t x = box->x;
    int16_t y = box->y;
    int16_t w = box->width;
    int16_t h = box->height;
    uint8_t bw = box->border_width;
    uint8_t alpha = box->alpha;
    
    if (bw == 0) bw = 2;  /* 默认边框宽度 */
    if (alpha == 0) alpha = ALPHA_BORDER;  /* 默认透明度 */
    
    /* 绘制四条边框 */
    /* 上边 */
    direct_display_set_region_alpha(x, y, w, bw, alpha);
    /* 下边 */
    direct_display_set_region_alpha(x, y + h - bw, w, bw, alpha);
    /* 左边 */
    direct_display_set_region_alpha(x, y + bw, bw, h - 2 * bw, alpha);
    /* 右边 */
    direct_display_set_region_alpha(x + w - bw, y + bw, bw, h - 2 * bw, alpha);
}

void direct_display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, 
                              uint8_t border_width, uint8_t alpha)
{
    rect_box_t box = {
        .x = x,
        .y = y,
        .width = w,
        .height = h,
        .border_width = border_width,
        .alpha = alpha
    };
    direct_display_draw_box(&box);
}

void direct_display_process_rect_cmd(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) return;
    
    /* 解析格式: "RECT x,y,w,h" 或 "RECT x,y,w,h,bw,alpha" */
    char buf[128];
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, data, len);
    buf[len] = '\0';
    
    /* 跳过命令前缀 */
    char *p = buf;
    if (strncmp(p, "RECT ", 5) == 0) {
        p += 5;
    } else if (strncmp(p, "rect ", 5) == 0) {
        p += 5;
    } else if (strncmp(p, "BOX ", 4) == 0) {
        p += 4;
    } else if (strncmp(p, "box ", 4) == 0) {
        p += 4;
    }
    
    /* 解析坐标 */
    int vals[6] = {0, 0, 50, 50, 2, ALPHA_BORDER};  /* 默认值 */
    int idx = 0;
    char *token = strtok(p, ",");
    while (token != NULL && idx < 6) {
        vals[idx++] = atoi(token);
        token = strtok(NULL, ",");
    }
    
    /* 清除旧框，绘制新框 */
    direct_display_clear_boxes();
    direct_display_draw_rect((int16_t)vals[0], (int16_t)vals[1], 
                             (int16_t)vals[2], (int16_t)vals[3],
                             (uint8_t)vals[4], (uint8_t)vals[5]);
    
    printf("[DirectDisplay] Draw rect: (%d,%d) %dx%d border=%d alpha=%d\r\n",
           vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]);
}

void direct_display_switch_buffer(uint8_t buf_idx)
{
    if (buf_idx == 0u) {
        REG32(REG_FRAME0) = 1u;
        while ((REG32(REG_FRAME0) & 0x1u) != 0u) { /* 等待硬件受理 */ }
        s_front_idx = 0u;
    } else {
        REG32(REG_FRAME1) = 1u;
        while ((REG32(REG_FRAME1) & 0x1u) != 0u) { /* 等待硬件受理 */ }
        s_front_idx = 1u;
    }
}

void direct_display_refresh(void)
{
    /* 切换到另一个缓冲区 */
    uint8_t next_idx = (s_front_idx == 0u) ? 1u : 0u;

    /* S300 M4 usually has no D-Cache enabled in this config.
     * We use DSB to ensure write buffer drain.
     */
    __DSB();

    direct_display_switch_buffer(next_idx);
}

uint8_t direct_display_get_front_idx(void)
{
    return s_front_idx;
}
