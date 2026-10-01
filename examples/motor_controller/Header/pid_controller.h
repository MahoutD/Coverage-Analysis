/**
 * @file pid_controller.h
 * @brief 工业级闭环速度与电流 PID 调节器 (带抗积分饱和与微分一阶低通滤波)
 * 符合 ISO 26262 车规级控制算法规范
 */
#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;               /**< 比例增益比例系数 */
    float ki;               /**< 积分增益时间常数 */
    float kd;               /**< 微分增益响应因子 */
    float integral_min;     /**< 积分项输出下限抗饱和阈值 */
    float integral_max;     /**< 积分项输出上限抗饱和阈值 */
    float out_min;          /**< 调节器最终输出下限 (如 0.0f) */
    float out_max;          /**< 调节器最终输出上限 (如 100.0f PWM 占空比) */
    float deadband;         /**< 误差死区阈值 (避免零点附近高频抖动) */
    float alpha_filter;     /**< 微分项一阶低通滤波系数 (0.0 ~ 1.0) */

    float integral_sum;     /**< 内部积分状态累加和 */
    float prev_error;       /**< 上一控制周期的误差量 */
    float filtered_deriv;   /**< 经过一阶滤波的微分量 */
    bool is_saturated;      /**< 当前输出是否已进入限幅饱和区 */
} PIDController_t;

/**
 * @brief 初始化 PID 调节器参数
 */
void pid_init(PIDController_t* pid, float kp, float ki, float kd, float out_min, float out_max);

/**
 * @brief 复位 PID 调节器内部状态 (清零积分与微分历史)
 */
void pid_reset(PIDController_t* pid);

/**
 * @brief 执行单步 PID 闭环计算
 * @param pid 调节器实例
 * @param setpoint 目标设定值 (如目标转速 RPM)
 * @param actual 实际采样反馈值 (如霍尔编码器实际转速)
 * @param dt 离散调度周期时长 (秒，如 0.001f 代表 1ms)
 * @return 调节器限幅输出值 (如 PWM 占空比百分比)
 */
float pid_calculate(PIDController_t* pid, float setpoint, float actual, float dt);

#ifdef __cplusplus
}
#endif

#endif /* PID_CONTROLLER_H */
