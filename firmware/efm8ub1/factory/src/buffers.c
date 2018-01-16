/**************************************************************************//**
 * @file
 * @brief   Buffers and buffer managment
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

#include <SI_EFM8UB1_Register_Enums.h>
#include "buffers.h"

//Data buffers
/** Receive buffer A */
SI_SEGMENT_VARIABLE(uartRxBuf_A[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA);
/** Receive buffer B */
SI_SEGMENT_VARIABLE(uartRxBuf_B[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA);
/** Transmit buffer A */
SI_SEGMENT_VARIABLE(uartTxBuf_A[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA); 
/** Transmit buffer B */
SI_SEGMENT_VARIABLE(uartTxBuf_B[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA); 

//Data buffer lengths
/** Length of data in Receive ping */
SI_SEGMENT_VARIABLE(uartRxLen_A, uint16_t, SI_SEG_XDATA);  
/** Length of data in Receive pong */
SI_SEGMENT_VARIABLE(uartRxLen_B, uint16_t, SI_SEG_XDATA);
/** //Length of data in Transmit ping */
SI_SEGMENT_VARIABLE(uartTxLen_A, uint16_t, SI_SEG_XDATA);  
/** Length of data in Transmit pong */
SI_SEGMENT_VARIABLE(uartTxLen_B, uint16_t, SI_SEG_XDATA); 

//Peripheral status
bool usbTxReady;        //!< Signals USB  Library ready for TX to host.
bool usbRxReady;        //!< Signals USB  Library has RX data from host.
bool uartTxReady;       //!< Signals UART Library ready for TX to host.
bool uartRxReady;       //!< Signals UART Library has RX data from host.

//USB Side active buffers
//=========================================
/** Current USB Tx buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbTxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
/** Current num bytes in USB Tx buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbTxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);
/** Current USB Rx buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbRxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
/** Current num bytes in USB Rx buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbRxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);


//UART Side active buffers
//=========================================
/** current Tx Buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartTxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
/** Num words in current Tx Buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartTxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);
/** Current Rx Buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartRxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
/** Num words in current Rx Buffer */
SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartRxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);


/**************************************************************************//**
 *  @brief
 *    Switch to the next Rx Buffer
 *    
 *  @returns
 *    Number of bytes in new buffer. If 0 bufer is ready for reception
 *    else buffer is in use. 
 *   
 *   Updates the number of bytes in the current buffer, switches to 
 *   the new one, and returns if the new buffer is busy or not (has data)
 *   Also resets the current uartRxPosition.
 *   
 *****************************************************************************/
void switchRxBuffer()
{

  if (uartRxBuf == uartRxBuf_A)
  {
    //switch to B buffer
    uartRxBuf   = uartRxBuf_B;     // Select opposite buffer
    uartRxLen   = &uartRxLen_B;    // Select opposite length
  }
  else
  {
    //switch to A buffer
    uartRxBuf   = uartRxBuf_A;     // Select opposite buffer
    uartRxLen   = &uartRxLen_A;    // Select opposite length
  }
}// switchRxBuffer

/**************************************************************************//**
 *  @brief
 *    Switch to the next Tx Buffer
 *    
 *   Updates the number of bytes in the current buffer (0 since buffer just 
 *   dleard)), switches to the new one, and resets teh current uartTxPosition
 *   
 *****************************************************************************/
void switchTxBuffer()
{
  if (uartTxBuf == uartTxBuf_A)
  {
    //switch to B buffer
    uartTxBuf   = uartTxBuf_B;   // Select opposite buffer
    uartTxLen   = &uartTxLen_B;  // Select opposite buffer
  }
  else
  {
    //switch to A buffer
    uartTxBuf   = uartTxBuf_A;    // Select opposite buffer
    uartTxLen   = &uartTxLen_A;   // Select opposite length
  }

}// switchTxBuffer

/**************************************************************************//**
 * @brief
 *   Point usb side to next buffer
 *   
 *****************************************************************************/
void usbSwitchTxbuffer()
{
  //usb Tx buffer is uart Rx buffer
  if (usbTxBuf == uartRxBuf_A)
  {
    usbTxBuf = uartRxBuf_B;
    usbTxLen = &uartRxLen_B;
  }
  else
  {
    usbTxBuf = uartRxBuf_A;
    usbTxLen = &uartRxLen_A;
  }
}

/**************************************************************************//**
 * @brief
 *   Point usb side to next buffer
 *   
 *****************************************************************************/
void usbSwitchRxbuffer()
{
  //usb Rx buffer is uart Tx buffer
  if (usbRxBuf == uartTxBuf_A)
  {
    usbRxBuf = uartTxBuf_B;
    usbRxLen = &uartTxLen_B;
  }
  else
  {
    usbRxBuf = uartTxBuf_A;
    usbRxLen = &uartTxLen_A;
  }
}

/**************************************************************************//**
 *  @brief
 *    Reset status variables.
 *    
 *  Reset all status variables to default. Used for init and re-connect of host. 
 *   
 *****************************************************************************/
void resetState()
{
  // Reset data buffer status and start witn ping buffers
  uartRxLen_A = 0;
  uartRxLen_B = 0;
  uartTxLen_A = 0;
  uartTxLen_B = 0;
  
  // uart Rx/Tx setup for buffer A
  uartRxBuf      = uartRxBuf_A;
  uartTxBuf      = uartTxBuf_A;
  uartTxLen      = &uartTxLen_A;
  uartRxLen      = &uartRxLen_A;
  
  //Init uart buffers
  usbRxBuf = uartTxBuf_A;
  usbRxLen = &uartTxLen_A;
  usbTxBuf = uartRxBuf_A;
  usbTxLen = &uartRxLen_A;

  //Reset peripheral status
  usbTxReady  = 1; // initial state is ready for tx data
  usbRxReady  = 1; // initial state is no received data.
  uartTxReady = 1; // initial state is ready for tx data
  uartRxReady = 1; // initial state is no received data.
}
