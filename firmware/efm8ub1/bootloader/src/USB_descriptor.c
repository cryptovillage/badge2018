/**************************************************************************//**
 * Copyright (c) 2015 by Silicon Laboratories Inc. All rights reserved.
 *
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *****************************************************************************/

#include <endian.h>
#include "efm8_device.h"
#include "USB_main.h"
#include "USB_descriptor.h"

#define BSIZE(t)  sizeof(t)
#define WSIZE(t)  htole16(sizeof(t))

// -----------------------------------------------------------------------------
// Descriptor Definitions
// -----------------------------------------------------------------------------

USB_DeviceDesc_t SI_SEG_CODE USB_DeviceDesc =
{
  BSIZE(USB_DeviceDesc_t),          // bLength
  0x01,                             // bDescriptorType (device)
  htole16(0x0200),                  // bcdUSB (2.0)
  0x00,                             // bDeviceClass
  0x00,                             // bDeviceSubClass
  0x00,                             // bDeviceProtocol
  USB_EP0_SIZE,                     // bMaxPacketSize0
  htole16(BL_USB_VID),              // idVendor
  htole16(BL_USB_PID),              // idProduct
  htole16(0x0100),                  // bcdDevice
  0x00,                             // iManufacturer
  0x00,                             // iProduct
  0x00,                             // iSerialNumber
  0x01                              // bNumConfigurations
};

USB_HidConfigDesc_t SI_SEG_CODE USB_HidConfigDesc =
{
  // USB_ConfigDesc_t config;
  {
    BSIZE(USB_ConfigDesc_t),        // bLength
    0x02,                           // bDescriptorType (configuration)
    WSIZE(USB_HidConfigDesc_t),     // wTotalLength
    0x01,                           // bNumInterfaces
    0x01,                           // bConfigurationValue
    0x00,                           // iConfiguration
    0x80,                           // bmAttributes (bus powered)
    0x32                            // bMaxPower (in 2mA units)
  },
  // USB_InterfaceDesc_t interface;
  {
    BSIZE(USB_InterfaceDesc_t),     // bLength
    0x04,                           // bDescriptorType (interface)
    0x00,                           // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x01,                           // bNumEndpoints
    0x03,                           // bInterfaceClass (3 = HID)
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProcotol
    0x00                            // iInterface
  },
  // USB_HidClassDesc_t hid_class;
  {
    BSIZE(USB_HidClassDesc_t),      // bLength
    0x21,                           // bDescriptorType (HID class)
    htole16(0x0111),                // bcdHID (1.11)
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType
    WSIZE(USB_HidReportDesc_t)      // wItemLength (length of report desc)
  },
  // USB_EndpointDesc_t endpoint_in;
  { 
    BSIZE(USB_EndpointDesc_t),      // bLength
    0x05,                           // bDescriptorType (endpoint)
    0x81,                           // bEndpointAddress (IN1)
    0x03,                           // bmAttributes (interrupt)
    htole16(USB_EP1_SIZE),          // MaxPacketSize
    1                               // bInterval (msec)
  },
};

USB_HidReportDesc_t SI_SEG_CODE USB_HidReportDesc =
{
  // Descriptor prefix
  0x06, 0x00, 0xff,       // USAGE_PAGE (Vendor Defined Page 1)
  0x09, 0x01,             // USAGE (Vendor Usage 1)
  0xa1, 0x01,             // COLLECTION (Application)
  0x15, 0x00,             // LOGICAL_MINIMUM (0)
  0x26, 0xff, 0x00,       // LOGICAL_MAXIMUM (255)
  0x75, 0x08,             // REPORT_SIZE (8)
  // Input report
  0x95, USB_HID_IN_SIZE,  // REPORT_COUNT (4)
  0x09, 0x01,             // USAGE (Vendor Usage 1)
  0x81, 0x02,             // INPUT (Data,Var,Abs)
  // Output report
  0x95, USB_HID_OUT_SIZE, // REPORT_COUNT (64)
  0x09, 0x01,             // USAGE (Vendor Usage 1)
  0x91, 0x02,             // OUTPUT (Data,Var,Abs)
  // Descriptor postfix
  0xC0                    // END_COLLECTION
};

