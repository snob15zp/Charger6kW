
//-------------------------------------------------------------------
// File: telem.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#include "telem.h"
#include "cfg.h"
#include "flash.h"
#include "util.h"
#include "meas.h"
#include "iqmath.h"
#include "comms.h"
#include "flag.h"

/*	Telemetry flash storage structure
	---------------------------------

	Each sector consists of:
		Address	| Contents
		--------|---------
		  000h	| Valid sector indicator: will read 0x5A5A if sector contains at least a valid config string
				|		This word should only be written to after the config string for this sector has been verified as correct
		  002h	| Block number
		  004h	| Timestamp: 8 bytes, milliseconds since unix epoch (1/1/1970 00:00)
		  00Ch	| Sample period
		  00Eh	| Enable bitfield
		  010h	| Start of data

	The data will be written in 1s complement, and 0xFFFF will be reserved to indicate invalid data, i.e. data that retains its
	erased value.  To convert from 2s complement before writing, subtract 1 from negative values, unless the value is 0x8000 as 
	this would underflow and create a large positive number.  The same should be done for 4-byte data.
*/

#define TELEM_VALID_SECTOR_MARKER	0x5A5A

#define TELEM_HEADER_OFFSET				2
#define TELEM_BLOCKNUM_OFFSET			TELEM_HEADER_OFFSET
#define TELEM_TIMESTAMP_OFFSET			( TELEM_BLOCKNUM_OFFSET + sizeof(unsigned int) )
#define TELEM_SAMPLE_PERIOD_OFFSET		( TELEM_TIMESTAMP_OFFSET + sizeof(Time) )
#define TELEM_ENABLE_BITFIELD_OFFSET	( TELEM_SAMPLE_PERIOD_OFFSET + sizeof(TELEM_SamplePeriod) )
#define TELEM_DATA_OFFSET				( TELEM_ENABLE_BITFIELD_OFFSET + sizeof(unsigned int) )

const unsigned int TELEM_PERIOD_MULTIPLIERS[TELEM_PERIOD_NUM] = 
{
	200 / TELEM_BASE_PERIOD_MS,
	( 1000 / TELEM_BASE_PERIOD_MS),
	10 * ( 1000 / TELEM_BASE_PERIOD_MS),
	60 * ( 1000 / TELEM_BASE_PERIOD_MS),
	10 * 60 * ( 1000 / TELEM_BASE_PERIOD_MS),
	60 * 60 * ( 1000 / TELEM_BASE_PERIOD_MS) 
};

typedef struct TelemHeader_
{
	unsigned int blockNum;
	Time tm;
	TELEM_SamplePeriod period;
	unsigned int enableBitfield;
} TelemHeader;

typedef struct Telem_
{
	int isReady;
	int isFull;
	unsigned int callCount;
	unsigned int queuedWrites;
	unsigned int writesPerSector;
	unsigned int writesLeftThisSector;
	unsigned long startAddr;
	unsigned long endAddr;
	unsigned long currentAddr;
	unsigned int currentTelemBlock;
	unsigned long memLen;
	TelemHeader header;
} Telem;

Telem telem;

void TELEM_findCurrentSector();
int TELEM_prepareSector();

void TELEM_init()
{
	// Just to the basic init here.  The modules won't be readied for actual writing until the TIME module has acquired a 
	// valid time (either broadcast over the CAN network, or estimated from data in flash, etc.

	telem.isReady = 0;
	telem.isFull = 0;

	telem.callCount = 0;
	telem.queuedWrites = 0;

	// Initialise the flash addresses
	telem.startAddr = FLASH_TELEM_START_ADDR;
	telem.endAddr = FLASH_TELEM_END_ADDR;
	telem.memLen = telem.endAddr - telem.startAddr;	

#if 0
	TELEM_findCurrentSector();

	telem.currentTelemBlock = ( ( telem.currentAddr & ERASE_MASK1 ) - telem.startAddr ) / BLOCK_SIZE;
	
	// Erase current sector
	FLASH_eraseBlockBusy( telem.currentAddr );
#endif
}

#if 0

