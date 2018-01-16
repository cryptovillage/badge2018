#include <SI_EFM8UB1_Register_Enums.h>
#include "efm8_usb.h"
#include "board.h"

uint8_t i2cMBuf[MAX_I2C_PKT_SIZE];
uint8_t i2cSBuf[MAX_I2C_PKT_SIZE];
uint8_t i2cMAddr;
uint8_t i2cMBufPtr;
uint8_t i2cSBufPtr;
uint8_t i2cMBufLen;
uint8_t i2cSBufLen;
uint8_t i2cFlags;

void i2c_enable_slave()
{
	i2cSBufPtr = i2cSBuf;
	i2cSBufLen = sizeof(i2cSBuf);
	// FIXME(supersat): What happens when we inhibit the slave but have EHACK on?
	SFRPAGE = 0;
	SMB0CF &= ~SMB0CF_INH__SLAVE_DISABLED;
}

void i2c_init()
{
	i2cFlags = 0;
	i2cMAddr = 0;
	i2cMBufPtr = 0;
	i2cSBufPtr = 0;
	i2cMBufLen = 0;
	i2cSBufLen = 0;

	i2c_enable_slave();
}

void i2c_update()
{
	uint8_t i;

	if (i2cFlags & I2C_FLAGS_SLAVE_MSG_RECV) {
		if (i2cSBuf[0] == 'T') {
			TE1 = (i2cSBuf[1] & (1 << 0)) ? 1 : 0;
			TE2 = (i2cSBuf[1] & (1 << 1)) ? 1 : 0;
			TE3 = (i2cSBuf[1] & (1 << 2)) ? 1 : 0;
			TE4 = (i2cSBuf[1] & (1 << 3)) ? 1 : 0;
			TE5 = (i2cSBuf[1] & (1 << 4)) ? 1 : 0;
			TE6 = (i2cSBuf[1] & (1 << 5)) ? 1 : 0;
		} else if (i2cSBuf[0] == 'H') { // HID packet
			if (USBD_EpIsBusy(EP3IN)) {
				return; // Don't re-enable I2C slave until we can send the HID msg
			}
			USBD_Write(EP3IN, i2cSBuf + 1, (uint16_t)i2cSBufPtr - (uint16_t)i2cSBuf - 1, false);
		} else if (i2cSBuf[0] == 'B') {
			reboot_into_bootloader();
		} else if (i2cSBuf[0] == 'E') {
			IE_EA = 0;
			flash_erasePage((i2cSBuf[1] << 8) | i2cSBuf[2]);
			IE_EA = 1;
		} else if (i2cSBuf[0] == 'W') {
			IE_EA = 0;
			flash_writeByte((i2cSBuf[1] << 8) | i2cSBuf[2], i2cSBuf[3]);
			IE_EA = 1;
		} else if (i2cSBuf[0] == 'C') {
			flash_start_crc16((i2cSBuf[1] << 8) | i2cSBuf[2], (i2cSBuf[3] << 8) | i2cSBuf[4]);
			i2cSBuf[1] = flash_get_upper_crc16();
			i2cSBuf[2] = flash_get_lower_crc16();
		} else if (i2cSBuf[0] == 'S') {
			for (i = 0; i < 16; i++) {
				i2cSBuf[i + 1] = uuid[i];
			}
		}
		IE_EA = 0;
		i2cFlags &= ~I2C_FLAGS_SLAVE_MSG_RECV;
		i2c_enable_slave();
		IE_EA = 1;
	}
}

int i2c_write(uint8_t addr, uint8_t *buf, uint8_t len, uint8_t attempts)
{
	uint8_t i;
	if (buf) {
		for (i = 0; i < len; i++) {
			i2cMBuf[i] = buf[i];
		}
	}
	i2cMAddr = addr;
	i2cMBufPtr = 0;
	i2cMBufLen = len;
	SFRPAGE = 0;
	SMB0CN0_STA = 1;
	while (i2cMAddr) {
		idle();

		// Retry transaction if we are no longer the master
		if (!SMB0CN0_MASTER) {
			if (!attempts--) {
				return 0;
			}

			i2cMBufPtr = 0;
			SMB0CN0_STA = 1;
		}
	}

	return 1;
}

