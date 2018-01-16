#include "board.h"

uint16_t battVoltage = 0;
uint16_t powerFlags = POWER_FLAG_BEGIN_CHARGER_DETECTION;
uint16_t dtrLowTime = 0;
uint16_t esp32FlashTime = 0;
uint16_t lastBattADCTime = 64000;
uint16_t lastUARTTxTime = 0;
uint16_t lastHIDTime = 0;
uint16_t chgsbHistory = 0xffff;
uint8_t batteryLEDFlags = 0;
uint16_t lastChargeAnimTime = 0;
uint8_t chargeAnimCycle = 0;
uint16_t chargeAnimDelay = 100;

const uint8_t gamma64[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 10, 12, 14, 16, 18, 20, 22,
	24, 26, 29, 32, 35, 38, 41, 44,
	47, 50, 53, 57, 61, 65, 69, 73,
	77, 81, 85, 89, 94, 99, 104, 109,
	114, 119, 124, 129, 134, 140, 146, 152,
	158, 164, 170, 176, 182, 188, 195, 202,
	209, 216, 223, 230, 237, 244, 251, 255
};

void power_set_charge_mode(uint8_t count) {
	ENSET = 1;

	// If count is 0, leave the charger disabled. Otherwise,
	// decrement by one and send that number of pulses
	if (!count--) {
		return;
	}

	waithus(20);
	ENSET = 0;

	while (count--) {
		waithus(3);
		ENSET = 1;
		waithus(3);
		ENSET = 0;
	}

}

void power_init()
{
	// Start the charger off in 100 mA mode until we know we can draw more power
	power_set_charge_mode(CHARGE_MODE_USB100);

	// Check power switch comparator in case the initial rising edge is missed
	SFRPAGE = 0;
	if (CMP0CN0 & (1 << 6)) {
		powerFlags |= POWER_FLAG_PHYSICAL_SWITCH_ON | POWER_FLAG_POWER_FLAGS_CHANGED;
	}
}

