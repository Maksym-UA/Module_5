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

    void set_invalid_low_floor(uint16_t min_valid_raw);
    void set_invalid_low_hold_cycles(uint8_t cycles);

    esp_err_t read(SensorSample *sample);

private:
    esp_err_t read_median_raw(uint16_t *raw_value);

    size_t sample_count_ = 0;
    uint16_t min_valid_raw_ = 0;
    uint8_t max_invalid_low_hold_cycles_ = 12;
    uint8_t rising_spike_confirm_cycles_ = 2;

    bool has_last_valid_raw_ = false;
    uint16_t last_valid_raw_ = 0;
    uint8_t zero_raw_streak_ = 0;
    uint16_t pending_rising_raw_ = 0;
    uint8_t pending_rising_count_ = 0;
};