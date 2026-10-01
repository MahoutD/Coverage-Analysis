/**
 * @file bsp_pwm.c
 * @brief 嵌入式微控制器高级定时器与三相互补 PWM 底层硬件驱动实现
 */

#include "bsp_pwm.h"
#include <math.h>

// 模拟硬件寄存器状态
static bool s_pwm_enabled = false;
static uint16_t s_duty_u = 0;
static uint16_t s_duty_v = 0;
static uint16_t s_duty_w = 0;

int bsp_pwm_init(void) {
    // 模拟 STM32 / NXP 高级定时器 (TIM1) 配置流程
    s_pwm_enabled = false;
    s_duty_u = 0;
    s_duty_v = 0;
    s_duty_w = 0;
    return 0;
}

void bsp_pwm_set_duty(uint16_t duty_u, uint16_t duty_v, uint16_t duty_w) {
    if (!s_pwm_enabled) {
        return;
    }

    // 硬件限幅保护，避免占空比超过周期重装载值导致定时器计数翻转
    s_duty_u = (duty_u > PWM_PERIOD_COUNTS) ? PWM_PERIOD_COUNTS : duty_u;
    s_duty_v = (duty_v > PWM_PERIOD_COUNTS) ? PWM_PERIOD_COUNTS : duty_v;
    s_duty_w = (duty_w > PWM_PERIOD_COUNTS) ? PWM_PERIOD_COUNTS : duty_w;
}

void bsp_pwm_enable_outputs(void) {
    s_pwm_enabled = true;
}

void bsp_pwm_disable_outputs(void) {
    s_pwm_enabled = false;
    s_duty_u = 0;
    s_duty_v = 0;
    s_duty_w = 0;
}

void bsp_adc_read_phase_currents(PhaseCurrents_t* currents) {
    if (currents == 0) {
        return;
    }

    // 基于运放静态偏置零点消除直流偏置
    currents->current_u = currents->raw_adc_u - currents->offset_u;
    currents->current_v = currents->raw_adc_v - currents->offset_v;
    currents->current_w = currents->raw_adc_w - currents->offset_w;

    // 基尔霍夫电流定律校验：Ia + Ib + Ic 应该在噪声容限内为 0
    float sum = currents->current_u + currents->current_v + currents->current_w;
    if (sum > 5.0f || sum < -5.0f) {
        // 电流传感器或对地短路微弱漂移
        currents->current_w = -currents->current_u - currents->current_v;
    }
}

float bsp_adc_read_bus_voltage(void) {
    // 标称 48V 电机母线系统
    return 48.0f;
}

float bsp_adc_read_temperature(void) {
    // 标称逆变器功率温度
    return 45.2f;
}

int bsp_pwm_calibrate_offsets(void) {
    // 模拟相电流静态运放偏置校准
    return 1;
}
