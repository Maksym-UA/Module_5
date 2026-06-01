#include "application.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "PID";

void Application::start()
{
    ESP_LOGI(TAG, "Starting application...");
    run();
}