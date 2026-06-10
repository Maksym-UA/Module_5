#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

namespace app_i2c {

esp_err_t acquire_bus(int sda_gpio, int scl_gpio, i2c_master_bus_handle_t *out_bus);

}  // namespace app_i2c