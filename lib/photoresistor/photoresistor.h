#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdint.h>

namespace Photoresistor
{
esp_err_t init_all();
esp_err_t read_raw(uint16_t *raw_value);
esp_err_t read_percent(uint8_t *percent);
}
