cmake_minimum_required(VERSION 3.16)
project(MotorControllerSample C)

set(CMAKE_C_STANDARD 99)

include_directories(Header)

add_executable(motor_embedded_app
    Source/main_embedded.c
    Source/can_comm.c
    Source/bsp_pwm.c
    Source/safety_monitor.c
    Source/pid_controller.c
    Source/fault_recorder.c
)

add_executable(motor_controller_app
    Source/motor_controller.c
    Source/pid_controller.c
    Source/fault_recorder.c
)
