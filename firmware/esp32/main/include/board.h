#ifndef MAIN_BOARD_H_
#define MAIN_BOARD_H_

#include "sdkconfig.h"

#define PIN_I2C_SCL			22
#define PIN_I2C_SDA			21

#define I2C_CLK_SPEED		100000

#define IS31FL3736_I2C_ADDR	0xA0
#define EFM8_I2C_ADDR		0xC8
#define ESP32_I2C_ADDR		0xCC

#define EFM8_APP_START			0x0000
#define EFM8_APP_STOP			0x39FF
#define EFM8_BOOTLOADER_START	0x3A00
#define EFM8_BOOTLOADER_STOP	0x3FFF

#endif /* MAIN_BOARD_H_ */
