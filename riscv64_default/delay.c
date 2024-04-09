/*
 * @Author       : panxinhao
 * @Date         : 2024-03-28 11:47:06
 * @LastEditors  : panxinhao
 * @LastEditTime : 2024-04-09 11:32:16
 * @FilePath     : \\ne004-plus\\riscv64_default\\delay.c
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
