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

#include "oled.h"

static const char *TAG = "application";

static constexpr gpio_num_t kLedGpio          = GPIO_NUM_16;
static constexpr uint32_t   kPublishIntervalMs = 15 * 1000;

// I2C pins wired to the BME280 breakout.
static constexpr int kI2cSda = 8;
static constexpr int kI2cScl = 9;

static int32_t to_scaled_100(float value)
{
  const float scaled = value * 100.0F;
  return static_cast<int32_t>(scaled >= 0.0F ? scaled + 0.5F : scaled - 0.5F);
}

static void format_scaled_100(char *buffer, size_t buffer_size, float value)
{
  const int32_t scaled = to_scaled_100(value);
  const int32_t whole = scaled / 100;
  const int32_t fraction = scaled >= 0 ? (scaled % 100) : -(scaled % 100);
  snprintf(buffer, buffer_size, "%ld.%02ld", static_cast<long>(whole), static_cast<long>(fraction));
}

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

    format_scaled_100(payload, sizeof(payload), value);
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

    if (oled_app::init(kI2cSda, kI2cScl) == ESP_OK) {
        oled_app::showStartup();
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

        char temp_text[16];
        char humidity_text[16];
        char pressure_text[16];
        format_scaled_100(temp_text, sizeof(temp_text), data.temperatureC);
        format_scaled_100(humidity_text, sizeof(humidity_text), data.humidityPercent);
        format_scaled_100(pressure_text, sizeof(pressure_text), data.pressureHpa);

        ESP_LOGI(TAG, "BME280 T=%s C H=%s%% P=%s hPa",
           temp_text,
           humidity_text,
           pressure_text);

        oled_app::SensorDisplayData display_data{
            .temperatureC = data.temperatureC,
            .humidityPercent = data.humidityPercent,
            .pressureHpa = data.pressureHpa,
        };
        oled_app::showSensorData(display_data);

        esp_mqtt_client_handle_t client = mqtt_get_client();
        if (client != NULL) {
            publish_sensor_value(client, "temperature", data.temperatureC);
            publish_sensor_value(client, "humidity", data.humidityPercent);
            publish_sensor_value(client, "pressure", data.pressureHpa);
        }
    }
}
