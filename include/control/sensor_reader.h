#pragma once

#include "esp_err.h"
#include <stdint.h>

class SensorReader
{
public:
    SensorReader(size_t sample_count, uint8_t zero_raw_debounce_cycles);

    esp_err_t read_raw(uint16_t *raw_value);

private:
    esp_err_t read_median_raw(uint16_t *raw_value);

    size_t sample_count_ = 0;
    uint8_t zero_raw_debounce_cycles_ = 0;

    bool has_last_nonzero_raw_ = false;
    uint16_t last_nonzero_raw_ = 0;
    uint8_t zero_raw_streak_ = 0;
};