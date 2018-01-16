#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>

#include "events.h"

static const char* TAG = "WiFiSTA";

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	badge_event_t evt;

	memset(&evt, 0, sizeof(evt));

	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		evt.type = BADGE_EVENT_STA_STARTED;
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		evt.type = BADGE_EVENT_STA_CONNECTED;
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		evt.type = BADGE_EVENT_GOT_IP;
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		evt.type = BADGE_EVENT_STA_DISCONNECTED;
		break;
	case SYSTEM_EVENT_SCAN_DONE:
		evt.type = BADGE_EVENT_WIFI_SCAN_DONE;
		break;
	case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
		evt.type = BADGE_EVENT_AUTHMODE_CHANGED;
		break;
	case SYSTEM_EVENT_STA_STOP:
		evt.type = BADGE_EVENT_STA_STOPPED;
		break;
	default:
		ESP_LOGI(TAG, "Unknown system event type %d", event->event_id);
		return ESP_OK;
	}

	xQueueSendToBack(eventQueue, &evt, 10 / portTICK_PERIOD_MS);
    return ESP_OK;
}

esp_err_t wifi_sta_init()
{
	esp_err_t err;

	esp_event_loop_init(&wifi_event_handler, NULL);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	if ((err = esp_wifi_init(&cfg))) {
		ESP_LOGW(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
		return err;
	}

	/*
	if (err = esp_wifi_set_storage(WIFI_STORAGE_RAM)) {
		ESP_LOGW(TAG, "esp_wifi_set_storage failed: %s", esp_err_to_name(err))
		return err;
	}
	*/

	if ((err = esp_wifi_set_mode(WIFI_MODE_STA))) {
		ESP_LOGW(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
		return err;
	}

	/*
	wifi_config_t sta_config = {
		.sta = {
			.ssid = "DEFCON-Open",
			.password = "",
	        .bssid_set = false
	    }
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	*/

	if ((err = esp_wifi_start())) {
		ESP_LOGW(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
	}

	return ESP_OK;
}
