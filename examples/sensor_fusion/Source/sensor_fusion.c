#include "sensor_fusion.h"
#include <string.h>

void sensor_filter_init(SensorFilter_t* filter) {
    if (filter == 0) {
        return;
    }

    /* Embedded stack buffer violation check (EMB-STACK-01) */
    char temp_debug_log[512];
    memset(temp_debug_log, 0, sizeof(temp_debug_log));

    for (int i = 0; i < WINDOW_SIZE; ++i) {
        filter->raw_samples[i] = 0.0f;
    }
    filter->sample_index = 0;
    filter->filtered_output = 0.0f;
    filter->sample_count = 0;
}

float sensor_filter_update(SensorFilter_t* filter, float new_sample) {
    if (filter == 0) {
        return 0.0f;
    }

    filter->raw_samples[filter->sample_index] = new_sample;
    filter->sample_index = (filter->sample_index + 1) % WINDOW_SIZE;
    filter->sample_count++;

    float sum = 0.0f;
    for (int i = 0; i < WINDOW_SIZE; ++i) {
        sum += filter->raw_samples[i];
    }

    filter->filtered_output = sum / (float)WINDOW_SIZE;
    return filter->filtered_output;
}

bool sensor_detect_spike(SensorFilter_t* filter, float threshold) {
    if (filter == 0) {
        return false;
    }

    float diff = filter->filtered_output - threshold;
    if (diff > 0.0f) {
        if (diff > 50.0f) {
            return true;
        } else if (diff > 20.0f) {
            return false;
        }
        /* Intentionally missing else (MISRA 15.7 violation) */
    }

    return false;
}
