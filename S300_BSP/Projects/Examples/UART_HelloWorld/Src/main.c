#include "s300_bsp.h"

int main(void)
{
    BSP_Clock_Init();
    BSP_UART0_Init();
    /* small delay before first TX */
    for (volatile int i = 0; i < 10000; ++i)
    {
        __asm volatile("nop");
    }
    S300_UART_PutStringI(3u, "Hello, PiMCHIP S300 (UART3)!\n");
    S300_UART_PutStringI(3u, "Hello again.\n");
    for (;;)
    {
        /* idle */
    }
}
