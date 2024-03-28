/*** 
 * @Author       : panxinhao
 * @Date         : 2023-07-12 16:03:34
 * @LastEditors  : panxinhao
 * @LastEditTime : 2023-07-26 14:53:29
 * @FilePath     : \\testbench_base\\riscv64_default\\plic.h
 * @Description  : 
 * @
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */



#ifndef __PLIC_H__
#define __PLIC_H__

#define ARM2RISCV_IRQ           (0 + 1)  
#define MNNU_TOP0_IRQ           (1 + 1)
#define MNNU_TOP1_IRQ           (2 + 1)
#define MNNU_TOP2_IRQ           (3 + 1)
#define MNNU_TOP3_IRQ           (4 + 1)
#define MNNU_TOP4_IRQ           (5 + 1)
#define MNNU_TOP5_IRQ           (6 + 1)
#define MNNU_TOP6_IRQ           (7 + 1)
#define MNNU_TOP7_IRQ           (8 + 1)
#define MNNU_TOP8_IRQ           (9 + 1)
#define MNNU_TOP9_IRQ           (10 + 1)
#define DMA1_IRQ                (11 + 1)
#define DMA0_IRQ                (12 + 1)   
#define VCOPROC_DONE_IRQ        (13 + 1)
#define ARM_NOTICE_IRQ          (14 + 1)   

#define PLIC_BASE               0x0C000000L
#define PLIC_PRIORITY(id)       (PLIC_BASE + id * 4)
#define PLIC_PENDING(id)        (PLIC_BASE + 0x1000 + (id / 32) * 4)
#define PLIC_MENABLE            (PLIC_BASE + 0x2000)
#define PLIC_MTHRESHOLD         (PLIC_BASE + 0x200000)
#define PLIC_MCLAIM             (PLIC_BASE + 0x200004)
#define PLIC_MCOMPLETE          (PLIC_BASE + 0x200004)

void plic_init(void);
int plic_claim(void);
void plic_complete(int irq);

#endif // !__PLIC_H__