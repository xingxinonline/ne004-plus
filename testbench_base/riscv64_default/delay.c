/*
 * @Author       : panxinhao
 * @Date         : 2024-04-10 18:18:28
 * @LastEditors  : panxinhao
 * @LastEditTime : 2024-04-10 18:27:09
 * @FilePath     : \\ne004-plus\\testbench_base\\riscv64_default\\delay.c
 * @Description  : 
 * 
 * Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#include "delay.h"

void delayus(unsigned long dly)
{
    unsigned long Delay = dly * 400 / 4;
    do
    {
        asm("nop\nnop");
    }
    while (Delay --);
}
