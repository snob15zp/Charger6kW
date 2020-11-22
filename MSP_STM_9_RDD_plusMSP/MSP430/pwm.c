
//-------------------------------------------------------------------
// File: pwm.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

//#include <msp430x24x.h>
#include <signal.h>
#include "iqmath.h"
#include "pwm.h"
#include "sch.h"
#include "io.h"
//#include "adc.h"
#include "meas.h"
#include "ctrl.h"
#include "cfg.h"
#include "flash.h"
#include "safety.h"
#include "comms.h"
#include "variant.h"
#include "temp.h"
#include "dcdc.h"
#include "HiResTim.h"


//#if ( IQ_Q > PWM_BITS )
//	#define IQ_TO_PWM( pwm ) ( pwm >> ( IQ_Q - PWM_BITS ) )
//#elif ( IQ_Q == PWM_BITS )
//	#define IQ_TO_PWM( pwm ) ( pwm )
//#else
//	#define IQ_TO_PWM( pwm ) ( pwm << ( PWM_BITS - IQ_Q ) )
//#endif

#define IQ_TO_PWM( pwm ) (((float)(pwm)) *((float) MEAS_PVVOLT_IQBASE )) 

typedef struct PwmCalib_
{
	Iq offset;
	Iq scale;
} PwmCalib;

PwmCalib mpptSamplePtCal;
PwmCalib vinLimCal;
PwmCalib flTrimCal;

float mpptSamplePtShadow;  //RDD Target Vin in Volts
unsigned int vinLimShadow;
unsigned int flTrimShadow;

#define PWM_MPPTSAMPLEPT_CCR	TACCR2
#define PWM_VINLIM_CCR			TACCR0
#define PWM_FLTRIM_CCR			TACCR1

#define PWM_MPPTSAMPLEPT_CCTL	TACCTL2
#define PWM_VINLIM_CCTL			TACCTL0
#define PWM_FLTRIM_CCTL			TACCTL1

void PWM_init( void )
{
	// Init calibration
	mpptSamplePtCal.offset = CFG_localCfg.mpptSamplePtDacCal.offset;
	mpptSamplePtCal.scale = CFG_localCfg.mpptSamplePtDacCal.scale;
	vinLimCal.offset = CFG_localCfg.vinLimDacCal.offset;
	vinLimCal.scale = CFG_localCfg.vinLimDacCal.scale;
	flTrimCal.offset = CFG_localCfg.flTrimDacCal.offset;
	flTrimCal.scale = CFG_localCfg.flTrimDacCal.scale;

	// Init timer A
//	TACTL = TASSEL_2 | ID_0 | TACLR | TAIE;			// SMCLK/1, clear TAR, enable overflow interrupt

//	TAR = CONT_TIMER_LOAD_VAL;					// Load so will overflow after period elapsed
//	
//	PWM_MPPTSAMPLEPT_CCTL = OUTMOD_5 | OUT;
//	PWM_VINLIM_CCTL = OUTMOD_5 | OUT;
//	PWM_FLTRIM_CCTL = OUTMOD_5 | OUT;

//	PWM_setMpptSamplePt( MIN_DUTY_CYCLE );
//	PWM_setVinLim( MIN_DUTY_CYCLE );
//	PWM_setFlTrim( MIN_DUTY_CYCLE );
//	
//	TACTL |= MC_2;								// Set timer to continuous count mode
}

void PWM_setMpptSamplePt( Iq val )
{
	Iq tmp=val;// = IQ_mpy( mpptSamplePtCal.scale, val + mpptSamplePtCal.offset );
	if ( tmp >= IQ_cnst(1.0) ) tmp = IQ_cnst(1.0) - 1;
	if ( tmp < IQ_cnst(MIN_DUTY_CYCLE) ) tmp = IQ_cnst(MIN_DUTY_CYCLE);
	mpptSamplePtShadow = IQ_TO_PWM( (unsigned int)tmp );
	setVin(mpptSamplePtShadow);
}
void IO_enablePwmCtrl() //from io.c
{
	DCDC_Start_Stop(1);///RDD statusFlags.CONTROL_START=1;
//	P2OUT |= ENABLE;
//	//CTRL_vmpNormal();	
//	IO_pwmEnabled = 1;
}

