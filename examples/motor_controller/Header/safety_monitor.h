/**
 * @file safety_monitor.h
 * @brief 车规级 ASIL-B 嵌入式功能安全监控、看门狗守护与故障安全状态机
 */

#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include "motor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// 安全阈值定义
#define SAFETY_MAX_BUS_VOLTAGE      58.0f   ///< 母线过压阈值 (V)
#define SAFETY_MIN_BUS_VOLTAGE      18.0f   ///< 母线欠压阈值 (V)
#define SAFETY_MAX_PHASE_CURRENT    45.0f   ///< 瞬间峰值过流硬件阈值 (A)
#define SAFETY_DERATE_TEMP_C        95.0f   ///< 结温功率降额警戒阈值 (℃)
#define SAFETY_TRIP_TEMP_C          115.0f  ///< 结温超温保护跳闸阈值 (℃)
#define SAFETY_WATCHDOG_LIMIT_TICKS 10U     ///< 调度器喂狗最大允许失步周期数

/**
 * @brief 初始化安全监控子系统
 */
void safety_monitor_init(void);

/**
 * @brief 周期性安全健康度全量扫描 (100Hz 周期任务中调用)
 * @param telem 当前系统传感器输入与状态
 * @return 活跃的故障位掩码
 */
uint32_t safety_monitor_evaluate(const MotorTelemetry_t* telem);

/**
 * @brief 主控任务向安全看门狗喂狗
 */
void safety_watchdog_kick(void);

/**
 * @brief 触发紧急硬线安全断开 (Emergency Shutdown)
 */
void safety_trigger_emergency_stop(const char* reason);

/**
 * @brief 尝试清除所有非闭锁类可恢复故障
 * @return true 表示成功恢复，false 表示依然存在严重硬件级持续故障
 */
bool safety_clear_faults(void);

/**
 * @brief 获取当前系统最高严重等级的安全动作指示
 */
MotorState_t safety_get_recommended_state(MotorState_t current_state);

/**
 * @brief 获取当前已锁存的故障状态掩码
 */
uint32_t safety_monitor_get_faults(void);

/**
 * @brief 主动向安全监控器上报特定故障
 */
void safety_monitor_report_fault(uint32_t fault);

/* 兼容性别名宏定义 */
#define Safety_Monitor_Init               safety_monitor_init
#define Safety_Monitor_KickWatchdog       safety_watchdog_kick
#define Safety_Monitor_GetFaultFlags      safety_monitor_get_faults
#define Safety_Monitor_ReportFault        safety_monitor_report_fault

#ifdef __cplusplus
}
#endif

#endif // SAFETY_MONITOR_H
