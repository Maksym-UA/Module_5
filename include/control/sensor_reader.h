#pragma once

#include "esp_err.h"
#include <stdint.h>

struct SensorSample
{
    uint16_t measured_raw = 0;
    uint16_t effective_raw = 0;
};

class SensorReader
{
public:
    SensorReader() = default;

    esp_err_t read(SensorSample *sample);
};