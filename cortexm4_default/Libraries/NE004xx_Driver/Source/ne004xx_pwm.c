/*
 * @Author       : xingxinonline
 * @Date         : 2024-08-07 14:59:26
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-07 15:46:48
 * @FilePath     : \\ne004-plus\\cortexm4_default\\Libraries\\NE004xx_Driver\\Source\\ne004xx_pwm.c
 * @Description  :
 *
 * Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include "ne004xx_pwm.h"

void pwm_init(uint32_t pwm_base) {
    // 初始化为默认值，禁用所有PWM通道
    PWM_EN(pwm_base) = 0x0;
    PWM_MODE(pwm_base) = 0x0;
    PWM_INT_MASK(pwm_base) = 0x0;
}

void pwm_channel_init(uint32_t channel_base, uint32_t one_cycle_num, uint32_t rise_time, uint32_t hlevel_num, uint32_t num_cycle) {
    PWM_ONE_CYCLE_NUM(channel_base) = one_cycle_num;
    PWM_RISE_TIME(channel_base) = rise_time;
    PWM_HLEVEL_NUM(channel_base) = hlevel_num;
    PWM_NUM_CYCLE(channel_base) = num_cycle;
}

void pwm_configure_output(uint32_t channel_base, uint32_t pulseWidth) {
    PWM_HLEVEL_NUM(channel_base) = pulseWidth;
}

void pwm_enable(uint32_t pwm_base, uint8_t pwm_id) {
    PWM_EN(pwm_base) |= (1 << pwm_id);
}

void pwm_configure_input(uint32_t pwm_base, uint8_t pwm_id) {
    PWM_MODE(pwm_base) |= (1 << pwm_id);
}
