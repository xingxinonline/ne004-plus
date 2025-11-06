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
#define BOARD_UART_DEBUG_IDX 1u
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
