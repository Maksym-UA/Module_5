#include "control/sensor_reader.h"

#include "photoresistor.h"

SensorReader::SensorReader(size_t sample_count, uint8_t zero_raw_debounce_cycles)
    : sample_count_(sample_count),
      zero_raw_debounce_cycles_(zero_raw_debounce_cycles)
{
}

esp_err_t SensorReader::read_median_raw(uint16_t *raw_value)
{
    if (raw_value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (sample_count_ == 0) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint16_t samples[16] = {};
    if (sample_count_ > 16) {
        return ESP_ERR_INVALID_SIZE;
    }

    for (size_t i = 0; i < sample_count_; ++i) {
        const esp_err_t err = Photoresistor::read_raw(&samples[i]);
        if (err != ESP_OK) {
            return err;
        }
    }

    for (size_t i = 1; i < sample_count_; ++i) {
        uint16_t key = samples[i];
        size_t j = i;
        while (j > 0 && samples[j - 1] > key) {
            samples[j] = samples[j - 1];
            --j;
        }
        samples[j] = key;
    }

    *raw_value = samples[sample_count_ / 2];
    return ESP_OK;
}

esp_err_t SensorReader::read_raw(uint16_t *raw_value)
{
    if (raw_value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t raw = 0;
    const esp_err_t err = read_median_raw(&raw);
    if (err != ESP_OK) {
        return err;
    }

    if (raw == 0 && has_last_nonzero_raw_ && zero_raw_streak_ < zero_raw_debounce_cycles_) {
        ++zero_raw_streak_;
        raw = last_nonzero_raw_;
    } else if (raw == 0) {
        if (zero_raw_streak_ < 255) {
            ++zero_raw_streak_;
        }
    } else {
        last_nonzero_raw_ = raw;
        has_last_nonzero_raw_ = true;
        zero_raw_streak_ = 0;
    }

    *raw_value = raw;
    return ESP_OK;
}