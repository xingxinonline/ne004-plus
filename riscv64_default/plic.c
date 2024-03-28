/*
 * @Author       : panxinhao
 * @Date         : 2023-06-13 15:53:38
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-07-21 15:24:00
 * @FilePath     : \\testbench_base\\riscv64_default\\plic.c
 * @Description  : 
 * 
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#include "plic.h"

#include <stdio.h>


void plic_init(void)
{
    int source = plic_claim();
    while (source)
    {
        /* code */
        plic_complete(source);
        source = plic_claim();
    }
    
    /* 
     *
     * Set priority for IRQ .
     *
     * Each PLIC interrupt source can be assigned a priority by writing
     * to its 32-bit memory-mapped priority register.
     * The QEMU-virt (the same as FU540-C000) supports 7 levels of priority.
     * A priority value of 0 is reserved to mean "never interrupt" and
     * effectively disables the interrupt.
     * Priority 1 is the lowest active priority, and priority 7 is the highest.
     * Ties between global interrupts of the same priority are broken by
     * the Interrupt ID; interrupts with the lowest ID have the highest
     * effective priority.
     */
    *(uint32_t *)PLIC_PRIORITY(ARM2RISCV_IRQ) = 1; // 0xC000004 = 1 
    *(uint32_t *)PLIC_PRIORITY(DMA1_IRQ) = 1; // 0xC000030 = 1 
    *(uint32_t *)PLIC_PRIORITY(ARM_NOTICE_IRQ) = 1; // 0xC00003C = 1 
    /*
     * Enable DMA1
     *
     * Each global interrupt can be enabled by setting the corresponding
     * bit in the enables registers.
     */
    *(uint32_t *)PLIC_MENABLE = (1 << ARM_NOTICE_IRQ) | (1 << DMA1_IRQ) | (1 << ARM2RISCV_IRQ); // 0xC002000 = 0x9000
    /*
     * Set priority threshold for DMA1.
     *
     * PLIC will mask all interrupts of a priority less than or equal to threshold.
     * Maximum threshold is 7.
     * For example, a threshold value of zero permits all interrupts with
     * non-zero priority, whereas a value of 7 masks all interrupts.
     * Notice, the threshold is global for PLIC, not for each interrupt source.
     */
    *(uint32_t *)PLIC_MTHRESHOLD = 0; // 0xC200000 = 0x0
}

/*
 * DESCRIPTION:
 *  Query the PLIC what interrupt we should serve.
 *  Perform an interrupt claim by reading the claim register, which
 *  returns the ID of the highest-priority pending interrupt or zero if there
 *  is no pending interrupt.
 *  A successful claim also atomically clears the corresponding pending bit
 *  on the interrupt source.
 * RETURN VALUE:
 *  the ID of the highest-priority pending interrupt or zero if there
 *  is no pending interrupt.
 */
int plic_claim(void)
{
    int irq = *(uint32_t *)PLIC_MCLAIM;
    return irq;
}

/*
 * DESCRIPTION:
 *  Writing the interrupt ID it received from the claim (irq) to the
 *  complete register would signal the PLIC we've served this IRQ.
 *  The PLIC does not check whether the completion ID is the same as the
 *  last claim ID for that target. If the completion ID does not match an
 *  interrupt source that is currently enabled for the target, the completion
 *  is silently ignored.
 * RETURN VALUE: none
 */
void plic_complete(int irq)
{
    *(uint32_t *)PLIC_MCOMPLETE = irq;
}
