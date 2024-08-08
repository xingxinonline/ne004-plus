/*** 
 * @Author       : xingxinonline
 * @Date         : 2024-08-07 15:37:40
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-07 17:59:36
 * @FilePath     : \\ne004-plus\\cortexm4_default\\servo.h
 * @Description  : 
 * @
 * @Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#ifndef SERVO_H
#define SERVO_H

#include "ne004xx_pwm.h"
#include <stdint.h>

#define MIN_ANGLE 0
#define MAX_ANGLE 180
#define MIN_PULSE_WIDTH 50000  // 0.5ms 对应 500us
#define MID_PULSE_WIDTH 150000  // 1.5ms 对应 1500us
#define MAX_PULSE_WIDTH 250000 // 2.5ms 对应 2500us
#define PERIOD_PULSE_WIDTH 2000000 // 20ms 对应 20000us

typedef struct {
    int currentAngle;
    int targetAngle;
    int stepSize;
    uint32_t lastUpdate;
    uint32_t current_ticks;
    int isMoving;
    PWMConfig pwmConfig;    // PWM配置
} ServoState;

// #define NUM_SERVOS 4
// ServoState servos[NUM_SERVOS];

void servo_init(ServoState* servo, uint32_t pwm_base, uint32_t channel_base, int initial_angle);
void set_pwm_angle(ServoState *servo, int angle);
void servo_set_angle(ServoState* servo, int angle);
void UpdateServos(void);

#endif // SERVO_H
