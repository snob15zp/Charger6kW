/*!
\file
\brief Discrete I / O control

//-------------------------------------------------------------------
// File: io.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

Schematic Digital Input Output

*CASETMP thermistor, no in this file

*ON_OFF_GFD

           io.c:IO_getGroundFault()
					 
					 ctrl.c:CTRL_tick()->io.c:IO_getGroundFault()
					 
					 RDD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! set with delay disablePWM according to  GroundFault: J10 on PWRboard
					 
*RELAY_1_EN

*RELAY_2_EN

           mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->
					 FLAG_checkAndWrite()->FLAG_checkAllFlags->io.c:IO_setRelay

					 
*TMPCMP Digital
				
        io.c: int IO_getDigitalTemp()
				
        temp.c:TEMP_init()->io.c: int IO_getDigitalTemp()
				
				pwm.c:PWM_isr()->temp.c:TEMP_tick()->io.c: int IO_getDigitalTemp()
				
*ONOFF/SLAVE#

         pwm.c:PWM_isr->cntrl.c:CTRL_tick()->io.c:int IO_getOnOff() 
				
In io.c:

        IO_getIsSlave
				
				FAN                                                     high level logic       limit fanDuty
				
				mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->io.c:IO_fanSetSpeed->io.c:IO_setFanDutyCycle( unsigned int fanDuty);
				
        mainMSP.c:TIM3_IRQHandler(void)->PWM.c:PWM_isr(void)->io.c:IO_fanSenseSpeed(void);
				
	      mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->io.c:IO_fanDrvPWM(void);


*/

//#include <msp430x24x.h>
#include "Stm32f3xx.h"
#include "variant.h"
#include "io.h"
#include "ctrl.h"
#include "flag.h"
#include "meas.h"
#include "safety.h"
#include "pwm.h"
#include "comms.h"
#include "cfg.h"
#include "adc.h"


/*
 * Initialise I/O port directions and states
 *	- Drive unused pins as outputs to avoid floating inputs
 *
 */

#if (AER_PRODUCT_ID == AER07_WALL)

	#define IO_FAN_PWM_PERIOD 10

	//To stop the fan running when the device is very cold:
	//Hysteresis thresholds for turning the fan on based off heatsink temperature.
	//Threshold has been lowered for COOLMAX G3 Because of larger heatsink
	#define IO_HEATSINK_TEMP_FAN_THRESHOLD  31.0	//60.0
	#define IO_HEATSINK_TEMP_FAN_RESET		10.0	//45.0
	
	//To stop the fan running at night/when device disabled/shutdown:
	//The fan will never be on below this output current threshold unless the device is dangerously hot.
	// (above SAFETY_CASETMP_RESET)
	#define IO_FAN_MINIMUM_CURRENT 0.0	//2.0	
	
	//The slowest that the fan is allowed to go before a safety shutdown is triggered
	// 20% speed for small fan at 4ms per pulse
	// 40% speed for large fan at 8ms per pulse
	#define IO_FAN_COUNT_TIMEOUT			100

	#define IO_SYNCRECT_UPPER_THRESHOLD		(10.0 / MEAS_OUTCURR_BASE) //High side switching is enabled above this output current threshold
	#define IO_SYNCRECT_LOWER_THRESHOLD		(4.0 / MEAS_OUTCURR_BASE) //Hysteresis setpoint for disabling high side switching.

	#define IO_SYNCRECT_TIME_HYST_TICKS		(unsigned long)( 1.0 * 1000000.0 / (float)PWM_PERIOD_US )


	unsigned int fanPWMCounter=0;
	unsigned int fanDutyCompare;
	
	extern floatValue_t calculatedValue;

	unsigned int IO_pwmEnabled;
	long TicksUntilHiSideAllowed;
	float heatsinkTempFanHysThresh = IO_HEATSINK_TEMP_FAN_THRESHOLD;

	int fanFBCount1;
#ifndef ONE_FAN
	int fanFBCount2;
	int prevFan2State;
