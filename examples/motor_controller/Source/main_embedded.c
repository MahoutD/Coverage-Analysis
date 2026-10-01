/**
 * @file main_embedded.c
 * @brief 车载永磁同步电机(PMSM)控制器主入口与多速率周期调度器
 * @note 遵循 MISRA-C:2012 架构规范与 ISO 26262 ASIL-B 功能安全周期执行模型
 */

#include "motor_types.h"
#include "bsp_pwm.h"
#include "safety_monitor.h"
#include "can_comm.h"
#include <stdio.h>
#include <stdbool.h>

/* 全局系统遥测与状态结构 */
static MotorTelemetry_t g_telemetry = {
    .state = MOTOR_STATE_UNINITIALIZED,
    .mode = CTRL_MODE_SPEED,
    .speed_rpm = 0.0f,
    .torque_est = 0.0f,
    .vbus_volts = 48.0f,
    .ibus_amps = 0.0f,
    .temperature_c = 35.0f,
    .fault_flags = FAULT_NONE
};

static uint32_t g_system_tick_ms = 0;
static bool g_system_running = true;

/**
 * @brief 10kHz 高频矢量控制电流环 (模拟硬件定时器更新中断 ISR)
 */
void ISR_10kHz_CurrentLoop(void)
{
    // 1. 读取三相电流与母线电压
    PhaseCurrents_t currents;
    bsp_adc_read_phase_currents(&currents);
    g_telemetry.ibus_amps = currents.current_u;

    // 2. 只有在闭环或降额状态下才更新 PWM 矢量占空比
    if (g_telemetry.state == MOTOR_STATE_CLOSED_LOOP || g_telemetry.state == MOTOR_STATE_DERATING) {
        // 简化的空间矢量调制占空比计算
        float duty_u = 0.5f + 0.3f;
        float duty_v = 0.5f - 0.15f;
        float duty_w = 0.5f - 0.15f;
        BSP_PWM_SetDutyCycles(duty_u, duty_v, duty_w);
    } else {
        // 非运行状态保持安全关断
        bsp_pwm_disable_outputs();
    }
}

/**
 * @brief 100Hz 中频速度与状态机主控任务 (10ms 周期执行)
 */
void Task_100Hz_StateMachine(void)
{
    // 1. 喂安全看门狗
    safety_watchdog_kick();

    // 2. 模拟母线电压与温度动态波动
    g_telemetry.vbus_volts = 48.0f + 0.5f;
    g_telemetry.temperature_c = 42.0f;

    // 3. 执行功能安全全维度健康检查 (ASIL-B 门限判定)
    uint32_t active_faults = safety_monitor_evaluate(&g_telemetry);
    bool is_safe = (active_faults == FAULT_NONE);

    // 4. 周期检查 CAN 总线通信超时
    if (!CAN_Comm_CheckTimeout(10)) {
        safety_monitor_report_fault(FAULT_CAN_BUS_OFF);
        is_safe = false;
    }

    // 5. 获取最新故障掩码
    g_telemetry.fault_flags = safety_monitor_get_faults();

    // 6. 核心运行状态机转移逻辑
    switch (g_telemetry.state) {
        case MOTOR_STATE_UNINITIALIZED:
            // 初始未完成状态，等待初始化
            break;

        case MOTOR_STATE_STANDBY:
            if (!is_safe) {
                g_telemetry.state = MOTOR_STATE_FAULT;
            } else {
                // 模拟收到使能指令，转入电容预充
                g_telemetry.state = MOTOR_STATE_PRECHARGE;
            }
            break;

        case MOTOR_STATE_PRECHARGE:
            // 预充电完成检查，母线电压建立平稳后转入自定位
            if (g_telemetry.vbus_volts > 36.0f) {
                g_telemetry.state = MOTOR_STATE_ALIGNMENT;
            }
            break;

        case MOTOR_STATE_ALIGNMENT:
            // 完成转子初角磁极对齐，开启动力逆变器
            bsp_pwm_enable_outputs();
            g_telemetry.state = MOTOR_STATE_CLOSED_LOOP;
            break;

        case MOTOR_STATE_CLOSED_LOOP:
            if (!is_safe) {
                g_telemetry.state = MOTOR_STATE_FAULT;
                bsp_pwm_disable_outputs();
            } else if (g_telemetry.temperature_c > 85.0f) {
                // 达到降额温控线，进入保护降额模式
                g_telemetry.state = MOTOR_STATE_DERATING;
            } else {
                // 闭环稳态转速爬升跟踪
                if (g_telemetry.speed_rpm < 2000.0f) {
                    g_telemetry.speed_rpm += 20.0f;
                }
            }
            break;

        case MOTOR_STATE_DERATING:
            if (!is_safe) {
                g_telemetry.state = MOTOR_STATE_FAULT;
                bsp_pwm_disable_outputs();
            } else if (g_telemetry.temperature_c <= 75.0f) {
                // 温度恢复正常，切回全功率闭环
                g_telemetry.state = MOTOR_STATE_CLOSED_LOOP;
            }
            break;

        case MOTOR_STATE_FAULT:
            // 故障状态下确保 PWM 封锁
            bsp_pwm_disable_outputs();
            break;

        case MOTOR_STATE_EMERGENCY_STOP:
        default:
            bsp_pwm_disable_outputs();
            break;
    }
}