//Disables all PWM outputs, low side and hi side.
//Sets vmp high to rail pwm controller to low duty cycle.
void IO_disablePwmCtrl() //from io.c
{
    DCDC_Start_Stop(0);///RDD	  statusFlags.CONTROL_STOP=1;
//	P3OUT |= nHISIDE_EN;
//	P2OUT &= ~(ENABLE);
//	CTRL_vmpHigh();		
//	IO_pwmEnabled = 0;
}

void PWM_setVinLim( Iq val )
{
//	Iq tmp = IQ_mpy( vinLimCal.scale, val + vinLimCal.offset );
//	if ( tmp >= IQ_cnst(1.0) ) tmp = IQ_cnst(1.0) - 1;
//	if ( tmp < IQ_cnst(MIN_DUTY_CYCLE) ) tmp = IQ_cnst(MIN_DUTY_CYCLE);
//	vinLimShadow = CONT_TIMER_LOAD_VAL + IQ_TO_PWM( (unsigned int)tmp );
}

void PWM_setFlTrim( Iq val )
{
	
//	Iq tmp = IQ_mpy( flTrimCal.scale, val + flTrimCal.offset );
//	if ( tmp >= IQ_cnst(1.0) ) tmp = IQ_cnst(1.0) - 1;
//	if ( tmp < IQ_cnst(MIN_DUTY_CYCLE) ) tmp = IQ_cnst(MIN_DUTY_CYCLE);
	//COMMS_sendDebugPacket( (int)tmp, 0, 0, 0 );
//	flTrimShadow = CONT_TIMER_LOAD_VAL + IQ_TO_PWM( (unsigned int)tmp );
}

void PWM_isr(void)
{
//	if ( TAIV == TAIV_OVERFLOW )
//	{
//		eint();
//		//P5OUT |= CONTROL_2 | CONTROL_1;

//#ifdef ENABLE_PWM_TEST
//		if (P2OUT & IOUTSP_CA)
//		{
//			P2OUT &= ~IOUTSP_CA;
//		}
//		else
//		{
//			P2OUT |= IOUTSP_CA;
//		}
//#endif

//		// Set all PWM outputs
//		/*if ( PWM_MPPTSAMPLEPT_CCR > CONT_TIMER_LOAD_VAL )*/ PWM_MPPTSAMPLEPT_CCTL = OUTMOD_0 | OUT;
//		/*if ( PWM_VINLIM_CCR > CONT_TIMER_LOAD_VAL )*/ PWM_VINLIM_CCTL = OUTMOD_0 | OUT;
//		/*if ( PWM_FLTRIM_CCR > CONT_TIMER_LOAD_VAL )*/ PWM_FLTRIM_CCTL = OUTMOD_0 | OUT;

//		// Perform software shadowing of compare registers
///      Regulator(mpptSamplePtShadow); //RDD
		//	LimutDuty(mpptSamplePtShadow);
//		PWM_VINLIM_CCR = vinLimShadow;
//		PWM_FLTRIM_CCR = flTrimShadow;

//		// Set PWM outputs to reset on compare match
//		PWM_MPPTSAMPLEPT_CCTL = OUTMOD_5 | OUT;
//		PWM_VINLIM_CCTL = OUTMOD_5 | OUT;
//		PWM_FLTRIM_CCTL = OUTMOD_5 | OUT;

//		// Reload timer to near overflow
//		TAR += CONT_TIMER_LOAD_VAL;

//		// Process ADC
//		MEAS_update();
//		ADC_STARTIFDONE;

//		// Tick other modules
//		SAFETY_tick();
//		SAFETY_monitor();
		CTRL_tick();
//		TIME_tick();
//		FLASH_tick();
		IO_fanSenseSpeed();
//		IO_controlSyncRect();
//#ifdef DIGITAL_TEMP
		TEMP_tick();
//#endif
//		//FLAG_tick();
//	}
}

/*
Set up for using timer B in up/down mode -- could be good for digital control

// Initialise timer B for PWM generation
void PWM_init( void )
{
	TBCTL = TBSSEL_2 | ID_0 | TBCLR;			// MCLK/1, clear TBR
	TBCCR0 = MAX_PWM;							// 
	//TBCCTL0 = CCIE;
	TBCCR1 = MAX_PWM / 2;						// Set compare 1 period to 50% duty
	TBCCTL1 = OUTMOD_7;							// Reset on CCR1 match, set (trigger ADC) on CCR0 match								
	TBCTL |= MC_3;								// Set timer to 'up/down' count mode
}
*/