#endif
	int fanFBTransition1;
	int fanFBTransition2;
	unsigned int fanDriven;
	unsigned int fanShutdown;
	unsigned int fanRestartTimer;

	void IO_init( void )
	{
//		P1OUT = 0x00;
//		P1DIR = FAN_DRV | FLSET10 | FLSET20 | FLSET40 | FLSET80;
//		
//		P2OUT = 0x00;
//		P2DIR = ENABLE | FLTRIM_PWM | SAMPLE_PWM | VINLIM_PWM;
//		P2SEL = FLTRIM_PWM | SAMPLE_PWM | VINLIM_PWM;

//		P3OUT = CAN_nCS | nHISIDE_EN;
//		P3DIR = CAN_nCS | CAN_MOSI | CAN_SCLK | LCD_TX | nHISIDE_EN | RELAY;		
//		
//		P4OUT = 0x00;
//		P4DIR = DCONTROL | FLSET1 | FLSET2 | FLSET4 | FLSET8 | FLSET100 | FLSET200 | P4_UNUSED;
//		
//		P5OUT = FLASH_nCS | FLASH_HOLD;
//		P5DIR = FLASH_nCS | FLASH_SIMO | FLASH_SCLK | FLASH_HOLD | P5_UNUSED;
//		P5SEL = FLASH_SIMO | FLASH_SOMI | FLASH_SCLK;
//		
//		P6OUT = 0x00;
//		P6DIR = P6_UNUSED;
		
		RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN |RCC_AHBENR_GPIODEN;
		GPIOC->MODER &= ~(GPIO_MODER_MODER6_0 | GPIO_MODER_MODER6_1 |
											GPIO_MODER_MODER7_0 | GPIO_MODER_MODER7_1);
		GPIOC->MODER |= GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0;			//PC6, PC7 - outputs REL1, REL2
		GPIOD->MODER &= ~(GPIO_MODER_MODER2_0 | GPIO_MODER_MODER2_1);
		GPIOD->MODER |= GPIO_MODER_MODER2_0;														//PD2 - output
		GPIOC->MODER &= ~(GPIO_MODER_MODER8_0 | GPIO_MODER_MODER8_1 |		//PC8 - input
											GPIO_MODER_MODER9_0 | GPIO_MODER_MODER9_1);		//PC9 - input
		
		TicksUntilHiSideAllowed = 0;
		IO_pwmEnabled = 0;

		fanFBCount1 = IO_FAN_COUNT_TIMEOUT;
#ifndef ONE_FAN
		fanFBCount2 = IO_FAN_COUNT_TIMEOUT;
#endif
		fanDriven = 0;
		fanShutdown = 0;
		fanFBTransition1 = 0;
		fanFBTransition2 = 0;
		fanRestartTimer = 0;
		prevFan2State = 1;
	}


/**
 *  @defgroup groupFAN FAN
	
					FAN                                                     high level logic       limit fanDuty
				
				mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->io.c:IO_fanSetSpeed->io.c:IO_setFanDutyCycle( unsigned int fanDuty);
				
        mainMSP.c:TIM3_IRQHandler(void)->PWM.c:PWM_isr(void)->io.c:IO_fanSenseSpeed(void);
				
	      mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->io.c:IO_fanDrvPWM(void);

	
 */	
	
	
/**
 *  @ingroup groupFAN
 *  @brief Limits fanDuty
 */
void IO_setFanDutyCycle(unsigned int fanDuty)
	{	
		if(fanDuty <= IO_FAN_PWM_PERIOD)
		{
			fanDutyCompare = fanDuty;
		} else {
			fanDutyCompare = IO_FAN_PWM_PERIOD;
		}
	}
/**
 *  @brief Just returns the state of the input 
	
	pwm.c:PWM_isr->cntrl.c:CTRL_tick()->io.c:int IO_getOnOff()
 */
	int IO_getOnOff()
	{
//		return ( ( P2IN & ONOFF ) == 0 );
		return ((GPIOB->IDR & GPIO_IDR_0)==0);
	}
	
/**
  *  @brief Just returns the state of the input
	
	    set with delay disablePWM according to  GroundFault: J10 on PWRboard
 */
	int IO_getGroundFault()
	{
//		return ((P5IN & GF_ONOFF) == 0);
		return ((GPIOC->IDR & GPIO_IDR_0)==0); //ToDo fix
	}
	
/**	
@defgroup groupDigitalTemp DigitalTemp MAX6577
*/	
	