/**
 * @brief 10Hz 低频总线通讯与诊断广播任务 (100ms 周期执行)
 */
void Task_10Hz_Diagnostics(void)
{
    CanFrame_t tx_status_frame;
    CanFrame_t tx_telemetry_frame;

    // 1. 打包状态上报报文并广播
    if (CAN_Comm_PackStatusFrame(&tx_status_frame, &g_telemetry)) {
        // 模拟底层驱动发送
    }

    // 2. 打包电气参数遥测报文并广播
    if (CAN_Comm_PackTelemetryFrame(&tx_telemetry_frame, &g_telemetry)) {
        // 模拟底层驱动发送
    }
}

/**
 * @brief 系统硬件板级支持包与底层外设上电自检与初始化
 */
bool System_Hardware_Init(void)
{
    // 1. 初始化定时器与三相驱动桥
    if (bsp_pwm_init() != 0) {
        return false;
    }

    // 2. 初始化 ISO 26262 功能安全监控单元
    safety_monitor_init();

    // 3. 初始化车载 CAN 通信总线协议栈
    if (!CAN_Comm_Init()) {
        return false;
    }

    // 4. 执行相电流传感器上电静态校准
    if (!bsp_pwm_calibrate_offsets()) {
        safety_monitor_report_fault(FAULT_CALIBRATION_FAIL);
        return false;
    }

    return true;
}

/**
 * @brief 嵌入式固件主入口函数
 */
int main(void)
{
    // 1. 底层板级硬件与驱动协议初始化
    if (!System_Hardware_Init()) {
        g_telemetry.state = MOTOR_STATE_FAULT;
    } else {
        g_telemetry.state = MOTOR_STATE_STANDBY;
    }

    // 2. 确定性周期多速率主轮询调度循环 (模拟 RTOS 或裸机大循环)
    uint32_t counter_100hz = 0;
    uint32_t counter_10hz = 0;

    while (g_system_running && g_system_tick_ms < 5000) {
        // 模拟 1ms 硬件定时器系统滴答
        g_system_tick_ms++;
        counter_100hz++;
        counter_10hz++;

        // 模拟 10kHz 中断 (每毫秒调用 10 次)
        for (int i = 0; i < 10; ++i) {
            ISR_10kHz_CurrentLoop();
        }

        // 10ms 周期任务 (100Hz)
        if (counter_100hz >= 10) {
            counter_100hz = 0;
            Task_100Hz_StateMachine();
        }

        // 100ms 周期任务 (10Hz)
        if (counter_10hz >= 100) {
            counter_10hz = 0;
            Task_10Hz_Diagnostics();
        }
    }

    // 安全关断退出
    bsp_pwm_disable_outputs();
    return 0;
}
