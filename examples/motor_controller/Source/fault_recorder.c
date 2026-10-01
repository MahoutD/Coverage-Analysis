#include "fault_recorder.h"
#include <string.h>

void fault_recorder_init(FaultRecorder_t* recorder) {
    if (recorder == NULL) {
        return;
    }
    memset(recorder, 0, sizeof(FaultRecorder_t));
    recorder->record_count = 0;
    recorder->head_index = 0;
    recorder->has_fatal_fault = false;
}

bool fault_recorder_log(FaultRecorder_t* recorder, uint32_t dtc, FaultSeverity_t sev,
                        uint32_t timestamp_ms, float v, float t, float speed) {
    if (recorder == NULL) {
        return false;
    }

    /* 1. 查找是否已存在相同故障码 (去重与计数累加) */
    for (uint16_t i = 0; i < recorder->record_count; ++i) {
        if (recorder->records[i].dtc_code == dtc) {
            recorder->records[i].occurrence_count++;
            recorder->records[i].snapshot_voltage = v;
            recorder->records[i].snapshot_temperature = t;
            recorder->records[i].snapshot_speed = speed;
            if (sev >= FAULT_SEV_CRITICAL) {
                recorder->has_fatal_fault = true;
            }
            return true;
        }
    }

    /* 2. 插入新记录到环形队列缓冲区 */
    uint16_t slot = recorder->head_index;
    recorder->records[slot].dtc_code = dtc;
    recorder->records[slot].severity = sev;
    recorder->records[slot].timestamp_ms = timestamp_ms;
    recorder->records[slot].snapshot_voltage = v;
    recorder->records[slot].snapshot_temperature = t;
    recorder->records[slot].snapshot_speed = speed;
    recorder->records[slot].occurrence_count = 1;

    recorder->head_index = (recorder->head_index + 1) % MAX_FAULT_RECORDS;
    if (recorder->record_count < MAX_FAULT_RECORDS) {
        recorder->record_count++;
    }

    if (sev >= FAULT_SEV_CRITICAL) {
        recorder->has_fatal_fault = true;
    }

    return true;
}

bool fault_recorder_has_critical(const FaultRecorder_t* recorder) {
    if (recorder == NULL) {
        return false;
    }
    return recorder->has_fatal_fault;
}

void fault_recorder_clear(FaultRecorder_t* recorder) {
    if (recorder == NULL) {
        return;
    }
    memset(recorder, 0, sizeof(FaultRecorder_t));
}
