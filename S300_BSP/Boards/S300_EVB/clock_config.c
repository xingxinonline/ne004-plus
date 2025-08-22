#include "s300.h"
#include "s300_rcc.h"

void Board_Clock_Init(void)
{
    /* For now, keep default 24MHz HSE; placeholder for future PLL, etc. */
    (void)SystemCoreClock; /* silence unused warning */
}
