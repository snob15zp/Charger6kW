
//-------------------------------------------------------------------
// File: safety.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-09-30: original
//-------------------------------------------------------------------

///#include <msp430x24x.h>
#include "mainMSP.h"
#include "safety.h"
#include "meas.h"
#include "io.h"
///#include "pwm.h"
#include "cfg.h"
///#include "flag.h"
///#include "comms.h"
///#include <signal.h>

#define PVCURR_NEG_MIN_TICKS 1 /// RDD OLD hardware (long)(3 * 1000000ull / PWM_PERIOD_US ) // (int) ( 1000000ull / PWM_PERIOD_US ) // 3 Seconds
#define SAFETY_DELAYED_RETRY_TICKS 1 /// RDD OLD hardware (long)( 900 * 1000000ull / PWM_PERIOD_US ) //15 mins (900) //30 mins (1800) //5 mins (300)
#define OUTVOLT_OC_DELAY_TICKS 1 /// RDD OLD hardware (long)( 5 * 1000000ull / PWM_PERIOD_US ) // 5 Seconds
#define OUTCURR_DELAY_TICKS 1 /// RDD OLD hardware (long)( 10 * 1000000ull / PWM_PERIOD_US ) // 10 Seconds
#define OUTVOLT_RST_DELAY_TICKS 1 /// RDD OLD hardware (long)( 30 * 1000000ull / PWM_PERIOD_US ) // 30 Seconds
#define PVVOLTLOW_SRTUP_DELAY_TICKS 1 /// RDD OLD hardware (long)( 5 * 1000000ull / PWM_PERIOD_US ) // 5 Seconds
#define PVVOLTLOW_RST_DELAY_TICKS 1 /// RDD OLD hardware (long)( 1800 * 1000000ull / PWM_PERIOD_US ) // 30 Minutes

#define SAFETY_IO_VOLTAGE_THRESHOLD		2 // Required PV Volts above the Battery Voltage
#define SAFETY_OUTCURRCRIT_LIMIT		90 // Immediate Output Overcurrent Shutdown - (Should help protect against a system short)

#define SAFETY_PVVOLT_LIMIT_HV			290.0 // PV Voltage Maximum Shutdown
#define SAFETY_OUTCURR_LIMIT_HV			65 // // Allow for 1.4x PV Overclock
#define SAFETY_OUTVOLT_OVER_LIMIT_HV	3 // Volts over the Absorb Voltage Overvoltage Shutdown
#define SAFETY_PULSE_CURRENT_LIMIT_HV	IQ_cnst(65.0 / MEAS_PVCURR_BASE) // PV Current Pulse Limit

#define SAFETY_PULSE_CURRENT_LIMIT_MV	IQ_cnst(85.0 / MEAS_PVCURR_BASE)
#define SAFETY_PVVOLT_LIMIT_MV			180.0
#define SAFETY_OUTCURR_LIMIT_MV			85 // Allow for 1.4x PV Overclock
#define SAFETY_OUTVOLT_OVER_LIMIT_MV	3 


Iq SAFETY_PVVoltLimit = 0;
Iq SAFETY_OutCurrLimit = 0;
Iq SAFETY_OutCurrCrit = 0;
Iq SAFETY_PulseCurrLimit = 0;
Iq SAFETY_OutVoltLimit = 0;
Iq SAFETY_OutVoltRstHigh = 0;
Iq SAFETY_OutVoltRstLow = 0;


typedef struct Safety_
{
	unsigned int sdBits;
	unsigned int pvCurrNegTickCnt;
	unsigned long delayedRetryTickCnt;
	unsigned long outVoltOCDelayTickCnt;
	unsigned long outVoltRstDelayTickCnt;
	unsigned long outCurrDelayTickCnt;
	unsigned long PVVoltLowSrtupDelayTickCnt;
	unsigned long PVVoltLowRstDelayTickCnt;
	unsigned long lowPVShutdownInt;
} Safety;

Safety safety;

unsigned char samplePending = 0;

void SAFETY_init()
{
	safety.pvCurrNegTickCnt = 0;
	safety.delayedRetryTickCnt = 0;
	safety.outVoltOCDelayTickCnt = 0;
	safety.outVoltRstDelayTickCnt = 0;
	safety.outCurrDelayTickCnt = 0;
	safety.PVVoltLowSrtupDelayTickCnt = 0;
	safety.PVVoltLowRstDelayTickCnt = 0;
	safety.lowPVShutdownInt = 0;
	VAR_SAFETY_setLimits();
}

