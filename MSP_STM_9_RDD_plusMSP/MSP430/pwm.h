
//-------------------------------------------------------------------
// File: pwm.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef PWM_H
#define PWM_H

#include "debug.h"
#include "iqmath.h"

// Tick timing
#define CLOCK_RATE_HZ			16000000
#define PWM_BITS				13			// MAX_PWM will be set to 2^PWM_BITS, and hence PWM_RATE_HZ = CLOCK_RATE_HZ / 2^PWM_BITS
#define PWM_RATE_HZ				( (float)CLOCK_RATE_HZ / ( 1 << PWM_BITS ) )
#define MAX_PWM					( 1 << PWM_BITS ) //RDD 8192
#define PWM_PERIOD_US			( 1000000ull * MAX_PWM / CLOCK_RATE_HZ )		// Careful if this isn't an integer, clock will drift  //RDD 512
#define CONT_TIMER_LOAD_VAL		( 0xFFFF - MAX_PWM )

#define MIN_DUTY_CYCLE			( 0.01 )	// to allow for timerA processing i.e. software shadowing, etc. at start of period

void PWM_init(void);
extern void PWM_isr(void);

void PWM_setMpptSamplePt( Iq val );
void PWM_setVinLim( Iq val );
void PWM_setFlTrim( Iq val );

#endif // PWM_H

