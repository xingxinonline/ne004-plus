/**
 * @file direct_display.h
 * @brief 简单显示Demo头文件 - 不使用LVGL，直接操作显存
 *
 * 功能说明：
 *   - 初始化图层全绿显示，透明度为0（完全透明）
 *   - 通过改变特定区域的透明度实现画框效果
 *   - 支持通过串口/邮箱接收坐标数据来绘制矩形框
 *
 * 注意：PSRAM仅支持16bit读写，不支持8bit读写
 */

#ifndef _DIRECT_DISPLAY_H_
#define _DIRECT_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RGB565颜色定义
 */
#define COLOR_GREEN     0x07E0  /* 纯绿色 RGB565 */
#define COLOR_RED       0xF800  /* 纯红色 RGB565 */
#define COLOR_BLUE      0x001F  /* 纯蓝色 RGB565 */
#define COLOR_WHITE     0xFFFF  /* 白色 RGB565 */
#define COLOR_BLACK     0x0000  /* 黑色 RGB565 */
#define COLOR_YELLOW    0xFFE0  /* 黄色 RGB565 */

/**
 * @brief 透明度定义（8bit）
 *   - 0x00: 完全透明（显示底层图像）
 *   - 0xFF: 完全不透明（显示图层颜色）
 */
#define ALPHA_TRANSPARENT   0x00
#define ALPHA_OPAQUE        0xFF
#define ALPHA_HALF          0x80
#define ALPHA_BORDER        0xC0  /* 边框透明度 */

/**
 * @brief 矩形框结构体
 */
typedef struct
{
    int16_t x;          /* 左上角X坐标 */
    int16_t y;          /* 左上角Y坐标 */
    int16_t width;      /* 宽度 */
    int16_t height;     /* 高度 */
    uint8_t border_width; /* 边框宽度（像素） */
    uint8_t alpha;      /* 边框透明度 */
} rect_box_t;

/**
 * @brief 初始化简单显示系统
 *   - 填充图层为全绿色
 *   - 设置透明度为0（完全透明）
 *
 * @return 0 成功，其他 失败
 */
int direct_display_init(void);

/**
 * @brief 填充图层颜色（16bit RGB565）
 *
 * @param color RGB565颜色值
 */
void direct_display_fill_color(uint16_t color);

/**
 * @brief 设置整个图层透明度
 *   注意：PSRAM仅支持16bit访问，内部会处理
 *
 * @param alpha 透明度值 (0x00-0xFF)
 */
void direct_display_set_alpha(uint8_t alpha);

/**
 * @brief 清除所有矩形框（将透明度全部设为0）
 */
void direct_display_clear_boxes(void);

/**
 * @brief 绘制矩形框（通过设置边框区域的透明度）
 *
 * @param box 矩形框参数
 */
void direct_display_draw_box(const rect_box_t *box);

/**
 * @brief 绘制简单矩形框
 *
 * @param x 左上角X
 * @param y 左上角Y
 * @param w 宽度
 * @param h 高度
 * @param border_width 边框宽度
 * @param alpha 透明度
 */
void direct_display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                              uint8_t border_width, uint8_t alpha);

/**
 * @brief 处理接收到的坐标数据，绘制矩形框
 *   格式: x,y,w,h 或 x,y,w,h,border,alpha
 *
 * @param data 接收到的数据缓冲区
 * @param len 数据长度
 */
void direct_display_process_rect_cmd(const uint8_t *data, uint32_t len);

/**
 * @brief 刷新显示（切换前后台缓冲区）
 */
void direct_display_refresh(void);

/**
 * @brief 切换显示的缓冲区索引（0或1）
 *
 * @param buf_idx 缓冲区索引
 */
void direct_display_switch_buffer(uint8_t buf_idx);

/**
 * @brief 获取当前前台缓冲区索引
 *
 * @return uint8_t 缓冲区索引 (0 或 1)
 */
uint8_t direct_display_get_front_idx(void);

/**
 * @brief 使用16bit方式设置指定区域的透明度
 *   由于PSRAM不支持8bit读写，使用16bit读改写方式
 *
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param w 宽度
 * @param h 高度
 * @param alpha 透明度值
 */
void direct_display_set_region_alpha(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t alpha);

#ifdef __cplusplus
}
#endif

#endif /* _DIRECT_DISPLAY_H_ */
