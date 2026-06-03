#include "control/threshold_controller.h"

ThresholdController::ThresholdController(const ThresholdControllerConfig &config)
    : config_(config)
{
}

bool ThresholdController::initialize(uint16_t off_raw, uint16_t on_raw)
{
    const uint16_t raw_min = (off_raw < on_raw) ? off_raw : on_raw;
    const uint16_t raw_max = (off_raw < on_raw) ? on_raw : off_raw;

    if (raw_max <= raw_min) {
        enabled_ = false;
        return false;
    }

    const float span = static_cast<float>(raw_max - raw_min);

    threshold_raw_low_ = raw_min + static_cast<uint16_t>(span * config_.raw_low_ratio);
    threshold_raw_high_ = raw_min + static_cast<uint16_t>(span * config_.raw_high_ratio);

    if (threshold_raw_high_ <= threshold_raw_low_) {
        threshold_raw_high_ = threshold_raw_low_ + 1;
    }

    increasing_raw_means_more_light_ = (on_raw >= off_raw);
    enabled_ = true;
    return true;
}

bool ThresholdController::is_enabled() const
{
    return enabled_;
}

float ThresholdController::update(uint16_t light_raw, float current_brightness)
{
    if (!enabled_) {
        return current_brightness;
    }

    if (increasing_raw_means_more_light_) {
        if (light_raw <= threshold_raw_low_) {
            if (low_hits_ < config_.debounce_cycles) {
                ++low_hits_;
            }
            high_hits_ = 0;
        } else if (light_raw >= threshold_raw_high_) {
            if (high_hits_ < config_.debounce_cycles) {
                ++high_hits_;
            }
            low_hits_ = 0;
        } else {
            low_hits_ = 0;
            high_hits_ = 0;
        }
    } else {
        if (light_raw >= threshold_raw_high_) {
            if (low_hits_ < config_.debounce_cycles) {
                ++low_hits_;
            }
            high_hits_ = 0;
        } else if (light_raw <= threshold_raw_low_) {
            if (high_hits_ < config_.debounce_cycles) {
                ++high_hits_;
            }
            low_hits_ = 0;
        } else {
            low_hits_ = 0;
            high_hits_ = 0;
        }
    }

    if (cooldown_ > 0) {
        --cooldown_;
        low_hits_ = 0;
        high_hits_ = 0;
        return current_brightness;
    }

    if (low_hits_ >= config_.debounce_cycles) {
        low_hits_ = 0;
        cooldown_ = config_.adjust_cooldown_cycles;
        return current_brightness + config_.step_up;
    }

    if (high_hits_ >= config_.debounce_cycles) {
        high_hits_ = 0;
        cooldown_ = config_.adjust_cooldown_cycles;
        return current_brightness - config_.step_down;
    }

    return current_brightness;
}