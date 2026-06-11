#pragma once

#include <stdbool.h>
#include "esp_err.h"

namespace oled_app {

struct SensorDisplayData {
    float temperatureC;
    float humidityPercent;
    float pressureHpa;
};

esp_err_t init(int sda_gpio = 8, int scl_gpio = 9, int reset_gpio = -1);
esp_err_t showStartup();
esp_err_t showMessage(const char *line1, const char *line2 = nullptr);
esp_err_t showSensorData(const SensorDisplayData &data);

}  // namespace oled_app
