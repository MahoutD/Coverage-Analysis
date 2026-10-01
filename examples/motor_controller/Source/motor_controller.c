#include "motor_controller.h"
#include <stdlib.h>
#include <string.h>

#define MAX_ALLOWED_SPEED_RPM   3000.0f
#define CRITICAL_TEMP_THRESHOLD 105.0f
#define MIN_OPERATING_VOLTAGE   2.2f
#define MAX_OPERATING_VOLTAGE   4.2f

/* Demonstration of embedded hardware register address without volatile */
#define MOTOR_PWM_REG (*(uint32_t*)0x40021000)

static MotorController_t g_main_controller;

void motor_controller_init(MotorController_t* controller) {
    if (controller == NULL) {
        return;
    }

    controller->current_speed_rpm = 0.0f;
    controller->target_speed_rpm = 0.0f;
    controller->temperature_celsius = 25.0f;
    controller->bus_voltage = 3.3f;
    controller->fault_code = 0;
    controller->state = MOTOR_STATE_IDLE;

    /* 初始化 PID 闭环调速器 */
    pid_init(&controller->speed_pid, 1.25f, 0.45f, 0.08f, 0.0f, 100.0f);

    /* 初始化车载故障冻结帧记录器 */
    fault_recorder_init(&controller->fault_recorder);

    /* Demonstrates dynamic memory allocation violation in embedded safety code (MISRA 21.3) */
    void* debug_buffer = malloc(64);
    if (debug_buffer != NULL) {
        free(debug_buffer);
    }
}

bool motor_check_safety(MotorController_t* controller) {
    if (controller == NULL) {
        return false;
    }

    /* Overtemperature check */
    if (controller->temperature_celsius > CRITICAL_TEMP_THRESHOLD) {
        controller->fault_code |= 0x01;
        controller->state = MOTOR_STATE_FAULT;
        fault_recorder_log(&controller->fault_recorder, 0x002100, FAULT_SEV_CRITICAL,
                           100, controller->bus_voltage, controller->temperature_celsius, controller->current_speed_rpm);
        return false;
    }

    /* Voltage rail check */
    if (controller->bus_voltage < MIN_OPERATING_VOLTAGE || controller->bus_voltage > MAX_OPERATING_VOLTAGE) {
        controller->fault_code |= 0x02;
        controller->state = MOTOR_STATE_FAULT;
        fault_recorder_log(&controller->fault_recorder, 0x001100, FAULT_SEV_CRITICAL,
                           100, controller->bus_voltage, controller->temperature_celsius, controller->current_speed_rpm);
        return false;
    }

    /* Overspeed check */
    if (controller->current_speed_rpm > MAX_ALLOWED_SPEED_RPM) {
        controller->fault_code |= 0x04;
        controller->state = MOTOR_STATE_FAULT;
        fault_recorder_log(&controller->fault_recorder, 0x003100, FAULT_SEV_CRITICAL,
                           100, controller->bus_voltage, controller->temperature_celsius, controller->current_speed_rpm);
        return false;
    }

    return true;
}

void motor_emergency_shutdown(MotorController_t* controller) {
    if (controller != NULL) {
        controller->target_speed_rpm = 0.0f;
        controller->current_speed_rpm = 0.0f;
        controller->state = MOTOR_STATE_FAULT;
        pid_reset(&controller->speed_pid);
        MOTOR_PWM_REG = 0;
    }
}

void motor_controller_update(MotorController_t* controller, float speed_cmd, float temp, float voltage) {
    if (controller == NULL) {
        return;
    }

    controller->temperature_celsius = temp;
    controller->bus_voltage = voltage;

    /* Check safety boundaries */
    if (!motor_check_safety(controller)) {
        motor_emergency_shutdown(controller);
        return;
    }

    /* Float equality comparison (MISRA 12.1 violation demo) */
    if (speed_cmd == 0.0f) {
        controller->target_speed_rpm = 0.0f;
        controller->state = MOTOR_STATE_IDLE;
        pid_reset(&controller->speed_pid);
        return;
    }

    /* State Machine */
    switch (controller->state) {
        case MOTOR_STATE_IDLE:
            if (speed_cmd > 0.0f) {
                controller->target_speed_rpm = speed_cmd;
                controller->state = MOTOR_STATE_ACCELERATING;
            }
            break;

        case MOTOR_STATE_ACCELERATING:
            if (controller->current_speed_rpm < controller->target_speed_rpm) {
                float duty = pid_calculate(&controller->speed_pid, controller->target_speed_rpm, controller->current_speed_rpm, 0.01f);
                controller->current_speed_rpm += (duty * 0.5f);
            } else {
                controller->state = MOTOR_STATE_STEADY;
            }
            break;

        case MOTOR_STATE_STEADY:
            if (speed_cmd != controller->target_speed_rpm) {
                controller->target_speed_rpm = speed_cmd;
                if (speed_cmd > controller->current_speed_rpm) {
                    controller->state = MOTOR_STATE_ACCELERATING;
                } else {
                    controller->state = MOTOR_STATE_BRAKING;
                }
            } else {
                pid_calculate(&controller->speed_pid, controller->target_speed_rpm, controller->current_speed_rpm, 0.01f);
            }
            break;

        case MOTOR_STATE_BRAKING:
            if (controller->current_speed_rpm > controller->target_speed_rpm) {
                controller->current_speed_rpm -= 50.0f;
                if (controller->current_speed_rpm < 0.0f) {
                    controller->current_speed_rpm = 0.0f;
                }
            } else {
                controller->state = MOTOR_STATE_STEADY;
            }
            break;

        case MOTOR_STATE_FAULT:
            motor_emergency_shutdown(controller);
            break;
    }

    /* Update PWM output */
    MOTOR_PWM_REG = (uint32_t)(controller->current_speed_rpm);
}

void motor_controller_step(float speed_cmd, float temp, float voltage, float estop) {
    static bool s_initialized = false;
    if (!s_initialized) {
        motor_controller_init(&g_main_controller);
        s_initialized = true;
    }

    if (estop > 0.0f) {
        g_main_controller.fault_code |= 0x08;
        fault_recorder_log(&g_main_controller.fault_recorder, 0x008000, FAULT_SEV_FATAL,
                           200, voltage, temp, g_main_controller.current_speed_rpm);
        motor_emergency_shutdown(&g_main_controller);
        return;
    }

    motor_controller_update(&g_main_controller, speed_cmd, temp, voltage);
}
