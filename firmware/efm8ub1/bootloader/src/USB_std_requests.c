/**************************************************************************//**
 * Copyright (c) 2015 by Silicon Laboratories Inc. All rights reserved.
 *
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *****************************************************************************/

#include "efm8_device.h"
#include "USB_main.h"
#include "USB_descriptor.h"

// Holds the next EP0 state while processing standard requests
static uint8_t next_state;

// Response data returned by several of the standard requests
static uint8_t SI_SEG_CODE ONES_PACKET[2] = {0x01, 0x00};
static uint8_t SI_SEG_CODE ZERO_PACKET[2] = {0x00, 0x00};

// -----------------------------------------------------------------------------
// sendOnesPacket, sendZeroPacket
// -----------------------------------------------------------------------------
static void sendOnesPacket(void)
{
  next_state = EP0_TX;
  usb_dataptr = ONES_PACKET;
}

static void sendZeroPacket(void)
{
  next_state = EP0_TX;
  usb_dataptr = ZERO_PACKET;
}

// -----------------------------------------------------------------------------
// Returns a two byte status packet to the host.
// -----------------------------------------------------------------------------
static void getStatus(void)
{
  // If the device is configured, return the halt status for EP1
  if ((usb_dev_state == DEV_CONFIGURED)
      && (usb_setup.bmRequestType == USB_REQ_IN_ENDPOINT)
      && (usb_setup.wIndex.i == (USB_EP_DIR_IN | 1)))
  {
    if (usb_ep1_halted)
    {
      sendOnesPacket();
    }
    else
    {
      sendZeroPacket();
    }
  }
  // Regardless of device state always return a zero response for
  // device, interface or endpoint 0
  else if (usb_setup.wIndex.i == 0)
  {
    sendZeroPacket();
  }
  // Return a request error (stall) for all other cases
}

// -----------------------------------------------------------------------------
// Clears or sets the Halt Endpoint feature on endpoint 1.
// -----------------------------------------------------------------------------
static void clearSetFeature(void)
{
  // If the device is configured, handle HALT requests for EP1
  if ((usb_dev_state == DEV_CONFIGURED)
      && (usb_setup.bmRequestType == USB_REQ_OUT_ENDPOINT)
      && (usb_setup.wIndex.i == (USB_EP_DIR_IN | 1))
      && (usb_setup.wValue.i == USB_FEATURE_ENDPOINT_HALT))
  {
    next_state = EP0_IDLE;

    // Clear or set halt status depending on the request
    USB_writeIndirectReg(INDEX, INDEX_EPSEL__ENDPOINT_1);
    if (usb_setup.bRequest == CLEAR_FEATURE)
    {
      usb_ep1_halted = false;
      USB_writeIndirectReg(EINCSRL, EINCSRL_CLRDT__BMASK);
    }
    else
    {
      usb_ep1_halted = true;
      USB_writeIndirectReg(EINCSRL, EINCSRL_SDSTL__SET);
    }
  }
  // All other features are not supported, return a request error (stall)
}

// -----------------------------------------------------------------------------
// Sets a new USB function address.
// -----------------------------------------------------------------------------
static void setAddress(void)
{
  // Only accept addresses less than 128
  if (usb_setup.wValue.i < 128)
  {
    next_state = EP0_IDLE;

    // Post the new function address. The controller waits until after the
    // status phase to do the update
    USB_writeIndirectReg(FADDR, usb_setup.wValue.c[LSB]);

    // If the address is 0, move to the default state
    if (usb_setup.wValue.c[LSB] == 0)
    {
      usb_dev_state = DEV_DEFAULT;
    }
    // Otherwise, move to the addressed state
    else
    {
      usb_dev_state = DEV_ADDRESS;
    }
  }
}

