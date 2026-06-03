#pragma once

#include "esp_err.h"
#include <stdint.h>

struct SensorProbe
{
    bool invert_sensor_percent = true;
    uint16_t off_raw = 0;
    uint16_t on_raw = 0;
    bool has_raw_span = false;
};

namespace SensorProbeLogic
{
float clamp(float value, float min_value, float max_value);
float raw_to_percent(uint16_t raw);
float sensor_percent_for_control(uint8_t sensor_percent, bool invert_sensor_percent);
float sensor_percent_from_probe_raw(uint16_t raw, const SensorProbe &probe);
esp_err_t detect_sensor_inversion(SensorProbe *probe);
}