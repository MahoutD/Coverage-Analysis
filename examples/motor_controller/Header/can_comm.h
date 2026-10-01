/**
 * @file can_comm.h
 * @brief 车载 CAN 2.0B/CAN-FD 通信收发协议栈与报文解析器
 * @note 符合车载 ISO 11898 及 AUTOSAR Com/CanIf 分层架构规范
 */

#ifndef CAN_COMM_H
#define CAN_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "motor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CAN 报文标识符 (CAN ID) 映射表
 */
#define CAN_ID_MOTOR_CMD         0x101U  ///< 上层整车控制器 (VCU) 指令报文 ID
#define CAN_ID_MOTOR_STATUS      0x201U  ///< 电机控制器状态及转速反馈报文 ID
#define CAN_ID_MOTOR_TELEMETRY   0x202U  ///< 母线电压、电流与功率遥测报文 ID
#define CAN_ID_MOTOR_FAULT       0x203U  ///< 故障报警与诊断报文 ID

/**
 * @brief CAN 报文帧基础结构体
 */
typedef struct {
    uint32_t id;          ///< 报文标准/扩展帧 ID (11位或29位)
    uint8_t dlc;          ///< 数据场长度 (Data Length Code, 0~8)
    bool is_extended;     ///< 是否为扩展帧标志
    uint8_t data[8];      ///< 报文有效载荷数据缓冲区
} CanFrame_t;

/**
 * @brief 上层 VCU 指令数据包结构体 (已解析格式)
 */
typedef struct {
    bool enable_cmd;          ///< 电机使能/停机指令 (true: 允许驱动, false: 停机)
    bool clear_fault_cmd;     ///< 故障复位指令
    MotorControlMode_t mode;  ///< 控制模式指令
    float target_speed_rpm;   ///< 目标转速 (-6000.0 ~ +6000.0 RPM)
    float target_torque_nm;   ///< 目标力矩 (-250.0 ~ +250.0 Nm)
    uint8_t rolling_counter;  ///< 活生心跳计数器 (0~15, 用于掉线诊断)
    uint8_t crc8;             ///< 数据校验和 (CRC-8/SAE J1850)
} VcuCommandPacket_t;

/**
 * @brief CAN 通信总线统计与诊断信息
 */
typedef struct {
    uint32_t rx_frame_count;  ///< 接收有效报文总计数
    uint32_t tx_frame_count;  ///< 发送成功报文总计数
    uint32_t crc_error_count; ///< CRC 校验错误帧数
    uint32_t timeout_count;   ///< 接收通讯超时丢失帧数
    uint8_t  last_counter;    ///< 上一次活生计数
    uint32_t silent_tick_ms;  ///< 接收静默时长计数器 (毫秒)
    bool     is_online;       ///< 总线在线激活状态
} CanDiagnostics_t;

/**
 * @brief 初始化车载 CAN 通信模块
 * @return true 初始化成功, false 硬件控制器自检失败
 */
bool CAN_Comm_Init(void);

/**
 * @brief 处理底层 CAN 接收中断送入的报文
 * @param frame 接收到的底层 CAN 原始报文指针
 * @param cmd_out 解析后的 VCU 指令结构体输出指针
 * @return 0 成功解析, -1 ID未知或数据长度非法, -2 CRC校验失败, -3 活生计数跳变
 */
int32_t CAN_Comm_ParseFrame(const CanFrame_t* frame, VcuCommandPacket_t* cmd_out);

/**
 * @brief 构建并打包电机控制器运行状态报文 (CAN_ID_MOTOR_STATUS)
 * @param frame 输出待发送的 CAN 报文指针
 * @param status 当前电机遥测数据指针
 * @return true 打包完成, false 指针无效
 */
bool CAN_Comm_PackStatusFrame(CanFrame_t* frame, const MotorTelemetry_t* status);

/**
 * @brief 构建并打包系统遥测数据报文 (CAN_ID_MOTOR_TELEMETRY)
 * @param frame 输出待发送的 CAN 报文指针
 * @param status 当前电机遥测数据指针
 * @return true 打包完成, false 指针无效
 */
bool CAN_Comm_PackTelemetryFrame(CanFrame_t* frame, const MotorTelemetry_t* status);

/**
 * @brief 周期性检查通讯看门狗与总线离线状态 (通常 10ms 周期调用)
 * @param period_ms 本次调用的周期步长 (毫秒)
 * @return true 通信健康在线, false 通信已丢失或超时
 */
bool CAN_Comm_CheckTimeout(uint32_t period_ms);

/**
 * @brief 计算 SAE J1850 标准 CRC8 校验码
 * @param data 待校验数据缓冲区
 * @param len 数据字节长度
 * @return uint8_t 8位循环冗余校验码
 */
uint8_t CAN_Comm_CalcCRC8(const uint8_t* data, uint8_t len);

/**
 * @brief 获取 CAN 模块当前的运行诊断统计指标
 * @param diag 输出诊断结构体指针
 */
void CAN_Comm_GetDiagnostics(CanDiagnostics_t* diag);

#ifdef __cplusplus
}
#endif

#endif // CAN_COMM_H
