/**************************************************************************//**
 * Copyright (c) 2015 by Silicon Laboratories Inc. All rights reserved.
 *
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *****************************************************************************/

#include "efm8_device.h"
#include "USB_main.h"
#include "USB_descriptor.h"

// -----------------------------------------------------------------------------
// Global Variable Definitions
// -----------------------------------------------------------------------------

// Counts USB start of frame interrupts
uint8_t usb_tick;

// Holds USB device state
uint8_t usb_dev_state;

// Holds state for endpoint 0
uint8_t usb_ep0_state;

// Tracks halt status for endpoint 1
bool usb_ep1_halted;

// Points at data returned by the standard chapter 9 control requests
uint8_t SI_SEG_CODE *usb_dataptr;

// Buffers setup packet from last control transfer
USB_SetupData_t usb_setup;

// Index of the next empty location in the receive circular buffer
uint8_t usb_rxHead;

// Number of bytes in transmit linear buffer
uint8_t usb_txCount;

// Circular buffer holds received OUTPUT report data.
// IMPORTANT! Size is 256, so indexes wrap properly without limit testing.
uint8_t SI_SEG_XDATA usb_rxBuf[256];

// Linear buffer holds INPUT report to transmit.
// IMPORTANT! Sized to hold one INPUT report.
uint8_t usb_txBuf[USB_HID_IN_SIZE];

// -----------------------------------------------------------------------------
// Local Function Prototypes
// -----------------------------------------------------------------------------

// Handler for USB reset interrupt
static void handleUsbReset(void);

// Handler for USB Endpoint 0 interrupt
static void handleUsbEp0(void);

// Used for multiple byte reads of Endpoint FIFO's
static void readSetupPacket(void);
static void readReportData(uint8_t addr, uint8_t count);

// Used for multiple byte writes of Endpoint FIFO's
static void writeRequestData(void);
static void writeReportData(uint8_t addr);

// -----------------------------------------------------------------------------
// Initializes the USB HID driver.
//
// This routine initializes the USB0 hardware and resets all driver data. It 
// should be called once at the start of the application.
// -----------------------------------------------------------------------------
void USB_initModule(void)
{
  // Select SFR page with USB configuration registers
  SET_SFRPAGE(USB0_PAGE);

  // Force an asynchronous USB reset
  USB_writeIndirectReg(POWER, POWER_USBRST__SET);

  // Enable USB transceiver and select full speed
  USB0XCN = USB0XCN_PREN__PULL_UP_ENABLED 
            | USB0XCN_PHYEN__ENABLED 
            | USB0XCN_SPEED__FULL_SPEED;

  // Enable EP0, SOF and Reset interrupts
  USB_writeIndirectReg(IN1IE, IN1IE_EP0E__ENABLED);
  USB_writeIndirectReg(CMIE, CMIE_SOFE__ENABLED | CMIE_RSTINTE__ENABLED);

  // Enable full speed clock recovery, single-step mode disabled
  USB_writeIndirectReg(CLKREC, CLKREC_CRE__ENABLED | 0xF);

  // Restore the default SFR page before leaving
  SET_SFRPAGE(0);

  // Reset USB driver data
  handleUsbReset();
}

// -----------------------------------------------------------------------------
// Attempt to send an INPUT report.
//
// This routine attempts to send the INPUT report held in usb_txBuf[] on EP1. The
// application should load usb_txBuf[] and set usb_txCount to USB_HID_IN_SIZE, then 
// call this function and USB_pollModule() until usb_txCount is reset to 0.
// -----------------------------------------------------------------------------
void USB_sendReport(void)
{
  // Read control register for EP1 IN
  USB_writeIndirectReg(INDEX, INDEX_EPSEL__ENDPOINT_1);

  // If endpoint 1 is not halted and the FIFO is empty
  if (!usb_ep1_halted && ((USB_readIndirectReg(EINCSRL) & EINCSRL_INPRDY__SET) == 0))
  {
    // Send data on the IN Endpoint
    writeReportData(FIFO1);
    USB_writeIndirectReg(EINCSRL, EINCSRL_INPRDY__SET);
  }
}

// -----------------------------------------------------------------------------
// Polls and handles all USB events.
// 
// This routine reads and clears the USB interrupt registers, then handles all 
// pending events. It must be called periodically from the application mainloop.
//
// NOTE: The current implementation does not handle suspend and resume.
// -----------------------------------------------------------------------------
void USB_pollModule(void)
{
  uint8_t reg_cmint, reg_in1int;

  // Read and clear all interrupt registers
  reg_cmint = USB_readIndirectReg(CMINT);
  reg_in1int = USB_readIndirectReg(IN1INT);

  // Resume signaling detected (not supported)
  // if (reg_cmint & CMINT_RSUINT__SET) {}

  // Reset signaling detected
  if (reg_cmint & CMINT_RSTINT__SET)
  {
    handleUsbReset();
  }
  // Start-Of-Frame detected
  if (reg_cmint & CMINT_SOF__SET)
  {
    usb_tick++;
  }
  // EP0 event - setup packet received or data transmitted
  if (reg_in1int & IN1INT_EP0__SET)
  {
    handleUsbEp0();
  }
  // Suspend signaling detected (not supported)
  // if (reg_cmint & CMINT_SUSINT__SET) {}
}

