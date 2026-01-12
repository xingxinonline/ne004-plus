#include "board.h"
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include <stdio.h>

static void board_uart_pins_init(void)
{
    // GPIO 复用为 UART
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    set_gpio_function(BOARD_DEBUG_UART_PORT, BOARD_DEBUG_UART_TX_PIN, BOARD_DEBUG_UART_FUNCTION);
    set_gpio_function(BOARD_DEBUG_UART_PORT, BOARD_DEBUG_UART_RX_PIN, BOARD_DEBUG_UART_FUNCTION);
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
    // 开启 UART 时钟
    // 注意：当前仅适配 UART3，如需切换 UART0-2 需根据 IDX 修改时钟
    if (BOARD_DEBUG_UART_IDX == 3) {
        set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART3, true);
    } else {
        // TODO: Handle other UART clocks
        set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART0, true); 
    }

    board_uart_pins_init();
    init_uart(BOARD_DEBUG_UART_IDX, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), BOARD_DEBUG_UART_BAUDRATE);
    
    // 关闭缓冲
    setvbuf(stdout, NULL, _IONBF, 0);
}

void board_init(void)
{
    board_clock_init();
    board_debug_uart_init();
}
