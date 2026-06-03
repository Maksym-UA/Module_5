#include "application.h"

#include "app_config.h"
#include "control/light_controller.h"
#include "control/pid_controller.h"
#include "control/sensor_reader.h"
#include "control/sensor_probe.h"
#include "control/signal_filter.h"
#include "threshold_controller.h"
#include "led.h"
#include "photoresistor.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace
{
static const char *TAG = "PID";

float clamp_float(float value, float min_value, float max_value)
{
    return SensorProbeLogic::clamp(value, min_value, max_value);
}
} // namespace

void application_init()
{
    Application app;
    app.start();
}

void Application::start()
{
    ESP_LOGI(TAG, "Starting application...");
    run();
}

void Application::run()
{
    const esp_err_t led_err = Led::init_all();
    if (led_err != ESP_OK) {
        ESP_LOGE(TAG, "LED init failed: %s", esp_err_to_name(led_err));
        return;
    }

    const esp_err_t photo_err = Photoresistor::init_all();
    if (photo_err != ESP_OK) {
        ESP_LOGE(TAG, "Photoresistor init failed: %s", esp_err_to_name(photo_err));
        return;
    }

    ESP_LOGI(TAG,
             "PID control started (target light: %.1f%%)",
             static_cast<double>(AppConfig::kTargetLightPercent));

    PidController pid(AppConfig::kPidKp,
                      AppConfig::kPidKi,
                      AppConfig::kPidKd,
                      AppConfig::kOutputMin,
                      AppConfig::kOutputMax,
                      AppConfig::kPidDeadband);

    SensorProbe probe;
    const esp_err_t probe_err = SensorProbeLogic::detect_sensor_inversion(&probe);
    if (probe_err != ESP_OK) {
        ESP_LOGW(TAG,
                 "Sensor polarity probe failed, fallback invert=%u (%s)",
                 static_cast<unsigned>(probe.invert_sensor_percent ? 1U : 0U),
                 esp_err_to_name(probe_err));
    } else {
        ESP_LOGI(TAG,
                 "Sensor polarity: invert=%u off_raw=%u on_raw=%u span_mode=%u",
                 static_cast<unsigned>(probe.invert_sensor_percent ? 1U : 0U),
                 static_cast<unsigned>(probe.off_raw),
                 static_cast<unsigned>(probe.on_raw),
                 static_cast<unsigned>(probe.has_raw_span ? 1U : 0U));
    }

    ThresholdControllerConfig threshold_config;
    threshold_config.raw_low_ratio = AppConfig::kThresholdRawLowRatio;
    threshold_config.raw_high_ratio = AppConfig::kThresholdRawHighRatio;
    threshold_config.step_up = AppConfig::kThresholdStepUp;
    threshold_config.step_down = AppConfig::kThresholdStepDown;
    threshold_config.debounce_cycles = AppConfig::kThresholdDebounceCycles;
    threshold_config.adjust_cooldown_cycles = AppConfig::kThresholdAdjustCooldownCycles;

    ThresholdController threshold_controller(threshold_config);
    LightController light_controller(pid, threshold_controller, AppConfig::kTargetLightPercent);
    light_controller.initialize(probe);

    if (light_controller.using_threshold_mode()) {
        ESP_LOGI(TAG, "Threshold control enabled");
    }

    SensorReader sensor_reader(AppConfig::kRawSamplesPerCycle, AppConfig::kZeroRawDebounceCycles);
    SignalFilter input_filter(AppConfig::kInputFilterAlpha);

    int64_t prev_time_us = esp_timer_get_time();
    int64_t next_log_us = prev_time_us + AppConfig::kLogPeriodUs;

    float filtered_input_percent = 0.0F;
    float commanded_brightness = 0.0F;

    while (true) {
        uint16_t light_raw = 0;
        const esp_err_t raw_err = sensor_reader.read_raw(&light_raw);
        if (raw_err != ESP_OK) {
            ESP_LOGE(TAG, "Photoresistor raw read failed: %s", esp_err_to_name(raw_err));
            vTaskDelay(AppConfig::kControlPeriod);
            continue;
        }

        const uint8_t light_percent =
            static_cast<uint8_t>(SensorProbeLogic::raw_to_percent(light_raw));

        const int64_t now_us = esp_timer_get_time();
        const float dt_seconds = static_cast<float>(now_us - prev_time_us) / 1000000.0F;
        prev_time_us = now_us;

        float sensor_control_percent = 0.0F;
        if (probe.has_raw_span) {
            sensor_control_percent = SensorProbeLogic::sensor_percent_from_probe_raw(light_raw, probe);
        } else {
            sensor_control_percent =
                SensorProbeLogic::sensor_percent_for_control(light_percent, probe.invert_sensor_percent);
        }

        filtered_input_percent = input_filter.update(sensor_control_percent);

        float brightness_target = light_controller.compute_brightness_target(
            light_raw,
            light_percent,
            filtered_input_percent,
            commanded_brightness,
            dt_seconds);

        const float brightness_delta = brightness_target - commanded_brightness;

        if (brightness_delta > AppConfig::kMaxBrightnessStepPerCycle) {
            commanded_brightness += AppConfig::kMaxBrightnessStepPerCycle;
        } else if (brightness_delta < -AppConfig::kMaxBrightnessStepPerCycle) {
            commanded_brightness -= AppConfig::kMaxBrightnessStepPerCycle;
        } else {
            commanded_brightness = brightness_target;
        }

        commanded_brightness = clamp_float(commanded_brightness, AppConfig::kOutputMin, AppConfig::kOutputMax);
        const uint8_t brightness = static_cast<uint8_t>(commanded_brightness);

        const esp_err_t pwm_err = Led::set_brightness(brightness);
        if (pwm_err != ESP_OK) {
            ESP_LOGE(TAG, "LED brightness update failed: %s", esp_err_to_name(pwm_err));
            vTaskDelay(AppConfig::kControlPeriod);
            continue;
        }

        if (now_us >= next_log_us) {
            ESP_LOGI(TAG,
                     "raw=%u sensor=%u%% control=%.1f%% target=%.1f%% brightness=%u",
                     static_cast<unsigned>(light_raw),
                     static_cast<unsigned>(light_percent),
                     static_cast<double>(filtered_input_percent),
                     static_cast<double>(AppConfig::kTargetLightPercent),
                     static_cast<unsigned>(brightness));

            next_log_us = now_us + AppConfig::kLogPeriodUs;
        }

        vTaskDelay(AppConfig::kControlPeriod);
    }
}