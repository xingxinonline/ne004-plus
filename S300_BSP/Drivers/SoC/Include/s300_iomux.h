#ifndef S300_IOMUX_H
#define S300_IOMUX_H

#include <stdint.h>
#include "s300.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Helpers for IO Matrix & IO MUX configuration */
static inline void S300_IOMUX_SetPadFunc(uint32_t pad, uint32_t func)
{
    volatile uint32_t *reg = (pad < 16) ? &IO_MUX_CFG0 : (pad < 32) ? &IO_MUX_CFG1 : &IO_MUX_CFG2;
    uint32_t shift = (pad % 16) * 2u;
    uint32_t v = *reg;
    v &= ~(0x3u << shift);
    v |= ((func & 0x3u) << shift);
    *reg = v;
}

static inline void S300_IOMAT_SetIoFunc(uint32_t io, uint32_t func)
{
    volatile uint32_t *reg = (io < 16) ? &IO_MAT_CFG0 : (io < 32) ? &IO_MAT_CFG1 : &IO_MAT_CFG2;
    uint32_t shift = (io % 16) * 2u;
    uint32_t v = *reg;
    v &= ~(0x3u << shift);
    v |= ((func & 0x3u) << shift);
    *reg = v;
}

#ifdef __cplusplus
}
#endif

#endif /* S300_IOMUX_H */
