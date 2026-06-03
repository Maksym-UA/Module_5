#include "application.h"

#include "app_config.h"
#include "control/light_controller.h"
#include "control/pid_controller.h"
#include "control/sensor_probe.h"
#include "control/sensor_reader.h"
#include "control/signal_filter.h"
#include "led.h"
#include "photoresistor.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace
{
float clamp_float(float value, float min_value, float max_value)
{
    return SensorProbeLogic::clamp(value, min_value, max_value);
}
}

void application_init()
{
    Application app;
    app.start();
}

void Application::start()
{
    ESP_LOGI(AppConfig::kLogTag, "Starting application...");
    run();
}

void Application::run()
{
    esp_err_t err = Led::init_all();
    if (err != ESP_OK) {
        ESP_LOGE(AppConfig::kLogTag, "LED init failed: %s", esp_err_to_name(err));
        return;
    }

    err = Photoresistor::init_all();
    if (err != ESP_OK) {
        ESP_LOGE(AppConfig::kLogTag, "Photoresistor init failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(
        AppConfig::kLogTag,
        "Closed-loop light control started (target light: %.1f%%)",
        static_cast<double>(AppConfig::kTargetLightPercent));

    SensorProbe probe;
    err = SensorProbeLogic::detect_sensor_inversion(&probe);
    if (err != ESP_OK) {
        ESP_LOGW(
            AppConfig::kLogTag,
            "Sensor polarity probe failed, fallback invert=%u (%s)",
            static_cast<unsigned>(probe.invert_sensor_percent ? 1U : 0U),
            esp_err_to_name(err));
    } else {
        ESP_LOGI(
            AppConfig::kLogTag,
            "Sensor polarity: invert=%u off_raw=%u on_raw=%u span_mode=%u",
            static_cast<unsigned>(probe.invert_sensor_percent ? 1U : 0U),
            static_cast<unsigned>(probe.off_raw),
            static_cast<unsigned>(probe.on_raw),
            static_cast<unsigned>(probe.has_raw_span ? 1U : 0U));
    }

    PidController pid(
        AppConfig::kPidKp,
        AppConfig::kPidKi,
        AppConfig::kPidKd,
        AppConfig::kOutputMin,
        AppConfig::kOutputMax,
        AppConfig::kPidDeadband);

    LightController light_controller(pid, AppConfig::kTargetLightPercent);
    SensorReader sensor_reader;
    SignalFilter input_filter(AppConfig::kInputFilterAlpha);

    int64_t next_log_us = esp_timer_get_time() + AppConfig::kLogPeriodUs;
    int64_t prev_time_us = esp_timer_get_time();

    float commanded_brightness = 0.0F;

    while (true) {
        SensorSample sample;
        err = sensor_reader.read(&sample);
        if (err != ESP_OK) {
            ESP_LOGE(AppConfig::kLogTag, "Photoresistor raw read failed: %s", esp_err_to_name(err));
            vTaskDelay(AppConfig::kControlPeriod);
            continue;
        }

        const uint16_t light_raw = sample.effective_raw;

        float measured_light_percent = 0.0F;
        if (probe.has_raw_span) {
            measured_light_percent = SensorProbeLogic::sensor_percent_from_probe_raw(light_raw, probe);
        } else {
            measured_light_percent = SensorProbeLogic::sensor_percent_from_control_range(
                light_raw,
                AppConfig::kControlRawMin,
                AppConfig::kControlRawMax,
                false);
        }

        const float filtered_light_percent = input_filter.update(measured_light_percent);

        const int64_t now_us = esp_timer_get_time();
        const float dt_seconds = static_cast<float>(now_us - prev_time_us) / 1000000.0F;
        prev_time_us = now_us;

        const float brightness_target =
            light_controller.compute_brightness_target(filtered_light_percent, dt_seconds);

        const float delta = brightness_target - commanded_brightness;
        if (delta > AppConfig::kMaxBrightnessStepPerCycle) {
            commanded_brightness += AppConfig::kMaxBrightnessStepPerCycle;
        } else if (delta < -AppConfig::kMaxBrightnessStepPerCycle) {
            commanded_brightness -= AppConfig::kMaxBrightnessStepPerCycle;
        } else {
            commanded_brightness = brightness_target;
        }

        commanded_brightness =
            clamp_float(commanded_brightness, AppConfig::kOutputMin, AppConfig::kOutputMax);

        const uint8_t brightness = static_cast<uint8_t>(commanded_brightness);

        err = Led::set_brightness(brightness);
        if (err != ESP_OK) {
            ESP_LOGE(AppConfig::kLogTag, "LED brightness update failed: %s", esp_err_to_name(err));
            vTaskDelay(AppConfig::kControlPeriod);
            continue;
        }

        if (now_us >= next_log_us) {
            ESP_LOGI(
                AppConfig::kLogTag,
                "raw=%u effective=%u measured=%.1f%% filtered=%.1f%% target=%.1f%% brightness=%u",
                static_cast<unsigned>(sample.measured_raw),
                static_cast<unsigned>(sample.effective_raw),
                static_cast<double>(measured_light_percent),
                static_cast<double>(filtered_light_percent),
                static_cast<double>(AppConfig::kTargetLightPercent),
                static_cast<unsigned>(brightness));

            next_log_us = now_us + AppConfig::kLogPeriodUs;
        }

        vTaskDelay(AppConfig::kControlPeriod);
    }
}