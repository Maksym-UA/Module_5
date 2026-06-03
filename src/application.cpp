#include "application.h"
#include "led.h"
#include "photoresistor.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "PID";

namespace
{
constexpr float kPidKp = 0.5F;
constexpr float kPidKi = 0.0F;
constexpr float kPidKd = 0.02F;
constexpr float kTargetLightPercent = 80.0F;
constexpr float kPidDeadband = 5.0F;
constexpr float kThresholdRawLowRatio = 0.08F;
constexpr float kThresholdRawHighRatio = 0.18F;
constexpr float kThresholdStepUp = 0.8F;
constexpr float kThresholdStepDown = 1.0F;
constexpr uint8_t kThresholdDebounceCycles = 3;
constexpr uint8_t kZeroRawDebounceCycles = 4;
constexpr uint8_t kThresholdAdjustCooldownCycles = 8;

// Fallback used only if automatic polarity probe cannot decide.
constexpr bool kDefaultInvertSensorPercent = true;

constexpr float kOutputMin = 0.0F;
constexpr float kOutputMax = 127.0F;
constexpr float kInputFilterAlpha = 0.15F;
constexpr float kMaxBrightnessStepPerCycle = 1.0F;
constexpr uint8_t kPolarityProbeBrightness = 127;
constexpr TickType_t kPolaritySettleDelay = pdMS_TO_TICKS(180);
constexpr int kPolarityMinDeltaPercent = 2;
constexpr uint16_t kPolarityMinDeltaRaw = 10;
constexpr size_t kRawSamplesPerCycle = 5;
constexpr TickType_t kControlPeriod = pdMS_TO_TICKS(50);
constexpr int64_t kLogPeriodUs = 500000;

struct SensorProbe
{
    bool invert_sensor_percent = kDefaultInvertSensorPercent;
    uint16_t off_raw = 0;
    uint16_t on_raw = 0;
    bool has_raw_span = false;
};

float clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }

    if (value > max_value) {
        return max_value;
    }

    return value;
}

float sensor_percent_for_pid(uint8_t sensor_percent, bool invert_sensor_percent)
{
    const float value = static_cast<float>(sensor_percent);
    if (invert_sensor_percent) {
        return 100.0F - value;
    }

    return value;
}

float sensor_percent_from_probe_raw(uint16_t raw, const SensorProbe &probe)
{
    if (!probe.has_raw_span) {
        const float raw_percent = (static_cast<float>(raw) * 100.0F) / 4095.0F;
        const float clamped_percent = clamp_float(raw_percent, 0.0F, 100.0F);
        return sensor_percent_for_pid(static_cast<uint8_t>(clamped_percent), probe.invert_sensor_percent);
    }

    if (probe.on_raw > probe.off_raw) {
        const float span = static_cast<float>(probe.on_raw - probe.off_raw);
        const float relative = static_cast<float>(raw) - static_cast<float>(probe.off_raw);
        const float percent = (relative * 100.0F) / span;
        return clamp_float(percent, 0.0F, 100.0F);
    }

    const float span = static_cast<float>(probe.off_raw - probe.on_raw);
    const float relative = static_cast<float>(probe.off_raw) - static_cast<float>(raw);
    const float percent = (relative * 100.0F) / span;
    return clamp_float(percent, 0.0F, 100.0F);
}

float sensor_percent_from_adc_raw(uint16_t raw)
{
    const float raw_percent = (static_cast<float>(raw) * 100.0F) / 4095.0F;
    return clamp_float(raw_percent, 0.0F, 100.0F);
}