/**
 *  @ingroup groupDigitalTemp
 *  @brief Just returns the state of the input
 */
	int IO_getDigitalTemp()
	{
//		return ((P1IN & TMPCMP_DIGITAL) != 0);
		return ((GPIOB->IDR & GPIO_IDR_1)==0);
	}

#elif (AER_PRODUCT_ID == AER05_RACK)
	
	//Code for AER05_RACK

	void IO_init( void )
	{
		P1OUT = SW_RED | SW_ORANGE | SW_GREEN;
		P1DIR = SW_RED | SW_ORANGE | SW_GREEN | ENABLE | FLSET10 | FLSET20 | FLSET40 | FLSET80;
	
		P2OUT = 0x00;
		P2DIR = CA_OUT | FLTRIM_PWM | SAMPLE_PWM | VINLIM_PWM;
		// TEMP: CA_OUT should actually be used as compare output for overcurrent shutdown
		P2SEL = IOUTSENSE_CA | /*CA_OUT |*/ IOUTSP_CA | FLTRIM_PWM | SAMPLE_PWM | VINLIM_PWM;
		*(unsigned int *)(0x041) = IOUTSENSE_CA | IOUTSP_CA | FLTRIM_PWM | SAMPLE_PWM;
		
		P3OUT = CAN_nCS | INBREAKER | OUTBREAKER;
		P3REN = INBREAKER | OUTBREAKER;
		P3DIR = CAN_nCS | CAN_MOSI | CAN_SCLK | HISIDE_EN | RELAY;		
		
		P4OUT = 0x00; // Have TMPCMPEN low (i.e. active) by default
		P4DIR = DCONTROL | TMPCMPEN | FLSET1 | FLSET2 | FLSET4 | FLSET8 | FLSET100 | FLSET200;
		
		P5OUT = FLASH_nCS | FLASH_HOLD | FLASH_WRP;
		P5DIR = FLASH_nCS | FLASH_SIMO | FLASH_SCLK | FLASH_WRP | FLASH_HOLD | P5_UNUSED;
		P5SEL = FLASH_SIMO | FLASH_SOMI | FLASH_SCLK;
		
		P6OUT = 0x00;
		P6DIR = P6_UNUSED;	

		hiSideSwitchingEnabled = 0;
	}


	//Functions that are only used for RACK
	void IO_toggleGreenLed()
	{
		P1OUT ^= SW_GREEN;
	}

	void IO_toggleOrangeLed()
	{
		P1OUT ^= SW_ORANGE;
	}

	void IO_toggleRedLed()
	{
		P1OUT ^= SW_RED;
	}

	void IO_setRedLed( unsigned int state )
	{
		if ( state )	P1OUT |= SW_RED;
		else			P1OUT &= ~(SW_RED);
	}

	

#endif
/**
 *  @brief not real input, return( (int)( CFG_localCfg.isSlave ) );
 */
//get from backplane for RACK, needs to get from config in WALL
int IO_getIsSlave()
{
#if (AER_PRODUCT_ID == AER05_RACK)
	return ( ( P2IN & nSLAVE ) == 0 );
#elif (AER_PRODUCT_ID == AER07_WALL)
	return( (int)( CFG_localCfg.isSlave ) );			//*****Change to get slave status from cfg
#endif
	//return 1;
}

/**
   @brief just on/off relay

mainMSP.c:mainMSPloop()->sch.c:SCH_runActiveTasks()->
					 FLAG_checkAndWrite()->FLAG_checkAllFlags->io.c:IO_setRelay

\todo 2 relay
 */

void IO_setRelay( unsigned long long bitfield )
{
	if ( bitfield != 0ull )	//ToDo fix
//		P3OUT |= RELAY;
		GPIOC->ODR |= GPIO_ODR_6;
	else					
//		P3OUT &= ~(RELAY);
		GPIOC->ODR &= ~GPIO_ODR_6;
}

//Only ever called from the control loop after a sample. Vmp will be set to normal in the control loop when this is called.
//void IO_enablePwmCtrl() move to PWM.c
//{
//	P2OUT |= ENABLE;
//	//CTRL_vmpNormal();	
//	IO_pwmEnabled = 1;
//}