// -----------------------------------------------------------------------------
// Sets up the data pointer and size for the requested descriptor.
// -----------------------------------------------------------------------------
static void getDescriptor(void)
{
  uint8_t size = 0;

  // Request has a transmit data phase
  next_state = EP0_TX;

  // Setup data pointer and size for the requested descriptor
  switch (usb_setup.wValue.c[MSB])
  {
    case USB_DEVICE_DESCRIPTOR:
      usb_dataptr = (uint8_t *) &USB_DeviceDesc;
      size = USB_DeviceDesc.bLength;
      break;

    case USB_CONFIG_DESCRIPTOR:
      usb_dataptr = (uint8_t *) &USB_HidConfigDesc;
      size = sizeof(USB_HidConfigDesc_t);
      break;

    case USB_HID_DESCRIPTOR:
      usb_dataptr = (uint8_t *) &USB_HidConfigDesc.hid_class;
      size = USB_HidConfigDesc.hid_class.bLength;
      break;

    case USB_HID_REPORT_DESCRIPTOR:
      usb_dataptr = (uint8_t *) &USB_HidReportDesc;
      size = sizeof(USB_HidReportDesc_t);
      break;

    default:
      next_state = EP0_STALL;
      break;
  }

  // Send only the requested amount of data
  if (size < usb_setup.wLength.c[LSB])
  {
    usb_setup.wLength.c[LSB] = size;
  }
}

// -----------------------------------------------------------------------------
// Returns the current configuration value.
// -----------------------------------------------------------------------------
static void getConfiguration(void)
{
  // If the device is configured, then return 0x01 as this device only has
  // one configuration
  if (usb_dev_state == DEV_CONFIGURED)
  {
    sendOnesPacket();
  }
  // Return 0x00 for other states
  else
  {
    sendZeroPacket();
  }
}

// -----------------------------------------------------------------------------
// Sets the active configuration.
// -----------------------------------------------------------------------------
static void setConfiguration(void)
{
  // Device must be addressed before setting the configuration
  if ((usb_dev_state != DEV_DEFAULT)
      && (usb_setup.wValue.i < 2))
  {
    next_state = EP0_IDLE;

    // Any positive configuration request sets the configuration to 1
    if (usb_setup.wValue.c[LSB] != 0)
    {
      // Update to the configured state
      usb_dev_state = DEV_CONFIGURED;
      usb_ep1_halted = false;
      // Configure endpoints
      USB_writeIndirectReg(INDEX, INDEX_EPSEL__ENDPOINT_1);
      USB_writeIndirectReg(EINCSRL, EINCSRL_CLRDT__BMASK);
      USB_writeIndirectReg(EINCSRH, EINCSRH_DIRSEL__IN);
    }
    else
    {
      // Update to the unconfigured state
      usb_dev_state = DEV_ADDRESS;
      usb_ep1_halted = true;
    }
  }
}

// -----------------------------------------------------------------------------
// Returns the interface alternate setting, which is always 0x00 for this device.
// -----------------------------------------------------------------------------
static void getInterface(void)
{
  // Return a value only if we are configured
  if (usb_dev_state == DEV_CONFIGURED)
  {
    sendZeroPacket();
  }
}

// -----------------------------------------------------------------------------
// Handle the USB chapter 9 standard control requests.
//
// Called by the EP0 event handler, this routine interprets and handles the 
// standard USB "Chapter 9" control requests. It returns the next EPO state 
// (EP0_STALL for errors, EP0_IDLE if there is no data phase, and EP0_TX for 
// an IN data phase). If the next state is EP0_TX, usb_dataptr points to the
// requested data and usb_setup.wLength is adjusted to the correct size.
// -----------------------------------------------------------------------------
uint8_t USB_handleStdRequest(void)
{
  // Return a stall if the request is not handled
  next_state = EP0_STALL;

  // Call appropriate handler based on request value
  switch (usb_setup.bRequest)
  {
    case GET_STATUS:
      getStatus();
      break;

    case CLEAR_FEATURE:
    case SET_FEATURE:
      clearSetFeature();
      break;

    case SET_ADDRESS:
      setAddress();
      break;

    case GET_DESCRIPTOR:
      getDescriptor();
      break;

    case GET_CONFIGURATION:
      getConfiguration();
      break;

    case SET_CONFIGURATION:
      setConfiguration();
      break;

    case GET_INTERFACE:
      getInterface();
      break;

    case SET_INTERFACE:
      // Only one interface, just accept the request
      next_state = EP0_IDLE;
      break;

    default:
      // Unknown or unsupported request, return a stall
      break;
  }

  // Reset the index register to EP0 and return the new state
  USB_writeIndirectReg(INDEX, INDEX_EPSEL__ENDPOINT_0);
  return next_state;
}
