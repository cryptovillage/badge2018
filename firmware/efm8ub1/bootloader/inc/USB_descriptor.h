/**************************************************************************//**
 * Copyright (c) 2015 by Silicon Laboratories Inc. All rights reserved.
 *
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *****************************************************************************/

#ifndef  __USB_DESCRIPTOR_H__
#define  __USB_DESCRIPTOR_H__

#include "si_toolchain.h"

// -----------------------------------------------------------------------------
// Standard Device Descriptor Type
// -----------------------------------------------------------------------------
typedef struct
{
  uint8_t bLength;                // Size of this Descriptor in Bytes
  uint8_t bDescriptorType;        // Descriptor Type (=1)
  uint16_t bcdUSB;                // USB Spec Release Number in BCD
  uint8_t bDeviceClass;           // Device Class Code
  uint8_t bDeviceSubClass;        // Device Subclass Code
  uint8_t bDeviceProtocol;        // Device Protocol Code
  uint8_t bMaxPacketSize0;        // Maximum Packet Size for EP0
  uint16_t idVendor;              // Vendor ID
  uint16_t idProduct;             // Product ID
  uint16_t bcdDevice;             // Device Release Number in BCD
  uint8_t iManufacturer;          // Index of String Desc for Manufacturer
  uint8_t iProduct;               // Index of String Desc for Product
  uint8_t iSerialNumber;          // Index of String Desc for SerNo
  uint8_t bNumConfigurations;     // Number of possible Configurations
}
USB_DeviceDesc_t;

// -----------------------------------------------------------------------------
// Standard Configuration Descriptor Type
// -----------------------------------------------------------------------------
typedef struct
{
  uint8_t bLength;                // Size of this Descriptor in Bytes
  uint8_t bDescriptorType;        // Descriptor Type (=2)
  uint16_t wTotalLength;          // Total Length of Data for this Conf
  uint8_t bNumInterfaces;         // Num of Interfaces supported by this Conf
  uint8_t bConfigurationValue;    // Designator Value for *this* Configuration
  uint8_t iConfiguration;         // Index of String Desc for this Conf
  uint8_t bmAttributes;           // Configuration Characteristics (see below)
  uint8_t bMaxPower;              // Max. Power Consumption in this Conf (*2mA)
}
USB_ConfigDesc_t;

// -----------------------------------------------------------------------------
// Standard Interface Descriptor Type
// -----------------------------------------------------------------------------
typedef struct
{
  uint8_t bLength;                // Size of this Descriptor in Bytes
  uint8_t bDescriptorType;        // Descriptor Type (=4)
  uint8_t bInterfaceNumber;       // Number of *this* Interface (0..)
  uint8_t bAlternateSetting;      // Alternative for this Interface (if any)
  uint8_t bNumEndpoints;          // No of EPs used by this IF (excl. EP0)
  uint8_t bInterfaceClass;        // Interface Class Code
  uint8_t bInterfaceSubClass;     // Interface Subclass Code
  uint8_t bInterfaceProtocol;     // Interface Protocol Code
  uint8_t iInterface;             // Index of String Desc for this Interface
}
USB_InterfaceDesc_t;

// -----------------------------------------------------------------------------
// Standard HID Class Descriptor Type
// -----------------------------------------------------------------------------
typedef struct
{
  uint8_t bLength;                // Size of this Descriptor in Bytes (=9)
  uint8_t bDescriptorType;        // Descriptor Type (HID=0x21)
  uint16_t bcdHID;                // HID Class Specification release number (=1.01)
  uint8_t bCountryCode;           // Localized country code
  uint8_t bNumDescriptors;        // Number of class descriptors to follow
  uint8_t bReportDescriptorType;  // Report descriptor type (HID=0x22)
  uint16_t wItemLength;           // Total length of report descriptor table
}
USB_HidClassDesc_t;

// -----------------------------------------------------------------------------
// Standard Endpoint Descriptor Type
// -----------------------------------------------------------------------------
typedef struct
{
  uint8_t bLength;                // Size of this Descriptor in Bytes
  uint8_t bDescriptorType;        // Descriptor Type (=5)
  uint8_t bEndpointAddress;       // Endpoint Address (Number + Direction)
  uint8_t bmAttributes;           // Endpoint Attributes (Transfer Type)
  uint16_t wMaxPacketSize;        // Max. Endpoint Packet Size
  uint8_t bInterval;              // Polling Interval (Interrupt) ms
}
USB_EndpointDesc_t;

// -----------------------------------------------------------------------------
// Aggregate HID Configuration Descriptor Type
// -----------------------------------------------------------------------------
typedef struct
{
  USB_ConfigDesc_t config;
  USB_InterfaceDesc_t interface;
  USB_HidClassDesc_t hid_class;
  USB_EndpointDesc_t endpoint_in;
}
USB_HidConfigDesc_t;

// -----------------------------------------------------------------------------
// HID Report Descriptor Type
// -----------------------------------------------------------------------------
typedef uint8_t USB_HidReportDesc_t[27];


extern USB_DeviceDesc_t SI_SEG_CODE USB_DeviceDesc;
extern USB_HidConfigDesc_t SI_SEG_CODE USB_HidConfigDesc;
extern USB_HidReportDesc_t SI_SEG_CODE USB_HidReportDesc;

#endif // __USB_DESCRIPTOR_H__
