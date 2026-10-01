#ifndef SENSOR_FUSION_H
#define SENSOR_FUSION_H

#include <stdint.h>
#include <stdbool.h>

#define WINDOW_SIZE 8

typedef struct {
    float raw_samples[WINDOW_SIZE];
    uint8_t sample_index;
    float filtered_output;
    uint32_t sample_count;
} SensorFilter_t;

void sensor_filter_init(SensorFilter_t* filter);
float sensor_filter_update(SensorFilter_t* filter, float new_sample);
bool sensor_detect_spike(SensorFilter_t* filter, float threshold);

#endif /* SENSOR_FUSION_H */