// -----------------------------------------------------------------------------
// Resets USB stack to default state after reset signaling is detected.
// -----------------------------------------------------------------------------
static void handleUsbReset(void)
{
  // Reset device and endpoint status to defaults
  usb_dev_state = DEV_DEFAULT;
  usb_ep0_state = EP0_IDLE;
  usb_ep1_halted = true;

  // Enable USB0 by clearing the USB Inhibit Bit and enable suspend detection
  USB_writeIndirectReg(POWER, POWER_SUSEN__ENABLED);
}

// -----------------------------------------------------------------------------
// Services endpoint 0 interrupts.
//
// This routine is called by USB_pollModule() whenever Endpoint 0 requires 
// service. It maintains a state machine to:
// - Receive and decode incoming requests (setup packets)
// - Send and receive data packets on the EP0 FIFO
// -----------------------------------------------------------------------------
static void handleUsbEp0(void)
{
  uint8_t reg_e0csr;

  // Read the EP0 control register
  USB_writeIndirectReg(INDEX, INDEX_EPSEL__ENDPOINT_0);
  reg_e0csr = USB_readIndirectReg(E0CSR);

  // If the USB hardware just completed a stall handshake, 
  // clear the stall flag and abort the current transfer
  if (reg_e0csr & E0CSR_STSTL__SET)
  {
    USB_writeIndirectReg(E0CSR, 0);
    usb_ep0_state = EP0_IDLE;
    return;
  }

  // If the current transfer ended prematurely, clear the error 
  // and return to idle to accept a new transfer
  if (reg_e0csr & E0CSR_SUEND__SET)
  {
    USB_writeIndirectReg(E0CSR, E0CSR_SSUEND__SET);
    usb_ep0_state = EP0_IDLE;
  }

  // EP0 is idle waiting for a setup packet
  if (usb_ep0_state == EP0_IDLE)
  {
    // Confirm EP0 has an OUT packet from the host waiting
    if (reg_e0csr & E0CSR_OPRDY__SET)
    {
      // Read and buffer the setup packet
      readSetupPacket();

      // Handle HID class-specific requests
      if ((usb_setup.bmRequestType & ~USB_REQ_DIRECTION_MASK) == USB_REQ_CLASS_INTERFACE)
      {
        switch (usb_setup.bRequest)
        {
          case USB_HID_GET_REPORT:
            usb_txCount = usb_setup.wLength.c[LSB];
            usb_ep0_state = EP0_TX;
            break;
          case USB_HID_SET_REPORT:
            usb_ep0_state = EP0_RX;
            break;
          // These optional HID class requests are not supported
          // case USB_HID_GET_IDLE:
          // case USB_HID_SET_IDLE:
          // case USB_HID_GET_PROTOCOL:
          // case USB_HID_SET_PROTOCOL:
          default:
            usb_ep0_state = EP0_STALL;
            break;
        }
      }
      // Handle the chapter 9 standard requests
      else
      {
        usb_ep0_state = USB_handleStdRequest();
      }

      // Update the EP0 controller based on the new state
      if (usb_ep0_state == EP0_STALL)
      {
        // There was a request error, send a stall
        USB_writeIndirectReg(E0CSR, E0CSR_SDSTL__SET);
      }
      else if (usb_ep0_state == EP0_IDLE)
      {
        // Setup processing is complete, indicate there is no data stage
        USB_writeIndirectReg(E0CSR, (E0CSR_SOPRDY__SET | E0CSR_DATAEND__SET));
      }
      else
      {
        // Setup has been processed, indicate we are ready for the data stage
        USB_writeIndirectReg(E0CSR, E0CSR_SOPRDY__SET);
      }

      // Reread EP0 control register as the value was modified
      reg_e0csr = USB_readIndirectReg(E0CSR);
    }
  }

  // EP0 is waiting to transmit data to the host
  if (usb_ep0_state == EP0_TX)
  {
    // If the transfer is active and the FIFO is empty
    if ((reg_e0csr & (E0CSR_SUEND__SET | E0CSR_INPRDY__SET | E0CSR_OPRDY__SET)) == 0)
    {
      // Note: Transfers larger than EP0_PACKET_SIZE are not supported !!!
      if ((usb_setup.bmRequestType & ~USB_REQ_DIRECTION_MASK) == USB_REQ_CLASS_INTERFACE)
      {
        writeReportData(FIFO0);
      }
      else
      {
        writeRequestData();
      }
      // Indicate data stage is complete and return to idle state
      USB_writeIndirectReg(E0CSR, (E0CSR_INPRDY__SET | E0CSR_DATAEND__SET));
      usb_ep0_state = EP0_IDLE;
    }
  }

  // EP0 is waiting to receive data from the host
  if (usb_ep0_state == EP0_RX)
  {
    // If data is waiting in the FIFO
    if (reg_e0csr & E0CSR_OPRDY__SET)
    {
      readReportData(FIFO0, USB_readIndirectReg(E0CNT));

      // Indicate data stage is complete and return to idle state
      USB_writeIndirectReg(E0CSR, (E0CSR_SOPRDY__SET | E0CSR_DATAEND__SET));
      usb_ep0_state = EP0_IDLE;
    }
  }
}

