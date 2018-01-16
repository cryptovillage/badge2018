#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"

#include "i2c.h"

static const char* TAG = "CPV2018";

int update_efm8(const uint8_t *boot_ptr, size_t boot_size)
{
	const uint8_t *ptr;
	uint8_t len;
	uint8_t resp;

	ESP_LOGI(TAG, "Updating EFM8...");

	esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_LF);

	vTaskDelay(500 / portTICK_PERIOD_MS);
	i2c_reboot_efm8_into_bootloader();
	vTaskDelay(500 / portTICK_PERIOD_MS);

	for (ptr = boot_ptr; ptr < boot_ptr + boot_size; ) {
		if (*ptr != '$') {
			return -1;
		}
		write(1, ptr++, 1);
		len = *ptr;
		write(1, ptr++, 1);
		while (len--) {
			write(1, ptr++, 1);
		}
		// The REPL will steal any bytes coming back, so assume
		// everything is fine after 100ms
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	vTaskDelay(500 / portTICK_PERIOD_MS);
	esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
	ESP_LOGI(TAG, "EFM8 Update complete!");

	return 0;
}

int update_efm8_bootloader(const uint8_t *boot_ptr, size_t boot_size)
{
	const uint8_t *ptr, *nextPtr;
	uint8_t len;
	uint16_t addr;

	//ESP_LOGI(TAG, "Updating the EFM8 bootloader...");

	for (ptr = boot_ptr; ptr < boot_ptr + boot_size; ) {
		if (*ptr++ != '$') {
			ESP_LOGW(TAG, "boot file desync!");
			return -1;
		}
		len = *ptr++;
		nextPtr = ptr + len;
		switch (*ptr++) {
		case 0x32:
			addr = *ptr++ << 8;
			addr |= *ptr++;
			if (addr < 0x4000 || addr >= 0xf800) {
				//ESP_LOGI(TAG, "Erasing page %04x", addr);
				i2c_efm8_erase_page(addr);
				vTaskDelay(200 / portTICK_PERIOD_MS);
				len -= 3;
				while (len--) {
					while (i2c_efm8_write_flash(addr, *ptr));
					addr++;
					ptr++;
				}
			} else {
				//ESP_LOGI(TAG, "Ignoring address %04x", addr);
			}
			break;
		case 0x33:
			addr = *ptr++ << 8;
			addr |= *ptr++;
			if (addr < 0x4000 || addr >= 0xf800) {
				//ESP_LOGI(TAG, "Writing to %04x", addr);
				len -= 3;
				while (len--) {
					while (i2c_efm8_write_flash(addr, *ptr));
					addr++;
					ptr++;;
				}
			}
			break;
		}
		ptr = nextPtr;
	}

	//ESP_LOGI(TAG, "EFM8 bootloader update complete!");
	return 0;
}
