#pragma once

#include "control/pid_controller.h"

class LightController
{
public:
    LightController(PidController pid, float target_percent);

    float compute_brightness_target(float measured_percent, float dt_seconds);

private:
    PidController pid_;
    float target_percent_ = 0.0F;
};