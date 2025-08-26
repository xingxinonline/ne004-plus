#include "s300.h"
#include <stdio.h>
#include <stdbool.h>
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "board.h"

#ifndef UART_ECHO_IDX
    #define UART_ECHO_IDX UART_IDX3
#endif

static void init_uart3_pins(void)
{
    /* GPIOA pin26/pin27 -> FUNCTION_3 per legacy demo */
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    set_gpio_function(GPIOA, 26, FUNCTION_3);
    set_gpio_function(GPIOA, 27, FUNCTION_3);
}

int main(void)
{
    /* Assuming core clock already configured by system_S300.c */
    board_debug_uart_init();
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, true);
    init_uart(UART_ECHO_IDX, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), 115200);
    init_uart3_pins();
    /* simple echo loop */
    for (;;)
    {
        uint16_t d = read_uart(UART_ECHO_IDX, UARTTYPE_STD_SERIAL);
        write_uart(UART_ECHO_IDX, UARTTYPE_STD_SERIAL, d);
    }
}
