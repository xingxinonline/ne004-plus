/* S300 INT CTRL driver: route IRQs to CM4/DSP or both */

#ifndef S300_INTCTRL_H
#define S300_INTCTRL_H

#include <stdint.h>
#include "s300.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INT_CTRL_BASE
#define INT_CTRL_BASE (0x40007000UL)
#endif

/* Mode field per IRQ (2 bits) */
typedef enum {
    S300_INT_MODE_BOTH = 0u, /* 同时上报 CPU 与 DSP */
    S300_INT_MODE_CM4  = 1u, /* 仅上报 CPU */
    S300_INT_MODE_DSP  = 2u, /* 仅上报 DSP */
    S300_INT_MODE_NONE = 3u  /* 不上报 */
} S300_IntRouteMode;

/* 基础寄存器访问 */
#define INTCTRL_REG(ofs) (*(volatile uint32_t *)((INT_CTRL_BASE) + (uint32_t)(ofs)))

/* 文档定义的寄存器偏移 */
enum {
    INTCTRL_OFS_INT_CFG0 = 0x00,
    INTCTRL_OFS_INT_CFG1 = 0x04,
    INTCTRL_OFS_INT_STU  = 0x08,
};

/* 设置单个 IRQ 的路由模式 */
void S300_INTCTRL_SetIrqMode(uint8_t irq, S300_IntRouteMode mode);

/* 便捷函数：将指定 IRQ 路由至 CM4（仅 CPU） */
static inline void S300_INTCTRL_RouteToCM4(uint8_t irq)
{
    S300_INTCTRL_SetIrqMode(irq, S300_INT_MODE_CM4);
}

/* 读取状态位（每一位对应一个 IRQ 是否有中断挂起，板级源应清除自身标志） */
static inline uint32_t S300_INTCTRL_GetStatus(void)
{
    return INTCTRL_REG(INTCTRL_OFS_INT_STU);
}

#ifdef __cplusplus
}
#endif

#endif /* S300_INTCTRL_H */
