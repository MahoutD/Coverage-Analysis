#include "pid_controller.h"
#include <stddef.h>

void pid_init(PIDController_t* pid, float kp, float ki, float kd, float out_min, float out_max) {
    if (pid == NULL) {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    pid->integral_min = out_min * 0.5f;
    pid->integral_max = out_max * 0.5f;
    pid->deadband = 1.0f;
    pid->alpha_filter = 0.2f;

    pid->integral_sum = 0.0f;
    pid->prev_error = 0.0f;
    pid->filtered_deriv = 0.0f;
    pid->is_saturated = false;
}

void pid_reset(PIDController_t* pid) {
    if (pid == NULL) {
        return;
    }
    pid->integral_sum = 0.0f;
    pid->prev_error = 0.0f;
    pid->filtered_deriv = 0.0f;
    pid->is_saturated = false;
}

float pid_calculate(PIDController_t* pid, float setpoint, float actual, float dt) {
    if (pid == NULL) {
        return 0.0f;
    }

    if (dt <= 0.00001f) {
        dt = 0.001f;
    }

    float error = setpoint - actual;

    /* 1. 死区判定逻辑 */
    if (error > -pid->deadband && error < pid->deadband) {
        error = 0.0f;
    }

    /* 2. 比例项计算 */
    float p_out = pid->kp * error;

    /* 3. 积分项累加与抗饱和 (Anti-Windup Clamping) */
    if (!pid->is_saturated || (error * pid->integral_sum < 0.0f)) {
        pid->integral_sum += (pid->ki * error * dt);
        if (pid->integral_sum > pid->integral_max) {
            pid->integral_sum = pid->integral_max;
        } else if (pid->integral_sum < pid->integral_min) {
            pid->integral_sum = pid->integral_min;
        }
    }
    float i_out = pid->integral_sum;

    /* 4. 微分项与一阶低通滤波 */
    float raw_deriv = (error - pid->prev_error) / dt;
    pid->filtered_deriv = (pid->alpha_filter * raw_deriv) + ((1.0f - pid->alpha_filter) * pid->filtered_deriv);
    float d_out = pid->kd * pid->filtered_deriv;

    pid->prev_error = error;

    /* 5. 总输出叠加与限幅 */
    float total_out = p_out + i_out + d_out;

    if (total_out > pid->out_max) {
        total_out = pid->out_max;
        pid->is_saturated = true;
    } else if (total_out < pid->out_min) {
        total_out = pid->out_min;
        pid->is_saturated = true;
    } else {
        pid->is_saturated = false;
    }

    return total_out;
}
