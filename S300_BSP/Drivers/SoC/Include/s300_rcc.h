#ifndef S300_RCC_H
#define S300_RCC_H

#include <stdint.h>
#include "s300.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Enable APB0 clock bits */
static inline void S300_RCC_EnableAPB0(uint32_t mask)
{
    RCC_APB0_CLK_EN |= mask;
}

/* Enable APB1 clock bits */
static inline void S300_RCC_EnableAPB1(uint32_t mask)
{
    RCC_APB1_CLK_EN |= mask;
}

/* Release APB1 reset (set bit = deassert reset) */
static inline void S300_RCC_ReleaseAPB1Reset(uint32_t mask)
{
    RCC_APB1_RST_CTL |= mask;
}

#ifdef __cplusplus
}
#endif

#endif /* S300_RCC_H */
