#include "s300.h"

/* Simple 1ms system tick */
static volatile uint32_t s_ticks_ms = 0;

void SysTick_Handler(void)
{
    s_ticks_ms++;
}

/* Initialize SysTick to 1ms interval using SystemCoreClock */
void S300_SysTick_Init(void)
{
    /* Reload for 1ms: SystemCoreClock/1000 - 1 */
    uint32_t reload = (SystemCoreClock / 1000u) - 1u;
    if (reload > SysTick_LOAD_RELOAD_Msk)
    {
        reload = SysTick_LOAD_RELOAD_Msk;
    }
    SysTick->LOAD = reload;
    SysTick->VAL  = 0;
    /* CLKSOURCE=1 (core clock), TICKINT=1 (enable interrupt), ENABLE=1 */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

uint32_t S300_SysTick_Millis(void)
{
    return s_ticks_ms;
}

void S300_DelayMs(uint32_t ms)
{
    uint32_t start = s_ticks_ms;
    while ((s_ticks_ms - start) < ms)
    {
        __NOP();
    }
}
