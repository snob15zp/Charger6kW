/*!
\file
\brief Discrete I / O control

Schematic Digital Input Output
*CASETMP thermistor, no in this file

*ON_OFF_GFD
           io.c:IO_getGroundFault()
					 ctrl.c:CTRL_tick()->io.c:IO_getGroundFault()
					 RDD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! set with delay disablePWM according to  GroundFault: J10 on PWRboard
*RELAY_1_EN
*RELAY_2_EN
           io.c: IO_setRelay
*TMPCMP or Digital?
        Digital:
        io.c: int IO_getDigitalTemp()
        temp.c:TEMP_init()->io.c: int IO_getDigitalTemp()
				pwm.c:PWM_isr()->temp.c:TEMP_tick()->io.c: int IO_getDigitalTemp()
*ONOFF/SLAVE#
        io.c:  epsent? 
				
In io.c:
        IO_getIsSlave
				
				FAN                                                     high level logic       limit fanDuty
				mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->io.c:IO_fanSetSpeed->io.c:IO_setFanDutyCycle( unsigned int fanDuty);
        mainMSP.c:TIM3_IRQHandler(void)->PWM.c:PWM_isr(void)->io.c:IO_fanSenseSpeed(void);
	      mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->io.c:IO_fanDrvPWM(void);


*/

//-------------------------------------------------------------------
// File: io.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef IO_H
#define IO_H

#include "mainMSP.h"

//#define ONE_FAN
#define TWO_FAN

#if (AER_PRODUCT_ID == AER07_WALL)

	// Pin Definitions
	// Port 1

	#define FAN_DRV			0x01
	#define UNUSED			0x02
	#define FAN_FB			0x04
	#define TMPCMP_DIGITAL  0x08
	#define FLSET10			0x10
	#define FLSET20			0x20
	#define FLSET40			0x40
	#define FLSET80			0x80

	// Port 2
	#define ENABLE			0x01
	#define CAN_nINT		0x02
	#define CA_OUT			0x04
	#define FLTRIM_PWM		0x08
	#define SAMPLE_PWM		0x10
	#define IOUTSP_CA		0x20
	#define ONOFF			0x40
	#define VINLIM_PWM		0x80

	// Port 3
	#define CAN_nCS			0x01
	#define CAN_MOSI		0x02
	#define CAN_MISO		0x04
	#define CAN_SCLK		0x08
	#define LCD_TX			0x10	
	#define LCD_RX			0x20	
	#define nHISIDE_EN		0x40
	#define RELAY			0x80

	// Port 4
	#define DCONTROL		0x01
	#define FLSET1			0x04
	#define FLSET2			0x08
	#define FLSET4			0x10
	#define FLSET8			0x20
	#define FLSET100		0x40
	#define FLSET200		0x80
	#define P4_UNUSED		0x02

	// Port 5
	#define FLASH_nCS		0x01
	#define FLASH_SIMO		0x02
	#define FLASH_SOMI		0x04
	#define FLASH_SCLK		0x08
	#define FAN_FB2			0x10
	#define FLASH_HOLD		0x20
	#define P5_UNUSED		0x40
	#define GF_ONOFF		0x80

	// Port 6
	// Analog setup done in adc.c
	//#define ANLG_PV_VOLT		0x01
	//#define ANLG_OUT_VOLT		0x02
	//#define ANLG_PWM_ERR_MINUS	0x04
	//#define ANLG_PV_CURR		0x08
	//#define ANLG_TMP_CMP_SENSE	0x10
	//#define ANLG_RAIL_12_VOLT	0x20
	//#define ANLG_OUT_CURR		0x40
	//#define ANLG_FL_SET_SENSE	0x80
	#define P6_UNUSED			0x00


// Prototypes

