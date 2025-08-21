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
    /* Set vector table base to start of SRAM1 (where .isr_vector is linked) */
    SCB->VTOR = (uint32_t)0x20000000U; /* matches ld placing vector into SRAM1 */
}
