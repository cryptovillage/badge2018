//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8UB1_Register_Enums.h>                  // SFR declarations
#include "bootloader.h"

void reboot_into_bootloader()
{
	SFRPAGE = 0x00;
	*((uint8_t SI_SEG_DATA *)0x00) = 0xa5;
	RSTSRC = RSTSRC_SWRSF__SET | RSTSRC_PORSF__SET;
}

// The following functions are adapted from Silicon Labs's bootloader

void flash_writeByte(uint16_t addr, uint8_t byte)
{
	int8_t SI_SEG_XDATA * pwrite = (uint8_t SI_SEG_XDATA *)addr;

	if (byte == 0xff)
		return;

	// Unlock flash by writing the key sequence
	FLKEY = 0xA5;
	FLKEY = 0xF1;

	// Enable flash writes, then do the write
	PSCTL |= PSCTL_PSWE__WRITE_ENABLED;
	*pwrite = byte;
	PSCTL &= ~(PSCTL_PSEE__ERASE_ENABLED | PSCTL_PSWE__WRITE_ENABLED);
}

void flash_erasePage(uint16_t addr)
{
	// Enable flash erasing, then start a write cycle on the selected page
	PSCTL |= PSCTL_PSEE__ERASE_ENABLED;
	flash_writeByte(addr, 0);
}

void flash_start_crc16(uint16_t startAddr, uint16_t stopAddr)
{
	CRC0CN0 |= CRC0CN0_CRCINIT__INIT;

	for (; startAddr <= stopAddr; startAddr++) {
		CRC0IN = *(uint8_t SI_SEG_CODE *)startAddr;
	}
}

uint8_t flash_get_upper_crc16()
{
	CRC0CN0 |= CRC0CN0_CRCPNT__ACCESS_UPPER;
	return CRC0DAT;
}

uint8_t flash_get_lower_crc16()
{
	return CRC0DAT;
}

uint16_t flash_get_crc16()
{
	uint16_t crc16;

	crc16  = flash_get_upper_crc16() << 8;
	crc16 |= flash_get_lower_crc16();

	return crc16;
}
