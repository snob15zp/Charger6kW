
//-------------------------------------------------------------------
// File: stats.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#include "stats.h"
#include "time.h"
#include "flash.h"
#include "meas.h"
///#include "comms.h"

#define STATS_MAX_ENTRIES	 366		// Higher?

typedef struct Stat_
{
	Iq min;
	Iq max;
	Iq avg;
	Iq24LongLong q;				// var_n = q_n / n
	TimeShort minTime;	// time in ms since midnight that max value occured at
	TimeShort maxTime;	// time in ms since midnight that min value occured at
	// Include variable for integrated value?
} Statistic;

typedef struct StatHelper_
{
	IqLong tempSum;
	long numTempUpdates;
} StatHelper;

// This is the structure that is actually written to flash
typedef struct Statistics
{
	Time lastWriteTime;
	long numUpdates;
	Statistic statistics[StatId_NUM];
	IqLong outCharge;
} Statistics;

typedef struct Stats_
{
	Statistics statistics;
	StatHelper statHelpers[StatId_NUM];
	unsigned long currentAddr;
	int isReadyToUpdate;
	int isReadyToWrite;
	int isErasing;
	int isWritePending;
	int isErasePending;
	int isFirst;
} Stats;

Stats stats;


int STATS_initEntryAddr();
void STATS_tryNextEntry();

void STATS_init()
{

	stats.isReadyToUpdate = 0;
	stats.isErasing = 0;
	stats.isWritePending = 0;
	stats.isErasePending = 0;

#if 0
	if ( STATS_initEntryAddr() == 0 )
	{
#endif
		stats.isReadyToWrite = 1;
#if 0 
	}

	STATS_initStats();

	stats.isFirst = 1;
#endif
}

#if 0 
void STATS_initStats()
{
	unsigned int ii;
	stats.statistics.lastWriteTime = 0;
	stats.statistics.numUpdates = 0;	
	for ( ii = 0; ii < StatId_NUM; ii++ )
	{
		stats.statistics.statistics[ii].min = IQ_MAX;
		stats.statistics.statistics[ii].max = IQ_MIN;
		stats.statistics.statistics[ii].avg = IQ_cnst(0.0);
		stats.statistics.statistics[ii].q = IQ_cnst(0.0);
		stats.statistics.statistics[ii].minTime = 0;
		stats.statistics.statistics[ii].maxTime = 0;
		stats.statHelpers[ii].tempSum = 0;
		stats.statHelpers[ii].numTempUpdates = 0;
	}
	stats.statistics.outCharge = IQ_cnst(0.0);
}


void STATS_timeHasUpdated()
{
	/*stats.dayIndex = STATS_getDayIndex();
	STATS_loadFromFlash();
	if ( !TIME_isToday( stats.statistics.lastWriteTime ) )
	{
		STATS_newDay();
	}*/

	stats.isReadyToUpdate = 1;
}

void STATS_eraseStarted()
{
	stats.isReadyToWrite = 0;
	stats.isErasing = 1;
}

void STATS_eraseDoneCallback( int retVal )
{
	stats.isErasing = 0;
	if ( retVal >= 0 )
	{
		stats.isReadyToWrite = 1;
	}
	else
	{
		stats.isReadyToWrite = 0;
	}
}

void STATS_resetAddr()
{
	stats.currentAddr = FLASH_STATS_ADDR;
}
#endif

unsigned char STATS_getStatus()
{
	return ( stats.isReadyToUpdate | (stats.isReadyToWrite<<1) | ((stats.isErasing|stats.isErasePending)<<2) | (stats.isWritePending<<3) );
}

#if 0 
void STATS_queueNextEntry()
{
	stats.isWritePending = 1;
}

void STATS_tryNextEntry()
{
	if ( STATS_saveToFlash() < 0 )
	{
		//COMMS_sendDebugPacket( 0, 0xFF );
		return;		
	}

	// We just wrote to flash, so increment the current address
	stats.currentAddr += sizeof( Statistics );

	stats.isWritePending = 0;

	STATS_initStats();
	
	// Now check if we have enough space left for another entry in this block
	if ( ( ( stats.currentAddr & ERASE_MASK1 ) + BLOCK_SIZE ) < ( stats.currentAddr + sizeof( Statistics ) ) )
	{
		// Time for a new block
		stats.currentAddr = ( stats.currentAddr & ERASE_MASK1 ) + BLOCK_SIZE;

		// Wrap if necessary
		if ( stats.currentAddr >= FLASH_STATS_ADDR + STATS_getStatsLenTotal() )
		{
			stats.currentAddr = FLASH_STATS_ADDR;
		}

		// Erase the new block
		stats.isErasePending = 1;
	}
	else
	{
		// We have enough space
	}
}

