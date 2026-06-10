#include "application.h"

#include "app_config.h"
#include "wifi.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_http_client.h"
#include "esp_netif.h"



void application_init()
{
    Application app;
    app.start();
}

void Application::start()
{
    ESP_LOGI(AppConfig::kLogTag, "Starting application...");
    run();
}

void Application::run()
{
     //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(AppConfig::kLogTag, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    while (1)
    {
		if (isConnected == 1)
		{
			int dummy1 = esp_random() %100;
			int dummy2 = esp_random() %100;
      ESP_LOGI(AppConfig::kLogTag, "Sending Values %d\t%d", dummy1, dummy2);
			
			vTaskDelay(pdMS_TO_TICKS(15000));
		}
	}
}