void power_update()
{
	uint16_t diff;
	uint16_t gammaRed;
	uint16_t gammaGreen;
	uint16_t gammaBlue;

	if (CHGSB) {
		if (chgsbHistory == 0x7fff) {
			// Force an LED update if we're about to be done charging
			batteryLEDFlags = 0;
		}
		chgsbHistory = chgsbHistory << 1 | 1;
	} else {
		chgsbHistory = chgsbHistory << 1;
	}

	IE_EA = 0;

	if (powerFlags & POWER_FLAG_BEGIN_CHARGER_DETECTION) {
		powerFlags &= ~POWER_FLAG_BEGIN_CHARGER_DETECTION;
		SFRPAGE = 0x20;
		USB0XCN = 0;
		USB0CDCN = USB0CDCN_CHDEN__ENABLED | USB0CDCN_DCDEN__ENABLED |
				USB0CDCN_PDEN__ENABLED | USB0CDCN_SDEN__ENABLED;
	}

	if (!(powerFlags & POWER_FLAG_VIRTUAL_SWITCH_ON)) {
		if (lastHIDTime || lastUARTTxTime) {
			powerFlags |= POWER_FLAG_VIRTUAL_SWITCH_ON | POWER_FLAG_POWER_FLAGS_CHANGED;
		}
	}

	if (powerFlags & POWER_FLAG_POWER_FLAGS_CHANGED) {
		powerFlags &= ~POWER_FLAG_POWER_FLAGS_CHANGED;
		if (powerFlags & (POWER_FLAG_PHYSICAL_SWITCH_ON | POWER_FLAG_VIRTUAL_SWITCH_ON)) {
			powerFlags |= POWER_FLAG_POWERED_ON;
			powerFlags = (powerFlags & ~POWER_FLAG_CHARGER_MODE_MASK) |
					POWER_FLAG_UPDATE_CHARGER_MODE | POWER_FLAG_POWERED_ON;
			if (powerFlags & POWER_FLAG_USB_CHARGER_CONNECTED) {
				powerFlags |= POWER_FLAG_CHARGER_MODE_USB500;
				chargeAnimDelay = 45;
			} else {
				powerFlags |= POWER_FLAG_CHARGER_MODE_USB100;
				chargeAnimDelay = 100;
			}
		} else {
			powerFlags = (powerFlags & ~POWER_FLAG_CHARGER_MODE_MASK & ~POWER_FLAG_POWERED_ON) |
					POWER_FLAG_UPDATE_CHARGER_MODE | POWER_FLAG_SHUT_DOWN_LEDS;
			if (powerFlags & POWER_FLAG_USB_CHARGER_CONNECTED) {
				powerFlags |= POWER_FLAG_CHARGER_MODE_ISET;
				chargeAnimDelay = 15;
			} else {
				powerFlags |= POWER_FLAG_CHARGER_MODE_USB500;
				chargeAnimDelay = 45;
			}
		}
	}

	if (uartFlags & UART_FLAGS_UPDATED) {
		if (!(uartFlags & UART_FLAGS_DTR)) {
			dtrLowTime = _cur_ms;
		} else { // DTR is set
			if ((uartFlags & UART_FLAGS_RTS) && dtrLowTime) {
				// Check for a signal to begin reflashing the ESP32
				diff = msDiff(dtrLowTime, _cur_ms);
				if (diff > 80 && diff < 1600) {
					powerFlags |= POWER_FLAG_VIRTUAL_SWITCH_ON | POWER_FLAG_POWER_FLAGS_CHANGED;
					esp32FlashTime = _cur_ms;
					ESP32_EN = 0;
				}
			}
			dtrLowTime = 0;
		}
		uartFlags &= ~UART_FLAGS_UPDATED;
	}

	if (battVoltage < 745) {
		if ((batteryLEDFlags & BATTERY_LED_FLAGS_COLOR_MASK) !=
				BATTERY_LED_FLAGS_LOW_BATT_VOLTAGE) {
			batteryLEDFlags &= ~BATTERY_LED_FLAGS_COLOR_MASK;
			batteryLEDFlags |= BATTERY_LED_FLAGS_LOW_BATT_VOLTAGE |
					BATTERY_LED_FLAGS_UPDATE_LED;
		}
	} else if (battVoltage < 845 && battVoltage > 750) {
		if ((batteryLEDFlags & BATTERY_LED_FLAGS_COLOR_MASK) !=
				BATTERY_LED_FLAGS_MED_BATT_VOLTAGE) {
			batteryLEDFlags &= ~BATTERY_LED_FLAGS_COLOR_MASK;
			batteryLEDFlags |= BATTERY_LED_FLAGS_MED_BATT_VOLTAGE |
					BATTERY_LED_FLAGS_UPDATE_LED;
		}
	} else if (battVoltage > 850) {
		if (chgsbHistory == 0) {
			if ((batteryLEDFlags & BATTERY_LED_FLAGS_COLOR_MASK) !=
					BATTERY_LED_FLAGS_HIGH_BATT_VOLTAGE) {
				batteryLEDFlags &= ~BATTERY_LED_FLAGS_COLOR_MASK;
				batteryLEDFlags |= BATTERY_LED_FLAGS_HIGH_BATT_VOLTAGE |
						BATTERY_LED_FLAGS_UPDATE_LED;
			}
		} else if (chgsbHistory == 0xffff) {
			if ((batteryLEDFlags & BATTERY_LED_FLAGS_COLOR_MASK) !=
					BATTERY_LED_FLAGS_BATT_CHARGED) {
				batteryLEDFlags &= ~BATTERY_LED_FLAGS_COLOR_MASK;
				batteryLEDFlags |= BATTERY_LED_FLAGS_BATT_CHARGED |
						BATTERY_LED_FLAGS_UPDATE_LED;
			}
		}
	}

	if (powerFlags & POWER_FLAG_UPDATE_CHARGER_MODE) {
		powerFlags &= ~POWER_FLAG_UPDATE_CHARGER_MODE;
		IE_EA = 1;
		power_set_charge_mode((powerFlags & POWER_FLAG_CHARGER_MODE_MASK)
				>> POWER_FLAG_CHARGER_MODE_SHIFT);
	} else {
		IE_EA = 1;
	}

	if (batteryLEDFlags & BATTERY_LED_FLAGS_UPDATE_LED) {
		batteryLEDFlags &= ~BATTERY_LED_FLAGS_UPDATE_LED;
		if (chgsbHistory == 0xffff) {
			switch (batteryLEDFlags & BATTERY_LED_FLAGS_COLOR_MASK) {
			case BATTERY_LED_FLAGS_LOW_BATT_VOLTAGE:
				i2c_setPowerLED(128, 0, 0);
				break;
			case BATTERY_LED_FLAGS_MED_BATT_VOLTAGE:
				i2c_setPowerLED(128, 24, 0);
				break;
			// We shouldn't ever show yellow unless charging, but just in case
			case BATTERY_LED_FLAGS_HIGH_BATT_VOLTAGE:
				i2c_setPowerLED(128, 96, 0);
				break;
			case BATTERY_LED_FLAGS_BATT_CHARGED:
				i2c_setPowerLED(0, 128, 0);
				break;
			}
		}
	}

	if (esp32FlashTime) {
		diff = msDiff(esp32FlashTime, _cur_ms);
		if (diff > 300) {
			ESP32_BOOT = ESP32_BOOT_IDLE;
			esp32FlashTime = 0;
		} else if (diff > 200) {
			ESP32_EN = 1;
		} else if (diff > 100) {
			ESP32_BOOT = ESP32_BOOT_ACTIVE;
			IE_EA = 0;
			powerFlags |= POWER_FLAG_ESP32_IN_BOOTLOADER;
			IE_EA = 1;
		}
	} else if (powerFlags & POWER_FLAG_ESP32_IN_BOOTLOADER) {
		diff = msDiff(lastUARTTxTime, _cur_ms);
		if (diff > 5100) {
			IE_EA = 0;
			powerFlags &= ~POWER_FLAG_ESP32_IN_BOOTLOADER;
			IE_EA = 1;
		} else if (diff > 5000) {
			ESP32_EN = 0;
		}
	} else {
		if (powerFlags & POWER_FLAG_POWERED_ON) {
			ESP32_EN = 1;
		} else {
			ESP32_EN = 0;
		}
	}

	if (powerFlags & POWER_FLAG_SHUT_DOWN_LEDS) {
		IE_EA = 0;
		powerFlags &= ~POWER_FLAG_SHUT_DOWN_LEDS;
		IE_EA = 1;
		i2c_shutdownLEDs();
	}

	if (chgsbHistory == 0 && !(powerFlags & POWER_FLAG_POWERED_ON)) {
		if (msDiff(lastChargeAnimTime, _cur_ms) >= chargeAnimDelay) {
			switch (batteryLEDFlags & BATTERY_LED_FLAGS_COLOR_MASK) {
			case BATTERY_LED_FLAGS_LOW_BATT_VOLTAGE:
				gammaRed = 128;
				gammaGreen = 0;
				gammaBlue = 0;
				break;
			case BATTERY_LED_FLAGS_MED_BATT_VOLTAGE:
				gammaRed = 128;
				gammaGreen = 24;
				gammaBlue = 0;
				break;
			case BATTERY_LED_FLAGS_HIGH_BATT_VOLTAGE:
				gammaRed = 128;
				gammaGreen = 96;
				gammaBlue = 0;
				break;
			}

			if (chargeAnimCycle < 56) {
				gammaRed *= gamma64[chargeAnimCycle + 8];
				gammaGreen *= gamma64[chargeAnimCycle + 8];
				gammaBlue *= gamma64[chargeAnimCycle + 8];
			} else {
				gammaRed *= gamma64[118 - chargeAnimCycle];
				gammaGreen *= gamma64[118 - chargeAnimCycle];
				gammaBlue *= gamma64[118 - chargeAnimCycle];
			}
			i2c_setPowerLED(gammaRed >> 8, gammaGreen >> 8, gammaBlue >> 8);

			if (++chargeAnimCycle >= 112) {
				chargeAnimCycle = 0;
			}

			lastChargeAnimTime = _cur_ms;
		}
	} else {
		lastChargeAnimTime = _cur_ms;
	}

	// Read the battery voltage every second
	if (msDiff(lastBattADCTime, _cur_ms) >= 1000) {
		SFRPAGE = 0;
		ADC0CN0_ADBUSY = 1;
		lastBattADCTime = _cur_ms;
	}
}

