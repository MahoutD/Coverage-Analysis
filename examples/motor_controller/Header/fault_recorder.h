/**
 * @file fault_recorder.h
 * @brief 车载诊断系统故障冻结帧记录器 (符合 ISO 14229 UDS / OBD-II 规范)
 */
#ifndef FAULT_RECORDER_H
#define FAULT_RECORDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FAULT_RECORDS 16

typedef enum {
    FAULT_SEV_INFO = 0,
    FAULT_SEV_WARNING = 1,
    FAULT_SEV_CRITICAL = 2,
    FAULT_SEV_FATAL = 3
} FaultSeverity_t;

typedef struct {
    uint32_t dtc_code;          /**< 诊断故障代码 (DTC) */
    FaultSeverity_t severity;   /**< 故障严重程度 */
    uint32_t timestamp_ms;      /**< 故障发生时标 */
    float snapshot_voltage;     /**< 母线电压快照 */
    float snapshot_temperature; /**< 电机温度快照 */
    float snapshot_speed;       /**< 电机转速快照 */
    uint16_t occurrence_count;  /**< 重复出现计数 */
} FaultRecord_t;

typedef struct {
    FaultRecord_t records[MAX_FAULT_RECORDS];
    uint16_t record_count;
    uint16_t head_index;
    bool has_fatal_fault;
} FaultRecorder_t;

/**
 * @brief 初始化故障记录器
 */
void fault_recorder_init(FaultRecorder_t* recorder);

/**
 * @brief 记录一条故障冻结帧事件
 */
bool fault_recorder_log(FaultRecorder_t* recorder, uint32_t dtc, FaultSeverity_t sev,
                        uint32_t timestamp_ms, float v, float t, float speed);

/**
 * @brief 查询是否存在未消除的致命或严重故障
 */
bool fault_recorder_has_critical(const FaultRecorder_t* recorder);

/**
 * @brief 清空已记录的所有故障码与冻结帧
 */
void fault_recorder_clear(FaultRecorder_t* recorder);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_RECORDER_H */
