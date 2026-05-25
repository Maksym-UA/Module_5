#include "application.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "debounce";

#define BUTTON_GPIO     GPIO_NUM_8
#define LED_GPIO        GPIO_NUM_9

/* Minimum time (µs) between two accepted edges — software guard on top of RC */
#define DEBOUNCE_US     (5 * 1000)   /* 5 ms */

static QueueHandle_t s_evt_queue = NULL;

/* ISR — runs in IRAM; only records the timestamp and defers processing */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time();
    xQueueSendFromISR(s_evt_queue, &now, NULL);
}

/* Task that processes edge events from the queue */
static void button_task(void *arg)
{
    int64_t last_edge_us = 0;
    int64_t edge_time_us = 0;
    uint32_t press_count = 0;

    while (true) {
        if (xQueueReceive(s_evt_queue, &edge_time_us, portMAX_DELAY)) {

            /* Software guard: ignore edges that arrive within DEBOUNCE_US */
            if ((edge_time_us - last_edge_us) < DEBOUNCE_US) {
                continue;
            }
            last_edge_us = edge_time_us;

            /* Read the stable level that the RC network has settled to */
            int level = gpio_get_level(BUTTON_GPIO);

            if (level == 1) {   /* active-high wiring: button pressed */
                press_count++;
                ESP_LOGI(TAG, "Button pressed  (#%lu)", (unsigned long)press_count);
                gpio_set_level(LED_GPIO, 1);
            } else {            /* button released */
                ESP_LOGI(TAG, "Button released (#%lu)", (unsigned long)press_count);
                gpio_set_level(LED_GPIO, 0);
            }
        }
    }
}

void application_init(void)
{
    /* ── LED output ── */
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_cfg);
    gpio_set_level(LED_GPIO, 0);

    /*
     * ── Button input ──
     * No internal pull-up/pull-down — the RC network already biases the pin.
     * Interrupt on both edges so we catch press AND release.
     */
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&btn_cfg);

    /* Queue holds timestamps (int64_t) for up to 10 pending events */
    s_evt_queue = xQueueCreate(10, sizeof(int64_t));

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, NULL);

    ESP_LOGI(TAG, "Button debounce ready (RC τ≈1ms + 5ms SW guard)");
}