void STATS_updateCharge( IqLong val )
{
	stats.statistics.outCharge = val;
}
#endif


void STATS_updateAll()
{
	int retVal;
#if 0 
	if ( stats.isReadyToUpdate && !(stats.isWritePending) )
	{
		// Try to get ready to write if we've failed to do so up until now
		if ( !stats.isReadyToWrite && !stats.isErasing && !stats.isErasePending )
		{
			if ( STATS_initEntryAddr() == 0 )
			{
				stats.isReadyToWrite = 1;
			}
		}

		stats.statistics.lastWriteTime = TIME_get();

		// Update the stats provided the PV OC voltage is high enough (i.e. isn't night-time!)
		if ( MEAS_isPvActive() | ( stats.statistics.numUpdates == 0 ) )
		{
			stats.statistics.numUpdates++;

			STATS_updateStat( OUT_VOLT, meas.outVolt.val );
			STATS_updateStat( OUT_CURR, meas.outCurr.val );
			STATS_updateStat( PV_VOLT, meas.pvVolt.val );
			STATS_updateStat( PV_CURR, meas.pvCurr.val );
			STATS_updateStat( PV_POWER, meas.pvPower.val );
			STATS_updateStat( PV_OC_VOLT, meas.pvOcVolt.val );

			STATS_updateCharge( meas.outCharge.val );
		}

	}
	else if ( stats.isWritePending && !(stats.isErasePending) )
	{
		STATS_tryNextEntry();
	}
	else if ( stats.isErasePending )
	{
		STATS_eraseStarted(); 
		retVal = FLASH_erase( stats.currentAddr, BLOCK_SIZE, STATS_eraseDoneCallback );
		if ( retVal < 0 ) 
		{
			// Erase refused
			STATS_eraseDoneCallback(retVal);
		}
		else
		{
			// Erase accepted
			stats.isErasePending = 0;
		}
	}
#endif
}

#if 0
void STATS_updateStat( StatId id, Iq val )
{
	Iq oldAvg;
	IqLong diff;
	IqLong tempSumT;
	long numTempUpdatesT, numUpdatesT;

	// ATTN: TEMP
	oldAvg = stats.statistics.statistics[id].avg;
	
	if ( val < stats.statistics.statistics[id].min ) 
	{
		stats.statistics.statistics[id].min = val;
		stats.statistics.statistics[id].minTime = TIME_getSinceToday();
	}
	if ( val > stats.statistics.statistics[id].max ) 
	{
		stats.statistics.statistics[id].max = val;
		stats.statistics.statistics[id].maxTime = TIME_getSinceToday();
	}
	
	stats.statHelpers[id].tempSum += val;
	stats.statHelpers[id].numTempUpdates++;
	// Make sure avg * numTempUpdates won't overflow
	numTempUpdatesT = stats.statHelpers[id].numTempUpdates;
	numUpdatesT = stats.statistics.numUpdates;
	tempSumT = stats.statHelpers[id].tempSum;
	while ( numTempUpdatesT >= (((long)1)<<15) )
	{
		numTempUpdatesT >>= 1;
		numUpdatesT >>= 1;
		tempSumT >>= 1;
	}

	diff = tempSumT - ((IqLong)stats.statistics.statistics[id].avg) * numTempUpdatesT;

	if ( numUpdatesT > IQ_abs(diff) )
	{
		// Do nothing -- tempSum will have been added to, but isn't enough yet to tick over the average
	}
	else
	{
		// Ready to combine temp sample store with actual average
		stats.statistics.statistics[id].avg += (Iq)( diff / numUpdatesT );
		stats.statHelpers[id].tempSum = 0;
		stats.statHelpers[id].numTempUpdates = 0;
	}

	// Update q if we have had more than one sample
	if ( stats.statistics.numUpdates > 1 )
	{
		stats.statistics.statistics[id].q += IQ_max( 0, IQ_mpyTo24( val - oldAvg, val - stats.statistics.statistics[id].avg ) );
	}

	/*if ( id == PV_VOLT )
	{
		//COMMS_sendDebugPacket( meas.pvVolt.val, stats.statistics.statistics[id].avg, diff & 0xFFFF, (Iq)( diff / (long)numUpdatesT ) );
	}*/
}

