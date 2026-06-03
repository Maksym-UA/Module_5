#include "control/light_controller.h"

LightController::LightController(PidController pid, ThresholdController threshold_controller, float target_percent)
    : pid_(pid),
      threshold_controller_(threshold_controller),
      target_percent_(target_percent)
{
}

void LightController::initialize(const SensorProbe &probe)
{
    use_threshold_controller_ =
        probe.has_raw_span &&
        threshold_controller_.initialize(probe.off_raw, probe.on_raw);
}

float LightController::compute_brightness_target(
    uint16_t raw,
    uint8_t raw_percent,
    float filtered_control_percent,
    float current_brightness,
    float dt_seconds)
{
    (void)raw_percent;

    if (use_threshold_controller_) {
        return threshold_controller_.update(raw, current_brightness);
    }

    return pid_.compute(target_percent_, filtered_control_percent, dt_seconds);
}

bool LightController::using_threshold_mode() const
{
    return use_threshold_controller_;
}