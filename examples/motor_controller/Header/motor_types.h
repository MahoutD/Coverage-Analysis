/**
 * @file motor_types.h
 * @brief 车载高压无刷电机驱动与矢量控制核心数据结构定义
 * @note 符合 ISO 26262 ASIL-B 功能安全标准规范及 MISRA-C:2012 架构设计
 */

#ifndef MOTOR_TYPES_H
#define MOTOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 系统运行状态机枚举
 */
typedef enum {
    MOTOR_STATE_UNINITIALIZED = 0, ///< 未初始化状态
    MOTOR_STATE_STANDBY       = 1, ///< 待机就绪，逆变器下桥臂开通
    MOTOR_STATE_PRECHARGE     = 2, ///< 母线电容预充阶段
    MOTOR_STATE_ALIGNMENT     = 3, ///< 转子初始零位预定位与自校准
    MOTOR_STATE_CLOSED_LOOP   = 4, ///< 闭环 FOC 矢量正常运行状态
    MOTOR_STATE_DERATING      = 5, ///< 功率降额保护运行状态 (超温或轻微欠压)
    MOTOR_STATE_FAULT         = 6, ///< 故障闭锁，六路 PWM 紧急封锁
    MOTOR_STATE_EMERGENCY_STOP= 7  ///< 外部硬线急停触发安全断开
} MotorState_t;

/**
 * @brief 电机控制模式枚举
 */
typedef enum {
    CTRL_MODE_OPEN_LOOP   = 0,     ///< 开环标量恒频恒压 (V/f) 模式 (调试专用)
    CTRL_MODE_TORQUE      = 1,     ///< 电流环力矩控制模式 (内环)
    CTRL_MODE_SPEED       = 2,     ///< 双闭环速度控制模式 (外环速度 + 内环电流)
    CTRL_MODE_POSITION    = 3      ///< 三闭环位置伺服定位模式
} MotorControlMode_t;

/**
 * @brief 功能安全故障掩码标志定义 (按位定义)
 */
typedef enum {
    FAULT_NONE              = 0x0000, ///< 无故障
    FAULT_OVER_CURRENT      = 0x0001, ///< 相电流瞬间峰值过流 (硬件硬件比较器脱扣)
    FAULT_OVER_VOLTAGE      = 0x0002, ///< 直流母线过压 (再生制动反灌过高)
    FAULT_UNDER_VOLTAGE     = 0x0004, ///< 直流母线欠压 (电源瞬态跌落)
    FAULT_OVER_TEMPERATURE  = 0x0008, ///< 逆变器 IGBT/MOSFET 结温超限
    FAULT_HALL_SENSOR       = 0x0010, ///< 磁编码器/霍尔信号失效或不合理跳变
    FAULT_WATCHDOG_TIMEOUT  = 0x0020, ///< 任务调度看门狗喂狗超时
    FAULT_CAN_BUS_OFF       = 0x0040, ///< 车载 CAN 通信总线离线断链
    FAULT_CALIBRATION_FAIL  = 0x0080  ///< 上电相电流采样零点漂移超标
} FaultFlags_t;

/**
 * @brief 经典抗积分饱和 PID 控制器结构体
 */
typedef struct {
    float kp;             ///< 比例系数
    float ki;             ///< 积分系数
    float kd;             ///< 微分系数
    float integral;       ///< 积分累计量
    float prev_error;     ///< 上一次误差
    float out_min;        ///< 输出下限饱和限幅
    float out_max;        ///< 输出上限饱和限幅
    float anti_windup;    ///< 积分抗饱和削峰阈值
    float output;         ///< 当前周期控制器最终输出
} PIDController_t;

/**
 * @brief 三相采样相电流结构体
 */
typedef struct {
    float raw_adc_u;      ///< U 相原始采样电压
    float raw_adc_v;      ///< V 相原始采样电压
    float raw_adc_w;      ///< W 相原始采样电压
    float offset_u;       ///< U 相运放零点静态偏移
    float offset_v;       ///< V 相运放零点静态偏移
    float offset_w;       ///< W 相运放零点静态偏移
    float current_u;      ///< U 相校准后实际相电流 (A)
    float current_v;      ///< V 相校准后实际相电流 (A)
    float current_w;      ///< W 相校准后实际相电流 (A)
} PhaseCurrents_t;

/**
 * @brief FOC 矢量核心控制变量结构体
 */
typedef struct {
    // 磁场定向坐标变换变量
    float i_alpha;        ///< 静止坐标系 Alpha 轴电流 (A)
    float i_beta;         ///< 静止坐标系 Beta 轴电流 (A)
    float i_d;            ///< 旋转坐标系直轴有功/励磁电流 (A)
    float i_q;            ///< 旋转坐标系交轴力矩电流 (A)
    
    // 目标参考输入量
    float id_ref;         ///< D 轴参考目标电流 (通常弱磁前为 0A)
    float iq_ref;         ///< Q 轴参考目标力矩电流 (A)
    float speed_ref_rpm;  ///< 目标电机转速 (RPM)
    
    // 控制输出电压
    float v_d;            ///< D 轴输出控制电压 (V)
    float v_q;            ///< Q 轴输出控制电压 (V)
    float v_alpha;        ///< 反 Park 变换输出 Alpha 电压 (V)
    float v_beta;         ///< 反 Park 变换输出 Beta 电压 (V)
    
    // 空间矢量 PWM (SVPWM) 三相驱动占空比
    uint16_t duty_u;      ///< U 相定时器比较寄存器 CCR 占空比 (0 ~ 4095)
    uint16_t duty_v;      ///< V 相定时器比较寄存器 CCR 占空比 (0 ~ 4095)
    uint16_t duty_w;      ///< W 相定时器比较寄存器 CCR 占空比 (0 ~ 4095)
    
    // 转子角度与速度
    float electrical_angle; ///< 电角度 (rad, 0 ~ 2*PI)
    float mechanical_rpm;   ///< 机械转速 (RPM)
} FOCContext_t;

/**
 * @brief 系统遥测与实时健康状态总线结构体
 */
typedef struct {
    float speed_rpm;           ///< 实际机械转速 (RPM)
    float torque_est;          ///< 估算电磁转矩 (Nm)
    float vbus_voltage;        ///< 母线电压 (V)
    float total_current_rms;   ///< 综合有效相电流 (A)
    float inverter_temp_c;     ///< 逆变器温度 (℃)
    float motor_temp_c;        ///< 电机定子线圈温度 (℃)
    MotorState_t state;        ///< 当前系统运行状态
    MotorControlMode_t mode;   ///< 当前控制模式
    uint32_t active_faults;    ///< 当前激活的故障位掩码 (FaultFlags_t)
    uint32_t uptime_ms;        ///< 控制器上电运行时间 (ms)
} MotorTelemetry_t;

#define vbus_volts        vbus_voltage
#define ibus_amps         total_current_rms
#define temperature_c     inverter_temp_c
#define fault_flags       active_faults

#ifdef __cplusplus
}
#endif

#endif // MOTOR_TYPES_H
