#include "s300.h"

uint32_t SystemCoreClock = HSE_CLOCK_HZ; /* default 24MHz */

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = HSE_CLOCK_HZ;
}

void SystemInit(void)
{
    /* Minimal: keep default clock source = HSE per reset, no PLL. */
    SystemCoreClockUpdate();
    /* Enable FPU (CP10 & CP11 full access) before any FP instruction executes */
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1) && defined(__FPU_USED) && (__FPU_USED == 1)
    SCB->CPACR |= (0xFu << 20);
    __DSB();
    __ISB();
#endif
    /* Set vector table base to start of SRAM1 (where .isr_vector is linked) */
    SCB->VTOR = (uint32_t)0x20000000U; /* matches ld placing vector into SRAM1 */
}

/* ===== SysTick (moved from systick.c) ===== */
static volatile uint32_t s_ticks_ms = 0; /* 1ms tick counter */

void SysTick_Handler(void)
{
    s_ticks_ms++;
}

/* Initialize SysTick to 1ms interval using SystemCoreClock */
void S300_SysTick_Init(void)
{
    uint32_t reload = (SystemCoreClock / 1000u) - 1u; /* 1ms */
    if (reload > SysTick_LOAD_RELOAD_Msk) {
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
    while ((uint32_t)(s_ticks_ms - start) < ms) {
        __NOP();
    }
}
