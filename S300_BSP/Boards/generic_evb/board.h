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

// LCD Backlight Configuration
// Port: Default GPIOA if not specified (will be resolved in driver)
#ifndef BOARD_LCD_BL_PORT
#define BOARD_LCD_BL_PORT GPIOA
#endif
// Pin: 0xFF for none (connected to VCC or always on)
#ifndef BOARD_LCD_BL_PIN
#define BOARD_LCD_BL_PIN 0xFF
#endif
// Active Level: 1=High, 0=Low
#ifndef BOARD_LCD_BL_ACTIVE_LEVEL
#define BOARD_LCD_BL_ACTIVE_LEVEL 1
#endif

// Display Configuration (must be <= LCD size)
#ifndef BOARD_DISPLAY_WIDTH
#define BOARD_DISPLAY_WIDTH BOARD_LCD_WIDTH
#endif
#ifndef BOARD_DISPLAY_HEIGHT
#define BOARD_DISPLAY_HEIGHT BOARD_LCD_HEIGHT
#endif

// Debug UART Configuration
#ifndef BOARD_DEBUG_UART_IDX
#define BOARD_DEBUG_UART_IDX 3
#endif
#ifndef BOARD_DEBUG_UART_BAUDRATE
#define BOARD_DEBUG_UART_BAUDRATE 921600
#endif
#ifndef BOARD_DEBUG_UART_PORT
#define BOARD_DEBUG_UART_PORT GPIOA
#endif
#ifndef BOARD_DEBUG_UART_TX_PIN
#define BOARD_DEBUG_UART_TX_PIN 26
#endif
#ifndef BOARD_DEBUG_UART_RX_PIN
#define BOARD_DEBUG_UART_RX_PIN 27
#endif
#ifndef BOARD_DEBUG_UART_FUNCTION
#define BOARD_DEBUG_UART_FUNCTION FUNCTION_3
#endif

// Camera I2C Configuration (Soft I2C)
#ifndef BOARD_CAMERA_I2C_IDX
#define BOARD_CAMERA_I2C_IDX 1
#endif
#ifndef BOARD_CAMERA_I2C_PORT
#define BOARD_CAMERA_I2C_PORT GPIOA
#endif
#ifndef BOARD_CAMERA_I2C_SCL_PIN
#define BOARD_CAMERA_I2C_SCL_PIN 0
#endif
#ifndef BOARD_CAMERA_I2C_SDA_PIN
#define BOARD_CAMERA_I2C_SDA_PIN 1
#endif
#ifndef BOARD_CAMERA_I2C_FUNCTION
#define BOARD_CAMERA_I2C_FUNCTION FUNCTION_2
#endif

#if (BOARD_DISPLAY_WIDTH > BOARD_LCD_WIDTH) || (BOARD_DISPLAY_HEIGHT > BOARD_LCD_HEIGHT)
#error "Display configuration cannot exceed LCD screen size"
#endif

// Camera Configuration
// 0: RGB565, 1: YUV422 (Default: 0 - RGB565)
#ifndef BOARD_CAMERA_FORMAT
#define BOARD_CAMERA_FORMAT 0
#endif

// Video / Image Processing Configuration
#ifndef RD_SOURCE_FRAME_START_X
#define RD_SOURCE_FRAME_START_X (0)
#endif
#ifndef RD_SOURCE_FRAME_START_Y
#define RD_SOURCE_FRAME_START_Y (0)
#endif

#ifndef BINNING_SIZE
#define BINNING_SIZE            (1) //binning = 4 @ Sensor image size (1280*1920) 尽量binning到足够小
#endif

/* binning */
#ifndef BINNING_IMAGE_WIDTH
#define BINNING_IMAGE_WIDTH         (288)
#endif
#ifndef BINNING_IMAGE_HEIGHT
#define BINNING_IMAGE_HEIGHT        (360)
#endif
/* sensor */
#ifndef SENSOR_IMAGE_WIDTH
#define SENSOR_IMAGE_WIDTH          (BINNING_IMAGE_WIDTH * (1U << BINNING_SIZE))
#endif
#ifndef SENSOR_IMAGE_HEIGHT
#define SENSOR_IMAGE_HEIGHT         (BINNING_IMAGE_HEIGHT * (1U << BINNING_SIZE))
#endif
/* downscale */
#ifndef DOWNSCALE_IMAGE_WIDTH
#define DOWNSCALE_IMAGE_WIDTH       (128)
#endif
#ifndef DOWNSCALE_IMAGE_HEIGHT
#define DOWNSCALE_IMAGE_HEIGHT      (160)
#endif
/* display  */
#ifndef DISP_START_X
#define DISP_START_X                (0)
#endif
#ifndef DISP_START_Y
#define DISP_START_Y                (0)
#endif

// Map DISP_IMAGE_* to generic BOARD_DISPLAY_*
#define DISP_IMAGE_WIDTH            BOARD_DISPLAY_WIDTH
#define DISP_IMAGE_HEIGHT           BOARD_DISPLAY_HEIGHT

/* snap */
#ifndef SNAP_IMAGE_WIDTH
#define SNAP_IMAGE_WIDTH            (128)
#endif
#ifndef SNAP_IMAGE_HEIGHT
#define SNAP_IMAGE_HEIGHT           (160)
#endif

#ifndef BINNING_LINE_MAX_SIZE
#define BINNING_LINE_MAX_SIZE       (1280)
#endif
#ifndef DOWNSCALE_FACTOR
#define DOWNSCALE_FACTOR            (8192)
#endif

#ifndef OFFLINE_IMAGE_BASE_ADDRESS
#define OFFLINE_IMAGE_BASE_ADDRESS  (0x44000000)
#endif

/* SRAM0/SRAM1 Memory Layout for Video */
/* SRAM0 */
#ifndef DISP_RALPHA0_ADDR
#define DISP_RALPHA0_ADDR           (0x44040000 - (DISP_IMAGE_WIDTH * DISP_IMAGE_HEIGHT * 1))
#endif
#ifndef DISP_RALPHA1_ADDR
#define DISP_RALPHA1_ADDR           (0x44040000 - (DISP_IMAGE_WIDTH * DISP_IMAGE_HEIGHT * 1))
#endif
/* SRAM1 */
#ifndef DISP_RFRAME0_ADDR
#define DISP_RFRAME0_ADDR           (0x44080000 - (SNAP_IMAGE_WIDTH * SNAP_IMAGE_HEIGHT * 2))
#endif
#ifndef DISP_RFRAME1_ADDR
#define DISP_RFRAME1_ADDR           (0x44080000 - (SNAP_IMAGE_WIDTH * SNAP_IMAGE_HEIGHT * 4))
#endif
#ifndef DISP_WFRAME0_ADDR
#define DISP_WFRAME0_ADDR           (0x44080000 - (SNAP_IMAGE_WIDTH * SNAP_IMAGE_HEIGHT * 6))
#endif
#ifndef DISP_WFRAME1_ADDR
#define DISP_WFRAME1_ADDR           (0x44080000 - (SNAP_IMAGE_WIDTH * SNAP_IMAGE_HEIGHT * 6))
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
