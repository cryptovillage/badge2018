#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "fs.h"

static const char *TAG = "fs";

void fs_init()
{
	ESP_LOGI(TAG, "Initializing /user partition");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/user",
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
    	ESP_LOGW(TAG, "esp_vfs_spiffs_register failed: %s", esp_err_to_name(ret));
    }
}