void TELEM_findCurrentSector()
{
	Time tmOldest = 0xFFFFFFFFFFFFFFFF;
	unsigned long addrOldest;
	Time tmCurrent;
	
	// Find the first unwritten sector, if there is one
	for ( telem.currentAddr = telem.startAddr; telem.currentAddr < telem.endAddr; telem.currentAddr += BLOCK_SIZE )
	{
		if ( FLASH_readU16( telem.currentAddr ) != TELEM_VALID_SECTOR_MARKER )
		{
			// Found a free sector
			telem.isFull = 0;
			break;
		}
	}

	if ( telem.currentAddr == telem.endAddr )
	{
		// Didn't find a free sector, so find the sector with the oldest timestamp
		telem.isFull = 1;
		addrOldest = telem.startAddr;
		for ( telem.currentAddr = telem.startAddr; telem.currentAddr < telem.endAddr; telem.currentAddr += BLOCK_SIZE )
		{
			tmCurrent = FLASH_readU64( telem.currentAddr + TELEM_HEADER_OFFSET );
			if ( tmCurrent < tmOldest )
			{
				tmOldest = tmCurrent;
				addrOldest = telem.currentAddr;
			}
		}

		telem.currentAddr = addrOldest;
	}
	//telem.currentAddr = telem.startAddr;
}

void TELEM_timeHasUpdated()
{
	// The current time has changed, so start writing from a new block.  This shouldn't happen very often...
	// =====================================================================================================
	telem.isReady = 0;

	telem.callCount = 0;
	telem.queuedWrites = 0;

	//return;
	
	TELEM_findCurrentSector();

	// telem.currentAddr will now point to either the first unused sector, or the sector with the oldest timestamp
	if ( TELEM_prepareSector() >= 0 )
	{
		// Telemetry module is now ready to record data
		telem.isReady = 1;
	}
}
#endif

unsigned char TELEM_getStatus()
{
	return ( telem.isReady | ( (telem.queuedWrites>0) << 1 ) );
}

int TELEM_isFull()
{
	return telem.isFull;
}

#if 0
void TELEM_eraseStarted()
{
	telem.isReady = 0;
}

void TELEM_eraseDoneCallback( int retVal )
{
	if ( retVal >= 0 )
	{
		// Erase succeeded, to jump to first sector and prepare it for logging
		telem.writesLeftThisSector = 0;
		telem.currentAddr = telem.startAddr;
		TELEM_prepareSector();
	}
	else
	{
		// Erase failed, just keep going as normal
	}

	telem.isReady = 1;
}



int TELEM_switchToNextSector()
{
	unsigned long oldAddr = telem.currentAddr;
	telem.writesLeftThisSector = 0;	// Make sure we keep trying to switch even if we fail this time
	telem.currentAddr = telem.startAddr + ( (unsigned long)UTIL_modAdd( telem.currentTelemBlock, 1, telem.memLen / BLOCK_SIZE ) ) * BLOCK_SIZE;
	if ( TELEM_prepareSector() < 0 )
	{
		// Failed to prepare the sector -- just try again next time
		//telem.isReady = 0; 
		if ( telem.currentAddr < oldAddr )
		{
			// Must have wrapped, so signal log file full
			FLAG_logFlag( FLAG_CODE_LOG_FULL, 1 );
		}
		telem.currentAddr = oldAddr;
		return -1;
	}
	return 0;
}

