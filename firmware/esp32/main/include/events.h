#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define EVENT_QUEUE_LENGTH	8

typedef enum {
	BADGE_EVENT_STA_STARTED,
	BADGE_EVENT_STA_CONNECTED,
	BADGE_EVENT_GOT_IP,
	BADGE_EVENT_STA_DISCONNECTED,
	BADGE_EVENT_WIFI_SCAN_DONE,
	BADGE_EVENT_AUTHMODE_CHANGED,
	BADGE_EVENT_STA_STOPPED,

	BADGE_EVENT_TOUCH_PAD_PRESSED,
	BADGE_EVENT_TOUCH_PAD_RELEASED,

	BADGE_EVENT_INCOMING_HID_MSG,
	BADGE_EVENT_POWER_CHANGE,
} badge_event_type_t;

typedef struct {
	uint16_t battVoltage;
	uint8_t flags;
} badge_event_power_data_t;

typedef union {
	uint8_t hidMsg[64];
	int touchPad;
	badge_event_power_data_t powerData;
} badge_event_data_t;

typedef struct {
	badge_event_type_t type;
	badge_event_data_t data;
} badge_event_t;

extern QueueHandle_t eventQueue;
extern void eventq_insert_touch_event(bool pressed, int pad);
extern void eventq_init();
