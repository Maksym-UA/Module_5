#pragma once

#include <stdint.h>

struct ThresholdControllerConfig
{
    float raw_low_ratio = 0.08F;
    float raw_high_ratio = 0.18F;
    float step_up = 0.8F;
    float step_down = 1.0F;
    uint8_t debounce_cycles = 3;
    uint8_t adjust_cooldown_cycles = 8;
};

class ThresholdController
{
public:
    explicit ThresholdController(const ThresholdControllerConfig &config);

    bool initialize(uint16_t off_raw, uint16_t on_raw);
    bool is_enabled() const;
    float update(uint16_t light_raw, float current_brightness) const;

private:
    ThresholdControllerConfig config_{};

    bool enabled_ = false;
    bool increasing_raw_means_more_light_ = true;
    uint16_t threshold_raw_low_ = 0;
    uint16_t threshold_raw_high_ = 0;

    mutable uint8_t low_hits_ = 0;
    mutable uint8_t high_hits_ = 0;
    mutable uint8_t cooldown_ = 0;
};