#include "control/sensor_reader.h"

#include "photoresistor.h"

SensorReader::SensorReader(size_t sample_count)
    : sample_count_(sample_count)
{
}

esp_err_t SensorReader::read_filtered_raw(uint16_t *raw_value)
{
    if (raw_value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (sample_count_ == 0 || sample_count_ > 16) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint16_t samples[16] = {};
    uint32_t nonzero_sum = 0;
    size_t nonzero_count = 0;

    for (size_t i = 0; i < sample_count_; ++i) {
        const esp_err_t err = Photoresistor::read_raw(&samples[i]);
        if (err != ESP_OK) {
            return err;
        }

        if (samples[i] > 0) {
            nonzero_sum += samples[i];
            ++nonzero_count;
        }
    }

    if (nonzero_count == 0) {
        *raw_value = 0;
    } else {
        *raw_value = static_cast<uint16_t>(nonzero_sum / nonzero_count);
    }

    return ESP_OK;
}

esp_err_t SensorReader::read(SensorSample *sample)
{
    if (sample == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t raw = 0;
    const esp_err_t err = read_filtered_raw(&raw);
    if (err != ESP_OK) {
        return err;
    }

    sample->measured_raw = raw;
    sample->effective_raw = raw;
    return ESP_OK;
}