/**
 * @file bsp_pwm.h
 * @brief 嵌入式微控制器高级定时器与三相互补 PWM 底层硬件驱动接口 (BSP)
 */

#ifndef BSP_PWM_H
#define BSP_PWM_H

#include "motor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_PERIOD_COUNTS      4000U  ///< 10kHz PWM 开关频率对应定时器重装载值
#define PWM_DEADTIME_COUNTS    80U    ///< 插入硬件互补死区时间 (防止桥臂直通)

/**
 * @brief 初始化三相高级定时器互补 PWM 及死区硬件发生器
 * @return 0 表示成功，非 0 表示底层外设配置异常
 */
int bsp_pwm_init(void);

/**
 * @brief 设置 U/V/W 三相定时器比较捕获通道占空比
 */
void bsp_pwm_set_duty(uint16_t duty_u, uint16_t duty_v, uint16_t duty_w);

/**
 * @brief 开启逆变器三相六路栅极驱动使能 (MOE = 1)
 */
void bsp_pwm_enable_outputs(void);

/**
 * @brief 紧急硬件关断三相高低侧 PWM 输出 (MOE = 0)
 */
void bsp_pwm_disable_outputs(void);

/**
 * @brief 采样三相下桥臂相电流
 */
void bsp_adc_read_phase_currents(PhaseCurrents_t* currents);

/**
 * @brief 读取高压母线电容实时直流分压 (V)
 */
float bsp_adc_read_bus_voltage(void);

/**
 * @brief 读取逆变器功率级 NTC 热敏电阻温度 (℃)
 */
float bsp_adc_read_temperature(void);

/**
 * @brief 执行相电流采样零点静态偏移自校准
 * @return 1 表示校准成功，0 表示运放零偏超差
 */
int bsp_pwm_calibrate_offsets(void);

/* 兼容性宏定义与硬件抽象别名 */
#define BSP_PWM_Init                 bsp_pwm_init
#define BSP_PWM_EnableInverter       bsp_pwm_enable_outputs
#define BSP_PWM_DisableInverter      bsp_pwm_disable_outputs
#define BSP_PWM_EmergencyTrip        bsp_pwm_disable_outputs
#define BSP_PWM_ReadPhaseCurrents    bsp_adc_read_phase_currents
#define BSP_PWM_CalibrateOffsets     bsp_pwm_calibrate_offsets
#define BSP_PWM_SetDutyCycles(u,v,w) bsp_pwm_set_duty((uint16_t)((u)*PWM_PERIOD_COUNTS), (uint16_t)((v)*PWM_PERIOD_COUNTS), (uint16_t)((w)*PWM_PERIOD_COUNTS))

#ifdef __cplusplus
}
#endif

#endif // BSP_PWM_H
