#include "control/light_controller.h"

LightController::LightController(PidController pid, float target_percent)
    : pid_(pid),
      target_percent_(target_percent)
{
}

float LightController::compute_brightness_target(float measured_percent, float dt_seconds)
{
    return pid_.compute(target_percent_, measured_percent, dt_seconds);
}