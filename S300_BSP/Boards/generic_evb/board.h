#ifndef S300_BSP_BOARD_H
#define S300_BSP_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// 开关：是否自动初始化并重定向 UART3（默认开启）
#ifndef BOARD_UART3_DEBUG_ENABLE
#define BOARD_UART3_DEBUG_ENABLE 1
#endif

// 允许用户覆盖调试 UART 索引
#ifndef BOARD_UART_DEBUG_IDX
#define BOARD_UART_DEBUG_IDX 3u
#endif

// OV5640 Camera PINs (GPIOA)
#ifndef BOARD_CAM_RST_PIN
#define BOARD_CAM_RST_PIN 15u
#endif
#ifndef BOARD_CAM_PWDN_PIN
#define BOARD_CAM_PWDN_PIN 6u
#endif

// LCD Configuration
// Type: 0=ST7735S, 1=ST7789
#ifndef BOARD_LCD_TYPE
#define BOARD_LCD_TYPE 0
#endif
#ifndef BOARD_LCD_WIDTH
#define BOARD_LCD_WIDTH 128
#endif
#ifndef BOARD_LCD_HEIGHT
#define BOARD_LCD_HEIGHT 160
#endif

// Display Configuration (must be <= LCD size)
#ifndef BOARD_DISPLAY_WIDTH
#define BOARD_DISPLAY_WIDTH BOARD_LCD_WIDTH
#endif
#ifndef BOARD_DISPLAY_HEIGHT
#define BOARD_DISPLAY_HEIGHT BOARD_LCD_HEIGHT
#endif

#if (BOARD_DISPLAY_WIDTH > BOARD_LCD_WIDTH) || (BOARD_DISPLAY_HEIGHT > BOARD_LCD_HEIGHT)
#error "Display configuration cannot exceed LCD screen size"
#endif

// Camera Configuration
// 0: RGB565, 1: YUV422 (Default: 0 - RGB565)
#ifndef BOARD_CAMERA_FORMAT
#define BOARD_CAMERA_FORMAT 0
#endif

// 初始化板级系统时钟（切换 CM4 到 PLL 等），需在 UART 之前调用
void board_clock_init(void);

// 统一板级初始化：先时钟后 UART（推荐在 main 最先调用）
void board_init(void);

// 初始化调试串口（若启用）
void board_debug_uart_init(void);

#ifdef __cplusplus
}
#endif

#endif /* S300_BSP_BOARD_H */
