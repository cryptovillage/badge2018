#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

// FIXME(supersat): This is a little broken because components shoudn't depend
// on the main application... But this is #badgelife
#include "../../../main/include/i2c.h"
#include "../../../main/include/misc.h"
#include "../../../main/include/ota.h"

STATIC mp_obj_t ota_reboot_efm8_into_bootloader()
{
	i2c_reboot_efm8_into_bootloader();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ota_reboot_efm8_into_bootloader_obj, ota_reboot_efm8_into_bootloader);

STATIC mp_obj_t ota_get_efm8_bootloader_crc()
{
	uint16_t crc;
	esp_err_t ret;

	ret = i2c_get_efm8_bootloader_crc(&crc);
	if (ret != ESP_OK) {
		mp_raise_OSError(ret);
	}
	return mp_obj_new_int(crc);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(ota_get_efm8_bootloader_crc_obj, ota_get_efm8_bootloader_crc);

STATIC mp_obj_t ota_get_efm8_app_crc()
{
	uint16_t crc;
	esp_err_t ret;

	ret = i2c_get_efm8_app_crc(&crc);
	if (ret != ESP_OK) {
		mp_raise_OSError(ret);
	}
	return mp_obj_new_int(crc);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ota_get_efm8_app_crc_obj, ota_get_efm8_app_crc);

STATIC mp_obj_t ota_update_efm8(mp_obj_t data_obj)
{
	mp_buffer_info_t bufinfo;

	mp_get_buffer_raise(data_obj, &bufinfo, MP_BUFFER_READ);
	update_efm8(bufinfo.buf, bufinfo.len);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ota_update_efm8_obj, ota_update_efm8);

STATIC mp_obj_t ota_update_efm8_bootloader(mp_obj_t data_obj)
{
	mp_buffer_info_t bufinfo;

	mp_get_buffer_raise(data_obj, &bufinfo, MP_BUFFER_READ);
	update_efm8_bootloader(bufinfo.buf, bufinfo.len);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ota_update_efm8_bootloader_obj, ota_update_efm8_bootloader);

// TODO: Make this a QSTR in flash
STATIC mp_obj_t ota_get_esp_idf_version()
{
	return mp_obj_new_str(IDF_VER, strlen(IDF_VER));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ota_get_esp_idf_version_obj, ota_get_esp_idf_version);

STATIC mp_obj_t ota_get_app_version()
{
	const char* verStr = get_app_version_str();
	return mp_obj_new_str(verStr, strlen(verStr));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ota_get_app_version_obj, ota_get_app_version);

STATIC mp_obj_t ota_get_running_partition_label()
{
	const esp_partition_t *p = esp_ota_get_running_partition();
	if (p) {
		return mp_obj_new_str(p->label, strlen(p->label));
	} else {
		return mp_const_none;
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ota_get_running_partition_label_obj, ota_get_running_partition_label);

STATIC mp_obj_t ota_begin(mp_obj_t size_obj)
{
	esp_err_t ret;
	esp_ota_handle_t handle;
	size_t size = mp_obj_get_int(size_obj);

	ret = esp_ota_begin(esp_ota_get_next_update_partition(NULL), size, &handle);
	if (ret != ESP_OK) {
		mp_raise_OSError(ret);
	}

	// TODO: Make this return a unique object type instead?
	return mp_obj_new_int((uint32_t)handle);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ota_begin_obj, ota_begin);

STATIC mp_obj_t ota_write(mp_obj_t handle_obj, mp_obj_t data_obj)
{
	esp_err_t ret;
	esp_ota_handle_t handle = (esp_ota_handle_t)mp_obj_get_int(handle_obj);
	mp_buffer_info_t bufinfo;

	mp_get_buffer_raise(data_obj, &bufinfo, MP_BUFFER_READ);
	ret = esp_ota_write(handle, bufinfo.buf, bufinfo.len);
	if (ret != ESP_OK) {
		mp_raise_OSError(ret);
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ota_write_obj, ota_write);

STATIC mp_obj_t ota_end(mp_obj_t handle_obj)
{
	esp_err_t ret;
	esp_ota_handle_t handle = (esp_ota_handle_t)mp_obj_get_int(handle_obj);

	ret = esp_ota_end(handle);
	if (ret != ESP_OK) {
		mp_raise_OSError(ret);
	}

	ret = esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
	if (ret != ESP_OK) {
		mp_raise_OSError(ret);
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ota_end_obj, ota_end);

STATIC const mp_rom_map_elem_t ota_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ota) },

	{ MP_ROM_QSTR(MP_QSTR_reboot_efm8_into_bootloader), MP_ROM_PTR(&ota_reboot_efm8_into_bootloader_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_efm8_bootloader_crc), MP_ROM_PTR(&ota_get_efm8_bootloader_crc_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_efm8_app_crc), MP_ROM_PTR(&ota_get_efm8_app_crc_obj) },
	{ MP_ROM_QSTR(MP_QSTR_update_efm8), MP_ROM_PTR(&ota_update_efm8_obj) },
	{ MP_ROM_QSTR(MP_QSTR_update_efm8_bootloader), MP_ROM_PTR(&ota_update_efm8_bootloader_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_esp_idf_version), MP_ROM_PTR(&ota_get_esp_idf_version_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_app_version), MP_ROM_PTR(&ota_get_app_version_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_running_partition_label), MP_ROM_PTR(&ota_get_running_partition_label_obj) },
	{ MP_ROM_QSTR(MP_QSTR_begin), MP_ROM_PTR(&ota_begin_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&ota_write_obj) },
	{ MP_ROM_QSTR(MP_QSTR_end), MP_ROM_PTR(&ota_end_obj) },
};

STATIC MP_DEFINE_CONST_DICT(ota_module_globals, ota_module_globals_table);

const mp_obj_module_t ota_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&ota_module_globals,
};
