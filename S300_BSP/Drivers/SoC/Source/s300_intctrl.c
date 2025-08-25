/* S300 INT CTRL driver implementation */

#include "s300_intctrl.h"

void S300_INTCTRL_SetIrqMode(uint8_t irq, S300_IntRouteMode mode)
{
    /* 两个配置寄存器：INT_CFG0 控制 IRQ0..15，每个 2bit；INT_CFG1 控制 IRQ16..31（及以上按实现扩展） */
    uint32_t ofs;
    uint8_t index;
    if (irq < 16) {
        ofs = INTCTRL_OFS_INT_CFG0;
        index = irq;
    } else {
        ofs = INTCTRL_OFS_INT_CFG1;
        index = (uint8_t)(irq - 16);
    }
    uint32_t shift = (uint32_t)index * 2u;
    volatile uint32_t *reg = &INTCTRL_REG(ofs);
    uint32_t v = *reg;
    v &= ~(0x3u << shift);
    v |= (((uint32_t)mode) & 0x3u) << shift;
    *reg = v;
}
