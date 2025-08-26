#include "board.h"
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include <stdio.h>

#ifndef UART_DEBUG_IDX
    #define UART_DEBUG_IDX BOARD_UART_DEBUG_IDX
#endif

static void board_uart3_pins_init(void)
{
    // GPIOA 26/27 复用为 UART3
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    set_gpio_function(GPIOA, 26, FUNCTION_3);
    set_gpio_function(GPIOA, 27, FUNCTION_3);
}

void board_debug_uart_init(void)
{
#if BOARD_UART3_DEBUG_ENABLE
    // 开启 UART3 时钟并初始化 115200 8N1
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, true);
    board_uart3_pins_init();
    init_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), 115200);
    // 关闭缓冲，避免半主机影响
    setvbuf(stdout, NULL, _IONBF, 0);
#else
    (void)UART_DEBUG_IDX;
#endif
}
