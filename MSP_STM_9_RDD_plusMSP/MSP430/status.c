
//-------------------------------------------------------------------
// File: status.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-08-18: original
//-------------------------------------------------------------------

#include "status.h"
#include "cfg.h"
#include "stats.h"
#include "flag.h"
#include "telem.h"
#include "comms.h"
#include "flash.h"
#include "ctrl.h"
#include "safety.h"

typedef unsigned char(*StatusFunc)();

// Array holding addresses of status functions for the various modules
// Each function should return some meaningful value in the lower 4 bytes of the returned byte only
#define STATUS_NUM_FIELD_BITS	4
#define STATUS_NUM_FIELDS		( 64 / STATUS_NUM_FIELD_BITS )
const StatusFunc statusFuncs[STATUS_NUM_FIELDS] = 
{
	&CFG_getStatus,
	&STATS_getStatus,
	&FLAG_getStatus,
	&TELEM_getStatus,
	&COMMS_getStatus,
	&FLASH_getStatus,
	&CTRL_getStatus,
	&SAFETY_getStatus,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

unsigned long long statusBitfield;

void STATUS_init()
{
	statusBitfield = 0;
}

void STATUS_update()
{
	unsigned int ii;
	for ( ii = 0; ii < STATUS_NUM_FIELDS; ii++ )
	{
		if ( statusFuncs[ii] != 0 )
		{
			statusBitfield &= ~( 0x0Full << (ii*STATUS_NUM_FIELD_BITS) );
			statusBitfield |= ( statusFuncs[ii]() & 0x0Full ) << (ii*STATUS_NUM_FIELD_BITS);
		}
	}
}

unsigned long long STATUS_getStatus()
{
	return statusBitfield;
}
