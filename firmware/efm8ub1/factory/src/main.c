/**************************************************************************//**
 * (C) Copyright 2014 Silicon Labs Inc,
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 * 
 *****************************************************************************/

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8UB1_Register_Enums.h>                // SI_SFR declarations
#include "initialization.h"
#include "buffers.h"
#include "uart_0.h"
#include "smb_0.h"
#include "bootloader.h"

SI_SBIT(ESP32_BOOT, SFR_P0, 3);
SI_SBIT(ESP32_EN, SFR_P1, 4);

extern void Wait(uint16_t ms);

SI_SEGMENT_VARIABLE(i2c_cmd[5], uint8_t, SI_SEG_XDATA);
SI_SEGMENT_VARIABLE(i2c_resp[3], uint8_t, SI_SEG_XDATA);


void SMB0_commandReceivedCb()
{
	// Clear SI immediately to keep the ESP32 happy
	SMB0CN0_SI = 0;

	if (i2c_cmd[0] == 'B') {
		reboot_into_bootloader();
	} else if (i2c_cmd[0] == 'E') {
		IE_EA = 0;
		flash_erasePage((i2c_cmd[1] << 8) | i2c_cmd[2]);
		IE_EA = 1;
	} else if (i2c_cmd[0] == 'W') {
		IE_EA = 0;
		flash_writeByte((i2c_cmd[1] << 8) | i2c_cmd[2], i2c_cmd[3]);
		IE_EA = 1;
	} else if (i2c_cmd[0] == 'C') {
		flash_start_crc16((i2c_cmd[1] << 8) | i2c_cmd[2], (i2c_cmd[3] << 8) | i2c_cmd[4]);
		// Zero out the cmd byte because we get this callback on a read as well (but don't know what type it is)
		i2c_cmd[0] = 0;
		i2c_resp[0] = 'C';
		i2c_resp[1] = flash_get_upper_crc16();
		i2c_resp[2] = flash_get_lower_crc16();
		SMB0_sendResponse(i2c_resp, 3);
	}
}

void SMB0_transferCompleteCb()
{
}

void SMB0_errorCb(SMB0_TransferError_t error)
{
}

/**************************************************************************//**
 *
 * Main thread for VCPXpress UART demo.
 * 
 * This example implements a USB to uart bridge using a ping-pong buffering 
 * scheme for both Rx and Tx paths. It includes no flow control and has a 
 * fixed baud-rate of 115200 baud (ignores change requests from the host).
 *
 * The MCU enumerates as a COM port and connects to the board controllers COM port providing
 * bidirectional communication between the two COM ports.
 *
 * This example also displays characters received and transmitted on the LCD of the STK.
 *
 *****************************************************************************/

/**************************************************************************//**
 * @brief
 *   Main thread. Loops forever servicing usb->uart copies
 *   
 *****************************************************************************/
int main (void)
{  
   uint8_t result = VCP_STATUS_OK;

   // Disable watchdog timer
   WDTCN = 0xDE;
   WDTCN = 0xAD;
   
   System_Init ();   // Call top-level initialization routine
   resetState();     // reset buffer statues and init uart
   IE_EA = 0;
   UART0_readBuffer(uartRxBuf, USB_BLOCK_SIZE);// init first uart receive

   // Reset I2C
   SFRPAGE = 0x20;
   SMB0FCN0 = (1 << 6) | (1 << 2); // Flush FIFOs
   SMB0CN0 = 0; // Clear interrupt flags
   SMB0CF = 0; // Disable SMBus
   // Enable I2C
   SMB0_init(SMB0_TIMER1, false);
   SMB0_initSlave(0xc8, i2c_cmd, 5);
   SFRPAGE = 0;
   EIE1 |= EIE1_ESMB0__ENABLED;

   IE_EA = 1;        // enable global interrupts
   Wait(50);
   ESP32_EN = 1;

   //Main Thread. Here we monitor the current buffer for each
   //  transfer and kick the transfer when appropriate.
   while(1)
   {
     // if usb is ready for the next Tx packet and the current Tx
     // buffer has data then send the current buffer.
     if(usbTxReady && *usbTxLen){

       //VCPXpress library is not reentrant so we disable
       //  the USB interrupt to ensure it doesn't fire during
       //  the block write call
       SFRPAGE = 0x10;
       EIE2 &= ~EIE2_EUSB0__BMASK;
       usbTxReady = 0;
       result = Block_Write(usbTxBuf, *usbTxLen, usbTxLen);
       EIE2 |= EIE2_EUSB0__BMASK;
       SFRPAGE = 0x00;

       //If our write request failed retry
       if(result != VCP_STATUS_OK)
       {
      	 usbTxReady = 1;
       }

       //Buffer will be switched by callback when transfer completes

     }
      
      //If USB receive has completed and the current buffer
      // is empty. Prime for next receive.
      if(usbRxReady && !*usbRxLen)
      {
        //VCPXpress library is not reentrant so we disable
        //  the USB interrupt to ensure it doesn't fire during
        //  the block read call
      	SFRPAGE = 0x10;
      	EIE2 &= ~EIE2_EUSB0__BMASK;
        usbRxReady = 0;
        Block_Read(usbRxBuf, USB_BLOCK_SIZE, usbRxLen);
        EIE2 |= EIE2_EUSB0__BMASK;
      	SFRPAGE = 0x00;

        //If our read request failed retry
        if(result != VCP_STATUS_OK)
        {
        	usbRxReady = 1;
        }

        //Buffer will be switched by callback when transfer completes.

      }// If usb ready for next receive

      //If uart tx has completed and the current buffer
      // has data then start next uart transmit
      if(uartTxReady && *uartTxLen)
      {
    	  //We need to avoid having the UART ISR fire when we are
    	  // modifying the library. However we also don't want lower
    	  // priority interrupts to fire so we turn off all interrupts
    	  IE_EA = 0;
    	  UART0_writeBuffer(uartTxBuf, *uartTxLen);
    	  uartTxReady = 0;
    	  IE_EA = 1;

     	  //Buffer will be switched by callback when transfer completes.
      }

      //If uart rx has completed and the current buffer
      // is empty then start receiving new data.
      if(uartRxReady && !*uartRxLen)
      {
    	  //Block UART (and lower priority) interrupts
    	  IE_EA = 0;
    	  UART0_readBuffer(uartRxBuf, USB_BLOCK_SIZE);
    	  uartRxReady = 0;
    	  IE_EA = 1;

    	  //Buffer will be switched by callback when transfer completes.
      }

   }// while(forever)
}//main()
