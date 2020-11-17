
//-------------------------------------------------------------------
// File: time.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef TIME_H
#define TIME_H

#include "debug.h"

// Used to represent absolute times since 1970, and large time intervals
typedef unsigned long long Time;

// Used to represent intervals of time less than approx 49.71 days (in ms)
typedef unsigned long TimeShort;

void TIME_init(void);

void TIME_tick(void);

void time_updateTimeFromLCD(void);
void TIME_recFromBc( Time tm );
void TIME_set( Time tm );
int TIME_isSet(void);
Time TIME_get(void);
Time TIME_getToday(void);
Time TIME_getSinceToday(void);
int TIME_isToday( Time tm );

#endif // TIME_H
