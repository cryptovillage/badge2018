#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

// FIXME(supersat): This is a little broken because components shoudn't depend
// on the main application... But this is #badgelife
#include "../../../main/include/events.h"
#include "../../../main/include/i2c.h"
#include "../../../main/include/is31fl3736.h"
#include "../../../main/include/gf.h"
#include "../../../main/include/misc.h"

extern is31fl3736_handle_t leds;

static const char* TAG = "modbadge";

typedef struct _badge_event_type_obj_t {
    mp_obj_base_t base;
    badge_event_t event;
} badge_event_obj_t;

// TODO(supersat): Make this return an actual type instead of a number?
STATIC mp_obj_t badge_event_get_type(const mp_obj_t event) {
    badge_event_obj_t *eo = MP_OBJ_TO_PTR(event);
    return mp_obj_new_int(eo->event.type);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(badge_event_get_type_obj, badge_event_get_type);

// TODO(supersat): Return useful Python objects instead of a bytearray
STATIC mp_obj_t badge_event_get_data(const mp_obj_t event) {
    badge_event_obj_t *eo = MP_OBJ_TO_PTR(event);
    return mp_obj_new_bytes((const uint8_t *)&eo->event.data, sizeof(badge_event_data_t));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(badge_event_get_data_obj, badge_event_get_data);

STATIC const mp_map_elem_t badge_event_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_type), (mp_obj_t)&badge_event_get_type_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_data), (mp_obj_t)&badge_event_get_data_obj },
};

STATIC MP_DEFINE_CONST_DICT(badge_event_locals_dict, badge_event_locals_dict_table);

STATIC void badge_event_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    badge_event_obj_t *eo = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<Event type=%d>", eo->event.type);
}

STATIC const mp_obj_type_t badge_event_type = {
    { &mp_type_type },
    .name = MP_QSTR_Event,
	.print = badge_event_print,
	.locals_dict = (mp_obj_t)&badge_event_locals_dict
};