//-----------------------------------------------------------------------------
// ADC0EOC_ISR
//-----------------------------------------------------------------------------
//
// ADC0EOC ISR Content goes here. Remember to clear flag bits:
// ADC0CN0::ADINT (Conversion Complete Interrupt Flag)
//
//-----------------------------------------------------------------------------
SI_INTERRUPT (ADC0EOC_ISR, ADC0EOC_IRQn)
{
	battVoltage = ADC0;
	powerFlags |= POWER_FLAG_BATTERY_VOLTAGE_UPDATED;
	ADC0CN0_ADINT = 0;

	/*
	if (!(powerFlags & POWER_FLAG_MEASURE_VSW)) {
		battVoltage = ADC0;
		powerFlags |= POWER_FLAG_BATTERY_VOLTAGE_UPDATED |
				POWER_FLAG_MEASURE_VSW;
		ADC0CN0_ADINT = 0;
		ADC0MX = VSW_ADC0MX;
		ADC0CN0_ADBUSY = 1;
	} else {
		vswVoltage = ADC0;
		powerFlags = (powerFlags & ~POWER_FLAG_MEASURE_VSW) |
				POWER_FLAG_VSW_VOLTAGE_UPDATED;
		ADC0MX = ADC0MX_ADC0MX__ADC0P23;
		ADC0CN0_ADINT = 0;
	}
	*/
}


//-----------------------------------------------------------------------------
// CMP0_ISR
//-----------------------------------------------------------------------------
//
// CMP0 ISR Content goes here. Remember to clear flag bits:
// CMP0CN0::CPFIF (Comparator Falling-Edge Flag)
// CMP0CN0::CPRIF (Comparator Rising-Edge Flag)
//
//-----------------------------------------------------------------------------

#define CPRIF (1 << 5)
#define CPFIF (1 << 4)

SI_INTERRUPT (CMP0_ISR, CMP0_IRQn)
{
	if (CMP0CN0 & CPRIF) {
		CMP0CN0 &= ~CPRIF;
		powerFlags |= POWER_FLAG_PHYSICAL_SWITCH_ON | POWER_FLAG_POWER_FLAGS_CHANGED;
	}
	if (CMP0CN0 & CPFIF) {
		CMP0CN0 &= ~CPFIF;
		powerFlags &= ~POWER_FLAG_PHYSICAL_SWITCH_ON;
		powerFlags |= POWER_FLAG_POWER_FLAGS_CHANGED;
	}
}