// Duty cycle is integer between 0 and IO_FAN_PWM_PERIOD
// Full duty cycle is IO_FAN_PWM_PERIOD, zero duty cycle is zero. Duty cycle is roughly fanDuty / IO_FAN_PWM_PERIOD
	void IO_setFanDutyCycle( unsigned int fanDuty);
	void IO_fanSenseSpeed(void);
	void IO_fanSetSpeed(void);
	void IO_fanDrvPWM(void);

	//Shared with nSlave pin (rack)
	int IO_getOnOff(void);
	int IO_getGroundFault(void);

	int IO_getDigitalTemp(void);

	#define IO_INBREAKER_OPEN (0)		//temp so flag code will compile - breakers are never open in WALL
	#define IO_OUTBREAKER_OPEN (0)

#elif ( AER_PRODUCT_ID == AER05_RACK )


	//code for AER05_RACK
	generate errors if i accidentally compile any rack mount code
	
	// Pin Definitions
	// Port 1

	#define SW_RED			0x01
	#define SW_ORANGE		0x02
	#define SW_GREEN		0x04
	#define ENABLE			0x08
	#define FLSET10			0x10
	#define FLSET20			0x20
	#define FLSET40			0x40
	#define FLSET80			0x80
	#define P1_UNUSED		0x00

	// Port 2
	#define IOUTSENSE_CA	0x01
	#define CAN_nINT		0x02
	#define CA_OUT			0x04
	#define FLTRIM_PWM		0x08
	#define SAMPLE_PWM		0x10
	#define IOUTSP_CA		0x20
	#define nSLAVE			0x40
	#define VINLIM_PWM		0x80
	#define P2_UNUSED		0x00

	// Port 3
	#define CAN_nCS			0x01
	#define CAN_MOSI		0x02
	#define CAN_MISO		0x04
	#define CAN_SCLK		0x08
	#define INBREAKER		0x10	
	#define OUTBREAKER		0x20	
	#define nHISIDE_EN		0x40
	#define RELAY			0x80
	#define P3_UNUSED		0x00

	// Port 4
	#define DCONTROL		0x01
	#define TMPCMPEN		0x02
	#define FLSET1			0x04
	#define FLSET2			0x08
	#define FLSET4			0x10
	#define FLSET8			0x20
	#define FLSET100		0x40
	#define FLSET200		0x80
	#define P4_UNUSED		0x00

	// Port 5
	#define FLASH_nCS		0x01
	#define FLASH_SIMO		0x02
	#define FLASH_SOMI		0x04
	#define FLASH_SCLK		0x08
	#define FLASH_WRP		0x10
	#define FLASH_HOLD		0x20
	#define P5_UNUSED		0x40 | 0x80

	// Port 6
	// Analog setup done in adc.c
	//#define ANLG_PV_VOLT		0x01
	//#define ANLG_OUT_VOLT		0x02
	//#define ANLG_PWM_ERR_MINUS	0x04
	//#define ANLG_PV_CURR		0x08
	//#define ANLG_TMP_CMP_SENSE	0x10
	//#define ANLG_RAIL_12_VOLT	0x20
	//#define ANLG_OUT_CURR		0x40
	//#define ANLG_FL_SET_SENSE	0x80
	#define P6_UNUSED			0x00



	#define IO_INBREAKER_OPEN ( ( P3IN & INBREAKER ) == 0 )
	#define IO_OUTBREAKER_OPEN ( ( P3IN & OUTBREAKER ) == 0 )

// Prototypes
	void IO_toggleGreenLed();
	void IO_toggleOrangeLed();
	void IO_toggleRedLed();
	void IO_setRedLed( unsigned int state );


#endif

// Prototypes

int IO_getIsSlave(void); //works differently for RACK and WALL


void IO_init( void );
void IO_setRelay( unsigned long long bitfield );

void IO_enablePwmCtrl(void);
void IO_disablePwmCtrl(void);

void IO_controlSyncRect(void);


void IO_flset( unsigned int fls );

//void IO_flsetAllOn( void );
//void IO_flsetAllOff( void );

#endif // IO_H

