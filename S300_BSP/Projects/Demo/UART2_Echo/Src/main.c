#include "s300.h"
#include <stdio.h>
#include <stdbool.h>
#include "rcc.h"
#include "gpio.h"
#include "uart.h"
#include "board.h"

#ifndef UART_ECHO_IDX
    #define UART_ECHO_IDX UART_IDX2
#endif

/* 允许用户通过宏覆盖引脚与复用功能，默认按 GPIOA23=RX, GPIOA24=TX，FUNC=FUNCTION_3 */
#ifndef UART2_RX_PIN_NUM
    #define UART2_RX_PIN_NUM 23
#endif
#ifndef UART2_TX_PIN_NUM
    #define UART2_TX_PIN_NUM 24
#endif
#ifndef UART2_PIN_FUNC
    #define UART2_PIN_FUNC FUNCTION_3
#endif

static void init_uart2_pins(void)
{
    /* GPIOA pin23/pin24 -> UART2 */
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_GPIO, true);
    set_gpio_function(GPIOA, UART2_RX_PIN_NUM, UART2_PIN_FUNC);
    set_gpio_function(GPIOA, UART2_TX_PIN_NUM, UART2_PIN_FUNC);
}

static void uart2_send_str(const char *s)
{
    while (*s)
    {
        write_uart(UART_ECHO_IDX, UARTTYPE_STD_SERIAL, (uint8_t)(*s++));
    }
}

int main(void)
{
    /* 调试串口（UART3）初始化，便于打印提示信息 */
    board_debug_uart_init();
    /* 打开 UART2 APB1 时钟并初始化 115200 8N1 */
    set_cortex_m4_apb1_clock(RCC_CM4_APB1_UART2, true);
    init_uart(UART_ECHO_IDX, UARTTYPE_STD_SERIAL, rcc_get_clock(RCC_CLOCK_APB1), 115200);
    init_uart2_pins();
    /* 通过 UART2 先打一行 hello */
    uart2_send_str("Hello from UART2!\r\n");
    printf("UART2 Echo demo start (RX=A%u, TX=A%u, FUNC=%d)\n",
           (unsigned)UART2_RX_PIN_NUM, (unsigned)UART2_TX_PIN_NUM, (int)UART2_PIN_FUNC);
    for (;;)
    {
        uint16_t d = read_uart(UART_ECHO_IDX, UARTTYPE_STD_SERIAL);
        write_uart(UART_ECHO_IDX, UARTTYPE_STD_SERIAL, d);
    }
}
