#include "application.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "rc_button";

#define BUTTON_GPIO GPIO_NUM_8
#define LED_GPIO GPIO_NUM_9
#define BUTTON_ACTIVE_LEVEL 1
#define BUTTON_PULLUP_ENABLE 0
#define BUTTON_PULLDOWN_ENABLE 0
#define LED_ACTIVE_LEVEL 1
#define BUTTON_TASK_STACK_WORDS 4096
#define BUTTON_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

#define SAMPLE_PERIOD_MS 5

// Ensure the sample period is at least 1 tick to avoid zero-delay issues
static TickType_t sample_period_ticks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);
    return (ticks == 0) ? 1 : ticks;
}


static void button_task(void *arg)
{
    (void)arg;

    TickType_t last_wake = xTaskGetTickCount(); // returns the current FreeRTOS tick count
    TickType_t sample_ticks = sample_period_ticks(); // Convert sample period from milliseconds to ticks

    int led_on = (gpio_get_level(LED_GPIO) == LED_ACTIVE_LEVEL) ? 1 : 0;
    int last_level = gpio_get_level(BUTTON_GPIO);

    while (true) {
        int level = gpio_get_level(BUTTON_GPIO);

        if (last_level != BUTTON_ACTIVE_LEVEL && level == BUTTON_ACTIVE_LEVEL) {
            led_on = !led_on;
            gpio_set_level(LED_GPIO, led_on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
            ESP_LOGI(TAG, "Button press detected, LED=%s", led_on ? "ON" : "OFF");
        }

        last_level = level;
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
             "RC test ready (GPIO=%d, active_level=%d, sample=%dms)",
             (int)BUTTON_GPIO,
             BUTTON_ACTIVE_LEVEL,
             SAMPLE_PERIOD_MS);
    ESP_LOGI(TAG, "Startup raw levels: button=%d, led_gpio=%d",
             gpio_get_level(BUTTON_GPIO), gpio_get_level(LED_GPIO));
}
