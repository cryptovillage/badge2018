#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/i2c.h"

#include "board.h"

static const char* TAG = "I2C";

static const i2c_config_t i2c_conf = {
	.mode = I2C_MODE_MASTER,
	.scl_io_num = PIN_I2C_SCL,
	.sda_io_num = PIN_I2C_SDA,
	.sda_pullup_en = GPIO_PULLUP_DISABLE,
	.scl_pullup_en = GPIO_PULLUP_DISABLE,
	.master.clk_speed = I2C_CLK_SPEED
};

void i2c_init()
{
	ESP_LOGD(TAG, "Initializing I2C interface");
	i2c_param_config(I2C_NUM_0, &i2c_conf);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

esp_err_t i2c_efm8_erase_page(uint16_t addr)
{
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 'E', 1);
	i2c_master_write_byte(cmd, addr >> 8, 1);
	i2c_master_write_byte(cmd, addr & 0xff, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	// FIXME: this is a hack
	// Wait a reasonable amount of time for the page to be erased
	vTaskDelay(100 / portTICK_PERIOD_MS);

	if (ret != ESP_OK)
		ESP_LOGW(TAG, "i2c_efm8_erase_page(%04x) result: %s", addr, esp_err_to_name(ret));
	return ret;
}

esp_err_t i2c_efm8_write_flash(uint16_t addr, uint8_t data)
{
	esp_err_t ret;

	// No need to write 0xFF
	if (data == 0xff)
		return ESP_OK;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 'W', 1);
	i2c_master_write_byte(cmd, addr >> 8, 1);
	i2c_master_write_byte(cmd, addr & 0xff, 1);
	i2c_master_write_byte(cmd, data, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	// FIXME: this is a hack
	// Wait a reasonable amount of time for the page to be written
	vTaskDelay(10 / portTICK_PERIOD_MS);

	if (ret != ESP_OK)
		ESP_LOGW(TAG, "i2c_efm8_write_page(%04x) result: %s", addr, esp_err_to_name(ret));
	return ret;
}
esp_err_t i2c_reboot_efm8_into_bootloader()
{
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 'B', 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK)
		ESP_LOGW(TAG, "i2c_reboot_efm8_into_bootloader() result: %s", esp_err_to_name(ret));
	return ret;
}

esp_err_t i2c_get_efm8_crc(uint16_t * crc, uint16_t startAddr, uint16_t endAddr) {
	esp_err_t ret;
	uint8_t replyCmd;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 'C', 1);
	i2c_master_write_byte(cmd, startAddr >> 8, 1);
	i2c_master_write_byte(cmd, startAddr & 0xff, 1);
	i2c_master_write_byte(cmd, endAddr >> 8, 1);
	i2c_master_write_byte(cmd, endAddr & 0xff, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		ESP_LOGI(TAG, "i2c_get_efm8_crc() write result: %s", esp_err_to_name(ret));
		//return ret;
	}

	// Sleep for enough time for the CRC to be computed
	vTaskDelay(100 / portTICK_PERIOD_MS);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR | 1, 1);
	i2c_master_read_byte(cmd, &replyCmd, 0);
	i2c_master_read_byte(cmd, ((uint8_t *)crc) + 1, 0);
	i2c_master_read_byte(cmd, (uint8_t *)crc, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		ESP_LOGI(TAG, "i2c_get_efm8_crc() read result: %s", esp_err_to_name(ret));
	}

	return ret;
}

esp_err_t i2c_get_efm8_bootloader_crc(uint16_t * crc)
{
	return i2c_get_efm8_crc(crc, EFM8_BOOTLOADER_START, EFM8_BOOTLOADER_STOP);
}

esp_err_t i2c_get_efm8_app_crc(uint16_t * crc)
{
	return i2c_get_efm8_crc(crc, EFM8_APP_START, EFM8_APP_STOP);
}

esp_err_t i2c_get_efm8_uuid(uint8_t uuid[16])
{
	esp_err_t ret;
	uint8_t replyCmd;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 'S', 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		ESP_LOGI(TAG, "i2c_get_efm8_uuid() write result: %s", esp_err_to_name(ret));
		return ret;
	}

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR | 1, 1);
	i2c_master_read_byte(cmd, &replyCmd, 0);
	i2c_master_read(cmd, uuid, 15, 0);
	i2c_master_read_byte(cmd, uuid + 15, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		ESP_LOGI(TAG, "i2c_get_efm8_uuid() read result: %s", esp_err_to_name(ret));
	}

	return ret;
}

esp_err_t i2c_set_touch_electrodes(uint8_t e)
{
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, EFM8_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 'T', 1);
	i2c_master_write_byte(cmd, e, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	ESP_LOGI(TAG, "i2c_set_touch_electrodes() result: %s", esp_err_to_name(ret));
	return ret;
}

esp_err_t i2c_config_is31fl3736()
{
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, IS31FL3736_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 0xFE, 1);
	i2c_master_write_byte(cmd, 0xC5, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret)
		goto fail;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, IS31FL3736_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 0xFD, 1);
	i2c_master_write_byte(cmd, 0x03, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret)
		goto fail;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, IS31FL3736_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 0x00, 1);
	i2c_master_write_byte(cmd, 0x03, 1);
	i2c_master_write_byte(cmd, 0x40, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret)
		goto fail;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, IS31FL3736_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 0xFE, 1);
	i2c_master_write_byte(cmd, 0xC5, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret)
		goto fail;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, IS31FL3736_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 0xFD, 1);
	i2c_master_write_byte(cmd, 0x00, 1);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret)
		goto fail;

	/*
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, IS31FL3736_I2C_ADDR, 1);
	i2c_master_write_byte(cmd, 0x00, 1);
	for (int i = 0; i < 0x18; i++) {
		i2c_master_write_byte(cmd, 0x55, 1);
	}
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret)
		goto fail;
	*/

	fail:
	ESP_LOGI(TAG, "i2c_config_config_is31fl3736() result: %s", esp_err_to_name(ret));
	return ret;
}
