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

#define SAMPLE_PERIOD_MS   1
#define DEBOUNCE_SAMPLES  20   /* consecutive samples required to confirm press or release (20 ms) */

static TickType_t sample_period_ticks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);
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

/*
 * Button debounce using a 4-state FSM.
 *
 * Both press and release must be stable for DEBOUNCE_SAMPLES consecutive
 * samples before a state transition is accepted.  This eliminates the
 * lockout/latch interaction that could cause a press edge to be consumed
 * during a lockout window and then never re-detected.
 *
 * States:
 *   BTN_RELEASED   – idle, waiting for the first active sample
 *   BTN_PRESSING   – counting consecutive active samples; any inactive
 *                    sample resets the counter back to RELEASED
 *   BTN_PRESSED    – confirmed press (LED already toggled); waiting for
 *                    the first inactive sample
 *   BTN_RELEASING  – counting consecutive inactive samples; any active
 *                    sample resets the counter back to PRESSED
 */
typedef enum {
    BTN_RELEASED,
    BTN_PRESSING,
    BTN_PRESSED,
    BTN_RELEASING
} btn_state_t;

static void button_task(void *arg)
{
    (void)arg;

    TickType_t sample_ticks = sample_period_ticks();
    TickType_t last_wake    = xTaskGetTickCount();
    uint32_t   press_count  = 0;
    int        led_on       = (gpio_get_level(LED_GPIO) == LED_ACTIVE_LEVEL) ? 1 : 0;

    btn_state_t state         = BTN_RELEASED;
    uint8_t     debounce_cnt  = 0;

    while (true) {
        int raw = gpio_get_level(BUTTON_GPIO);

        switch (state) {

        case BTN_RELEASED:
            if (raw == BUTTON_ACTIVE_LEVEL) {
                debounce_cnt = 1;
                state = BTN_PRESSING;
            }
            break;

        case BTN_PRESSING:
            if (raw == BUTTON_ACTIVE_LEVEL) {
                if (++debounce_cnt >= DEBOUNCE_SAMPLES) {
                    /* Confirmed stable press – toggle LED once */
                    state = BTN_PRESSED;
                    press_count++;
                    led_on = !led_on;
                    gpio_set_level(LED_GPIO, led_on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
                    ESP_LOGI(TAG, "Click #%lu", (unsigned long)press_count);
                    log_button_led_state(raw, led_on);
                }
            } else {
                /* Bounce – restart */
                debounce_cnt = 0;
                state = BTN_RELEASED;
            }
            break;

        case BTN_PRESSED:
            if (raw != BUTTON_ACTIVE_LEVEL) {
                debounce_cnt = 1;
                state = BTN_RELEASING;
            }
            break;

        case BTN_RELEASING:
            if (raw != BUTTON_ACTIVE_LEVEL) {
                if (++debounce_cnt >= DEBOUNCE_SAMPLES) {
                    /* Confirmed stable release – re-arm for next press */
                    state = BTN_RELEASED;
                }
            } else {
                /* Still pressed – bounce back */
                debounce_cnt = 0;
                state = BTN_PRESSED;
            }
            break;
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
             "Button debounce ready (GPIO=%d, active_level=%d, sample=%dms, debounce=%d samples)",
             (int)BUTTON_GPIO,
             BUTTON_ACTIVE_LEVEL,
             SAMPLE_PERIOD_MS,
             DEBOUNCE_SAMPLES);
    ESP_LOGI(TAG, "Startup raw levels: button=%d, led_gpio=%d",
             gpio_get_level(BUTTON_GPIO), gpio_get_level(LED_GPIO));
    log_button_led_state(gpio_get_level(BUTTON_GPIO), gpio_get_level(LED_GPIO) == LED_ACTIVE_LEVEL);
}
