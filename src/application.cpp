#include "application.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt.h"
#include "nvs_flash.h"
#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "application";

static constexpr gpio_num_t kLedGpio = GPIO_NUM_16;
static constexpr uint32_t kPublishIntervalMs = 10 * 1000;

static void handle_mqtt_message(const char *topic, const char *data)
{
  if (topic == NULL || data == NULL) {
    return;
  }

  if (strcmp(topic, MQTT_COMMANDS) != 0) {
    return;
  }

  esp_mqtt_client_handle_t client = mqtt_get_client();

  if (strcmp(data, "ON") == 0) {
    ESP_LOGI(TAG, "Command: LED ON");
    if (gpio_set_level(kLedGpio, 1) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to set LED ON");
    }
  } else if (strcmp(data, "OFF") == 0) {
    ESP_LOGI(TAG, "Command: LED OFF");
    if (gpio_set_level(kLedGpio, 0) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to set LED OFF");
    }
  } else if (strcmp(data, "STATUS") == 0) {
    if (client) {
      if (esp_mqtt_client_publish(client, MQTT_STATUS, "ESP32-S3 is running", 0, 0, 0) < 0) {
        ESP_LOGE(TAG, "Failed to publish status");
      } else {
        ESP_LOGI(TAG, "Status sent");
      }
    }
  } else {
    ESP_LOGW(TAG, "Unknown command: %s", data);
  }
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
  uint32_t count = 0;
  char msg[64];

  // NVS is required by Wi-Fi driver.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << kLedGpio);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  ESP_ERROR_CHECK(gpio_config(&io_conf));
  ESP_ERROR_CHECK(gpio_set_level(kLedGpio, 0));

    wifi_init_sta();
  mqtt_set_message_handler(handle_mqtt_message);
  mqtt_app_start();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(kPublishIntervalMs));
    count++;
    snprintf(msg, sizeof(msg), "Hello from ESP32-S3! #%" PRIu32, count);

    esp_mqtt_client_handle_t client = mqtt_get_client();
    if (client) {
      if (esp_mqtt_client_publish(client, MQTT_TOPIC, msg, 0, 0, 0) < 0) {
        ESP_LOGE(TAG, "Failed to publish message");
      } else {
        ESP_LOGI(TAG, "Published: %s", msg);
      }
    }
  }
}
