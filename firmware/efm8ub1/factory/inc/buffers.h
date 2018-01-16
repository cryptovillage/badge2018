/**************************************************************************//**
 * @file
 * @brief   Buffers header.
 * @author  Silicon Laboratories
 * @version 1.0.0 (DM: July 14, 2014)
 *
 *******************************************************************************
 * @section License
 * (C) Copyright 2014 Silicon Labs Inc,
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *****************************************************************************/

#ifndef BUFFERS_H_
#define BUFFERS_H_

#include <VCPXpress.h>
#include <stdint.h>

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
#define USB_BLOCK_SIZE  0x40 //!< Size of USB packst

//-----------------------------------------------------------------------------
// External Global Variables
//-----------------------------------------------------------------------------
//Data buffers
extern SI_SEGMENT_VARIABLE(uartRxBuf_A[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA); 
extern SI_SEGMENT_VARIABLE(uartRxBuf_B[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE(uartTxBuf_A[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE(uartTxBuf_B[USB_BLOCK_SIZE], uint8_t, SI_SEG_XDATA);

//Data buffer status
extern SI_SEGMENT_VARIABLE(uartRxLen_A, uint16_t, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE(uartRxLen_B, uint16_t, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE(uartTxLen_A, uint16_t, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE(uartTxLen_B, uint16_t, SI_SEG_XDATA);

extern bool usbTxReady;
extern bool usbRxReady;
extern bool uartTxReady;
extern bool uartRxReady;

//USB Side active buffers
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbTxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbTxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbRxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(usbRxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);


//UART Side active buffers
//=========================================
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartTxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartTxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartRxBuf, uint8_t, SI_SEG_XDATA, SI_SEG_XDATA);
extern SI_SEGMENT_VARIABLE_SEGMENT_POINTER(uartRxLen, uint16_t, SI_SEG_XDATA, SI_SEG_XDATA);


//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void resetState();
void switchRxBuffer();
void switchTxBuffer();
void usbSwitchTxbuffer();
void usbSwitchRxbuffer();

#endif /* BUFFERS_H_ */
