

/***
 * @Author       : panxinhao
 * @Date         : 2023-04-06 17:41:11
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-04-06 17:44:38
 * @FilePath     : \\testbench_base\\riscv64_default\\tick.c
 * @Description  :
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All rights reserved.
 */

#include <stdio.h>

#define set_csr(reg, bit) ({ unsigned long __tmp;                           \
    if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32)             \
        asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit));   \
    else                                                                    \
        asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit));   \
            __tmp; })

#define clear_csr(reg, bit) ({ unsigned long __tmp;                         \
    if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32)             \
        asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit));   \
    else                                                                    \
        asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit));   \
            __tmp; })

volatile unsigned long tick_cycles = 0;

#define RT_TICK_PER_SECOND  2000000UL
#define MIP_MSIP            (1 << 3)
#define MIP_MTIP            (1 << 7)
/* Sets and enable the timer interrupt */
int rt_hw_tick_init(void)
{
    /* Clear the Machine-Timer bit in MIE */
    clear_csr(mie, MIP_MTIP);
    /* calculate the tick cycles */
    tick_cycles = 100000000UL / RT_TICK_PER_SECOND - 1;
    /* Set mtimecmp by core id */
    (*(uint64_t *)0x2004000UL) = (*(uint64_t *)0x200BFF8UL) + tick_cycles;
    /* Enable the Machine-Timer bit in MIE */
    set_csr(mie, MIP_MTIP);
    return 0;
}

/* Sets and enable the timer interrupt */
int rt_hw_soft_init(void)
{
    /* Clear the Machine-Timer bit in MIE */
    set_csr(mie, MIP_MSIP);
    return 0;
}
