The factory EFM8 firmware just emulates a CP2102 and provides a limited I2C
interface for reflashing the bootloader (which allows the main EFM8 firmware to
be reflashed from the ESP32). Unfortunately, the VCPXpress library from SiLabs
will suspend the device when there is no active USB connection. This means that
OTA bootloader updates from the ESP32 will not work without a USB connection,
nor will querying the EFM8 for its bootloader and app CRCs. To keep the EFM8
active without a USB connection, the call to USBD_Suspend() needs to be patched
out from handleUsbSuspendInt(). To do this, find these bytes in the hex file:

785ee6c39402....7f0512....12....

Replace the last three bytes with 000000 and adjust the CRC of the
corresponding hex lines. To calculate the correct CRC, you can use 
https://www.fischl.de/hex_checksum_calculator/

For example:

:102A60000CE4FF0214C4785EE6C3940240087F05BC
:102A700012247F121CF722C0E053917F0558E558BD

becomes

:102A60000CE4FF0214C4785EE6C3940240087F05BC
:102A700012247F00000022C0E053917F0558E558E2