// TODO(supersat): Make this take an optional timeout value
STATIC mp_obj_t badge_get_event() {
	badge_event_obj_t *eventObj;
	eventObj = m_new_obj(badge_event_obj_t);
	MP_THREAD_GIL_EXIT();
	xQueueReceive(eventQueue, &eventObj->event, portMAX_DELAY);
	eventObj->base.type = &badge_event_type;
	MP_THREAD_GIL_ENTER();
	return MP_OBJ_FROM_PTR(eventObj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(badge_get_event_obj, badge_get_event);

STATIC mp_obj_t badge_get_efm8_uuid() {
	uint8_t uuid[16];
	memset(uuid, 0, sizeof(uuid));
	i2c_get_efm8_uuid(uuid);
	return mp_obj_new_bytes((const uint8_t *)uuid, sizeof(uuid));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(badge_get_efm8_uuid_obj, badge_get_efm8_uuid);

STATIC mp_obj_t badge_get_os_heap_free() {
	return mp_obj_new_int(xPortGetFreeHeapSize());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(badge_get_os_heap_free_obj, badge_get_os_heap_free);

STATIC mp_obj_t badge_get_os_minimum_ever_heap_free() {
	return mp_obj_new_int(xPortGetMinimumEverFreeHeapSize());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(badge_get_os_minimum_ever_heap_free_obj, badge_get_os_minimum_ever_heap_free);

STATIC mp_obj_t badge_gf_computeShards(mp_obj_t key_obj, mp_obj_t threshold_obj, mp_obj_t num_shards_obj)
{
	size_t threshold = mp_obj_get_int(threshold_obj);
	size_t num_shards = mp_obj_get_int(num_shards_obj);
	mp_buffer_info_t key_buf_info;
	uint8_t** raw_shards;
	mp_obj_t shardList;
	mp_obj_t *shards;
	size_t i;

	mp_get_buffer_raise(key_obj, &key_buf_info, MP_BUFFER_READ);
	shards = malloc(sizeof(mp_obj_t *) * num_shards);
	raw_shards = malloc(sizeof(uint8_t *) * num_shards);
	for (i = 0; i < num_shards; i++) {
		raw_shards[i] = m_new(byte, key_buf_info.len);
		shards[i] = mp_obj_new_bytearray_by_ref(key_buf_info.len, raw_shards[i]);
	}
	shardList = mp_obj_new_list(num_shards, shards);
	gf_computeShards((uint8_t *)key_buf_info.buf, key_buf_info.len, raw_shards, threshold, num_shards);

	return shardList;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(badge_gf_computeShards_obj, badge_gf_computeShards);

STATIC mp_obj_t badge_gf_computeCombine(mp_obj_t shares_x_obj, mp_obj_t shards_obj)
{
	mp_buffer_info_t shard_buf_info;
	//mp_buffer_info_t shares_x_info;
	size_t shard_len = 0;
	size_t num_x_coords = 0;
	size_t num_shards = 0;
	mp_obj_t *x_coords;
	mp_obj_t *shards;
	uint8_t** raw_shards = NULL;
	uint8_t* out = NULL;
	uint8_t* x_coords_buf = NULL;
	size_t i;

	//mp_get_buffer_raise(shares_x_obj, &shares_x_info, MP_BUFFER_READ);
	mp_obj_get_array(shares_x_obj, &num_x_coords, &x_coords);
	mp_obj_get_array(shards_obj, &num_shards, &shards);
	if (num_x_coords != num_shards) {
		mp_raise_ValueError("x coordinate list and shard list must be the same length");
	}

	x_coords_buf = alloca(num_x_coords);
	for (i = 0; i < num_x_coords; i++) {
		x_coords_buf[i] = mp_obj_get_int(x_coords[i]);
	}

	for (i = 0; i < num_shards; i++) {
		mp_get_buffer_raise(shards[i], &shard_buf_info, MP_BUFFER_READ);
		if (i == 0) {
			shard_len = shard_buf_info.len;
			out = alloca(shard_buf_info.len);
			raw_shards = alloca(sizeof(uint8_t *) * num_shards);
		} else {
			if (shard_buf_info.len != shard_len) {
				mp_raise_ValueError("All shard lengths must be the same");
			}
		}
		raw_shards[i] = (uint8_t *)shard_buf_info.buf;
	}
	gf_computeCombine(x_coords_buf, raw_shards, num_shards, shard_len, out);
	return mp_obj_new_bytearray(shard_len, out);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(badge_gf_computeCombine_obj, badge_gf_computeCombine);

STATIC mp_obj_t badge_set_pwm_duty_matrix(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t duty)
{
	iot_is31fl3736_set_pwm_duty_matrix(leds, (uint16_t)mp_obj_get_int(x_obj),
			(uint16_t)mp_obj_get_int(y_obj), (uint8_t)mp_obj_get_int(duty));
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(badge_set_pwm_duty_matrix_obj, badge_set_pwm_duty_matrix);

STATIC const mp_rom_map_elem_t badge_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_badge) },

	{ MP_ROM_QSTR(MP_QSTR_get_event), MP_ROM_PTR(&badge_get_event_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_efm8_uuid), MP_ROM_PTR(&badge_get_efm8_uuid_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_os_heap_free), MP_ROM_PTR(&badge_get_os_heap_free_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_os_minimum_ever_heap_free), MP_ROM_PTR(&badge_get_os_minimum_ever_heap_free_obj) },
	{ MP_ROM_QSTR(MP_QSTR_gf_computeShards), MP_ROM_PTR(&badge_gf_computeShards_obj) },
	{ MP_ROM_QSTR(MP_QSTR_gf_computeCombine), MP_ROM_PTR(&badge_gf_computeCombine_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_pwm_duty_matrix), MP_ROM_PTR(&badge_set_pwm_duty_matrix_obj) },
};

STATIC MP_DEFINE_CONST_DICT(badge_module_globals, badge_module_globals_table);

const mp_obj_module_t badge_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&badge_module_globals,
};
