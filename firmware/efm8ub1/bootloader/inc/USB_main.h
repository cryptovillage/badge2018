/**************************************************************************//**
 * Copyright (c) 2015 by Silicon Laboratories Inc. All rights reserved.
 *
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *****************************************************************************/

#ifndef __USB_INTERRUPT_H__
#define __USB_INTERRUPT_H__

#include "si_toolchain.h"

// -----------------------------------------------------------------------------
// USB HID Driver Definitions
// -----------------------------------------------------------------------------

// Define Endpoint Packet Sizes
#define USB_EP0_SIZE 64
#define USB_EP1_SIZE 64

// Define HID Report Sizes
#define USB_HID_IN_SIZE   4
#define USB_HID_OUT_SIZE 64

// SETUP.bmRequestType bitmasks and values
#define USB_REQ_DIRECTION_MASK    0x80    ///< Direction is bmRequestType[7]
#define USB_REQ_DIRECTION_IN      0x80    ///< Data stage direction IN value.
#define USB_REQ_DIRECTION_OUT     0x00    ///< Data stage direction OUT value.

#define USB_REQ_TYPE_MASK         0x60    ///< Type is bmRequestType[6:5]
#define USB_REQ_TYPE_STANDARD     0x00    ///< Standard request type value.
#define USB_REQ_TYPE_CLASS        0x20    ///< Class request type value.
#define USB_REQ_TYPE_VENDOR       0x40    ///< Vendor request type value.

#define USB_REQ_RCVR_MASK         0x1F    ///< Recipient is bmRequestType[4:0]
#define USB_REQ_RCVR_DEVICE       0       ///< Device recipient value.
#define USB_REQ_RCVR_INTERFACE    1       ///< Interface recipient value.
#define USB_REQ_RCVR_ENDPOINT     2       ///< Endpoint recipient value.

// SETUP.bRequest standard request codes for Full Speed devices
#define GET_STATUS                0       ///< Standard setup request GET_STATUS.
#define CLEAR_FEATURE             1       ///< Standard setup request CLEAR_FEATURE.
#define SET_FEATURE               3       ///< Standard setup request SET_FEATURE.
#define SET_ADDRESS               5       ///< Standard setup request SET_ADDRESS.
#define GET_DESCRIPTOR            6       ///< Standard setup request GET_DESCRIPTOR.
#define SET_DESCRIPTOR            7       ///< Standard setup request SET_DESCRIPTOR.
#define GET_CONFIGURATION         8       ///< Standard setup request GET_CONFIGURATION.
#define SET_CONFIGURATION         9       ///< Standard setup request SET_CONFIGURATION.
#define GET_INTERFACE             10      ///< Standard setup request GET_INTERFACE.
#define SET_INTERFACE             11      ///< Standard setup request SET_INTERFACE.
#define SYNCH_FRAME               12      ///< Standard setup request SYNCH_FRAME.

// SETUP.bRequest HID class request codes
#define USB_HID_GET_REPORT        0x01    ///< HID class setup request GET_REPORT.
#define USB_HID_GET_IDLE          0x02    ///< HID class setup request GET_IDLE.
#define USB_HID_GET_PROTOCOL      0x03    ///< HID class setup request GET_PROTOCOL.
#define USB_HID_SET_REPORT        0x09    ///< HID class setup request SET_REPORT.
#define USB_HID_SET_IDLE          0x0A    ///< HID class setup request SET_IDLE.
#define USB_HID_SET_PROTOCOL      0x0B    ///< HID class setup request SET_PROTOCOL.

// SETUP command GET/SET_DESCRIPTOR descriptor types
#define USB_DEVICE_DESCRIPTOR     1       ///< DEVICE descriptor value.
#define USB_CONFIG_DESCRIPTOR     2       ///< CONFIGURATION descriptor value.
#define USB_STRING_DESCRIPTOR     3       ///< STRING descriptor value.
#define USB_INTERFACE_DESCRIPTOR  4       ///< INTERFACE descriptor value.
#define USB_ENDPOINT_DESCRIPTOR   5       ///< ENDPOINT descriptor value.
#define USB_HID_DESCRIPTOR        0x21    ///< HID descriptor value.
#define USB_HID_REPORT_DESCRIPTOR 0x22    ///< HID REPORT descriptor value.

