#include "s300_bsp.h"
#include <stdio.h>

int main(void)
{
    BSP_Clock_Init();
    BSP_UART_Debug_Init();
    /* init 1ms SysTick and wait ~10ms */
    S300_SysTick_Init();
    S300_DelayMs(200);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Hello, PiMCHIP S300!\n");
    S300_DelayMs(200);
    S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "Hello again.\n");

    /* Retarget demo: printf via UART3 */
    long long big = 0x123456789ABCDEF0LL;
    double pi = 3.141592653589793;
    printf("printf demo: int=%d, ll=0x%llx, pi=%.6f\n", 42, big, pi);
    /* 1-second heartbeat print */
    uint32_t last_ms = S300_SysTick_Millis();
    for (;;)
    {
        uint32_t now = S300_SysTick_Millis();
        if ((uint32_t)(now - last_ms) >= 1000u) {
            last_ms += 1000u; /* maintain cadence even if delayed */
            S300_UART_PutStringI(BOARD_UART_DEBUG_ID, "[1s] heartbeat\n");
        }
        __NOP();
    }
}
