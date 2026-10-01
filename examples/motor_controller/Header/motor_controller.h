#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "pid_controller.h"
#include "fault_recorder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOTOR_STATE_IDLE = 0,
    MOTOR_STATE_ACCELERATING,
    MOTOR_STATE_STEADY,
    MOTOR_STATE_BRAKING,
    MOTOR_STATE_FAULT
} MotorState_t;

typedef struct {
    float current_speed_rpm;
    float target_speed_rpm;
    float temperature_celsius;
    float bus_voltage;
    uint32_t fault_code;
    MotorState_t state;
    PIDController_t speed_pid;
    FaultRecorder_t fault_recorder;
} MotorController_t;

void motor_controller_init(MotorController_t* controller);
void motor_controller_update(MotorController_t* controller, float speed_cmd, float temp, float voltage);
bool motor_check_safety(MotorController_t* controller);
void motor_emergency_shutdown(MotorController_t* controller);
void motor_controller_step(float speed_cmd, float temp, float voltage, float estop);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_CONTROLLER_H */