// -----------------------------------------------------------------------------
// Read an indirect register in the USB module
// -----------------------------------------------------------------------------
uint8_t USB_readIndirectReg(uint8_t addr)
{
  while (USB0ADR & USB0ADR_BUSY__SET)
    ;
  USB0ADR = (USB0ADR_BUSY__SET | addr);
  while (USB0ADR & USB0ADR_BUSY__SET)
    ;
  return USB0DAT;
}

// -----------------------------------------------------------------------------
// Write an indirect register in the USB module
// -----------------------------------------------------------------------------
void USB_writeIndirectReg(uint8_t addr, uint8_t value)
{
  while (USB0ADR & USB0ADR_BUSY__SET)
    ;
  USB0ADR = addr; 
  USB0DAT = value;
}

// -----------------------------------------------------------------------------
// Write the USB module address register
// -----------------------------------------------------------------------------
static void USB_writeAddressReg(uint8_t addr)
{
  while (USB0ADR & USB0ADR_BUSY__SET)
    ;
  USB0ADR = addr;
}

// -----------------------------------------------------------------------------
// Read the USB module data register
// -----------------------------------------------------------------------------
static uint8_t USB_readDataReg(void)
{
  while (USB0ADR & USB0ADR_BUSY__SET)
    ;
  return USB0DAT;
}

// -----------------------------------------------------------------------------
// Read a setup packet from Endpoint 0 and convert word data to big-endian.
// -----------------------------------------------------------------------------
static void readSetupPacket(void)
{
  // Select auto-read from EP0 FIFO
  usb_setup.bmRequestType = USB_readIndirectReg(USB0ADR_AUTORD__ENABLED | FIFO0);

  // Read the setup packet one byte at a time, while converting word 
  // parameters to big-endian format for the Keil compiler
  usb_setup.bRequest = USB_readDataReg();
  usb_setup.wValue.c[LSB] = USB_readDataReg();
  usb_setup.wValue.c[MSB] = USB_readDataReg();
  usb_setup.wIndex.c[LSB] = USB_readDataReg();
  usb_setup.wIndex.c[MSB] = USB_readDataReg();
  usb_setup.wLength.c[LSB] = USB_readDataReg();
  usb_setup.wLength.c[MSB] = USB_readDataReg();

  // Stop auto-read before returning
  USB_writeAddressReg(USB0ADR_AUTORD__DISABLED);
}

// -----------------------------------------------------------------------------
// Write control request data to the Endpoint 0 FIFO.
// -----------------------------------------------------------------------------
static void writeRequestData(void)
{
  uint8_t i;

  // Select the EP0 FIFO
  USB_writeAddressReg(FIFO0);

  // Copy request data from the data pointer to EP0 FIFO
  for (i = 0; i < usb_setup.wLength.c[LSB]; i++)
  {
    USB0DAT = usb_dataptr[i];
    while (USB0ADR & USB0ADR_BUSY__SET)
      ;
  }
}

// -----------------------------------------------------------------------------
// Read the specified EP FIFO into the receive circular buffer.
// -----------------------------------------------------------------------------
static void readReportData(uint8_t addr, uint8_t count)
{
  // Enable auto-read and start first read on the selected FIFO
  USB_writeAddressReg(USB0ADR_BUSY__SET | USB0ADR_AUTORD__ENABLED | addr);

  // Copy selected FIFO to circular receive buffer. The buffer size is 256, 
  // so indexes wrap properly without limit testing.
  while (count)
  {
    usb_rxBuf[usb_rxHead] = USB_readDataReg();
    usb_rxHead++;
    count--;
  }

  // Stop auto-read before returning
  USB_writeAddressReg(USB0ADR_AUTORD__DISABLED);
}

// -----------------------------------------------------------------------------
// Write the linear transmit buffer to the specified EP FIFO.
// -----------------------------------------------------------------------------
static void writeReportData(uint8_t addr)
{
  uint8_t i;

  // Set the FIFO address
  USB_writeAddressReg(addr);

  // Copy linear transmit buffer to selected FIFO
  for (i = 0; usb_txCount; i++)
  {
    USB0DAT = usb_txBuf[i];
    while (USB0ADR & USB0ADR_BUSY__SET)
      ;
    usb_txCount--;
  }
  // On return usb_txCount is 0
}
