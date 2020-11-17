
//-------------------------------------------------------------------
// File: stats.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef STATS_H
#define STATS_H

#include "debug.h"
#include "iqmath.h"

// Statistics
typedef enum StatId_
{
	StatId_MIN = 0,
	OUT_VOLT = StatId_MIN,
	OUT_CURR,
	PV_VOLT,
	PV_CURR,
	PV_POWER,
	PV_OC_VOLT,
	StatId_MAX = PV_OC_VOLT
} StatId;
enum { StatId_NUM = (StatId_MAX - StatId_MIN) + 1 };

void STATS_init(void);
void STATS_initStats(void);

void STATS_timeHasUpdated(void);

void STATS_eraseStarted(void);
void STATS_eraseDoneCallback( int retVal );

unsigned char STATS_getStatus(void);

void STATS_resetAddr(void);

void STATS_queueNextEntry(void);

void STATS_updateCharge( IqLong val );
void STATS_updateAll(void);
void STATS_updateStat( StatId, Iq val );

int STATS_saveToFlash(void);

unsigned long STATS_getStatsLenTotal(void);

unsigned char * STATS_getStatsMemAddr(void);

unsigned int STATS_getCurrentBlock(void);
unsigned long STATS_getStatsLen( unsigned long blockAddr );
unsigned int STATS_getNumBlocks(void);


#endif // STATS_H
