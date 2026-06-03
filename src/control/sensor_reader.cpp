#include "control/sensor_reader.h"

#include "photoresistor.h"

namespace
{
constexpr uint16_t kRisingSpikeBaselineRaw = 64;
constexpr uint16_t kRisingSpikeToleranceRaw = 32;
}

SensorReader::SensorReader(size_t sample_count)
    : sample_count_(sample_count)
{
}


void SensorReader::set_invalid_low_floor(uint16_t min_valid_raw)
{
    min_valid_raw_ = min_valid_raw;
}

void SensorReader::set_invalid_low_hold_cycles(uint8_t cycles)
{
    max_invalid_low_hold_cycles_ = cycles;
}

esp_err_t SensorReader::read_median_raw(uint16_t *raw_value)
{
    if (raw_value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (sample_count_ == 0 || sample_count_ > 16) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint16_t samples[16] = {};

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

esp_err_t SensorReader::read(SensorSample *sample)
{
    if (sample == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t measured_raw = 0;
    const esp_err_t err = read_median_raw(&measured_raw);
    if (err != ESP_OK) {
        return err;
    }

    // Reject isolated dark->bright spikes: require confirmation in consecutive cycles.
    const uint16_t rising_spike_threshold =
        (min_valid_raw_ > kRisingSpikeBaselineRaw) ? min_valid_raw_ : kRisingSpikeBaselineRaw;

    const bool large_rising_jump =
        has_last_valid_raw_ &&
        last_valid_raw_ < rising_spike_threshold &&
        measured_raw >= rising_spike_threshold;

    if (large_rising_jump) {
        if (pending_rising_count_ == 0) {
            pending_rising_raw_ = measured_raw;
            pending_rising_count_ = 1;
            measured_raw = last_valid_raw_;
        } else {
            uint16_t delta =
                (measured_raw > pending_rising_raw_)
                    ? static_cast<uint16_t>(measured_raw - pending_rising_raw_)
                    : static_cast<uint16_t>(pending_rising_raw_ - measured_raw);

            if (delta <= kRisingSpikeToleranceRaw) {
                ++pending_rising_count_;
                if (pending_rising_count_ < rising_spike_confirm_cycles_) {
                    measured_raw = last_valid_raw_;
                } else {
                    pending_rising_count_ = 0;
                }
            } else {
                pending_rising_raw_ = measured_raw;
                pending_rising_count_ = 1;
                measured_raw = last_valid_raw_;
            }
        }
    } else {
        pending_rising_count_ = 0;
    }

    const bool invalid_low_reading =
        (measured_raw == 0) ||
        (min_valid_raw_ > 0 && measured_raw < min_valid_raw_);

    uint16_t effective_raw = measured_raw;

    if (invalid_low_reading) {
        if (has_last_valid_raw_ && zero_raw_streak_ < max_invalid_low_hold_cycles_) {
            effective_raw = last_valid_raw_;
        } else {
            effective_raw = measured_raw;
        }

        if (zero_raw_streak_ < 255) {
            ++zero_raw_streak_;
        }
    } else {
        effective_raw = measured_raw;
        last_valid_raw_ = measured_raw;
        has_last_valid_raw_ = true;
        zero_raw_streak_ = 0;
    }

    sample->measured_raw = measured_raw;
    sample->effective_raw = effective_raw;
    return ESP_OK;
}