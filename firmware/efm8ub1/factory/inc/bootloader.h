extern void reboot_into_bootloader();
extern void flash_writeByte(uint16_t addr, uint8_t byte);
extern void flash_erasePage(uint16_t addr);
extern void flash_start_crc16(uint16_t startAddr, uint16_t stopAddr);
extern uint8_t flash_get_upper_crc16();
extern uint8_t flash_get_lower_crc16();
extern uint16_t flash_get_crc16();
