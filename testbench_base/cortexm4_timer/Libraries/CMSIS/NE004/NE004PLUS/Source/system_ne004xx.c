/*
 * @Author       : panxinhao
 * @Date         : 2023-07-25 16:21:23
 * @LastEditors  : panxinhao
 * @LastEditTime : 2024-04-10 18:24:40
 * @FilePath     : \\ne004-plus\\testbench_base\\cortexm4_timer\\Libraries\\CMSIS\\NE004\\NE004PLUS\\Source\\system_ne004xx.c
 * @Description  : 
 * 
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */


/* Copyright (c) 2012 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/

/* This file refers the CMSIS standard, some adjustments are made according to GigaDevice chips */

#include "ne004xx.h"

uint32_t SystemCoreClock;
uint32_t AHBClock;
uint32_t APBClock;

/* configure the system clock */
static void system_clock_config(void);

/*!
    \brief      setup the microcontroller system, initialize the system
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SystemInit(void)
{
    /* Configure the System clock source, PLL Multiplier and Divider factors,
        AHB/APBx prescalers and Flash settings */
    system_clock_config();
    SystemCoreClockUpdate();
    
    uint32_t temp = CHIP_MODE;

    switch (temp & 7)
    {
    case 1:
        /* code */
        SystemCoreClock = 200000000U;
        break;
    case 2:
        /* code */
        SystemCoreClock = 400000000U;
        break;
    case 4:
        /* code */
        SystemCoreClock = 25000000U;
        break;
    
    default:
        break;
    }

    switch ((temp >> 6) & 3)
    {
    case 0:
        /* code */
        AHBClock = SystemCoreClock / 2;
        APBClock = SystemCoreClock / 4;
        break;
    case 1:
        /* code */
        AHBClock = SystemCoreClock / 4;
        APBClock = SystemCoreClock / 8;
        break;
    case 2:
        /* code */
        AHBClock = SystemCoreClock / 2;
        APBClock = SystemCoreClock / 8;
        break;
    case 3:
        /* code */
        AHBClock = SystemCoreClock / 4;
        APBClock = SystemCoreClock / 16;
        break;
    
    default:
        break;
    }
}
/*!
    \brief      configure the system clock
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_config(void)
{
}

/*!
    \brief      update the SystemCoreClock with current core clock retrieved from cpu registers
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SystemCoreClockUpdate(void)
{
}