// SETUP.wIndex bitmaps
#define USB_EP_DIR_IN             0x80    ///< Endpoint IN direction mask.
#define USB_EP_DIR_OUT            0x00    ///< Endponit OUT direction mask.

// SETUP.wValue bitmaps for Standard Feature Selectors
#define USB_FEATURE_ENDPOINT_HALT 0       ///< Standard request CLEAR/SET_FEATURE bitmask.
#define USB_FEATURE_REMOTE_WAKEUP 1       ///< Standard request CLEAR/SET_FEATURE bitmask.

// SETUP.bmRequestType aggregate values
#define USB_REQ_CLASS_INTERFACE   (USB_REQ_TYPE_CLASS | USB_REQ_RCVR_INTERFACE)
#define USB_REQ_OUT_DEVICE        (USB_REQ_DIRECTION_OUT | USB_REQ_RCVR_DEVICE)
#define USB_REQ_IN_DEVICE         (USB_REQ_DIRECTION_IN | USB_REQ_RCVR_DEVICE)
#define USB_REQ_OUT_INTERFACE     (USB_REQ_DIRECTION_OUT | USB_REQ_RCVR_INTERFACE)
#define USB_REQ_IN_INTERFACE      (USB_REQ_DIRECTION_IN | USB_REQ_RCVR_INTERFACE)
#define USB_REQ_OUT_ENDPOINT      (USB_REQ_DIRECTION_OUT | USB_REQ_RCVR_ENDPOINT)
#define USB_REQ_IN_ENDPOINT       (USB_REQ_DIRECTION_IN | USB_REQ_RCVR_ENDPOINT)

// Define device states
#define DEV_DEFAULT               0x00    // Device is in Default State
#define DEV_ADDRESS               0x01    // Device is in Addressed State
#define DEV_CONFIGURED            0x02    // Device is in Configured State
#define DEV_SUSPENDED             0x03    // Device is in Suspended State

// Define endpoint states
#define EP0_IDLE                  0x00    // Endpoint 0 is idle (default)
#define EP0_TX                    0x01    // Endpoint 0 is transmitting
#define EP0_RX                    0x02    // Endpoint 0 is receiving
#define EP0_STALL                 0x03    // Endpoint 0 is stalled

typedef union
{
  uint16_t i;
  uint8_t c[2];
}
Word_t;

// Keil compiler is big endian
#define LSB 1
#define MSB 0

// EP0 control transfer setup packet definition
typedef struct
{
  uint8_t bmRequestType;    // Request direction, type and recipient
  uint8_t bRequest;         // Specific request value
  Word_t wValue;            // Request parameter
  Word_t wIndex;            // Request parameter
  Word_t wLength;           // Number of bytes to transfer
}
USB_SetupData_t;

// -----------------------------------------------------------------------------
// USB HID Driver Global Data
// -----------------------------------------------------------------------------

// Counts USB start of frame interrupts
extern uint8_t usb_tick;

// Holds USB device state
extern uint8_t usb_dev_state;

// Holds state for endpoint 0
extern uint8_t usb_ep0_state;

// Tracks halt status for endpoint 1
extern bool usb_ep1_halted;

// Points at data returned by the standard chapter 9 control requests
extern uint8_t SI_SEG_CODE *usb_dataptr;

// Buffers setup packet from last control transfer
extern USB_SetupData_t usb_setup;

// Index of the next empty location in the receive circular buffer
extern uint8_t usb_rxHead;

// Circular buffer holds received OUTPUT report data.
// IMPORTANT! Size is 256, so indexes wrap properly without limit testing.
extern uint8_t SI_SEG_XDATA usb_rxBuf[256];

// Number of bytes in transmit linear buffer
extern uint8_t usb_txCount;

// Linear buffer holds INPUT report to transmit.
// IMPORTANT! Sized to hold one INPUT report.
extern uint8_t usb_txBuf[USB_HID_IN_SIZE];

// -----------------------------------------------------------------------------
// USB HID Driver API
// -----------------------------------------------------------------------------

void USB_initModule(void);
void USB_pollModule(void);
void USB_sendReport(void);

uint8_t USB_handleStdRequest(void);
uint8_t USB_readIndirectReg(uint8_t addr);
void USB_writeIndirectReg(uint8_t addr, uint8_t value);

#endif // __USB_INTERRUPT_H__
