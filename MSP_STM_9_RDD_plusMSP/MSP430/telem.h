
//-------------------------------------------------------------------
// File: telem.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef TELEM_H
#define TELEM_H

#include "debug.h"
#include "time.h"

// Order of telemetry channels in e.g. channel enable bitfield, etc.
#define TELEM_OUTVOLT_BIT		0
#define TELEM_OUTCURR_BIT		1
#define TELEM_OUTCHARGE_BIT		2
#define TELEM_PVVOLT_BIT		3
#define TELEM_PVCURR_BIT		4
#define TELEM_PVPOWER_BIT		5
#define TELEM_PVOCVOLT_BIT		6

#define TELEM_OUTVOLT_MASK		( 1 << TELEM_OUTVOLT_BIT )
#define TELEM_OUTCURR_MASK		( 1 << TELEM_OUTCURR_BIT )
#define TELEM_OUTCHARGE_MASK	( 1 << TELEM_OUTCHARGE_BIT )
#define TELEM_PVVOLT_MASK		( 1 << TELEM_PVVOLT_BIT )
#define TELEM_PVCURR_MASK		( 1 << TELEM_PVCURR_BIT )
#define TELEM_PVPOWER_MASK		( 1 << TELEM_PVPOWER_BIT )
#define TELEM_PVOCVOLT_MASK		( 1 << TELEM_PVOCVOLT_BIT )

// Sizes of telemetry values
#define TELEM_OUTVOLT_SIZE		2
#define TELEM_OUTCURR_SIZE		2
#define TELEM_OUTCHARGE_SIZE	4
#define TELEM_PVVOLT_SIZE		2
#define TELEM_PVCURR_SIZE		2
#define TELEM_PVPOWER_SIZE		2
#define TELEM_PVOCVOLT_SIZE		2
#define TELEM_MAX_SIZE				(		TELEM_OUTVOLT_SIZE + TELEM_OUTCURR_SIZE + TELEM_OUTCHARGE_SIZE \
										+	TELEM_PVVOLT_SIZE + TELEM_PVCURR_SIZE + TELEM_PVPOWER_SIZE + TELEM_PVOCVOLT_SIZE )

#define TELEM_NUM_CHANNELS		( TELEM_PVOCVOLT_BIT + 1 )

#define TELEM_BASE_PERIOD_MS 200

typedef enum TELEM_SamplePeriod_
{
	TELEM_PERIOD_START = 0,
	TELEM_PERIOD_200MS = 0,
	TELEM_PERIOD_1S,
	TELEM_PERIOD_10S,
	TELEM_PERIOD_1MIN,
	TELEM_PERIOD_10MIN,
	TELEM_PERIOD_1H,
	TELEM_PERIOD_END
} TELEM_SamplePeriod;
#define TELEM_PERIOD_NUM ( TELEM_PERIOD_END - TELEM_PERIOD_START )

void TELEM_init(void);
void TELEM_timeHasUpdated(void);

int TELEM_switchToNextSector(void);

unsigned char TELEM_getStatus(void);
int TELEM_isFull(void);

void TELEM_eraseStarted(void);
void TELEM_eraseDoneCallback( int retVal );

//unsigned long TELEM_getPastBlockAddress( unsigned int numBlocksInPast );
unsigned int TELEM_getMostRecentBlock(void);
unsigned int TELEM_getNumBlocksByTime( Time minTm );
unsigned long TELEM_getTelemLen( unsigned long blockAddr );
unsigned int TELEM_getNumBlocks(void);

int TELEM_getIsReady(void);

void TELEM_logIfPeriodElapsed(void);

#endif // TELEM_H

