/**************************************************************************//**
 * @file
 * @brief   Hardware initialization functions.
 * @author  Silicon Laboratories
 * @version 1.0.0 (DM: July 14, 2014)
 *
 *******************************************************************************
 * @section License
 * (C) Copyright 2014 Silicon Labs Inc,
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *******************************************************************************
 *
 * initialization routines for VCPXpress UART example
 * 
 *****************************************************************************/

#ifndef INITIALIZATION_H_
#define INITIALIZATION_H_

#include <SI_EFM8UB1_Register_Enums.h>                // SI_SFR declarations
#include <VCPXpress.h>

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------

#define SYSCLK       48000000          //!< SYSCLK frequency in Hz
#define BAUDRATE       115200          //!< Baud rate of UART in bps


//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void System_Init (void);
void SYSCLK_Init (void);
void PORT_Init (void);
void UART0_Init (void);
void SPI0_Init (void);
void TIMER2_Init (void);
void TIMER3_Init (void);

#endif /* INITIALIZATION_H_ */
