#ifndef S300_PLATFORM_H
#define S300_PLATFORM_H

#include "s300.h"
#include "s300_uart.h"
#include "board.h"

static inline void BSP_Clock_Init(void)
{
    Board_Clock_Init();
    Board_Pinmux_Init();
}

static inline void BSP_UART0_Init(void)
{
    S300_UART_Init_115200(BOARD_UART_DEBUG_ID);
}

static inline void BSP_UART_Debug_Init(void)
{
    S300_UART_Init(BOARD_UART_DEBUG_ID, BOARD_UART_DEBUG_BAUD);
}

/* SysTick 由 CMSIS/Device 的 system_S300.c 提供实现，这里只声明接口（如需）。*/
void S300_SysTick_Init(void);
uint32_t S300_SysTick_Millis(void);
void S300_DelayMs(uint32_t ms);

#endif
