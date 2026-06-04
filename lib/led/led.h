#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdint.h>

namespace Led
{
esp_err_t init_all();
esp_err_t set_brightness(uint8_t brightness);
}