esp_err_t read_stable_raw(uint16_t *raw_value)
{
    if (raw_value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t samples[kRawSamplesPerCycle] = {};

    for (size_t i = 0; i < kRawSamplesPerCycle; ++i) {
        const esp_err_t read_err = Photoresistor::read_raw(&samples[i]);
        if (read_err != ESP_OK) {
            return read_err;
        }
    }

    // Insertion sort for tiny fixed-size sample set, then take median.
    for (size_t i = 1; i < kRawSamplesPerCycle; ++i) {
        uint16_t key = samples[i];
        size_t j = i;
        while (j > 0 && samples[j - 1] > key) {
            samples[j] = samples[j - 1];
            --j;
        }
        samples[j] = key;
    }

    *raw_value = samples[kRawSamplesPerCycle / 2];
    return ESP_OK;
}

esp_err_t detect_sensor_inversion(SensorProbe *probe)
{
    if (probe == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t off_err = Led::set_brightness(0);
    if (off_err != ESP_OK) {
        return off_err;
    }

    vTaskDelay(kPolaritySettleDelay);

    uint16_t off_raw = 0;
    const esp_err_t read_off_raw_err = Photoresistor::read_raw(&off_raw);
    if (read_off_raw_err != ESP_OK) {
        return read_off_raw_err;
    }

    uint8_t off_percent = 0;
    const esp_err_t read_off_err = Photoresistor::read_percent(&off_percent);
    if (read_off_err != ESP_OK) {
        return read_off_err;
    }

    const esp_err_t on_err = Led::set_brightness(kPolarityProbeBrightness);
    if (on_err != ESP_OK) {
        return on_err;
    }

    vTaskDelay(kPolaritySettleDelay);

    uint16_t on_raw = 0;
    const esp_err_t read_on_raw_err = Photoresistor::read_raw(&on_raw);
    if (read_on_raw_err != ESP_OK) {
        return read_on_raw_err;
    }

    uint8_t on_percent = 0;
    const esp_err_t read_on_err = Photoresistor::read_percent(&on_percent);
    if (read_on_err != ESP_OK) {
        return read_on_err;
    }

    const esp_err_t back_to_off_err = Led::set_brightness(0);
    if (back_to_off_err != ESP_OK) {
        return back_to_off_err;
    }

    probe->off_raw = off_raw;
    probe->on_raw = on_raw;

    const int delta = static_cast<int>(on_percent) - static_cast<int>(off_percent);
    int abs_delta = delta;
    if (abs_delta < 0) {
        abs_delta = -abs_delta;
    }

    if (abs_delta < kPolarityMinDeltaPercent) {
        probe->invert_sensor_percent = kDefaultInvertSensorPercent;
        return ESP_OK;
    }

    int raw_delta = static_cast<int>(on_raw) - static_cast<int>(off_raw);
    int abs_raw_delta = raw_delta;
    if (abs_raw_delta < 0) {
        abs_raw_delta = -abs_raw_delta;
    }

    probe->has_raw_span = (abs_raw_delta >= static_cast<int>(kPolarityMinDeltaRaw));

    // If percent decreases when LEDs are brighter, invert mapping for PID control.
    probe->invert_sensor_percent = (delta < 0);
    return ESP_OK;
}

class PidController
{
public:
    PidController(float kp, float ki, float kd, float out_min, float out_max)
        : kp_(kp), ki_(ki), kd_(kd), out_min_(out_min), out_max_(out_max)
    {
    }

    float compute(float setpoint, float input, float dt_seconds)
    {
        if (dt_seconds <= 0.0F) {
            dt_seconds = 0.001F;
        }

        const float error = setpoint - input;

        float abs_error = error;
        if (abs_error < 0.0F) {
            abs_error = -abs_error;
        }

        if (abs_error < kPidDeadband) {
            previous_error_ = error;
            return clamp_float(kp_ * error, out_min_, out_max_);
        }

        const float candidate_integral = integral_ + (error * dt_seconds);
        const float derivative = (error - previous_error_) / dt_seconds;

        const float proportional = kp_ * error;
        const float integral_term = ki_ * candidate_integral;
        const float derivative_term = kd_ * derivative;
        const float unconstrained_output = proportional + integral_term + derivative_term;
        const float constrained_output = clamp_float(unconstrained_output, out_min_, out_max_);

        if (constrained_output == unconstrained_output) {
            integral_ = candidate_integral;
        } else {
            const bool saturated_high = constrained_output >= out_max_;
            const bool saturated_low = constrained_output <= out_min_;
            const bool pulls_toward_center = (saturated_high && error < 0.0F) || (saturated_low && error > 0.0F);
            if (pulls_toward_center) {
                integral_ = candidate_integral;
            }
        }

        previous_error_ = error;
        return constrained_output;
    }

private:
    float kp_ = 0.0F;
    float ki_ = 0.0F;
    float kd_ = 0.0F;
    float out_min_ = 0.0F;
    float out_max_ = 0.0F;
    float integral_ = 0.0F;
    float previous_error_ = 0.0F;
};
}

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

    ESP_LOGI(TAG, "PID control started (target light: %.1f%%)", static_cast<double>(kTargetLightPercent));

    PidController pid(kPidKp, kPidKi, kPidKd, kOutputMin, kOutputMax);

    SensorProbe probe;
    const esp_err_t probe_err = detect_sensor_inversion(&probe);
    if (probe_err != ESP_OK) {
        ESP_LOGW(TAG, "Sensor polarity probe failed, fallback invert=%u (%s)",
                 static_cast<unsigned>(probe.invert_sensor_percent ? 1U : 0U),
                 esp_err_to_name(probe_err));
    } else {
        ESP_LOGI(
            TAG,
            "Sensor polarity: invert=%u off_raw=%u on_raw=%u span_mode=%u",
            static_cast<unsigned>(probe.invert_sensor_percent ? 1U : 0U),
            static_cast<unsigned>(probe.off_raw),
            static_cast<unsigned>(probe.on_raw),
            static_cast<unsigned>(probe.has_raw_span ? 1U : 0U));
    }

    int64_t prev_time_us = esp_timer_get_time();
    int64_t next_log_us = prev_time_us + kLogPeriodUs;
    bool has_filtered_input = false;
    float filtered_input_percent = 0.0F;
    float commanded_brightness = 0.0F;
    bool use_threshold_controller = false;
    uint16_t threshold_raw_low = 0;
    uint16_t threshold_raw_high = 0;
    uint8_t threshold_low_hits = 0;
    uint8_t threshold_high_hits = 0;
    uint8_t threshold_adjust_cooldown = 0;
    bool has_last_nonzero_raw = false;
    uint16_t last_nonzero_raw = 0;
    uint8_t zero_raw_streak = 0;

    if (probe.has_raw_span) {
        const uint16_t raw_min = (probe.off_raw < probe.on_raw) ? probe.off_raw : probe.on_raw;
        const uint16_t raw_max = (probe.off_raw < probe.on_raw) ? probe.on_raw : probe.off_raw;
        const float span = static_cast<float>(raw_max - raw_min);

        threshold_raw_low = raw_min + static_cast<uint16_t>(span * kThresholdRawLowRatio);
        threshold_raw_high = raw_min + static_cast<uint16_t>(span * kThresholdRawHighRatio);

        if (threshold_raw_high <= threshold_raw_low) {
            threshold_raw_high = threshold_raw_low + 1;
        }

        use_threshold_controller = true;
        ESP_LOGI(TAG,
                 "Threshold control enabled: raw_low=%u raw_high=%u",
                 static_cast<unsigned>(threshold_raw_low),
                 static_cast<unsigned>(threshold_raw_high));
    }

    while (true) {
        uint16_t light_raw = 0;
        const esp_err_t raw_err = read_stable_raw(&light_raw);
        if (raw_err != ESP_OK) {
            ESP_LOGE(TAG, "Photoresistor raw read failed: %s", esp_err_to_name(raw_err));
            vTaskDelay(kControlPeriod);
            continue;
        }

        if (light_raw == 0 && has_last_nonzero_raw && zero_raw_streak < kZeroRawDebounceCycles) {
            ++zero_raw_streak;
            light_raw = last_nonzero_raw;
        } else if (light_raw == 0) {
            if (zero_raw_streak < 255) {
                ++zero_raw_streak;
            }
        } else {
            last_nonzero_raw = light_raw;
            has_last_nonzero_raw = true;
            zero_raw_streak = 0;
        }

        const uint8_t light_percent = static_cast<uint8_t>(sensor_percent_from_adc_raw(light_raw));

        const int64_t now_us = esp_timer_get_time();
        const float dt_seconds = static_cast<float>(now_us - prev_time_us) / 1000000.0F;
        prev_time_us = now_us;

        float sensor_control_percent = 0.0F;
        if (probe.has_raw_span) {
            sensor_control_percent = sensor_percent_from_probe_raw(light_raw, probe);
        } else {
            sensor_control_percent = sensor_percent_for_pid(light_percent, probe.invert_sensor_percent);
        }

        if (!has_filtered_input) {
            filtered_input_percent = sensor_control_percent;
            has_filtered_input = true;
        } else {
            const float delta = sensor_control_percent - filtered_input_percent;
            filtered_input_percent = filtered_input_percent + (kInputFilterAlpha * delta);
        }

        float brightness_target = 0.0F;
        if (use_threshold_controller) {
            if (probe.on_raw >= probe.off_raw) {
                if (light_raw <= threshold_raw_low) {
                    if (threshold_low_hits < kThresholdDebounceCycles) {
                        ++threshold_low_hits;
                    }
                    threshold_high_hits = 0;
                } else if (light_raw >= threshold_raw_high) {
                    if (threshold_high_hits < kThresholdDebounceCycles) {
                        ++threshold_high_hits;
                    }
                    threshold_low_hits = 0;
                } else {
                    threshold_low_hits = 0;
                    threshold_high_hits = 0;
                }
            } else {
                if (light_raw >= threshold_raw_high) {
                    if (threshold_low_hits < kThresholdDebounceCycles) {
                        ++threshold_low_hits;
                    }
                    threshold_high_hits = 0;
                } else if (light_raw <= threshold_raw_low) {
                    if (threshold_high_hits < kThresholdDebounceCycles) {
                        ++threshold_high_hits;
                    }
                    threshold_low_hits = 0;
                } else {
                    threshold_low_hits = 0;
                    threshold_high_hits = 0;
                }
            }

            if (threshold_adjust_cooldown > 0) {
                --threshold_adjust_cooldown;
                brightness_target = commanded_brightness;
                threshold_low_hits = 0;
                threshold_high_hits = 0;
            } else if (threshold_low_hits >= kThresholdDebounceCycles) {
                brightness_target = commanded_brightness + kThresholdStepUp;
                threshold_low_hits = 0;
                threshold_adjust_cooldown = kThresholdAdjustCooldownCycles;
            } else if (threshold_high_hits >= kThresholdDebounceCycles) {
                brightness_target = commanded_brightness - kThresholdStepDown;
                threshold_high_hits = 0;
                threshold_adjust_cooldown = kThresholdAdjustCooldownCycles;
            } else {
                brightness_target = commanded_brightness;
            }
        } else {
            brightness_target = pid.compute(kTargetLightPercent, filtered_input_percent, dt_seconds);
        }

        const float brightness_delta = brightness_target - commanded_brightness;

        if (brightness_delta > kMaxBrightnessStepPerCycle) {
            commanded_brightness = commanded_brightness + kMaxBrightnessStepPerCycle;
        } else if (brightness_delta < -kMaxBrightnessStepPerCycle) {
            commanded_brightness = commanded_brightness - kMaxBrightnessStepPerCycle;
        } else {
            commanded_brightness = brightness_target;
        }

        commanded_brightness = clamp_float(commanded_brightness, kOutputMin, kOutputMax);
        const uint8_t brightness = static_cast<uint8_t>(commanded_brightness);

        const esp_err_t pwm_err = Led::set_brightness(brightness);
        if (pwm_err != ESP_OK) {
            ESP_LOGE(TAG, "LED brightness update failed: %s", esp_err_to_name(pwm_err));
            vTaskDelay(kControlPeriod);
            continue;
        }

        if (now_us >= next_log_us) {
            ESP_LOGI(
                TAG,
                "raw=%u sensor=%u%% control=%.1f%% target=%.1f%% brightness=%u",
                static_cast<unsigned>(light_raw),
                static_cast<unsigned>(light_percent),
                static_cast<double>(filtered_input_percent),
                static_cast<double>(kTargetLightPercent),
                static_cast<unsigned>(brightness));

            next_log_us = now_us + kLogPeriodUs;
        }

        vTaskDelay(kControlPeriod);
    }
}