// Prepare the current sector for logging telemetry
int TELEM_prepareSector()
{
	TelemHeader headerConfirm;
	unsigned int ii;
	unsigned int writeSize;
	unsigned int currentBlock;

	currentBlock = ( ( telem.currentAddr & ERASE_MASK1 ) - telem.startAddr ) / BLOCK_SIZE;

	// Fill the telemetry config struct
	telem.header.blockNum = currentBlock;
	telem.header.tm = TIME_get();
	telem.header.period = CFG_remoteCfg.telemSamplePeriod;
	if ( telem.header.period >= TELEM_PERIOD_END ) telem.header.period = TELEM_PERIOD_END-1;
	telem.header.enableBitfield = CFG_remoteCfg.telemEnableBitfield;

	// Determine how many writes will fit in the sector
	writeSize = 0;
	if ( telem.header.enableBitfield & TELEM_OUTVOLT_MASK ) writeSize += TELEM_OUTVOLT_SIZE;
	if ( telem.header.enableBitfield & TELEM_OUTCURR_MASK ) writeSize += TELEM_OUTCURR_SIZE;
	if ( telem.header.enableBitfield & TELEM_OUTCHARGE_MASK ) writeSize += TELEM_OUTCHARGE_SIZE;
	if ( telem.header.enableBitfield & TELEM_PVVOLT_MASK ) writeSize += TELEM_PVVOLT_SIZE;
	if ( telem.header.enableBitfield & TELEM_PVCURR_MASK ) writeSize += TELEM_PVCURR_SIZE;
	if ( telem.header.enableBitfield & TELEM_PVPOWER_MASK ) writeSize += TELEM_PVPOWER_SIZE;
	if ( telem.header.enableBitfield & TELEM_PVOCVOLT_MASK ) writeSize += TELEM_PVOCVOLT_SIZE;
	telem.writesPerSector = ( BLOCK_SIZE - TELEM_DATA_OFFSET ) / writeSize;

	// Erase this sector
	if ( FLASH_eraseBlockBusy( telem.currentAddr ) < 0 )
	{
		// signal a telemetry error
		return -1;
	}

	// Write the config struct (busy write)
	for ( ii = 0; ii < sizeof(telem.header); ii++ )
	{
		if ( FLASH_writeByteBusy( telem.currentAddr + TELEM_HEADER_OFFSET + ii, *( ((unsigned char *)&(telem.header)) + ii ) ) < 0 ) return -1;
	}
	
	// Read the config string to verify it
	FLASH_readStr( (unsigned char *)&headerConfirm, telem.currentAddr + TELEM_HEADER_OFFSET, sizeof(headerConfirm), 0 );
	if (		headerConfirm.blockNum != telem.header.blockNum
			||	headerConfirm.tm != telem.header.tm
			||	headerConfirm.period != telem.header.period
			||	headerConfirm.enableBitfield != telem.header.enableBitfield )
	{
		// Config write failed
		return -1;
	}

	// Config string must have verified ok, so write the valid sector marker
	if ( FLASH_writeByteBusy( telem.currentAddr, ( TELEM_VALID_SECTOR_MARKER >> 8 ) & 0xFF ) < 0 ) return -1;
	if ( FLASH_writeByteBusy( telem.currentAddr + 1, ( TELEM_VALID_SECTOR_MARKER ) & 0xFF ) < 0 ) return -1;

	telem.currentAddr += TELEM_DATA_OFFSET;

	telem.currentTelemBlock = currentBlock;

	telem.writesLeftThisSector = telem.writesPerSector;

	return 0;
}


/*unsigned long TELEM_getPastBlockAddress( unsigned int numBlocksInPast )
{
	return telem.startAddr + ( (unsigned long)UTIL_modAdd( telem.currentTelemBlock, -numBlocksInPast, telem.memLen / BLOCK_SIZE ) ) * BLOCK_SIZE;
	//return TELEM_getCurrentBlockAddress();
}*/


unsigned int TELEM_getMostRecentBlock()
{
	if ( telem.isReady && telem.writesLeftThisSector != telem.writesPerSector )
	{
		return telem.currentTelemBlock;
	}
	else
	{
		return UTIL_modAdd( telem.currentTelemBlock, -1, TELEM_getNumBlocks() );
	}
}

