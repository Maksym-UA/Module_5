#include "photoresistor.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

namespace
{
static const char *TAG = "Photoresistor";

constexpr gpio_num_t kPhotoresistorPin = GPIO_NUM_4;
constexpr adc_unit_t kAdcUnit = ADC_UNIT_1;
constexpr adc_channel_t kAdcChannel = ADC_CHANNEL_3; // GPIO4
constexpr adc_atten_t kAdcAtten = ADC_ATTEN_DB_12;
constexpr adc_bitwidth_t kAdcBitwidth = ADC_BITWIDTH_12;
constexpr int kAdcRawMin = 0;
constexpr int kAdcRawMax = 4095;

adc_oneshot_unit_handle_t s_adc_handle = nullptr;

bool is_adc_ready()
{
	return s_adc_handle != nullptr;
}
}

esp_err_t Photoresistor::init_all()
{
	adc_oneshot_unit_init_cfg_t unit_cfg = {};
	unit_cfg.unit_id = kAdcUnit;
	unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

	const esp_err_t new_unit_err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
	if (new_unit_err != ESP_OK) {
		ESP_LOGE(TAG, "ADC unit init failed on GPIO %d: %s", static_cast<int>(kPhotoresistorPin), esp_err_to_name(new_unit_err));
		return new_unit_err;
	}

	adc_oneshot_chan_cfg_t channel_cfg = {};
	channel_cfg.atten = kAdcAtten;
	channel_cfg.bitwidth = kAdcBitwidth;

	const esp_err_t channel_err = adc_oneshot_config_channel(s_adc_handle, kAdcChannel, &channel_cfg);
	if (channel_err != ESP_OK) {
		ESP_LOGE(TAG, "ADC channel config failed on GPIO %d: %s", static_cast<int>(kPhotoresistorPin), esp_err_to_name(channel_err));
		return channel_err;
	}

	ESP_LOGI(TAG, "Photoresistor ADC initialized on GPIO %d", static_cast<int>(kPhotoresistorPin));
	return ESP_OK;
}

esp_err_t Photoresistor::read_raw(uint16_t *raw_value)
{
	if (raw_value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (!is_adc_ready()) {
		return ESP_ERR_INVALID_STATE;
	}

	int sample = 0;
	const esp_err_t read_err = adc_oneshot_read(s_adc_handle, kAdcChannel, &sample);
	if (read_err != ESP_OK) {
		return read_err;
	}

	if (sample < kAdcRawMin) {
		sample = kAdcRawMin;
	}
	if (sample > kAdcRawMax) {
		sample = kAdcRawMax;
	}

	*raw_value = static_cast<uint16_t>(sample);
	return ESP_OK;
}

esp_err_t Photoresistor::read_percent(uint8_t *percent)
{
	if (percent == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	uint16_t raw = 0;
	const esp_err_t raw_err = read_raw(&raw);
	if (raw_err != ESP_OK) {
		return raw_err;
	}

	const uint32_t scaled = (static_cast<uint32_t>(raw) * 100U) / static_cast<uint32_t>(kAdcRawMax);
	*percent = static_cast<uint8_t>(scaled);
	return ESP_OK;
}
