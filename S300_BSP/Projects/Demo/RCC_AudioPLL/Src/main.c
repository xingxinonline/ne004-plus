#include "s300.h"
#include <stdio.h>
#include <stdbool.h>
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "board.h"

int main(void)
{
    /* UART3 for printf via board layer */
    board_debug_uart_init();
    /* Audio PLL settings from legacy demo (table2) */
    int err = init_audio_pll(3, 129, 500000, 7, 6);
    while (err != RCC_STATUS_OK)
    {
        /* spin until locked */
    }
    /* Enable audio clock before using I2S */
    set_audio_clock(0, true);
    printf("Audio clock set to %lu KHz.\r\n", (unsigned long)(get_clock(RCC_CLOCK_AUDIO) / 1000u));
    for (;;)
    {
        __WFI();
    }
}
