#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "board.h"
#include "events.h"

static const char* TAG = "TOUCH";
static uint16_t lastTouchStatus;
static uint16_t tpThreshold[TOUCH_PAD_MAX];

static void tp_read_callback(uint16_t *raw_value, uint16_t *filtered_value)
{
	uint16_t touchStatus = 0;

	for (int i = 2; i < 8; i++) {
		if (filtered_value[i] < tpThreshold[i]) {
			touchStatus |= (1 << i);
			if (!(lastTouchStatus & (1 << i))) {
				eventq_insert_touch_event(1, i);
			}
		} else {
			if (lastTouchStatus & (1 << i)) {
				eventq_insert_touch_event(0, i);
			}
		}
	}

	lastTouchStatus = touchStatus;
}

/*
static void tp_poll_task(void *pvParameter) {
	uint16_t touch_value[8];

	for (;;) {
		for (int i = 2; i < 8; i++) {
			ESP_ERROR_CHECK(touch_pad_read(i, &touch_value[i]));
		}

		ESP_LOGI(TAG, "%04d %04d %04d %04d %04d %04d %08x %08x",
				touch_value[2], touch_value[3],
				touch_value[4], touch_value[5],
				touch_value[6], touch_value[7],
				lastTouchStatus, touch_pad_get_status());

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
*/

void tp_init(void)
{
    touch_pad_init();
    lastTouchStatus = 0;
    memset(tpThreshold, 0, sizeof(tpThreshold));

    // First set up the touch pads with a threshold of 0 so we can measure the
    // current value to set the threshold properly
    for (int i = 2; i < 8; i++) {
    	touch_pad_config(i, 0);
    }

    // Wait for the touch pads to settle. Is this necessary?
    //vTaskDelay(100 / portTICK_PERIOD_MS);

    for (int i = 2; i < 8; i++) {
    	uint16_t reading;
    	if (touch_pad_read(i, &reading) != ESP_OK) {
    		ESP_LOGE(TAG, "Can't get touch pad %d reading in tp_init()!", i);
    	} else {
    		tpThreshold[i] = reading / 2;
    	}
    }

    touch_pad_filter_start(10);
    touch_pad_set_filter_read_cb(tp_read_callback);
}
