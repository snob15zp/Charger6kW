
//-------------------------------------------------------------------
// File: time.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#include "time.h"
#include "telem.h"
#include "stats.h"
#include "flag.h"
#include "pwm.h"
#include "sch.h"
#include "stats.h"
#include "meas.h"

#define MS_PER_DAY ( 24ull * 3600ull * 1000ull )	// 1 day
//#define MS_PER_DAY ( 10ull * 1000ull )			// 10 secs
//#define MS_PER_DAY ( 3600ull * 1000ull )			// 1 hour
//#define MS_PER_DAY ( 600ull * 1000ull )				// 10 min

// Integral control constant for time tracking
//#define TIME_K_TRACK 1

#define PWM_PERIOD_MS_MAX	( PWM_PERIOD_US + (PWM_PERIOD_US>>2) )
#define PWM_PERIOD_MS_MIN	( PWM_PERIOD_US - (PWM_PERIOD_US>>2) )

// Time in milliseconds since 1970
volatile Time currentTime_ms;
volatile unsigned int microSecs;
int pwmPeriodUs;
Time today_ms;
int timeIsValid;

void TIME_init()
{
	timeIsValid = 0;
	microSecs = 0;
	currentTime_ms = 0;
	today_ms = 0;
	pwmPeriodUs = 1; /// RDD OLD hardware PWM_PERIOD_US;
}

void TIME_tick()
{
	microSecs += pwmPeriodUs;
	while ( microSecs >= 1000 )
	{
		microSecs -= 1000;

		currentTime_ms++;
		if ( currentTime_ms - today_ms >= MS_PER_DAY )
		{
			today_ms += MS_PER_DAY;
			if ( timeIsValid )
			{
				//STATS_queueNextEntry();
//				MEAS_resetCharge();
			}
		}

//		SCH_incrMs();
	}
}

//void time_updateTimeFromLCD(){}

void TIME_recFromBc( Time tm )
{
	if ( !timeIsValid )
	{
		TIME_set( tm );
	}
#ifdef DBG_TRACK_SERVER_TIME
	else
	{
		pwmPeriodUs += (int)( ( (long long int)tm - (long long int)currentTime_ms ) >> 4 ); // gain of 1/16
		if ( pwmPeriodUs < PWM_PERIOD_MS_MIN ) pwmPeriodUs = PWM_PERIOD_MS_MIN;
		if ( pwmPeriodUs > PWM_PERIOD_MS_MAX ) pwmPeriodUs = PWM_PERIOD_MS_MAX;
	}
#endif
}

void TIME_set( Time tm )
{
	currentTime_ms = tm;
	today_ms = ( currentTime_ms / MS_PER_DAY ) * MS_PER_DAY;

	timeIsValid = 1;

	// inform other modules that the time has been updated
	//TELEM_timeHasUpdated();
	//STATS_timeHasUpdated();
	FLAG_timeHasUpdated();
}

int TIME_isSet()
{
	return timeIsValid;
}

Time TIME_get()
{
	return currentTime_ms;
}

Time TIME_getToday()
{
	return today_ms;
}

Time TIME_getSinceToday()
{
	return ( currentTime_ms - today_ms );
}

int TIME_isToday( Time tm )
{
	return ( ( tm - today_ms ) < MS_PER_DAY );
}	
