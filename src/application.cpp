#include "application.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "debounce";

#define BUTTON_GPIO     GPIO_NUM_8
#define LED_GPIO        GPIO_NUM_9
#define BUTTON_ACTIVE_LEVEL 1
#define BUTTON_PULLUP_ENABLE 1
#define BUTTON_PULLDOWN_ENABLE 0
#define LED_ACTIVE_LEVEL 1
#define BUTTON_TASK_STACK_WORDS 4096
#define BUTTON_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

#define SAMPLE_PERIOD_MS 1
#define CLICK_LOCKOUT_MS 30
#define RELEASE_STABLE_SAMPLES 3

static TickType_t sample_period_ticks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);
    return (ticks == 0) ? 1 : ticks;
}

static TickType_t click_lockout_ticks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(CLICK_LOCKOUT_MS);
    return (ticks == 0) ? 1 : ticks;
}

static void log_button_led_state(int button_level, int led_on)
{
    const char *button_state = (button_level == BUTTON_ACTIVE_LEVEL) ? "PRESSED" : "RELEASED";
    const char *led_state = led_on ? "ON" : "OFF";
    int led_level = gpio_get_level(LED_GPIO);

    ESP_LOGI(TAG,
             "Button=%s (%d), LED=%s (logic=%d, gpio=%d)",
             button_state,
             button_level,
             led_state,
             led_on,
             led_level);
}

/* Task with fast press detect + lockout debounce (press-only actions) */
static void button_task(void *arg)
{
    (void)arg;

    TickType_t sample_ticks = sample_period_ticks();
    TickType_t lockout_ticks = click_lockout_ticks();
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t press_count = 0;
    int led_on = (gpio_get_level(LED_GPIO) == LED_ACTIVE_LEVEL) ? 1 : 0;
    int stable_level = gpio_get_level(BUTTON_GPIO);
    int lockout_left = 0;
    bool press_latched = false;
    uint8_t release_stable_count = 0;

    while (true) {
        int raw_level = gpio_get_level(BUTTON_GPIO);

        if (lockout_left > 0) {
            lockout_left--;
        }

        /* Re-arm only after release is stable for multiple samples */
        if (raw_level != BUTTON_ACTIVE_LEVEL) {
            if (release_stable_count < RELEASE_STABLE_SAMPLES) {
                release_stable_count++;
            }
            if (release_stable_count >= RELEASE_STABLE_SAMPLES) {
                press_latched = false;
            }
        } else {
            release_stable_count = 0;
        }

        if (raw_level != stable_level) {
            stable_level = raw_level;

            if (stable_level == BUTTON_ACTIVE_LEVEL && lockout_left == 0 && !press_latched) {
                lockout_left = (int)lockout_ticks;
                press_latched = true;

                press_count++;
                led_on = !led_on;
                gpio_set_level(LED_GPIO, led_on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
                ESP_LOGI(TAG, "Click #%lu", (unsigned long)press_count);
                log_button_led_state(stable_level, led_on);
            }
        }

        vTaskDelayUntil(&last_wake, sample_ticks);
    }
}

void application_init(void)
{
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_cfg);
    gpio_set_level(LED_GPIO, !LED_ACTIVE_LEVEL);

    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = BUTTON_PULLUP_ENABLE ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = BUTTON_PULLDOWN_ENABLE ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);

    BaseType_t task_created = xTaskCreate(button_task,
                                          "button_task",
                                          BUTTON_TASK_STACK_WORDS,
                                          NULL,
                                          BUTTON_TASK_PRIORITY,
                                          NULL);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button_task");
        return;
    }

    ESP_LOGI(TAG,
             "Button debounce ready (GPIO=%d, active_level=%d, sample=%dms, lockout=%dms, release_stable=%d)",
             (int)BUTTON_GPIO,
             BUTTON_ACTIVE_LEVEL,
             SAMPLE_PERIOD_MS,
             CLICK_LOCKOUT_MS,
             RELEASE_STABLE_SAMPLES);
    ESP_LOGI(TAG, "Startup raw levels: button=%d, led_gpio=%d",
             gpio_get_level(BUTTON_GPIO), gpio_get_level(LED_GPIO));
    log_button_led_state(gpio_get_level(BUTTON_GPIO), gpio_get_level(LED_GPIO) == LED_ACTIVE_LEVEL);
}
