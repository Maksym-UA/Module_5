#include "i2c_bus.h"

#include "esp_log.h"

namespace app_i2c {

static const char *TAG = "i2c_bus";
static i2c_master_bus_handle_t s_bus_handle = nullptr;
static int s_sda_gpio = -1;
static int s_scl_gpio = -1;

esp_err_t acquire_bus(int sda_gpio, int scl_gpio, i2c_master_bus_handle_t *out_bus)
{
    if (out_bus == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_bus_handle != nullptr) {
        if (s_sda_gpio != sda_gpio || s_scl_gpio != scl_gpio) {
            ESP_LOGE(TAG,
                     "I2C bus already initialized on SDA=%d SCL=%d, requested SDA=%d SCL=%d",
                     s_sda_gpio,
                     s_scl_gpio,
                     sda_gpio,
                     scl_gpio);
            return ESP_ERR_INVALID_STATE;
        }

        *out_bus = s_bus_handle;
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = static_cast<gpio_num_t>(sda_gpio);
    bus_cfg.scl_io_num = static_cast<gpio_num_t>(scl_gpio);
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        return err;
    }

    s_sda_gpio = sda_gpio;
    s_scl_gpio = scl_gpio;
    *out_bus = s_bus_handle;
    return ESP_OK;
}

}  // namespace app_i2c