//Disables all PWM outputs, low side and hi side.
//Sets vmp high to rail pwm controller to low duty cycle.
//void IO_disablePwmCtrl() move to PWM.c
//{
//	P3OUT |= nHISIDE_EN;
//	P2OUT &= ~(ENABLE);
//	CTRL_vmpHigh();		
//	IO_pwmEnabled = 0;
//}

/**
 *  @ingroup groupFAN
 *  @brief Program PWM for FAN
 */
void IO_fanDrvPWM()
{
	if(fanPWMCounter >= (IO_FAN_PWM_PERIOD - 1))
	{
		fanPWMCounter = 0;
	}
	else
	{
		fanPWMCounter++;
	}
	if(fanPWMCounter < fanDutyCompare)
	{
//		P1OUT |= FAN_DRV;
		GPIOD->ODR |= GPIO_ODR_2;
	} else {
//		P1OUT &= ~(FAN_DRV);
		GPIOD->ODR &= ~GPIO_ODR_2;
		//P1DIR |= FAN_DRV;		//Not sure why but to stop FAN turn on at startup ended up having to do it this way - revert the pin to an output here as it is set to an input at startup.
	}
}


/*!  @brief program counter of edges
 
     @ingroup groupFAN

     \todo make fanFBCount1 like fanFBCount2

FAN speed: curFanFBCount1,  curFanFBCount2

Speed analized in IO_fanSetSpeed()

This is called at the PWM frequency which is about every 512us as of revision 133.
Rated fan speed is 4300RPM which is about 6.5 ms between falling edges, or 13ms for a full rotation of the fan.
This corresponds to fanFBPeriod of about 0x0C
The safety cutoff for fanFBPeriod is set to FAN_PERIOD_MAXIMUM = 0x14
*/
void IO_fanSenseSpeed()
{
//	if (P1IFG & FAN_FB)
	if(GPIOC->IDR & GPIO_IDR_8){  //RDD ToDo: need be detected edge
		// fan feedback 1
		if (fanFBCount1 < IO_FAN_COUNT_TIMEOUT){
			fanFBCount1++;
		}
//		P1IFG &= ~FAN_FB;
	}
#ifndef ONE_FAN
//	if ((P5IN & FAN_FB2) == 0)
	if(GPIOC->IDR & GPIO_IDR_9){
		// fan feedback 1
		if (prevFan2State == 1)
		{
			if (fanFBCount2 < IO_FAN_COUNT_TIMEOUT)
			{
				fanFBCount2++;
			}
		}
		prevFan2State = 0;
	}
	else
	{
		prevFan2State = 1;
	}
#endif
}

/**
   @ingroup groupFAN
   @brief High level logic for FAN

//Run by scheduler every 2 seconds
 */
void IO_fanSetSpeed()
{
	int curFanFBCount1 = fanFBCount1;
#ifndef ONE_FAN
	int curFanFBCount2 = fanFBCount2;
#endif

	fanFBCount1 = 0;
#ifndef ONE_FAN
	fanFBCount2 = 0;
#endif

	//Check to see if the fan was switched on last time this function was called, if it is running at correct speed.
	//As this function is called every 2 seconds it should have had plenty of time to start up
	if (fanDriven){
#ifdef ONE_FAN
		if (curFanFBCount1 < IO_FAN_COUNT_TIMEOUT)
#else
		if (curFanFBCount1 < IO_FAN_COUNT_TIMEOUT || curFanFBCount2 < IO_FAN_COUNT_TIMEOUT)
#endif
		{
			// Stop the fan and try again in about 30s
			fanShutdown = 1;
			SAFETY_fanShutdown();
			IO_setFanDutyCycle(0);
			fanDriven = 0;
			fanRestartTimer = 0;
			return;
		}
	}

	if (fanShutdown)
	{
		fanRestartTimer++;
		if (fanRestartTimer >= 15)
		{
			fanShutdown = 0;
		}
	}
	else  //RDD variable for FAN   meas.caseTempr.valReal, meas.caseTemprFault, meas.outCurr.valReal, meas.caseTempr.val
	{
		if((meas.caseTempr.valReal > heatsinkTempFanHysThresh) || (meas.caseTemprFault == 1))
		{
			//only switch the fan on if the device is actually running (there is output current) unless its dangerously hot
			if((meas.outCurr.valReal >= IO_FAN_MINIMUM_CURRENT) || (meas.caseTempr.val > IQ_cnst(SAFETY_CASETMP_RESET) ))	
			{
				heatsinkTempFanHysThresh = IO_HEATSINK_TEMP_FAN_RESET;
				IO_setFanDutyCycle(10);
				fanDriven = 1;
			}
			else
			{
				IO_setFanDutyCycle(0);
				fanDriven = 0;
			}
		}
		else
		{
			heatsinkTempFanHysThresh = IO_HEATSINK_TEMP_FAN_THRESHOLD;
			IO_setFanDutyCycle(0);
			fanDriven = 0;
		}
	}
	//COMMS_sendDebugPacket(0,0,0,TicksUntilHiSideAllowed);
}

