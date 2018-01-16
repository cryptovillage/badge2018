/**************************************************************************//**
 * @file
 * @brief   Callbacks and ISR's.
 * @author  Silicon Laboratories
 * @version 1.0.0 (DM: July 14, 2014)
 *
 *******************************************************************************
 * @section License
 * (C) Copyright 2014 Silicon Labs Inc,
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *******************************************************************************
 *
 * Definitions and external declarations for VCPXpress_callback.c
 * We also place the UART and TIMER ISR here.
 * 
 *****************************************************************************/

#include <SI_EFM8UB1_Register_Enums.h>                // SI_SFR declarations
#include "callback.h"
#include "buffers.h"
#include "initialization.h"
#include "uart_0.h"

extern void Wait(uint16_t ms);

SI_SBIT(ESP32_BOOT, SFR_P0, 3);
SI_SBIT(ESP32_EN, SFR_P1, 4);

//Here we say if we are idle for 10 flush.
#define RX_FLUSH_TIME 10   //<! Number of milliseconds the uart needs 
                           //   to be idle before we flush whatever
                           //   data we have.
static uint16_t rxIdleTime;   //!< Number of bit times since last byte
                              //   received.

uint8_t dontResetIntoBootloaderFlag;

/**************************************************************************//**
 *  @brief
 *    VCPXpress callback
 *    
 *  Called by VCPXpresss. We only need to handle TX/RX compelte and device_open
 *   
 *****************************************************************************/
VCPXpress_API_CALLBACK(myAPICallback)
{
   uint32_t INTVAL = Get_Callback_Source();
   uint8_t mhs[2];
   uint32_t delayLoop;

   // Device was opened so reset
   if (INTVAL & DEVICE_OPEN)
   {

     //Reset variable state
     resetState();
     rxIdleTime = 0;
     dontResetIntoBootloaderFlag = 0;
   }// if DEVICE_OPEN

   if (INTVAL & DEVICE_CLOSE)
   {
	  ESP32_EN = 0;
	  for (delayLoop = 0; delayLoop < 70520; delayLoop++);
	  ESP32_EN = 1;
   }// if DEVICE_CLOSE

   // ESP32 reset/flashing support
   if (INTVAL & VCP_CB_SET_MHS) {
	   VCP_Get_ModemStatus(&mhs);
	   // DTR = mhs[0] & 1
	   // RTS = mhs[0] & 2
	   if ((mhs[0] & 1) && !(mhs[0] & 2)) { // DTR / GPIO0
		   //P1MDIN |= 0x08;
		   //ESP32_BOOT = 0;
	   } else {
		   //P1MDIN &= ~0x08;
		   //ESP32_BOOT = 1;
	   }

	   if ((mhs[0] & 2) && !(mhs[0] & 1)) {
		   //ESP32_EN = 0;
	   } else {
		   //ESP32_EN = 1;
	   }
   }

   // Rx compete ready for next RX buffer.
   if (INTVAL & RX_COMPLETE)
   {
  	 if(*usbRxLen != 0)
  	 {
  		 //if we received a packet of non-zero length switch to next
  		 // buffer. *usbRxLen is already set by usb library.
  	   if (*usbRxLen >= 4 && usbRxBuf[0] == 0xc0 && usbRxBuf[1] == 0x00 &&
  		   usbRxBuf[2] == 0x08 && usbRxBuf[3] == 0x24 && !dontResetIntoBootloaderFlag) {
  		   ESP32_EN = 0;
  		   ESP32_BOOT = 0;
  		   for (delayLoop = 0; delayLoop < 70520; delayLoop++);
  		   ESP32_EN = 1;
  		   for (delayLoop = 0; delayLoop < 70520; delayLoop++);
  		   ESP32_BOOT = 1;
  	   }
  	   dontResetIntoBootloaderFlag = 1;
       usbSwitchRxbuffer();
  	 }

  	 // Flag main that next transfer can start
  	 // in the case of a zlp this will restart the receive.
  	 usbRxReady = 1;

   }// if RX_COMPLETE

   // Rx compete ready for next TX buffer.
   if (INTVAL & TX_COMPLETE)
   {
  	 //Only switch buffer if we completed a packet of non-zero length
  	 if(*usbTxLen != 0)
  	 {
    	 // When TX complete perform buffer switch
       *usbTxLen = 0;       // mark current buffer empty now that TX is done
       usbSwitchTxbuffer(); // switch usb buffer
  	 }

  	 // Flag main while loop that it can send data up
  	 usbTxReady = 1;
   
   }// if TX_COMPLETE
}//VCPXpress_API_CALLBACK()

void UART0_receiveCompleteCb()
{

	// When Rx completes (full) perform buffer switch
	*uartRxLen  = USB_BLOCK_SIZE; // mark current buffer full
  switchRxBuffer();             // switch to next buffer
	uartRxReady = 1;              // Flag main that next Read can start
}

void UART0_transmitCompleteCb()
{
    // When TX complete perform buffer switch
	*uartTxLen = 0;   // mark current buffer empty
	switchTxBuffer(); // Switch to next buffer
	uartTxReady = 1;  // Flag main while loop that it can start transfer
}

/**************************************************************************//**
 *  @brief
 *    TIMER0 Interrupt Service Routine
 *    
 *  The UART ISR only causes data to be sent to the host on a buffeer
 *  overflow. If a small number of bytes is received followed by a long
 *  pause we want to flush those bytes in a reasonable time frame. This 
 *  ISR measures the time since the last bye received. If that time exceeds
 *  a preset limit then the rx buffer is flushed imediatly. 
 *  
 *  Here we use TIMER0 to setup a static timeout. However for lower baud rates
 *  we could turn on the TIMER0 (baud rate generation) interrupt and make our 
 *  timeout in terms of uart bit widths. For high baud rates this does not work
 *  since the timer fires often and the ISR ends up eating a lot of cpu cycles.
 *   
 *****************************************************************************/
SI_INTERRUPT(TIMER0_ISR, TIMER0_IRQn)
{
  static uint8_t previousBytes;  //!< Previous number of bytes remaining
  uint8_t numBytes;              //!< Num bytes in the uartRx buffer

  //Reload time for next 1ms.
  TH0 = -(SYSCLK/1000)>>8;
  TL0 = -(SYSCLK/1000) & 0x0FF;

  numBytes = USB_BLOCK_SIZE - UART0_rxBytesRemaining();

  //If the Rx buffer is empty we don't need to do anything (bail)
  if (!numBytes)
  {
	  return;
  }

  //if the buffer isn't empty but has received a new
  // byte then reset our idle time
  if (previousBytes != numBytes)
  {
	  rxIdleTime = 0;
	  previousBytes = numBytes;
  }

  //if timeout occurs then flush RX buffer.
  if(rxIdleTime > RX_FLUSH_TIME)
  {
	  //Block UART (and lower priority) interrupts
	  IE_EA = 0;
	  *uartRxLen = numBytes; //Set length of current rxBuffer
      switchRxBuffer();      //Switch buffer
      //Prime next read
      UART0_readBuffer(uartRxBuf, USB_BLOCK_SIZE);
	  IE_EA = 1;

	  //reset timeout
	  rxIdleTime = 0;
  }
  else
  {
    //Increment idle time
    rxIdleTime++;
  }


}// timer0 IST


