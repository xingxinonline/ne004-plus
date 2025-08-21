#include "s300_bsp.h"

int main(void)
{
    BSP_Clock_Init();
    BSP_UART0_Init();
    /* init 1ms SysTick and wait ~10ms */
    S300_SysTick_Init();
    S300_DelayMs(200);
    S300_UART_PutStringI(3u, "Hello, PiMCHIP S300 (UART3)!\n");
    S300_DelayMs(200);
    S300_UART_PutStringI(3u, "Hello again.\n");
    /* 1-second heartbeat print */
    uint32_t last_ms = S300_SysTick_Millis();
    for (;;)
    {
        uint32_t now = S300_SysTick_Millis();
        if ((uint32_t)(now - last_ms) >= 1000u) {
            last_ms += 1000u; /* maintain cadence even if delayed */
            S300_UART_PutStringI(3u, "[1s] heartbeat\n");
        }
        __NOP();
    }
}
