#include "application.h"

#include <stdio.h>
#include "led.h"
#include "esp_log.h"

static const char *TAG = "PID";

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
    const esp_err_t init_err = Led::init_all();
    if (init_err != ESP_OK) {
        ESP_LOGE(TAG, "LED init failed: %s", esp_err_to_name(init_err));
        return;
    }
}