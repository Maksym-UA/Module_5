#include "application.h"

#include <stdio.h>
#include <string.h>

#include "bm280.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt.h"
#include "nvs_flash.h"
#include "wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "application";

static constexpr gpio_num_t kLedGpio          = GPIO_NUM_16;
static constexpr uint32_t   kPublishIntervalMs = 15 * 1000;

// I2C pins wired to the BME280 breakout.
static constexpr int kI2cSda = 8;
static constexpr int kI2cScl = 9;

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

static void publish_sensor_value(esp_mqtt_client_handle_t client,
                                 const char *suffix,
                                 float value)
{
    char payload[32];
    char topic[64];

    snprintf(payload, sizeof(payload), "%.2f", value);
    snprintf(topic, sizeof(topic), "%s%s", MQTT_TOPIC, suffix);

    if (esp_mqtt_client_publish(client, topic, payload, 0, 1, 1) < 0) {
        ESP_LOGE(TAG, "Failed to publish %s", suffix);
    } else {
        ESP_LOGI(TAG, "Published -> %s : %s", topic, payload);
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

    // Initialise the BME280 sensor.
    bme280_app::BME280 sensor;
    const bool sensor_ok = sensor.begin(kI2cSda, kI2cScl);
    if (!sensor_ok) {
        ESP_LOGE(TAG, "BME280 init failed — check wiring (SDA=%d SCL=%d)", kI2cSda, kI2cScl);
    }

    // Wait for the MQTT broker connection before entering the publish loop.
    ESP_LOGI(TAG, "Waiting for MQTT connection...");
    while (!mqtt_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "MQTT connected — starting publish loop");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(kPublishIntervalMs));

        if (!mqtt_is_connected()) {
            ESP_LOGW(TAG, "MQTT not connected, skipping publish");
            continue;
        }

        if (!sensor_ok) {
            continue;
        }

        bme280_app::BME280Data data;
        if (!sensor.readData(data)) {
            ESP_LOGE(TAG, "BME280 read failed");
            continue;
        }

        ESP_LOGI(TAG, "BME280 T=%.2f°C H=%.2f%% P=%.2fhPa",
                 data.temperatureC, data.humidityPercent, data.pressureHpa);

        esp_mqtt_client_handle_t client = mqtt_get_client();
        if (client != NULL) {
            publish_sensor_value(client, "temperature", data.temperatureC);
            publish_sensor_value(client, "humidity", data.humidityPercent);
            publish_sensor_value(client, "pressure", data.pressureHpa);
        }
    }
}
