#ifndef S300_BSP_H
#define S300_BSP_H

#include "s300.h"

static inline void BSP_Clock_Init(void)
{
    /* default 24MHz HSE, nothing to do for now */
}

static inline void BSP_UART0_Init(void)
{
    S300_UART_Init_115200(3u); /* use UART3 per request */
}

/* SysTick */
void S300_SysTick_Init(void);
uint32_t S300_SysTick_Millis(void);
void S300_DelayMs(uint32_t ms);

#endif
