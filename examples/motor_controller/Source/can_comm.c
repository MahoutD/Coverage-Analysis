/**
 * @file can_comm.c
 * @brief 车载 CAN 2.0B/CAN-FD 通信收发协议栈实现
 */

#include "can_comm.h"
#include <string.h>

/* 私有静态诊断变量 */
static CanDiagnostics_t s_can_diag = {
    .rx_frame_count = 0,
    .tx_frame_count = 0,
    .crc_error_count = 0,
    .timeout_count = 0,
    .last_counter = 0,
    .silent_tick_ms = 0,
    .is_online = false
};

/**
 * @brief SAE J1850 CRC-8 快速查表 (多项式 0x1D, 初值 0xFF)
 */
static const uint8_t s_crc8_table[256] = {
    0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53,
    0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB,
    0xCD, 0xD0, 0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E,
    0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76,
    0x87, 0x9A, 0xBD, 0xA0, 0xF3, 0xEE, 0xC9, 0xD4,
    0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C,
    0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19,
    0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1,
    0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
    0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8,
    0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D,
    0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65,
    0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7,
    0x7C, 0x61, 0x46, 0x5B, 0x08, 0x15, 0x32, 0x2F,
    0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A,
    0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2,
    0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75,
    0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
    0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8,
    0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50,
    0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2,
    0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A,
    0x6C, 0x71, 0x56, 0x4B, 0x18, 0x05, 0x22, 0x3F,
    0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7,
    0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66,
    0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E,
    0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
    0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43,
    0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1,
    0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09,
    0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C,
    0x97, 0x8A, 0xAD, 0xB0, 0xE3, 0xFE, 0xD9, 0xC4
};

/**
 * @brief 初始化车载 CAN 通信硬件与协议栈
 */
bool CAN_Comm_Init(void)
{
    // 重置内部诊断状态
    s_can_diag.rx_frame_count = 0;
    s_can_diag.tx_frame_count = 0;
    s_can_diag.crc_error_count = 0;
    s_can_diag.timeout_count = 0;
    s_can_diag.last_counter = 0;
    s_can_diag.silent_tick_ms = 0;
    s_can_diag.is_online = true;

    return true;
}

/**
 * @brief 计算 SAE J1850 规范 CRC-8 校验和
 */
uint8_t CAN_Comm_CalcCRC8(const uint8_t* data, uint8_t len)
{
    if (data == NULL || len == 0) {
        return 0;
    }

    uint8_t crc = 0xFF; // 初值 0xFF
    for (uint8_t i = 0; i < len; ++i) {
        crc = s_crc8_table[crc ^ data[i]];
    }
    return crc ^ 0xFF;  // 异或结果掩码
}

/**
 * @brief 解析接收到的底层 CAN 报文
 */
int32_t CAN_Comm_ParseFrame(const CanFrame_t* frame, VcuCommandPacket_t* cmd_out)
{
    if (frame == NULL || cmd_out == NULL) {
        return -1;
    }

    // 仅接收目标 VCU 指令报文
    if (frame->id != CAN_ID_MOTOR_CMD || frame->dlc < 8) {
        return -1;
    }

    // 校验第 7 字节的 CRC8 校验码 (前 7 个字节参与校验)
    uint8_t computed_crc = CAN_Comm_CalcCRC8(frame->data, 7);
    if (computed_crc != frame->data[7]) {
        s_can_diag.crc_error_count++;
        return -2; // CRC 校验失配
    }

    // 解析控制字 Byte 0: [Bit0: Enable], [Bit1: Clear Fault], [Bit2..3: Mode]
    cmd_out->enable_cmd = (frame->data[0] & 0x01) != 0;
    cmd_out->clear_fault_cmd = (frame->data[0] & 0x02) != 0;
    cmd_out->mode = (MotorControlMode_t)((frame->data[0] >> 2) & 0x03);

    // 解析目标速度 Byte 1-2: 16位有符号整数，分辨率 0.5 RPM，偏移 0
    int16_t raw_speed = (int16_t)(((uint16_t)frame->data[1] << 8) | frame->data[2]);
    cmd_out->target_speed_rpm = (float)raw_speed * 0.5f;

    // 解析目标力矩 Byte 3-4: 16位有符号整数，分辨率 0.05 Nm，偏移 0
    int16_t raw_torque = (int16_t)(((uint16_t)frame->data[3] << 8) | frame->data[4]);
    cmd_out->target_torque_nm = (float)raw_torque * 0.05f;

    // 解析心跳计数器 Byte 5 低4位
    cmd_out->rolling_counter = frame->data[5] & 0x0F;
    cmd_out->crc8 = frame->data[7];

    // 更新诊断记录与心跳
    s_can_diag.rx_frame_count++;
    s_can_diag.silent_tick_ms = 0;
    s_can_diag.is_online = true;
    s_can_diag.last_counter = cmd_out->rolling_counter;

    return 0;
}