int STATS_saveToFlash()
{
	int retVal;

	if ( stats.isFirst )
	{
		stats.isFirst = 0;
		//return 0;
	}

	if ( stats.isReadyToWrite )
	{
		retVal = FLASH_startWrite( stats.currentAddr, sizeof( stats.statistics ) );
		if ( retVal >= 0 )
		{
			FLASH_writeStr( (unsigned char *)&stats.statistics, sizeof( stats.statistics ) );	
			FLASH_endWriteData();
			return 0;
		}
		else
		{
			return retVal;
		}
	}
	else
	{
		return -3;
	}
}

/*void STATS_loadFromFlash()
{
	unsigned int ii;
	for ( ii = 0; ii < StatId_NUM; ii++ )
	{
		stats.statistics.statistics[ii].min = IQ_MIN;
		stats.statistics.statistics[ii].max = IQ_MAX;
		stats.statistics.statistics[ii].avg = IQ_cnst(0.0);
		stats.statistics.statistics[ii].q = IQ_cnst(0.0);
	}
	stats.statistics.numUpdates = 0;
	stats.statistics.lastWriteTime = 3600000;
}*/

int STATS_initEntryAddr()
{
	// Read flash to determine the index of the most recently written stats struct
	//return 0;
	int retVal;
	unsigned long block;
	Time tmEarliest = 0xFFFFFFFFFFFFFFFF;
	unsigned long addrEarliest = FLASH_STATS_ADDR;

	for ( block = 0; block < FLASH_NUM_STATS_BLOCKS; block++ )
	{
		for ( stats.currentAddr = FLASH_STATS_ADDR + block*BLOCK_SIZE; stats.currentAddr <= ( FLASH_STATS_ADDR + (block+1)*BLOCK_SIZE - sizeof(Statistics) ); stats.currentAddr += sizeof(Statistics) )
		{
			retVal = FLASH_readStr( (unsigned char *)&(stats.statistics), stats.currentAddr, sizeof(stats.statistics), 0 );
			if ( retVal < 0 )
			{
				return retVal;
			}

			if ( stats.statistics.lastWriteTime == 0xFFFFFFFFFFFFFFFF )
			{
				// We have found an empty entry, use it
				goto FOUND_EMPTY;
			}
		}
	}
	
	// No empty addresses, so sector that starts with the earliest address and erase
	for ( block = 0; block < FLASH_NUM_STATS_BLOCKS; block++ )
	{
		stats.currentAddr = FLASH_STATS_ADDR + block*BLOCK_SIZE;
		retVal = FLASH_readStr( (unsigned char *)&(stats.statistics), stats.currentAddr, sizeof(stats.statistics), 0 );
		if ( stats.statistics.lastWriteTime < tmEarliest )
		{
			tmEarliest = stats.statistics.lastWriteTime;
			addrEarliest = stats.currentAddr;
		}
	}

	stats.currentAddr = addrEarliest;

	stats.isErasePending = 1;

	return -1;


FOUND_EMPTY:
	// Address should be valid now
	return 0;
}

unsigned long STATS_getStatsLenTotal()
{
	return ( FLASH_NUM_STATS_BLOCKS * BLOCK_SIZE );
	//return ( 1 * BLOCK_SIZE );
	//return ( sizeof( stats.statistics ) );
}

unsigned char * STATS_getStatsMemAddr()
{
	return (unsigned char *)&stats.statistics;
}

unsigned int STATS_getCurrentBlock()
{
	return ( stats.currentAddr - FLASH_STATS_ADDR ) / BLOCK_SIZE;
}

unsigned long STATS_getStatsLen( unsigned long blockAddr )
{
	return BLOCK_SIZE;
}

unsigned int STATS_getNumBlocks()
{
	return FLASH_NUM_STATS_BLOCKS;
}

#endif

