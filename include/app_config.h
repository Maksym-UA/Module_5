#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

namespace AppConfig
{
inline constexpr const char* kLogTag = "application";

#define ESP_WIFI_SSID      "TP-Link_E3AC"
#define ESP_WIFI_PASS      "I&Mmansion2021"
#define ESP_MAXIMUM_RETRY  5
}