void SAFETY_setLimitsMV()	//needs to happen after MEAS_setOutVoltBase()
{
	SAFETY_PVVoltLimit = IQ_cnst( SAFETY_PVVOLT_LIMIT_MV / MEAS_PVVOLT_BASE );
	SAFETY_OutCurrLimit = IQ_cnst( SAFETY_OUTCURR_LIMIT_MV / MEAS_OUTCURR_BASE ); 
	SAFETY_OutCurrCrit = IQ_cnst ( SAFETY_OUTCURRCRIT_LIMIT / MEAS_OUTCURR_BASE );
	SAFETY_OutVoltLimit = IQ_cnst( (SAFETY_OUTVOLT_OVER_LIMIT_MV + CFG_remoteCfg.bulkVolt) / MEAS_OUTVOLT_BASE );
	SAFETY_OutVoltRstHigh = IQ_cnst( CFG_remoteCfg.floatVolt / MEAS_OUTVOLT_BASE );
	SAFETY_OutVoltRstLow = IQ_cnst( ( CFG_remoteCfg.bulkResetVolt * 0.9 ) / MEAS_OUTVOLT_BASE );
	SAFETY_PulseCurrLimit = SAFETY_PULSE_CURRENT_LIMIT_MV;	
}

void SAFETY_setLimitsHV()	//needs to happen after MEAS_setOutVoltBase()
{
	SAFETY_PVVoltLimit = IQ_cnst( SAFETY_PVVOLT_LIMIT_HV / MEAS_PVVOLT_BASE );
	SAFETY_OutCurrLimit = IQ_cnst( SAFETY_OUTCURR_LIMIT_HV / MEAS_OUTCURR_BASE );
	SAFETY_OutCurrCrit = IQ_cnst ( SAFETY_OUTCURRCRIT_LIMIT / MEAS_OUTCURR_BASE );	
	SAFETY_OutVoltLimit = IQ_cnst( ( SAFETY_OUTVOLT_OVER_LIMIT_HV + CFG_remoteCfg.bulkVolt ) / MEAS_OUTVOLT_BASE );
	SAFETY_OutVoltRstHigh = IQ_cnst( CFG_remoteCfg.floatVolt / MEAS_OUTVOLT_BASE );
	SAFETY_OutVoltRstLow = IQ_cnst( ( CFG_remoteCfg.bulkResetVolt * 0.9 ) / MEAS_OUTVOLT_BASE );
	SAFETY_PulseCurrLimit = SAFETY_PULSE_CURRENT_LIMIT_HV;
}

void SAFETY_tick()
{

	if ( meas.pvCurr.valPreFilter < IQ_cnst(-2.0/MEAS_PVCURR_BASE) ) // PV Negative Current Shutdown
	{
		safety.pvCurrNegTickCnt++;
		if ( safety.pvCurrNegTickCnt >= PVCURR_NEG_MIN_TICKS )
		{
			safety.sdBits |= ( 1 << SAFETY_SD_BIT_PVCURR_NEG );
		}
	}
	else
	{
		safety.pvCurrNegTickCnt = 0;
	}
	
	if ( ( IQ_abs(meas.pvCurr.valPreFilter) > SAFETY_PulseCurrLimit ) ) // PV Pulse Current Shutdown
	{
		safety.sdBits |= ( 1 << SAFETY_SD_BIT_PVCURR_POS );
	}

	if ( meas.pvVolt.val > SAFETY_PVVoltLimit ) // PV Voltage Shutdown
	{
		safety.sdBits |= ( 1 << SAFETY_SD_BIT_PVVOLT );
	}

	if ( meas.outCurr.val > SAFETY_OutCurrCrit ) // Immediate Output Overcurrent Shutdown
	{
		safety.sdBits |= ( 1 << SAFETY_SD_BIT_OUTCURR_POS );
	}

	if ( meas.outCurr.val > SAFETY_OutCurrLimit ) // Backup Delayed Overcurrent Shutdown (for Current Limiting)
	{
		if (safety.outCurrDelayTickCnt >= OUTCURR_DELAY_TICKS )
		{
		safety.sdBits |= ( 1 << SAFETY_SD_BIT_OUTCURR_POS );
		}
		else
		{
			safety.outCurrDelayTickCnt++;
		}
	}
	else
	{
		safety.outCurrDelayTickCnt = 0;
	}
	
	if ( meas.outVolt.val > SAFETY_OutVoltLimit ) // Output Overvoltage Shutdown
	{
		if ( safety.outVoltOCDelayTickCnt >= OUTVOLT_OC_DELAY_TICKS )
		{
			safety.sdBits |= ( 1 << SAFETY_SD_BIT_OUTVOLT );
		}
		else
		{
			safety.outVoltOCDelayTickCnt++; // Delay Out overvolt check for MPPT Track.
		}
	}
	else
	{
		safety.outVoltOCDelayTickCnt = 0;
	}
	
	if ( meas.pvVolt.valReal <= (meas.outVolt.valReal + SAFETY_IO_VOLTAGE_THRESHOLD)) // Protects against PV Breaker Disconnect/Reconnect Hard Starts & Reverse PV & Output being enabled before PV is connected.
	{
		safety.sdBits |= ( 1 << SAFETY_SD_BIT_PANEL_MISSING );
	}

	if ( meas.caseTempr.val > IQ_cnst(SAFETY_CASETMP_LIMIT) )
	{
		safety.sdBits |= ( 1 << SAFETY_SD_BIT_CASETMP );
	}
	else if ( meas.caseTempr.val < IQ_cnst(SAFETY_CASETMP_RESET) )
	{
		if(safety.sdBits & ( 1 << SAFETY_SD_BIT_CASETMP ) )
		{
			safety.sdBits &= ~( 1 << SAFETY_SD_BIT_CASETMP );
		}
	}

	// If any shutdown bits, go into shutdown mode
	// WORKAROUND: fan fault
	//if ( safety.sdBits != 0 )
	if ( (safety.sdBits & (~(1 << SAFETY_SD_BIT_FAN))) != 0 )
	{
		/// RDDtemp IO_disablePwmCtrl();
	}
}

