#pragma once

#include "esp_err.h"
#include <stdint.h>

struct SensorProbe // Represents the characteristics of the sensor for control logic.
{
    bool invert_sensor_percent = true;
    uint16_t off_raw = 0;
    uint16_t on_raw = 0;
    bool has_raw_span = false;
};

namespace SensorProbeLogic //
{
float clamp(float value, float min_value, float max_value); // Clamps a value to the specified range.
float raw_to_percent(uint16_t raw); // Converts raw sensor value to percentage.
float sensor_percent_for_control(uint8_t sensor_percent, bool invert_sensor_percent); // Adjusts sensor percentage based on inversion setting.
float sensor_percent_from_probe_raw(uint16_t raw, const SensorProbe &probe); // Calculates sensor percentage from probe raw values.
float sensor_percent_from_control_range(
    uint16_t raw,
    uint16_t raw_min,
    uint16_t raw_max,
    bool invert); // Calculates sensor percentage from control range.
esp_err_t detect_sensor_inversion(SensorProbe *probe);
}
