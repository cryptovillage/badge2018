#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

typedef struct _nvs_type_obj_t {
    mp_obj_base_t base;
    nvs_handle nvs_handle;
} nvs_obj_t;

STATIC const mp_obj_type_t nvs_type;

STATIC mp_obj_t modnvs_open(mp_obj_t namespace_obj)
{
	mp_uint_t namespace_len;
	const char *namespace;
	nvs_handle nvs_handle;
	esp_err_t ret;
	nvs_obj_t *nvs_obj;

	namespace = mp_obj_str_get_data(namespace_obj, &namespace_len);
	ret = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
	if (ret != ESP_OK) {
		mp_raise_OSError(ret);
	}

	nvs_obj = m_new_obj(nvs_obj_t);
	nvs_obj->base.type = &nvs_type;
	nvs_obj->nvs_handle = nvs_handle;
	return MP_OBJ_FROM_PTR(nvs_obj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modnvs_open_obj, modnvs_open);

STATIC mp_obj_t modnvs_commit(const mp_obj_t self_obj) {
	esp_err_t ret;
    nvs_obj_t *self = MP_OBJ_TO_PTR(self_obj);
    ret = nvs_commit(self->nvs_handle);
    if (ret != ESP_OK) {
    	mp_raise_OSError(ret);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modnvs_commit_obj, modnvs_commit);

STATIC mp_obj_t modnvs_close(const mp_obj_t self_obj) {
	nvs_obj_t *self = MP_OBJ_TO_PTR(self_obj);
    nvs_close(self->nvs_handle);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modnvs_close_obj, modnvs_close);

STATIC mp_obj_t modnvs_get_str(const mp_obj_t self_obj, const mp_obj_t key_obj) {
	esp_err_t ret;
	const char *key;
	unsigned int key_len;
	char *val;
	unsigned int val_len;

    nvs_obj_t *self = MP_OBJ_TO_PTR(self_obj);
    key = mp_obj_str_get_data(key_obj, &key_len);
    ret = nvs_get_str(self->nvs_handle, key, NULL, &val_len);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_INVALID_LENGTH) {
    	mp_raise_OSError(ret);
    }
    val = malloc(val_len);
    if (!val) {
    	mp_raise_OSError(MP_ENOMEM);
    }
    ret = nvs_get_str(self->nvs_handle, key, val, &val_len);
    if (ret != ESP_OK) {
    	mp_raise_OSError(ret);
    }
    return mp_obj_new_str(val, val_len - 1); // Exclude the NUL terminator
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(modnvs_get_str_obj, modnvs_get_str);

STATIC mp_obj_t modnvs_set_str(const mp_obj_t self_obj, const mp_obj_t key_obj, const mp_obj_t val_obj) {
	esp_err_t ret;
	const char *key;
	size_t key_len;
	const char *val;
	size_t val_len;

    nvs_obj_t *self = MP_OBJ_TO_PTR(self_obj);
    key = mp_obj_str_get_data(key_obj, &key_len);
    val = mp_obj_str_get_data(val_obj, &val_len);
    ret = nvs_set_str(self->nvs_handle, key, val);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_INVALID_LENGTH) {
    	mp_raise_OSError(ret);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(modnvs_set_str_obj, modnvs_set_str);

STATIC const mp_map_elem_t nvs_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_str), (mp_obj_t)&modnvs_get_str_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_set_str), (mp_obj_t)&modnvs_set_str_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_commit), (mp_obj_t)&modnvs_commit_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_close), (mp_obj_t)&modnvs_close_obj },
};
STATIC MP_DEFINE_CONST_DICT(nvs_locals_dict, nvs_locals_dict_table);

STATIC const mp_obj_type_t nvs_type = {
    { &mp_type_type },
    .name = MP_QSTR_nvs,
	.locals_dict = (mp_obj_t)&nvs_locals_dict
};

STATIC const mp_rom_map_elem_t nvs_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_nvs) },

	{ MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&modnvs_open_obj) },
};

STATIC MP_DEFINE_CONST_DICT(nvs_module_globals, nvs_module_globals_table);

const mp_obj_module_t nvs_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&nvs_module_globals,
};
