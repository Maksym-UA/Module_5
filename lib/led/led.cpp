#include "led.h"

#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace
{
static const char *TAG = "LED";

constexpr gpio_num_t kLedPins[] = {
	GPIO_NUM_5,
	GPIO_NUM_6,
	GPIO_NUM_7,
	GPIO_NUM_8,
	GPIO_NUM_9,
};

constexpr TickType_t kFlashOnTime = pdMS_TO_TICKS(120);
constexpr TickType_t kFlashOffTime = pdMS_TO_TICKS(80);

void flash_sequence_once()
{
	for (size_t i = 0; i < (sizeof(kLedPins) / sizeof(kLedPins[0])); ++i) {
		gpio_set_level(kLedPins[i], 1);
		vTaskDelay(kFlashOnTime);
		gpio_set_level(kLedPins[i], 0);
		vTaskDelay(kFlashOffTime);
	}
}
}

esp_err_t Led::init_all()
{
	uint64_t led_mask = 0;
	for (size_t i = 0; i < (sizeof(kLedPins) / sizeof(kLedPins[0])); ++i) {
		led_mask |= (1ULL << static_cast<uint32_t>(kLedPins[i]));
	}

	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = led_mask;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

	const esp_err_t cfg_err = gpio_config(&io_conf);
	if (cfg_err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to configure LEDs: %s", esp_err_to_name(cfg_err));
		return cfg_err;
	}

	for (size_t i = 0; i < (sizeof(kLedPins) / sizeof(kLedPins[0])); ++i) {
		gpio_set_level(kLedPins[i], 0);
	}

	flash_sequence_once();

	ESP_LOGI(TAG, "All LED GPIOs initialized");
	return ESP_OK;
}