void i2c_setCmdReg(uint8_t cmdReg)
{
	i2cMBuf[0] = 0xfe;
	i2cMBuf[1] = 0xc5;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 2, 5);

	i2cMBuf[0] = 0xfd;
	i2cMBuf[1] = cmdReg;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 2, 5);
}

void i2c_setPowerLED(uint8_t red, uint8_t green, uint8_t blue)
{
	i2c_setCmdReg(1);
	i2cMBuf[0] = 0x9e;
	i2cMBuf[1] = red;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 2, 5);
	i2cMBuf[0] = 0xae;
	i2cMBuf[1] = green;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 2, 5);
	i2cMBuf[0] = 0xbe;
	i2cMBuf[1] = blue;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 2, 5);
}

void i2c_shutdownLEDs()
{
	uint8_t i;

	// Turn off all LEDs except those in D32
	i2c_setCmdReg(0);
	for (i = 0; i < 0x19; i++) {
		i2cMBuf[i] = 0;
	}
	i2cMBuf[0x14] = 0x40;
	i2cMBuf[0x16] = 0x40;
	i2cMBuf[0x18] = 0x40;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 0x19, 5);
}

void i2c_initLEDController()
{
	uint8_t i;

	// Set MODE = 0
	i2c_setCmdReg(3);
	i2cMBuf[0] = 0;
	i2cMBuf[1] = 0;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 2, 5);

	i2c_shutdownLEDs();

	// Clear PWM data for all LEDs
	i2c_setCmdReg(1);
	i2cMBuf[1] = 0;
	for (i = 0; i < 96; i++) {
		i2cMBuf[0] = i << 1;
		i2c_write(IS31FL3736_I2C_ADDR, NULL, 2, 5);
	}

	// Set MODE = 1, global current = 121
	i2c_setCmdReg(3);
	i2cMBuf[0] = 0;
	i2cMBuf[1] = 1;
	i2cMBuf[2] = 121;
	i2c_write(IS31FL3736_I2C_ADDR, NULL, 3, 5);
}

SI_INTERRUPT (SMBUS0_ISR, SMBUS0_IRQn)
{
	if (SMB0CN0_ARBLOST == 0) {
		if (SMB0CN0_MASTER) {
			if (SMB0CN0_STA) {
				SMB0CN0_STA = 0;
				SMB0CN0_STO = 0;
				SMB0DAT = i2cMAddr;
			} else if (SMB0CN0_ACK) {
				if (i2cMAddr & 1) { // Read address
					if (i2cMBufPtr < i2cMBufLen) {
						i2cMBuf[i2cMBufPtr++] = SMB0DAT;
						if (i2cMBufPtr < i2cMBufLen) {
							SMB0CN0_ACK = 1;
						} else {
							i2cMAddr = 0;
							SMB0CN0_STO = 1;
						}
					} else {
						SMB0CN0_STO = 1;
					}
				} else { // Write address
					if (i2cMBufPtr < i2cMBufLen) {
						SMB0DAT = i2cMBuf[i2cMBufPtr++];
					} else {
						i2cMAddr = 0;
						SMB0CN0_STO = 1;
					}
				}
			} else {
				SMB0CN0_STO = 1;
			}
		} else { // Slave
			if (SMB0CN0_STA) {
				SMB0CN0_STA = 0;
				i2cSBufPtr = 0;
				if (SMB0DAT & 1) { // Read address
					SMB0DAT = i2cSBuf[i2cSBufPtr++];
				}
			} else if (SMB0CN0_STO) {
				i2cFlags |= I2C_FLAGS_SLAVE_MSG_RECV;
				// FIXME(supersat): Is inhibiting slave interrupts what we should do here? Even with EHACK?
				//SMB0CF |= SMB0CF_INH__SLAVE_DISABLED;
				SMB0CN0_STO = 0;
			} else {
				if (SMB0CN0_TXMODE) {
					if (SMB0CN0_ACK && (i2cSBufPtr < i2cSBufLen)) {
						SMB0DAT = i2cSBuf[i2cSBufPtr++];
					}
				} else {
					i2cSBuf[i2cSBufPtr++] = SMB0DAT;
					if (i2cSBufPtr < i2cSBufLen) {
						SMB0CN0_ACK = 1;
					}
				}
			}
		}
	} else { // Lost arbitration
		SMB0CN0_STA = 0;
		SMB0CN0_STO = 0;
		SMB0CN0_ACK = 1;
	}

	SMB0CN0_SI = 0;
}