/**
 * @brief 构建并打包电机控制器运行状态报文 (CAN_ID_MOTOR_STATUS)
 */
bool CAN_Comm_PackStatusFrame(CanFrame_t* frame, const MotorTelemetry_t* status)
{
    if (frame == NULL || status == NULL) {
        return false;
    }

    frame->id = CAN_ID_MOTOR_STATUS;
    frame->dlc = 8;
    frame->is_extended = false;

    // Byte 0: 运行状态机及降额标志
    frame->data[0] = (uint8_t)(status->state & 0x0F);
    if (status->state == MOTOR_STATE_DERATING) {
        frame->data[0] |= 0x80;
    }

    // Byte 1-2: 实际转速反馈 (0.5 RPM/LSB)
    int16_t speed_scaled = (int16_t)(status->speed_rpm * 2.0f);
    frame->data[1] = (uint8_t)((speed_scaled >> 8) & 0xFF);
    frame->data[2] = (uint8_t)(speed_scaled & 0xFF);

    // Byte 3-4: 实际估算转矩 (0.05 Nm/LSB)
    int16_t torque_scaled = (int16_t)(status->torque_est * 20.0f);
    frame->data[3] = (uint8_t)((torque_scaled >> 8) & 0xFF);
    frame->data[4] = (uint8_t)(torque_scaled & 0xFF);

    // Byte 5: 逆变器温度 (偏移 -40 ℃, 1℃/LSB)
    float temp_c = status->temperature_c;
    if (temp_c < -40.0f) temp_c = -40.0f;
    if (temp_c > 210.0f) temp_c = 210.0f;
    frame->data[5] = (uint8_t)(temp_c + 40.0f);

    // Byte 6: 滚动计数器 (递增)
    static uint8_t s_tx_counter = 0;
    frame->data[6] = (s_tx_counter++) & 0x0F;

    // Byte 7: CRC-8 校验和
    frame->data[7] = CAN_Comm_CalcCRC8(frame->data, 7);

    s_can_diag.tx_frame_count++;
    return true;
}

/**
 * @brief 构建并打包系统遥测数据报文 (CAN_ID_MOTOR_TELEMETRY)
 */
bool CAN_Comm_PackTelemetryFrame(CanFrame_t* frame, const MotorTelemetry_t* status)
{
    if (frame == NULL || status == NULL) {
        return false;
    }

    frame->id = CAN_ID_MOTOR_TELEMETRY;
    frame->dlc = 8;
    frame->is_extended = false;

    // Byte 0-1: 母线电压 (0.1 V/LSB, 0~1000V)
    uint16_t vbus_scaled = (uint16_t)(status->vbus_volts * 10.0f);
    frame->data[0] = (uint8_t)((vbus_scaled >> 8) & 0xFF);
    frame->data[1] = (uint8_t)(vbus_scaled & 0xFF);

    // Byte 2-3: 母线电流 (0.1 A/LSB, 0~500A)
    uint16_t ibus_scaled = (uint16_t)(status->ibus_amps * 10.0f);
    frame->data[2] = (uint8_t)((ibus_scaled >> 8) & 0xFF);
    frame->data[3] = (uint8_t)(ibus_scaled & 0xFF);

    // Byte 4-5: 故障掩码高低字节
    frame->data[4] = (uint8_t)((status->fault_flags >> 8) & 0xFF);
    frame->data[5] = (uint8_t)(status->fault_flags & 0xFF);

    // Byte 6: 保留
    frame->data[6] = 0x00;

    // Byte 7: CRC-8 校验和
    frame->data[7] = CAN_Comm_CalcCRC8(frame->data, 7);

    s_can_diag.tx_frame_count++;
    return true;
}

/**
 * @brief 周期性检查通讯看门狗与总线离线状态
 */
bool CAN_Comm_CheckTimeout(uint32_t period_ms)
{
    s_can_diag.silent_tick_ms += period_ms;

    // 超过 100ms 未收到任何有效 VCU 指令报文即判定通讯丢失
    if (s_can_diag.silent_tick_ms >= 100) {
        s_can_diag.is_online = false;
        s_can_diag.timeout_count++;
        return false;
    }

    return true;
}

/**
 * @brief 获取 CAN 模块当前的运行诊断统计指标
 */
void CAN_Comm_GetDiagnostics(CanDiagnostics_t* diag)
{
    if (diag != NULL) {
        *diag = s_can_diag;
    }
}