void SAFETY_monitor()
{

	// Delayed retry for PV_OVERVOLT
	// The counter will be reset anytime there is a shutdown event.
	// Counter will remain at zero if the PV_OVERVOLT condition persists.
	
if (safety.sdBits & ( 1 << SAFETY_SD_BIT_PVVOLT ) || safety.sdBits & ( 1 << SAFETY_SD_BIT_OUTCURR_POS ))  	
	{
		if ( meas.pvVolt.val < SAFETY_PVVoltLimit ) // Check if PV Voltage is Safe
		{
			if (safety.delayedRetryTickCnt == SAFETY_DELAYED_RETRY_TICKS)
			{
				safety.sdBits &= ~( 1 << SAFETY_SD_BIT_PVVOLT ); // Clear the bits after the retry period has elapsed.
				safety.sdBits &= ~( 1 << SAFETY_SD_BIT_OUTCURR_POS ); 
				safety.delayedRetryTickCnt = 0;
			}
			else
			{
				safety.delayedRetryTickCnt++;
			}
		}
		else
		{
			safety.delayedRetryTickCnt = 0;
		}
	}

if (safety.sdBits & ( 1 << SAFETY_SD_BIT_OUTVOLT ))
	{
		if ( ( meas.outVolt.val >= SAFETY_OutVoltRstLow ) && ( meas.outVolt.val <= SAFETY_OutVoltRstHigh ) ) // Check for a normal battery voltage reading before re-activating the output.
		{

			if ( safety.outVoltRstDelayTickCnt >= OUTVOLT_RST_DELAY_TICKS )
			{
				safety.sdBits &= ~( 1 << SAFETY_SD_BIT_OUTVOLT );
				safety.outVoltRstDelayTickCnt = 0;
			}
			else
			{
				safety.outVoltRstDelayTickCnt++;
			}
		}
		else
		{
			safety.outVoltRstDelayTickCnt = 0;
		}
	}
	
if (safety.sdBits & ( 1 << SAFETY_SD_BIT_PANEL_MISSING ))
	{
		
		if ( meas.pvVolt.valReal >= (meas.outVolt.valReal + SAFETY_IO_VOLTAGE_THRESHOLD) )
		{
			if ( safety.lowPVShutdownInt == 0 ) // If this is the first time this has triggered since startup, clears alarm in 5 seconds. 
			{
				if ( safety.PVVoltLowSrtupDelayTickCnt >= PVVOLTLOW_SRTUP_DELAY_TICKS )
				{
					safety.sdBits &= ~( 1 << SAFETY_SD_BIT_PANEL_MISSING );
					safety.PVVoltLowSrtupDelayTickCnt = 0;
					
					safety.lowPVShutdownInt = 1;
				}
				else
				{
					safety.PVVoltLowSrtupDelayTickCnt++;
				}
			}
			else // If not startup sequence/first time turning on unit/pv, delay alarm clear for 20 minutes to prevent end of day PV voltage bouncing during sunset.
			{
				if ( safety.PVVoltLowRstDelayTickCnt >= PVVOLTLOW_RST_DELAY_TICKS ) 
				{
					safety.sdBits &= ~( 1 << SAFETY_SD_BIT_PANEL_MISSING );
					safety.PVVoltLowRstDelayTickCnt = 0;
				}
				else
				{
					safety.PVVoltLowRstDelayTickCnt++;
				}
			}
		}
		else
		{
			safety.PVVoltLowSrtupDelayTickCnt = 0;
			safety.PVVoltLowRstDelayTickCnt = 0;
		}						
	
	}
}

void SAFETY_fanShutdown()  // RDD If the fan speed was insufficient
{
	safety.sdBits |= ( 1 << SAFETY_SD_BIT_FAN );
}

unsigned char SAFETY_getStatus() //RDD If the fan speed was insufficient
{
	#warning WORKAROUND: fan fault
	// return (unsigned char)( safety.sdBits != 0 );
	return (unsigned char)( (safety.sdBits & (~(1 << SAFETY_SD_BIT_FAN))) != 0 );
}

//int SAFETY_isShutdown()
//{
//	return ( safety.sdBits != 0 );
//}

int SAFETY_isShutdown( int reason )
{
	return ( safety.sdBits & ( 1 << reason ) );
}
