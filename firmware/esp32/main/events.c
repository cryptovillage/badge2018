#include <string.h>
#include "events.h"
#include "esp_log.h"

static const char* TAG = "eventq";
QueueHandle_t eventQueue;

void eventq_insert_touch_event(bool pressed, int pad) {
	badge_event_t evt;

	memset(&evt, 0, sizeof(evt));

	if (pressed) {
		evt.type = BADGE_EVENT_TOUCH_PAD_PRESSED;
	} else {
		evt.type = BADGE_EVENT_TOUCH_PAD_RELEASED;
	}

	evt.data.touchPad = pad;

	xQueueSendToBack(eventQueue, &evt, 0);
}

void eventq_init() {
	eventQueue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(badge_event_t));
	if (!eventQueue) {
		ESP_LOGW(TAG, "Could not create event queue!");
	}
}
