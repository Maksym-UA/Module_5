#include "mqtt.h"
#include "esp_log.h"
#include <string.h>
#include "esp_mac.h"

#define BUFFER_SIZE 128

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t s_client = NULL;
static mqtt_message_handler_t s_message_handler = NULL;
static volatile bool s_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    char topic_buf[BUFFER_SIZE] = {0};
    char data_buf[BUFFER_SIZE] = {0};
    int topic_len = 0;
    int data_len = 0;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            s_connected = true;
            if (esp_mqtt_client_subscribe(client, MQTT_COMMANDS, 0) < 0) {
                ESP_LOGE(TAG, "Failed to subscribe to %s", MQTT_COMMANDS);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            s_connected = false;
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Topic: %.*s, Data: %.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);

            if (s_message_handler != NULL) {
                topic_len = event->topic_len;
                data_len = event->data_len;

                if (topic_len >= BUFFER_SIZE) {
                    topic_len = BUFFER_SIZE - 1;
                }
                if (data_len >= BUFFER_SIZE) {
                    data_len = BUFFER_SIZE - 1;
                }

                memcpy(topic_buf, event->topic, topic_len);
                topic_buf[topic_len] = '\0';
                memcpy(data_buf, event->data, data_len);
                data_buf[data_len] = '\0';

                s_message_handler(topic_buf, data_buf);
            }
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;


        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;

    static char client_id[32];
    uint8_t mac[6] = {};
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    snprintf(client_id, sizeof(client_id),
             "esp32s3_%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    mqtt_cfg.credentials.client_id = client_id;
    mqtt_cfg.session.keepalive = 30;

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY,
                                                    mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));
}

esp_mqtt_client_handle_t mqtt_get_client(void)
{
    return s_client;
}

void mqtt_set_message_handler(mqtt_message_handler_t handler)
{
    s_message_handler = handler;
}

bool mqtt_is_connected(void)
{
    return s_connected;
}