unsigned int TELEM_getNumBlocksByTime( Time minTm )
{
	int isFound, writeSize;
	unsigned int marker;
	TelemHeader headerCheck;
	unsigned long addr;
	unsigned int numBlocksByTime;

	// Go back through the sectors to find the most recent sector before the given time, and return the sector after that
	
	numBlocksByTime = 0;
	addr = telem.currentAddr & ERASE_MASK1;
	isFound = 0;

	do
	{
		// Read the marker
		if ( FLASH_readStr( (unsigned char *)&marker, addr, 2, 0 ) != 0 ) 
		{
			return numBlocksByTime;
		}
		
		if ( marker != TELEM_VALID_SECTOR_MARKER )
		{
			if ( numBlocksByTime != 0 )
			{
				// Found an empty sector that is not the current sector (which could be empty 'cause we haven't written to it yet)
				isFound = 1;
				break;
			}
		}
		else
		{
			// Read the sector header
			if ( FLASH_readStr( (unsigned char *)&headerCheck, addr+2, sizeof(headerCheck), 0 ) != 0 ) 
			{
				return numBlocksByTime;
			}

			writeSize = 0;
			if ( headerCheck.enableBitfield & TELEM_OUTVOLT_MASK ) writeSize += TELEM_OUTVOLT_SIZE;
			if ( headerCheck.enableBitfield & TELEM_OUTCURR_MASK ) writeSize += TELEM_OUTCURR_SIZE;
			if ( headerCheck.enableBitfield & TELEM_OUTCHARGE_MASK ) writeSize += TELEM_OUTCHARGE_SIZE;
			if ( headerCheck.enableBitfield & TELEM_PVVOLT_MASK ) writeSize += TELEM_PVVOLT_SIZE;
			if ( headerCheck.enableBitfield & TELEM_PVCURR_MASK ) writeSize += TELEM_PVCURR_SIZE;
			if ( headerCheck.enableBitfield & TELEM_PVPOWER_MASK ) writeSize += TELEM_PVPOWER_SIZE;
			if ( headerCheck.enableBitfield & TELEM_PVOCVOLT_MASK ) writeSize += TELEM_PVOCVOLT_SIZE;

			if ( ( headerCheck.tm + (Time)(TELEM_PERIOD_MULTIPLIERS[headerCheck.period] * TELEM_BASE_PERIOD_MS) * (Time)(( BLOCK_SIZE - TELEM_DATA_OFFSET )/writeSize) ) 
					< minTm )
			{
				isFound = 1;
				//COMMS_sendDebugPacketU64( minTm - ( headerCheck.tm + (Time)(TELEM_PERIOD_MULTIPLIERS[headerCheck.period] * TELEM_BASE_PERIOD_MS) * (Time)(( BLOCK_SIZE - TELEM_DATA_OFFSET )/writeSize) ) );
				break;
			}
			else
			{
				// This sector was within the time range
				numBlocksByTime++;
			}
		}

		addr -= BLOCK_SIZE;
		if ( addr < telem.startAddr ) addr += telem.memLen;
	}
	while ( isFound == 0 && addr != ( telem.currentAddr & ERASE_MASK1 ) );
	
	return numBlocksByTime;
}

unsigned long TELEM_getTelemLen( unsigned long blockAddr )
{
	// Temp, just return the amount of data in this first sector
	//return ( telem.currentAddr - ( telem.currentAddr & ERASE_MASK1 ) );

	if ( FLASH_readU16( blockAddr ) != TELEM_VALID_SECTOR_MARKER )
	{
		return 0;
	}
	else
	{
		// Don't bother trying to figure out how much data there is, just send the whole block, it's only 4kb
		return BLOCK_SIZE;
	}
}

unsigned int TELEM_getNumBlocks()
{
	return (unsigned int)( telem.memLen / BLOCK_SIZE );
}


int TELEM_getIsReady()
{
	return telem.isReady;
}
unsigned char telemBuffer[TELEM_MAX_SIZE];
#endif

