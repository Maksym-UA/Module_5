#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

struct SensorSample
{
    uint16_t measured_raw = 0;
    uint16_t effective_raw = 0;
};

class SensorReader
{
public:
    explicit SensorReader(size_t sample_count);

    esp_err_t read(SensorSample *sample);

private:
    esp_err_t read_filtered_raw(uint16_t *raw_value);

    size_t sample_count_ = 0;
};