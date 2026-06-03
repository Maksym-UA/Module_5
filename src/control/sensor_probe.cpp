#include "control/sensor_probe.h"

#include "app_config.h"
#include "led.h"
#include "photoresistor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace
{
constexpr float kAdcMaxRaw = 4095.0F;
}

float SensorProbeLogic::clamp(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }

    if (value > max_value) {
        return max_value;
    }

    return value;
}

float SensorProbeLogic::raw_to_percent(uint16_t raw)
{
    const float raw_percent = (static_cast<float>(raw) * 100.0F) / kAdcMaxRaw;
    return clamp(raw_percent, 0.0F, 100.0F);
}

float SensorProbeLogic::sensor_percent_for_control(uint8_t sensor_percent, bool invert_sensor_percent)
{
    const float value = static_cast<float>(sensor_percent);
    return invert_sensor_percent ? (100.0F - value) : value;
}

float SensorProbeLogic::sensor_percent_from_probe_raw(uint16_t raw, const SensorProbe &probe)
{
    if (!probe.has_raw_span) {
        const float percent = raw_to_percent(raw);
        return sensor_percent_for_control(static_cast<uint8_t>(percent), probe.invert_sensor_percent);
    }

    if (probe.on_raw > probe.off_raw) {
        const float span = static_cast<float>(probe.on_raw - probe.off_raw);
        const float relative = static_cast<float>(raw) - static_cast<float>(probe.off_raw);
        return clamp((relative * 100.0F) / span, 0.0F, 100.0F);
    }

    const float span = static_cast<float>(probe.off_raw - probe.on_raw);
    const float relative = static_cast<float>(probe.off_raw) - static_cast<float>(raw);
    return clamp((relative * 100.0F) / span, 0.0F, 100.0F);
}

esp_err_t SensorProbeLogic::detect_sensor_inversion(SensorProbe *probe)
{
    if (probe == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    probe->invert_sensor_percent = AppConfig::kDefaultInvertSensorPercent;

    esp_err_t err = Led::set_brightness(0);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(AppConfig::kPolaritySettleDelay);

    uint16_t off_raw = 0;
    err = Photoresistor::read_raw(&off_raw);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t off_percent = 0;
    err = Photoresistor::read_percent(&off_percent);
    if (err != ESP_OK) {
        return err;
    }

    err = Led::set_brightness(AppConfig::kPolarityProbeBrightness);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(AppConfig::kPolaritySettleDelay);

    uint16_t on_raw = 0;
    err = Photoresistor::read_raw(&on_raw);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t on_percent = 0;
    err = Photoresistor::read_percent(&on_percent);
    if (err != ESP_OK) {
        return err;
    }

    err = Led::set_brightness(0);
    if (err != ESP_OK) {
        return err;
    }

    probe->off_raw = off_raw;
    probe->on_raw = on_raw;

    const int delta_percent = static_cast<int>(on_percent) - static_cast<int>(off_percent);
    const int abs_delta_percent = (delta_percent < 0) ? -delta_percent : delta_percent;

    if (abs_delta_percent < AppConfig::kPolarityMinDeltaPercent) {
        return ESP_OK;
    }

    const int delta_raw = static_cast<int>(on_raw) - static_cast<int>(off_raw);
    const int abs_delta_raw = (delta_raw < 0) ? -delta_raw : delta_raw;

    probe->has_raw_span = (abs_delta_raw >= static_cast<int>(AppConfig::kPolarityMinDeltaRaw));
    probe->invert_sensor_percent = (delta_percent < 0);

    return ESP_OK;
}