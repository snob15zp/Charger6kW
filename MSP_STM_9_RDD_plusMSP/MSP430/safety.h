
//-------------------------------------------------------------------
// File: safety.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-09-30: original
//-------------------------------------------------------------------

#ifndef SAFETY_H
#define SAFETY_H

#include "flag.h"
#include "meas.h"

// Shutdown bitfield
#define SAFETY_SD_BIT_PVCURR_NEG	( FLAG_CODE_PVCURR_NEG_SD	- FLAG_CODE_FIRST_SD )
#define SAFETY_SD_BIT_PVCURR_POS	( FLAG_CODE_PVCURR_POS_SD	- FLAG_CODE_FIRST_SD )
#define SAFETY_SD_BIT_PVVOLT		( FLAG_CODE_PVVOLT_SD		- FLAG_CODE_FIRST_SD )
#define SAFETY_SD_BIT_OUTCURR_POS	( FLAG_CODE_OUTCURR_POS_SD	- FLAG_CODE_FIRST_SD )
#define SAFETY_SD_BIT_OUTVOLT		( FLAG_CODE_OUTVOLT_SD		- FLAG_CODE_FIRST_SD )
#define SAFETY_SD_BIT_CASETMP		( FLAG_CODE_CASETMP_SD		- FLAG_CODE_FIRST_SD )
#define SAFETY_SD_BIT_FAN			( FLAG_CODE_FAN_SD			- FLAG_CODE_FIRST_SD )
#define SAFETY_SD_BIT_PANEL_MISSING ( FLAG_CODE_PANEL_MISSING   - FLAG_CODE_FIRST_SD )

#define SAFETY_NUM_SD_REASONS		(SAFETY_SD_BIT_FAN + 1)

#if (AER_PRODUCT_ID == AER05_RACK)
	#define SAFETY_CASETMP_LIMIT		??
	#define SAFETY_CASETMP_RESET		??
#elif (AER_PRODUCT_ID == AER07_WALL)
	#define SAFETY_CASETMP_LIMIT		(120.0 / MEAS_CASETEMPR_BASE)
	#define SAFETY_CASETMP_RESET		(80.0 / MEAS_CASETEMPR_BASE)
#endif
 
void SAFETY_init(void);
void SAFETY_setLimitsMV(void);
void SAFETY_setLimitsHV(void);

void SAFETY_tick(void);
void SAFETY_monitor(void);

void SAFETY_fanShutdown(void);
unsigned char SAFETY_getStatus(void);
int SAFETY_isShutdown( int reason );

void SAFETY_toggleRedLed(void);

#endif // SAFETY_H
