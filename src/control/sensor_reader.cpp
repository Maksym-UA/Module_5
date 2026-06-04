#include "control/sensor_reader.h"

#include "app_config.h"
#include "photoresistor.h"

esp_err_t SensorReader::read(SensorSample *sample)
{
    if (sample == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t raw = 0;
    const esp_err_t err = Photoresistor::read_raw(&raw);
    if (err != ESP_OK) {
        return err;
    }

    sample->measured_raw = raw;

    if (raw <= AppConfig::kAcceptedRawMax) {
        sample->effective_raw = raw;
    } else {
        sample->effective_raw = AppConfig::kAcceptedRawMax;
    }

    return ESP_OK;
}