//Called at PWM frequency
//This is the only function that can enable hiside switching.
//Will enable hiside switching iff instantaneous current has been > 10A for 1 second.
//void IO_controlSyncRect()
//{
//	if(IO_pwmEnabled)
//	{
//		if(meas.outCurr.valPreFilter > (IQ_cnst(IO_SYNCRECT_UPPER_THRESHOLD)))
//		{
//			if(TicksUntilHiSideAllowed == IO_SYNCRECT_TIME_HYST_TICKS)
//			{
//				P3OUT &= ~(nHISIDE_EN);						//enable hiside
//				TicksUntilHiSideAllowed++;					//saturate counter
//			} 
//			else if (TicksUntilHiSideAllowed < IO_SYNCRECT_TIME_HYST_TICKS)
//			{
//				TicksUntilHiSideAllowed++;
//			}
//		}
//		else
//		{
//			//a low current value was detected
//			TicksUntilHiSideAllowed = 0;
//			P3OUT |= nHISIDE_EN;	//disable high side
//		}
//	}
//	else
//	{
//			//pwm disabled entirely - always reset the counter and disable the hiside
//			TicksUntilHiSideAllowed = 0;
//			P3OUT |= nHISIDE_EN;	//disable high side
//	}
//}

	

/*void IO_flsetAllOn( void )
{
	P1OUT |= FLSET10 | FLSET20 | FLSET40 | FLSET80;
	P4OUT |= FLSET1 | FLSET2 | FLSET4 | FLSET8 | FLSET100 | FLSET200;
}

void IO_flsetAllOff( void )
{
	P1OUT &= ~( FLSET10 | FLSET20 | FLSET40 | FLSET80 );
	P4OUT &= ~( FLSET1 | FLSET2 | FLSET4 | FLSET8 | FLSET100 | FLSET200 );
}*/

//void IO_flset( unsigned int fls )
//{
//	if ( fls & (1<<0) ) P4OUT |= FLSET1;
//	else P4OUT &= ~FLSET1;

//	if ( fls & (1<<1) ) P4OUT |= FLSET2;
//	else P4OUT &= ~FLSET2;

//	if ( fls & (1<<2) ) P4OUT |= FLSET4;
//	else P4OUT &= ~FLSET4;

//	if ( fls & (1<<3) ) P4OUT |= FLSET8;
//	else P4OUT &= ~FLSET8;

//	if ( fls & (1<<4) ) P1OUT |= FLSET10;
//	else P1OUT &= ~FLSET10;

//	if ( fls & (1<<5) ) P1OUT |= FLSET20;
//	else P1OUT &= ~FLSET20;

//	if ( fls & (1<<6) ) P1OUT |= FLSET40;
//	else P1OUT &= ~FLSET40;

//	if ( fls & (1<<7) ) P1OUT |= FLSET80;
//	else P1OUT &= ~FLSET80;

//	if ( fls & (1<<8) ) P4OUT |= FLSET100;
//	else P4OUT &= ~FLSET100;

//	if ( fls & (1<<9) ) P4OUT |= FLSET200;
//	else P4OUT &= ~FLSET200;
//}

//interrupt(PORT1_VECTOR) port1_isr(void)
//{
// can enable interrupts to allow other interupts to preempt
//	if (P1IFG & 0x08)
//	{
//		handle temp interrupt
//	}
//}
