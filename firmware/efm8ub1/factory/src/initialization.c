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

#include <SI_EFM8UB1_Register_Enums.h>
#include "descriptor.h"
#include "callback.h"
#include "initialization.h"

SI_SBIT(ENSET, SFR_P0, 1);
SI_SBIT(ESP32_BOOT, SFR_P0, 3);
SI_SBIT(ESP32_EN, SFR_P1, 4);

/**************************************************************************//**
 *  @brief
 *    Init entire system
 *   
 *  Top-level initialization routine to call all other module initialization
 *  subroutines. 
  
 *****************************************************************************/
void System_Init (void)
{
   //Disable Watchdog with key sequence
   WDTCN = 0xDE;
   WDTCN = 0xAD;

   // Enable VDD monitor and set it as a reset source
   VDM0CN |= VDM0CN_VDMEN__ENABLED;
   RSTSRC = RSTSRC_PORSF__SET;

   SYSCLK_Init ();                     // Initialize system clock to
                                       // 48MHz
   PORT_Init ();                       // Initialize crossbar and GPIO
   USB_Init(&InitStruct);              // VCPXpress initialization
   API_Callback_Enable(myAPICallback); // Enable VCPXpress API interrupts
   UART0_Init();                       // Initialize UART0 for printf's
   TIMER3_Init();                      // Initialize tick timer
}



/**************************************************************************//**
 *  @brief
 *    Init Sysclk
 *  
 *  This routine initializes the system clock to use the internal 12MHz
 *  oscillator as its clock source.  Also enables missing clock detector reset.
 *   
 *****************************************************************************/
void SYSCLK_Init (void)
{
  SFRPAGE = 0x10;
  HFOCN  = HFOCN_HFO1EN__ENABLED;
  CLKSEL = CLKSEL_CLKDIV__SYSCLK_DIV_1 // Select hfosc (need 24Mhz before switching to 48Mhz)
         | CLKSEL_CLKSL__HFOSC0;
  CLKSEL = CLKSEL_CLKDIV__SYSCLK_DIV_1 // need to write register twice
         | CLKSEL_CLKSL__HFOSC0;
  while(!(CLKSEL & CLKSEL_DIVRDY__READY));
  CLKSEL = CLKSEL_CLKDIV__SYSCLK_DIV_1 // Select hfosc/1
         | CLKSEL_CLKSL__HFOSC1;
  CLKSEL = CLKSEL_CLKDIV__SYSCLK_DIV_1 // need to write register twice
         | CLKSEL_CLKSL__HFOSC1;
  while(!(CLKSEL & CLKSEL_DIVRDY__READY));
  SFRPAGE = 0x0;
}


/**************************************************************************//**
 *  @brief
 *    Init Ports and Xbar
 *  
 *  Configure the Crossbar and GPIO ports.
 *    * P0.4 - UART TX (push-pull)
 *    * P0.5 - UART RX
 *   
 *****************************************************************************/
void PORT_Init (void)
{
	SFRPAGE = 0;
	XBR0     = XBR0_URT0E__ENABLED | XBR0_SMB0E__ENABLED; // Enable UART0, I2C, and SPI
	XBR2     = XBR2_XBARE__ENABLED;      // Enable crossbar
	ENSET = 0;
	ESP32_BOOT = 1;
	ESP32_EN = 0;
	P1MDOUT |= 0x10;                    // Set ESP32_EN to be push-pull
	P0MDOUT |= 0x1a;                    // Set ENSET, ESP32_BOOT, and TX to push-pull.
	P2MDOUT |= 0;

	P0MDIN = P0MDIN_B0__DIGITAL | P0MDIN_B1__DIGITAL | P0MDIN_B2__DIGITAL
				| P0MDIN_B3__DIGITAL | P0MDIN_B4__DIGITAL | P0MDIN_B5__DIGITAL
	  			| P0MDIN_B6__ANALOG | P0MDIN_B7__ANALOG;
	P0SKIP = P0SKIP_B0__SKIPPED | P0SKIP_B1__SKIPPED | P0SKIP_B2__SKIPPED
	  			| P0SKIP_B3__SKIPPED | P0SKIP_B4__NOT_SKIPPED
	  			| P0SKIP_B5__NOT_SKIPPED | P0SKIP_B6__SKIPPED | P0SKIP_B7__SKIPPED;
	P1SKIP = P1SKIP_B0__SKIPPED | P1SKIP_B1__SKIPPED
				| P1SKIP_B2__SKIPPED | P1SKIP_B3__SKIPPED | P1SKIP_B4__SKIPPED
				| P1SKIP_B5__NOT_SKIPPED | P1SKIP_B6__NOT_SKIPPED
				| P1SKIP_B7__SKIPPED;
}


