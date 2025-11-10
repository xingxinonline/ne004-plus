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
    set_gpio_function(GPIOA, 16, FUNCTION_3);
    set_gpio_function(GPIOA, 17, FUNCTION_3);
}

void board_clock_init(void)
{
    /* 切到 CM4 PLL：与参考配置一致（192MHz） */
    (void)init_cortex_m4_pll(6, 768, 0, 4, 2);
    /* 可选：保持 APB0/APB1 分频为 0（不分频），确保 APB=SYS */
    // set_apb_clock_div(0, 0);
    // set_apb_clock_div(1, 0);
    SystemCoreClockUpdate();
}

void board_debug_uart_init(void)
{
#if BOARD_UART3_DEBUG_ENABLE
    // 开启 UART3 时钟并初始化 115200 8N1
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, true);
    board_uart3_pins_init();
    init_uart(UART_DEBUG_IDX, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), 115200);
    init_uart(3, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), 115200);
    // 关闭缓冲，避免半主机影响
    setvbuf(stdout, NULL, _IONBF, 0);
#else
    (void)UART_DEBUG_IDX;
#endif
}

void board_init(void)
{
    board_clock_init();
    board_debug_uart_init();
    /* 统一安全网：确保进入各 Demo 前全局中断已开启（防止早期启动路径或引导阶段残留 PRIMASK=1）。*/
    __enable_irq();
}
