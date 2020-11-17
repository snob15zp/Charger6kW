
//-------------------------------------------------------------------
// File: debug.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef DEBUG_H
#define DEBUG_H

#define NOP __asm__ __volatile__ ( "nop" )

// When compiling for use on BMS master hardware, during development when real control board not available
//#define DBG_USING_BMS_HARDWARE

//#define DBG_FULL_ON

// Options
// =======
//#define DBG_TRACK_SERVER_TIME
#define DBG_USE_INTERNAL_CLOCK

//#define DBG_HARDCODED_VIN_SETPOINT	(55.0)

#endif // DEBUG_H

