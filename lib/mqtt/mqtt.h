#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

#define MQTT_BROKER_URI  "mqtt://broker.hivemq.com:1883"
#define MQTT_TOPIC       "controller/bm280_"
#define MQTT_COMMANDS    "controller/commands"
#define MQTT_STATUS      "controller/status"

typedef void (*mqtt_message_handler_t)(const char *topic, const char *data);

void mqtt_app_start(void);
esp_mqtt_client_handle_t mqtt_get_client(void);
void mqtt_set_message_handler(mqtt_message_handler_t handler);

#endif // MQTT_H