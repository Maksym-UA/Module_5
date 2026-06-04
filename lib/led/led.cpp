#include "led.h"

#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
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

constexpr size_t kLedCount = sizeof(kLedPins) / sizeof(kLedPins[0]);

constexpr ledc_mode_t kLedSpeedMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t kLedTimer = LEDC_TIMER_0;
constexpr ledc_timer_bit_t kLedDutyResolution = LEDC_TIMER_8_BIT;
constexpr uint32_t kLedPwmFrequencyHz = 5000;

constexpr ledc_channel_t kLedChannels[] = {
    LEDC_CHANNEL_0,
    LEDC_CHANNEL_1,
    LEDC_CHANNEL_2,
    LEDC_CHANNEL_3,
    LEDC_CHANNEL_4,
};

constexpr TickType_t kFlashOnTime = pdMS_TO_TICKS(140);
constexpr TickType_t kFlashOffTime = pdMS_TO_TICKS(80);

// Builds a bitmask for the LED GPIO pins, where each bit corresponds
// to a pin number. This is used to configure multiple GPIOs at once.
uint64_t build_led_mask()
{
    uint64_t led_mask = 0;

    for (size_t i = 0; i < kLedCount; ++i) {
        led_mask |= (1ULL << static_cast<uint32_t>(kLedPins[i]));
    }

    return led_mask;
}

esp_err_t configure_pwm_timer()
{
    ledc_timer_config_t led_timer = {};
    led_timer.speed_mode = kLedSpeedMode;
    led_timer.timer_num = kLedTimer;
    led_timer.duty_resolution = kLedDutyResolution;
    led_timer.freq_hz = kLedPwmFrequencyHz;
    led_timer.clk_cfg = LEDC_AUTO_CLK;

    return ledc_timer_config(&led_timer);
}

esp_err_t configure_pwm_channels()
{
    for (size_t i = 0; i < kLedCount; ++i) {
        ledc_channel_config_t channel = {};
        channel.gpio_num = kLedPins[i];
        channel.speed_mode = kLedSpeedMode;
        channel.channel = kLedChannels[i];
        channel.intr_type = LEDC_INTR_DISABLE;
        channel.timer_sel = kLedTimer;
        channel.duty = 0;
        channel.hpoint = 0;

        const esp_err_t channel_err = ledc_channel_config(&channel);
        if (channel_err != ESP_OK) {
            return channel_err;
        }
    }

    return ESP_OK;
}

esp_err_t write_all_leds_duty(uint32_t duty)
{
    for (size_t i = 0; i < kLedCount; ++i) {
        const esp_err_t set_err = ledc_set_duty(kLedSpeedMode, kLedChannels[i], duty);
        if (set_err != ESP_OK) {
            return set_err;
        }

        const esp_err_t update_err = ledc_update_duty(kLedSpeedMode, kLedChannels[i]);
        if (update_err != ESP_OK) {
            return update_err;
        }
    }

    return ESP_OK;
}

void flash_sequence_once_startup()
{
    for (size_t i = 0; i < kLedCount; ++i) {
        ledc_set_duty(kLedSpeedMode, kLedChannels[i], 255);
        ledc_update_duty(kLedSpeedMode, kLedChannels[i]);
        vTaskDelay(kFlashOnTime);
        ledc_set_duty(kLedSpeedMode, kLedChannels[i], 0);
        ledc_update_duty(kLedSpeedMode, kLedChannels[i]);
        vTaskDelay(kFlashOffTime);
    }
}
}

esp_err_t Led::init_all()
{
    const uint64_t led_mask = build_led_mask();

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = led_mask;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    const esp_err_t gpio_err = gpio_config(&io_conf);
    if (gpio_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIOs: %s", esp_err_to_name(gpio_err));
        return gpio_err;
    }

    const esp_err_t timer_err = configure_pwm_timer();
    if (timer_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED PWM timer: %s", esp_err_to_name(timer_err));
        return timer_err;
    }

    const esp_err_t channels_err = configure_pwm_channels();
    if (channels_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED PWM channels: %s", esp_err_to_name(channels_err));
        return channels_err;
    }

    const esp_err_t off_err = write_all_leds_duty(0);
    if (off_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear LED duty: %s", esp_err_to_name(off_err));
        return off_err;
    }

    flash_sequence_once_startup();

    ESP_LOGI(TAG, "All LED PWM channels initialized");
    return ESP_OK;
}

esp_err_t Led::set_brightness(uint8_t brightness)
{
    return write_all_leds_duty(static_cast<uint32_t>(brightness));
}
