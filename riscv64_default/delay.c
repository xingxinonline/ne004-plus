/*** 
 * @Author       : xingxinonline
 * @Date         : 2023-03-22 14:59:44
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2023-03-22 14:59:49
 * @FilePath     : \\testbench_base\\riscv64_default\\delay.c
 * @Description  : 
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All rights reserved.
 */

#include "delay.h"

void delayus(unsigned long dly)
{
    unsigned long i, j;
    for (i = 0; i < dly; i++)
    {
        for (j = 0; j < 20; j++)
            asm("nop\nnop\nnop");
    }
}