void TELEM_logIfPeriodElapsed()
{

#if 0

	unsigned int len;
	unsigned char * pTmp;
	if ( telem.isReady && telem.header.enableBitfield )
	{
		if ( telem.callCount == 0 )
		{
			pTmp = telemBuffer;
			if ( telem.header.enableBitfield & TELEM_OUTVOLT_MASK )
			{
				*(Iq *)pTmp = meas.outVolt.val;
				if ( *(Iq *)pTmp < 0 && *(unsigned int *)pTmp != 0x8000)
					(*(Iq *)pTmp)--;	// Convert 2s complement to 1s complement
				pTmp += TELEM_OUTVOLT_SIZE;
			}
			if ( telem.header.enableBitfield & TELEM_OUTCURR_MASK )
			{
				*(Iq *)pTmp = meas.outCurr.val;
				if ( *(Iq *)pTmp < 0 && *(unsigned int *)pTmp != 0x8000)
					(*(Iq *)pTmp)--;	// Convert 2s complement to 1s complement
				pTmp += TELEM_OUTCURR_SIZE;
			}
			if ( telem.header.enableBitfield & TELEM_OUTCHARGE_MASK )
			{
				*(IqLong *)pTmp = meas.outCharge.val;
				if ( *(IqLong *)pTmp < 0 && *(unsigned long *)pTmp != 0x80000000)
					(*(Iq *)pTmp)--;	// Convert 2s complement to 1s complement
				pTmp += TELEM_OUTCHARGE_SIZE;
			}
			if ( telem.header.enableBitfield & TELEM_PVVOLT_MASK )
			{
				*(Iq *)pTmp = meas.pvVolt.val;
				if ( *(Iq *)pTmp < 0 && *(unsigned int *)pTmp != 0x8000)
					(*(Iq *)pTmp)--;	// Convert 2s complement to 1s complement
				pTmp += TELEM_PVVOLT_SIZE;
			}
			if ( telem.header.enableBitfield & TELEM_PVCURR_MASK )
			{
				*(Iq *)pTmp = meas.pvCurr.val;
				if ( *(Iq *)pTmp < 0 && *(unsigned int *)pTmp != 0x8000)
					(*(Iq *)pTmp)--;	// Convert 2s complement to 1s complement
				pTmp += TELEM_PVCURR_SIZE;
			}
			if ( telem.header.enableBitfield & TELEM_PVPOWER_MASK )
			{
				*(Iq *)pTmp = meas.pvPower.val;
				if ( *(Iq *)pTmp < 0 && *(unsigned int *)pTmp != 0x8000)
					(*(Iq *)pTmp)--;	// Convert 2s complement to 1s complement
				pTmp += TELEM_PVPOWER_SIZE;
			}
			if ( telem.header.enableBitfield & TELEM_PVOCVOLT_MASK )
			{
				*(Iq *)pTmp = meas.pvOcVolt.val;
				if ( *(Iq *)pTmp < 0 && *(unsigned int *)pTmp != 0x8000)
					(*(Iq *)pTmp)--;	// Convert 2s complement to 1s complement
				pTmp += TELEM_PVOCVOLT_SIZE;
			}

			//COMMS_sendDebugPacket( telem.writesLeftThisSector );

			if ( telem.writesLeftThisSector == 0 )
			{
				// Prepare the next sector for logging
				TELEM_switchToNextSector();
			}

			// Increment the write queue
			telem.queuedWrites++;

			// Limit the number of writes to what can fit in the sector
			if ( telem.queuedWrites > telem.writesLeftThisSector )
			{
				telem.queuedWrites = telem.writesLeftThisSector;
			}

			//telem.queuedWrites = 1;

			// Write out the values.  Write some extra copies if we missed some writes before, e.g. because flash was being used by some other module.
			// Should we have a proper buffer rather than just writing the same values out multiple times?
			if ( telem.queuedWrites )
			{
				/*if ( FLASH_startWrite( telem.currentAddr, pTmp - telemBuffer ) >= 0 )
				{
					if ( FLASH_writeStr( telemBuffer, pTmp - telemBuffer ) >= 0 )
					{
						telem.queuedWrites = 0;
						telem.writesLeftThisSector--;
						telem.currentAddr += pTmp - telemBuffer;
					}
				}*/

				len = (unsigned int)( pTmp - telemBuffer );
				len *= telem.queuedWrites;

				if ( FLASH_startWrite( telem.currentAddr, len ) >= 0 )
				{
					while ( telem.queuedWrites )
					{
						if ( FLASH_getFreeBufferSpace() >= pTmp - telemBuffer )
						{
							FLASH_writeStr( telemBuffer, pTmp - telemBuffer );
							telem.queuedWrites--;
							telem.writesLeftThisSector--;
							telem.currentAddr += pTmp - telemBuffer;
						}
						else
						{
							FLASH_endWriteData();
							break;
						}
					}
				}
			}

			telem.callCount = TELEM_PERIOD_MULTIPLIERS[telem.header.period];

		}
		telem.callCount--;
	}

#endif

	return;
}


