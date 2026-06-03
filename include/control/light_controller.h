#pragma once

#include "control/pid_controller.h"
#include "control/sensor_probe.h"
#include "control/threshold_controller.h"
#include <stdint.h>

class LightController
{
public:
    LightController(PidController pid, ThresholdController threshold_controller, float target_percent);

    void initialize(const SensorProbe &probe);

    float compute_brightness_target(
        uint16_t raw,
        uint8_t raw_percent,
        float filtered_control_percent,
        float current_brightness,
        float dt_seconds);

    bool using_threshold_mode() const;

private:
    PidController pid_;
    ThresholdController threshold_controller_;
    float target_percent_ = 0.0F;
    bool use_threshold_controller_ = false;
};