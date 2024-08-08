/*** 
 * @Author       : xingxinonline
 * @Date         : 2024-08-07 15:39:04
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-07 15:47:41
 * @FilePath     : \\ne004-plus\\cortexm4_default\\Libraries\\NE004xx_Driver\\Include\\ne004xx_pwm.h
 * @Description  : 
 * @
 * @Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

/***
 * @Author       : panxinhao
 * @Date         : 2023-07-27 16:18:15
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-07 15:14:30
 * @FilePath     : \\ne004-plus_kwsv\\ne004-plus\\cortexm4_default\\Libraries\\NE004xx_Driver\\Include\\ne004xx_pwm.h
 * @Description  :
 * @
 * @Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#ifndef __NE004XX_PWM_H__
#define __NE004XX_PWM_H__

#include "ne004xx.h"

#define PWM0                            PWM0_BASE
#define PWM1                            PWM1_BASE
#define PWM0_CH1                        PWM0_CH1_BASE
#define PWM0_CH2                        PWM0_CH2_BASE
#define PWM0_CH3                        PWM0_CH3_BASE
#define PWM1_CH1                        PWM1_CH1_BASE
#define PWM1_CH2                        PWM1_CH2_BASE
#define PWM1_CH3                        PWM1_CH3_BASE

// PWM 通用寄存器定义
#define PWM_EN_OFFSET                   0x00
#define PWM_MODE_OFFSET                 0x04
#define PWM_INT_MASK_OFFSET             0x08
#define PWM_INT_STU_OFFSET              0x0C

#define PWM_ONE_CYCLE_NUM_OFFSET        0x00
#define PWM_RISE_TIME_OFFSET            0x04
#define PWM_HLEVEL_NUM_OFFSET           0x08
#define PWM_NUM_CYCLE_OFFSET            0x0C

// 访问 PWM 通道的寄存器
#define PWM_ONE_CYCLE_NUM(base)         REG32((base) + PWM_ONE_CYCLE_NUM_OFFSET)
#define PWM_RISE_TIME(base)             REG32((base) + PWM_RISE_TIME_OFFSET)
#define PWM_HLEVEL_NUM(base)            REG32((base) + PWM_HLEVEL_NUM_OFFSET)
#define PWM_NUM_CYCLE(base)             REG32((base) + PWM_NUM_CYCLE_OFFSET)

// 访问 PWM 模块的控制寄存器
#define PWM_EN(base)                    REG32((base) + PWM_EN_OFFSET)
#define PWM_MODE(base)                  REG32((base) + PWM_MODE_OFFSET)
#define PWM_INT_MASK(base)              REG32((base) + PWM_INT_MASK_OFFSET)
#define PWM_INT_STU(base)               REG32((base) + PWM_INT_STU_OFFSET)

typedef struct {
    uint32_t pwmBase;       // PWM模块基地址
    uint32_t channelBase;   // PWM通道基地址
} PWMConfig;

void pwm_init(uint32_t pwm_base);
void pwm_channel_init(uint32_t channel_base, uint32_t one_cycle_num, uint32_t rise_time, uint32_t hlevel_num, uint32_t num_cycle);
void pwm_configure_output(uint32_t channel_base, uint32_t pulseWidth);
void pwm_enable(uint32_t pwm_base, uint8_t pwm_id);
void pwm_configure_input(uint32_t pwm_base, uint8_t pwm_id);

#endif // !__NE004XX_PWM_H__