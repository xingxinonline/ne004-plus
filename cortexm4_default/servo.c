/*
 * @Author       : xingxinonline
 * @Date         : 2024-08-07 15:37:32
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-07 18:38:09
 * @FilePath     : \\ne004-plus\\cortexm4_default\\servo.c
 * @Description  :
 *
 * Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include "servo.h"
#include "ne004xx_pwm.h"
#include <stdint.h>

extern volatile uint32_t msTicks;

void servo_init(ServoState *servo, uint32_t pwm_base, uint32_t channel_base, int initial_angle)
{
    servo->currentAngle = initial_angle;
    servo->targetAngle = initial_angle;
    servo->stepSize = 1;  // 每次移动1度
    servo->lastUpdate = 0;
    servo->current_ticks = 0;
    servo->isMoving = 0;  // 初始状态为停止
    servo->pwmConfig.pwmBase = pwm_base;
    servo->pwmConfig.channelBase = channel_base;
    pwm_channel_init(servo->pwmConfig.channelBase, PERIOD_PULSE_WIDTH, 1, MID_PULSE_WIDTH, 0xFFFFFFFF);
    uint8_t pwm_id = (servo->pwmConfig.channelBase - servo->pwmConfig.pwmBase - 0x10) / 0x10 ;
    pwm_enable(servo->pwmConfig.pwmBase, pwm_id); // 假设使用通道0，按实际需要修改
}

void set_pwm_angle(ServoState *servo, int angle)
{
    // 限制角度范围在 0° 到 180° 之间
    if (angle < MIN_ANGLE)
    {
        angle = MIN_ANGLE;
    }
    else if (angle > MAX_ANGLE)
    {
        angle = MAX_ANGLE;
    }
    // 根据角度计算对应的脉宽
    uint32_t pulseWidth = MIN_PULSE_WIDTH + (angle * (MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) / MAX_ANGLE);
    // 设置 PWM 输出
    pwm_configure_output(servo->pwmConfig.channelBase, pulseWidth);
}

void servo_set_angle(ServoState *servo, int angle)
{
    // 限制输入角度范围在 0° 到 180° 之间
    if (angle < MIN_ANGLE) {
        angle = MIN_ANGLE;
    } else if (angle > MAX_ANGLE) {
        angle = MAX_ANGLE;
    }

    servo->targetAngle = angle;
    servo->isMoving = 1;  // 标记舵机开始移动
}

void servo_update(ServoState *servo)
{
   
}

