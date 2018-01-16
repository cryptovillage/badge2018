#ifndef MAIN_I2C_H_
#define MAIN_I2C_H_

extern void i2c_init();
extern esp_err_t i2c_efm8_erase_page(uint16_t addr);
extern esp_err_t i2c_efm8_write_flash(uint16_t addr, uint8_t data);
extern esp_err_t i2c_reboot_efm8_into_bootloader();
extern esp_err_t i2c_get_efm8_bootloader_crc(uint16_t * crc);
extern esp_err_t i2c_get_efm8_app_crc(uint16_t * crc);
extern esp_err_t i2c_get_efm8_uuid(uint8_t uuid[16]);
extern esp_err_t i2c_set_touch_electrodes(uint8_t);
extern esp_err_t i2c_config_is31fl3736();

#endif /* MAIN_I2C_H_ */
