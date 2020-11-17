/*
 * main.c
 *
 *  Created on: Feb 11, 2020
 *      Author: RD
 */



//-------------------------------------------------------------------
// File: main.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description:
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

// Include files
///#include <msp430x24x.h>
///#include "svs.h"
#include <signal.h>  /// ?????
#include "variant.h"
#include "mainMSP.h"
#include "can.h"
#include "usci.h"
#include "sch.h"
#include "io.h"
#include "pwm.h"
#include "comms.h"
///#include "adc.h"
#include "meas.h"
#include "spi.h"
#include "flash.h"
#include "cfg.h"
#include "telem.h"
#include "status.h"
#include "ctrl.h"
#include "stats.h"
#include "safety.h"
#include "lcd.h"
#include "temp.h"

#define __special_area__ __attribute__((section(".specialarea")))
typedef union
{
	char force[32];
	struct
	{
		char unused[28];
		unsigned char ProductTypeWall;
		unsigned char FirmwareVersion;
		unsigned int p;
	};
} my_erasable_data_area;

#if (MODEL_ID_MV == 1)
const __special_area__ my_erasable_data_area my_bsl_info[] = {0x0,0x0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'M',AER_PRODUCT_ID,VAR_VERSION_NUMBER,0x00,0x00};
#else
#if (MODEL_ID_HV == 1)
const __special_area__ my_erasable_data_area my_bsl_info[] = {0x0,0x0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'H',AER_PRODUCT_ID,VAR_VERSION_NUMBER,0x00,0x00};
#else
   Neither MV or HV defined !!!!!
#endif
#endif

// Function prototypes
void clock_init( void );


// Global variables
volatile unsigned char send_serial_flag = FALSE;

// Main routine
int mainMSPinit( void )
{
	unsigned int ii;

	// Stop watchdog timer
///	WDTCTL = WDTPW + WDTHOLD;

///	SVSCTL=VLD3|VLD1|VLD0|PORON;	//RDD comment Vcc control

	MAIN_resetAllAndStart();

	// Wait 1 sec
	for(ii = 0; ii < 1000; ii++) brief_pause(100);
	return 1;
}	;

int mainMSPloop( char l )
{	
	do
	{
		/*if ( !( ADC12CTL1 & ADC12BUSY ) )
		{
			ADC12CTL0 |= ADC12SC;               	// Start A/D conversions
		}*/

		CAN_transmit();

		COMMS_receive();

		SCH_runActiveTasks();

		uart_receive();
	}
  while(l); 
	// Will never get here, keeps compiler happy
	return(1);
}

void MAIN_resetAllAndStart()
{
	unsigned int ii;

///	dint(); //RDD comment Disable interrupts ?

	// Initialise I/O ports
	IO_init();

	//P3OUT &= ~LCD_RX;
	// Initialise SPI port for CAN controller (running with SMCLK, to reset MCP2515)
	//usci_init(0);




	// Wait a bit for clocks etc to stabilise
	for(ii = 0; ii < 1000; ii++) brief_pause(10);

	// Reset CAN controller to give reliable clock output
	// change clock to faster rate (CANCTRL reg, change lower two bits, CLK/4)
	CAN_init( 0x600 );

	// Initialise clock module now that the MCP2515 is giving us the faster clock
	//clock_init();

#ifndef DBG_USE_INTERNAL_CLOCK
	// Re-initialise SPI port with faster clock
	//usci_init(1);
#endif

	VAR_retreive_hware();
//	SPI_init();
//	FLASH_init();
	lcd_init();
	CFG_init();
	TIME_init();
	SCH_init();
	STATUS_init();
//	ADC_init();
	TEMP_init();
//	PWM_init();
	MEAS_init();
	TELEM_init();
	FLAG_init();
	STATS_init();
	COMMS_init();
	SAFETY_init();
	CTRL_init();
	uart_init();
	



	// Redo CAN init now that we have loaded config to set rx mask correctly
	CAN_init( CFG_localCfg.canBaseId );

	// Enable interrupts
///	eint();

//#if (MODEL_ID_HV == 1) 	// Removed due to startup power issues 3/6/19 - P. Watkinson
//
//	IO_setFanDutyCycle( 10 );	//Fan comes on full ball initially as a test
//								//Should stay on about 2seconds until meas_updateTempr starts running
//#endif

	//IO_enablePwmCtrl();
}

void MAIN_resetRemoteCfg()
{
	lcd_init();
	CFG_init();
	CAN_init( CAN_getBaseId() );
	CTRL_init();
	CTRL_calcOutVoltSetpoints();
	FLAG_initTrigs();
	//TELEM_switchToNextSector();
}

/*
 * Delay function
 */
///void __inline__
void brief_pause(register unsigned int n)
{
///    __asm__ __volatile__ (
///		"1: \n"
///		" dec	%[n] \n"
///		" jne	1b \n"
///        : [n] "+r"(n));

}

void clock_init( void )
{

#ifdef DBG_USE_INTERNAL_CLOCK
	/*
	 * Initialise clock module
	 *	- Setup MCLK, ACLK, SMCLK dividers and clock sources
	 *	- ACLK  = 0
	 *	- MCLK  = 16 MHz internal oscillator
	 *	- SMCLK = 16 MHz internal oscillator
	 */
	/// RDD OLD hardware if (CALBC1_16MHZ != 0xFF)
	{
		/// RDD OLD hardware BCSCTL1 = CALBC1_16MHZ;
		/// RDD OLD hardware DCOCTL = CALDCO_16MHZ;
	}
	/// RDD OLD hardware else
	{
		// Set to a frequency less than 16MHz temporarily until the correct
		// calibration values can be read from the flash backup
		// This should allow the SPI to function correctly so the flash can be read.
		/// RDD OLD hardware BCSCTL1 = 0x8F;
		/// RDD OLD hardware DCOCTL = 0x00;
	}

#else
	/*
	 * Initialise clock module
	 *	- Setup MCLK, ACLK, SMCLK dividers and clock sources
	 *	- Make sure input clock (from CAN controller) has stabilised
	 *	- ACLK  = CLKIN/1
	 *	- MCLK  = CLKIN/1
	 *	- SMCLK = 0
	 */
	//unsigned int i;

/// hardware

#endif

}