/**************************************************************************//**
 *  @brief
 *    Init Uart and it's timer.
 *  
 *  Configure the UART0 using Timer1, for <BAUDRATE> and 8-N-1.
 *   
 *****************************************************************************/
void UART0_Init (void)
{
  //UART to 8-bit bidirectional
  SCON0 = SCON0_SMODE__8_BIT
           | SCON0_REN__RECEIVE_ENABLED
           | SCON0_RB8__NOT_SET
           | SCON0_TB8__NOT_SET
           | SCON0_MCE__MULTI_DISABLED
           | SCON0_RI__NOT_SET
           | SCON0_TI__NOT_SET;

  CKCON0 &= ~CKCON0_T1M__BMASK; //select prescaler

  if (SYSCLK/BAUDRATE/2/256 < 1) {
    TH1 = -(SYSCLK/BAUDRATE/2/1);
    CKCON0 |=  CKCON0_T1M__SYSCLK;                 // T1M = 1;
  }else if (SYSCLK/BAUDRATE/2/256 < 4) {
    TH1 = -(SYSCLK/BAUDRATE/2/4);
    CKCON0 &= ~CKCON0_SCA__FMASK;                  // T1M = 0; SCA1:0 = 01
    CKCON0 |=  CKCON0_SCA__SYSCLK_DIV_4;
  } else if (SYSCLK/BAUDRATE/2/256 < 12) {
    TH1 = -(SYSCLK/BAUDRATE/2/12);
    CKCON0 &= ~CKCON0_SCA__FMASK;                  // T1M = 0; SCA1:0 = 01
  } else if (SYSCLK/BAUDRATE/2/256 < 48) {
    TH1 = -(SYSCLK/BAUDRATE/2/48);
    CKCON0 &= ~CKCON0_SCA__FMASK;                  // T1M = 0; SCA1:0 = 01
    CKCON0 |=  CKCON0_SCA__SYSCLK_DIV_48;
  } else {
     while (1);                       // Error.  Unsupported baud rate
  }
  TL1 = TH1;                          // init Timer1 reload

   //Setup timer 0 as 1ms tick for timeout.
   CKCON0 |=  CKCON0_T0M__SYSCLK; // T0M = 1;
   TH0 = -(SYSCLK/1000)>>8;     
   TL0 = -(SYSCLK/1000) & 0x00FF;
   
   TMOD = TMOD_CT0__TIMER
          | TMOD_T0M__MODE1
          | TMOD_CT1__TIMER
          | TMOD_T1M__MODE2;
   
   TCON_TR1 = 1;                       // START Timer1
   TCON_TR0 = 1;                       // START Timer0

   IP   = IP_PS0__HIGH;                //Make UART0 high priority so we don't drop bytes
   IE_ES0 = 1;                         // Enable UART0 interrupts
   TCON_IT0 = 0;                       // clear timer flag;
   IE_ET0 = 1;                         // Enable TIMER0 interrupts;
}

void TIMER2_Init(void) {
  IE |= IE_ET2__ENABLED;
}

void TIMER3_Init(void) {
	uint8_t TMR3CN0_TR3_save = TMR3CN0 & TMR3CN0_TR3__BMASK;
	// Stop Timer
	TMR3CN0 &= ~(TMR3CN0_TR3__BMASK);

	TMR3RLH = (0xF0 << TMR3RLH_TMR3RLH__SHIFT);
	TMR3RLL = (0x60 << TMR3RLL_TMR3RLL__SHIFT);
	TMR3CN0 |= TMR3CN0_TR3__RUN;

	// Restore Timer Configuration
	TMR3CN0 |= TMR3CN0_TR3_save;

  SFRPAGE = 0x10;
  EIE1 |= EIE1_ET3__ENABLED;
  SFRPAGE = 0x00;

}
