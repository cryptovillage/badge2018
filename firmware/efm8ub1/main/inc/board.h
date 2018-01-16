#include <SI_EFM8UB1_Defs.h>
#include <SI_EFM8UB1_Register_Enums.h>
#include "fifo.h"

SI_SBIT(CHGSB, SFR_P0, 0);
SI_SBIT(ENSET, SFR_P0, 1);

#ifdef BOARD_PROTOTYPE
SI_SBIT(ESP32_EN, SFR_P0, 2);
#define VSW_ADC0MX ADC0MX_ADC0MX__ADC0P22;
#elif BOARD_PRODUCTION
SI_SBIT(ESP32_EN, SFR_P1, 4);
#define VSW_ADC0MX ADC0MX_ADC0MX__ADC0P2;
#else
#error Board type (BOARD_PROTOTYPE or BOARD_PRODUCTION) must be defined!
#endif

SI_SBIT(ESP32_BOOT, SFR_P0, 3);
SI_SBIT(VBUS, SFR_P3, 1);

SI_SBIT(TE3, SFR_P0, 6);
SI_SBIT(TE4, SFR_P0, 7);
SI_SBIT(TE5, SFR_P1, 0);
SI_SBIT(TE6, SFR_P1, 1);
SI_SBIT(TE1, SFR_P1, 2);
SI_SBIT(TE2, SFR_P1, 3);

typedef struct {
	uint8_t reportID;
	uint16_t powerFlags;
	uint16_t vbat;
} badgeHIDINReport_t;

extern void hid_sendBadgeINReport();

extern serialFifo_t rxFifo;
extern serialFifo_t txFifo;

extern uint32_t uartBPS;
extern uint8_t uartStopBits;
extern uint8_t uartParityType;
extern uint8_t uartDataBits;
extern uint8_t uartFlags;

extern void uart_init();

// These must match the bits used in SET_CONTROL_LINE_STATE
#define UART_FLAGS_DTR						1
#define UART_FLAGS_RTS						2
#define UART_FLAGS_UPDATED					128

extern uint8_t rxHIDBuf[64];
extern uint8_t txBuf[64];

extern void leds_init();
extern void leds_beginTx();
extern uint8_t leds_areBusy();
extern void leds_set_charging_pattern(uint8_t brightness);
extern void leds_set_charged_pattern(uint8_t brightness);

#define CHARGE_MODE_DISABLED				0
#define CHARGE_MODE_USB500					1
#define CHARGE_MODE_ISET					2
#define CHARGE_MODE_USB100					3

#define POWER_FLAG_CHARGER_MODE_SHIFT		6
#define POWER_FLAG_CHARGER_MODE_MASK		(3 << POWER_FLAG_CHARGER_MODE_SHIFT)

#define	POWER_FLAG_POWERED_ON				(1 << 0)
#define POWER_FLAG_PHYSICAL_SWITCH_ON		(1 << 1)
#define POWER_FLAG_VIRTUAL_SWITCH_ON		(1 << 2)
#define POWER_FLAG_ESP32_IN_BOOTLOADER		(1 << 3)
#define POWER_FLAG_BATTERY_VOLTAGE_UPDATED	(1 << 4)
#define POWER_FLAG_UPDATE_CHARGER_MODE		(1 << 5)
#define POWER_FLAG_CHARGER_MODE_USB500		(CHARGE_MODE_USB500 << POWER_FLAG_CHARGER_MODE_SHIFT)
#define POWER_FLAG_CHARGER_MODE_ISET		(CHARGE_MODE_ISET << POWER_FLAG_CHARGER_MODE_SHIFT)
#define POWER_FLAG_CHARGER_MODE_USB100		(CHARGE_MODE_USB100 << POWER_FLAG_CHARGER_MODE_SHIFT)
#define POWER_FLAG_SHUT_DOWN_LEDS			(1 << 8)
#define POWER_FLAG_USB_HOST_CONNECTED		(1 << 9)
#define POWER_FLAG_USB_HOST_CONFIGURED		(1 << 10)
#define POWER_FLAG_USB_DCP_CONNECTED		(1 << 11)
#define POWER_FLAG_USB_CDP_CONNECTED		(1 << 12)
#define POWER_FLAG_USB_CHARGER_CONNECTED	(POWER_FLAG_USB_DCP_CONNECTED | POWER_FLAG_USB_CDP_CONNECTED)
#define POWER_FLAG_BEGIN_CHARGER_DETECTION	(1 << 13)
#define POWER_FLAG_POWER_FLAGS_CHANGED		(1 << 15)

#define BATTERY_LED_FLAGS_OFF				0
#define BATTERY_LED_FLAGS_LOW_BATT_VOLTAGE	1 // Red
#define BATTERY_LED_FLAGS_MED_BATT_VOLTAGE	2 // Orange
#define BATTERY_LED_FLAGS_HIGH_BATT_VOLTAGE	3 // Yellow
#define BATTERY_LED_FLAGS_BATT_CHARGED		4 // Green
#define BATTERY_LED_FLAGS_COLOR_MASK		7
#define BATTERY_LED_FLAGS_CHARGE_OFF		(0 << 3)
#define BATTERY_LED_FLAGS_CHARGE_100		(1 << 3)
#define BATTERY_LED_FLAGS_CHARGE_500		(2 << 3)
#define BATTERY_LED_FLAGS_CHARGE_1000		(3 << 3)
#define BATTERY_LED_FLAGS_UPDATE_LED		(1 << 5)

extern uint16_t battVoltage;
extern uint16_t powerFlags;
extern uint16_t lastUARTTxTime;
extern uint16_t lastHIDTime;

extern void power_init();
extern void power_update();

#define MAX_I2C_PKT_SIZE					64

#define IS31FL3736_I2C_ADDR					0xA0
#define EFM8_I2C_ADDR						0xC8
#define ESP32_I2C_ADDR						0xCC

#define I2C_FLAGS_SLAVE_MSG_RECV			1
//#define I2C_FLAGS_SLAVE_READ_EXPECTED		2

extern uint8_t i2cFlags;

extern void i2c_init();
extern void i2c_update();
extern void i2c_setPowerLED(uint8_t red, uint8_t green, uint8_t blue);
extern void i2c_shutdownLEDs();
extern void i2c_initLEDController();

extern data uint8_t _cur_10khz_ctr;
extern data uint16_t _cur_ms;
extern void waithus(uint8_t count);
extern int16_t msDiff(uint16_t ts1, uint16_t ts2);

extern void reboot_into_bootloader();
extern void flash_writeByte(uint16_t addr, uint8_t byte);
extern void flash_erasePage(uint16_t addr);
extern void flash_start_crc16(uint16_t startAddr, uint16_t stopAddr);
extern uint8_t flash_get_upper_crc16();
extern uint8_t flash_get_lower_crc16();
extern uint16_t flash_get_crc16();

#define ESP32_BOOT_IDLE						1
#define ESP32_BOOT_ACTIVE					0

extern const uint8_t code uuid[16];

#define idle() do { \
	PCON0 |= PCON0_IDLE__IDLE; \
	NOP(); \
	NOP(); \
	NOP(); \
} while (0);
