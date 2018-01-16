//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8UB1_Register_Enums.h>                  // SFR declarations
#include "InitDevice.h"
#include "board.h"
// $[Generated Includes]
#include "efm8_usb.h"
// [Generated Includes]$

uint8_t rxHIDBuf[64];
uint8_t txBuf[64];

void SiLabs_Startup()
{
	// Disable the watchdog
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
}

//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
int main(void) {
	// Call hardware initialization routine
	enter_DefaultMode_from_RESET();

	// Bring up the rest of the system
	i2c_init();
	i2c_initLEDController();
	power_init();
	uart_init();

	while (1) {
// $[Generated Run-time code]
// [Generated Run-time code]$
#ifdef BOARD_ALWAYS_POWER_UP
		if (!(powerFlags & POWER_FLAG_VIRTUAL_SWITCH_ON) && _cur_ms > 10) {
			powerFlags |= POWER_FLAG_VIRTUAL_SWITCH_ON | POWER_FLAG_POWER_FLAGS_CHANGED;
		}
#endif
		power_update();

		// Disable interrupts to make fifo operations atomic
		// TODO(supersat): Make fifo operations work with interrupts enabled?
		IE_EA = 0;
		if (!USBD_EpIsBusy(EP2IN)) {
			uint8_t rxFifoLength = fifo_getContigLength(&rxFifo);
			if (rxFifoLength) {
				USBD_Write(EP2IN, fifo_getContigData(&rxFifo, rxFifoLength), rxFifoLength, false);
			}
		}

		if (!USBD_EpIsBusy(EP2OUT) && fifo_getFree(&txFifo) > 64) {
			USBD_Read(EP2OUT, txBuf, 64, true);
		}
		IE_EA = 1;

		if (!USBD_EpIsBusy(EP3OUT)) {
			USBD_Read(EP3OUT, rxHIDBuf, 64, true);
		}

		i2c_update();

		// Enter idle mode to save power
		// Will resume on interrupt

		// The commented out lines will toggle the pull-up on ENC_A,
		// allowing us to monitor processor load/sleep time
		//P0 = 0x0c;
		idle();
		//P0 = 0x0e;

		// WDTCN = 0xA5;
	}
}
