#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "apps/sntp/sntp.h"

#include "sdkconfig.h"

#include "events.h"
#include "fs.h"
#include "i2c.h"
#include "is31fl3736.h"
#include "touch.h"

static const char* TAG = "CPV2018";

// TODO: Move these into header files
extern esp_err_t wifi_sta_init();
extern void python_start(void);

is31fl3736_handle_t leds;

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void app_main(void)
{
	ESP_LOGI(TAG, "Booting the ESP32 / CPV 2018 Badge...");
	ESP_LOGI(TAG, "Version: %s", APP_GIT_VERSION);
	ESP_LOGI(TAG, "Heap: %d bytes free", xPortGetFreeHeapSize());

	eventq_init();
	nvs_flash_init();
	i2c_init();
	tp_init();
	fs_init();
	tcpip_adapter_init();
	wifi_sta_init();

	initialize_sntp();

	leds = iot_is31fl3736_create(0, 0, 121);

	ESP_LOGI(TAG, "Heap: %d bytes free", xPortGetFreeHeapSize());
	python_start();

	while (1) {
    	vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
