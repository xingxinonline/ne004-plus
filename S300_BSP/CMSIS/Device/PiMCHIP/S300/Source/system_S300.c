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
    /* Set vector table base to start of SRAM1 where .isr_vector is linked by sram.ld */
    SCB->VTOR = (uint32_t)0x20000000U;
}
