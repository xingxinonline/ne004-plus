#include <stdio.h>
#include "board.h"
#include "rcc.h"
#include "gpio.h"
#include "uart.h"

static rcc_cm4_apb1_t board_uart_clock_mask(uart_idx_t idx)
{
    switch (idx)
    {
    case UART_IDX0: return RCC_CM4_APB1_UART0;
    case UART_IDX1: return RCC_CM4_APB1_UART1;
    case UART_IDX2: return RCC_CM4_APB1_UART2;
    default:        return RCC_CM4_APB1_UART3;
    }
}

static void board_uart_pinmux(uart_idx_t idx)
{
    switch (idx)
    {
    case UART_IDX3:
        /* GPIOA26 -> UART3_TXD, GPIOA27 -> UART3_RXD */
        gpio_set_function(GPIOA, 26u, FUNCTION_3);
        gpio_set_function(GPIOA, 27u, FUNCTION_3);
        gpio_set_mode(GPIOA, 26u, GPIO_UP);
        gpio_set_mode(GPIOA, 27u, GPIO_UP);
        break;
    default:
        /* Add further pin mappings here if other UARTs are used */
        break;
    }
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

void board_init(void)
{
    board_clock_init();
    board_debug_uart_init();
}

void board_debug_uart_init(void)
{
#if BOARD_UART3_DEBUG_ENABLE
    const uart_idx_t uart_idx = (uart_idx_t)BOARD_UART_DEBUG_IDX;
    const rcc_cm4_apb1_t uart_mask = board_uart_clock_mask(uart_idx);

    /* Enable IO matrix/mux, GPIO and UART clocks */
    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_IOMATRIX, true);
    rcc_set_cortex_m4_apb0_clock(RCC_CM4_APB0_IOMUX, true);
    rcc_set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    rcc_set_cortex_m4_apb1_clock(uart_mask, true);

    /* Release resets */
    rcc_set_cortex_m4_apb0_reset(RCC_CM4_APB0_IOMATRIX, false);
    rcc_set_cortex_m4_apb0_reset(RCC_CM4_APB0_IOMUX, false);
    rcc_set_cortex_m4_apb1_reset(RCC_CM4_APB1_GPIO, false);
    rcc_set_cortex_m4_apb1_reset(uart_mask, false);

    board_uart_pinmux(uart_idx);

    uint32_t apb_clk = rcc_get_clock(RCC_CLOCK_APB1);
    if (apb_clk == 0u)
    {
        apb_clk = SystemCoreClock;
    }

    uart_init(uart_idx, UARTTYPE_STD_SERIAL, apb_clk, 115200u);
    setvbuf(stdout, NULL, _IONBF, 0);
#else
    /* Debug UART disabled at build time */
#endif
}
