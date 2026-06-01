#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

namespace Led
{
esp_err_t init_all();
}