
//-------------------------------------------------------------------
// File: flag.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

///#include <msp430x24x.h>
#include "flag.h"
#include "flash.h"
#include "util.h"
#include "time.h"
#include "cfg.h"
#include "meas.h"
///#include "comms.h"
#include "io.h"
#include "safety.h"
#ifdef DBG_HARDCODED_VIN_SETPOINT
#include "ctrl.h"
#endif

// This should be chosen such that it divides evenly into BLOCK_SIZE
#define FLAG_LOG_ENTRY_SIZE		8
#define FLAG_BUFFER_ENTRY_SIZE	( FLAG_LOG_ENTRY_SIZE - sizeof(FLAG_Code) )

typedef struct Flag_
{
	// Buffer to hold flags to be written to flash
	// The buffer is treated as an array of 6-byte timestamps that are indexed using the FLAG_Code enum
	// The buffer is initialised to 0xFF all
	// When a flag should be logged, the timestamp is written to the buffer at the index of the specified flag
	// This buffer is then read, and flags with timestamps of 0xFFFFFFFFFFFF are considered inactive,
	//		while any other timestamp value is logged as a flag and the buffer value returned to 0xFF...
	unsigned char buffer[ FLAG_CODE_NUM * FLAG_BUFFER_ENTRY_SIZE ];	
	unsigned long currentAddr;
	unsigned long activeSectorAddr;
	unsigned int activeSectorNum;
	FlagBitfield flagBitfield;
	FlagBitfield flagBitfieldRelay;
	int isReadyToCheck;
	int isReadyToWrite;
	int isErasing;
} Flag;

typedef struct FlagStates_
{
	FLAG_State	systemInitFlag;
	FlagState	lowOutVoltWarnFlag;
	FlagState	lowOutVoltFaultFlag;
	FlagState	lowOutVoltGensetFlag;
	FlagState	highOutVoltFaultFlag;
	FlagState	highOutCurrFaultFlag;
	FlagState	highDisCurrFaultFlag;
	FlagState	highTempFaultFlag;
	FLAG_State	inBreakerOpenFlag;
	FLAG_State	outBreakerOpenFlag;
	FLAG_State	tempSenseFaultFlag;
	FLAG_State	sdFlags[SAFETY_NUM_SD_REASONS];
	FLAG_State	logFullFlag;
	FlagState	panelMissingFlag;
	FLAG_State	badCfgRangeFlag;
} FlagStates;

Flag flag;

FlagStates flagStates;

int FLAG_swapActiveSector(void);

void FLAG_init(void)
{
	Time earliestTime, currentTime;
	unsigned long earliestSector = 0;
	unsigned int ii;

	flag.isReadyToCheck = 0;
	flag.isReadyToWrite = 0;
	flag.isErasing = 0;

	FLAG_initTrigs();

	// Init states 
	flagStates.systemInitFlag = FLAG_STATE_INACTIVE;
	flagStates.lowOutVoltWarnFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.lowOutVoltFaultFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.lowOutVoltGensetFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.highOutVoltFaultFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.highOutCurrFaultFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.highDisCurrFaultFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.highTempFaultFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.inBreakerOpenFlag = FLAG_STATE_INACTIVE;
	flagStates.outBreakerOpenFlag = FLAG_STATE_INACTIVE;
	flagStates.tempSenseFaultFlag = FLAG_STATE_INACTIVE;
	for ( ii = 0; ii < SAFETY_NUM_SD_REASONS; ii++ )
	{
		flagStates.sdFlags[ii] = FLAG_STATE_INACTIVE;
	}
	flagStates.logFullFlag = FLAG_STATE_INACTIVE;
	flagStates.panelMissingFlag.currentState = FLAG_STATE_INACTIVE;
	flagStates.badCfgRangeFlag = FLAG_STATE_INACTIVE;

	flag.flagBitfield = 0;
	flag.flagBitfieldRelay = 0;

	// Init last change times for time hysterisis / hold timing
	flagStates.lowOutVoltWarnFlag.hystStartTime = 0;
	flagStates.lowOutVoltFaultFlag.hystStartTime = 0;
	flagStates.lowOutVoltGensetFlag.hystStartTime = 0;
	flagStates.highOutVoltFaultFlag.hystStartTime = 0;
	flagStates.highOutCurrFaultFlag.hystStartTime = 0;
	flagStates.highDisCurrFaultFlag.hystStartTime = 0;
	flagStates.highTempFaultFlag.hystStartTime = 0;
	flagStates.panelMissingFlag.hystStartTime = 0;

	// Init the flag buffer to all 0xFF
	for ( ii = 0; ii < FLAG_CODE_NUM * FLAG_BUFFER_ENTRY_SIZE; ii++ )
	{
		flag.buffer[ii] = 0xFF;
	}

	// Find the first empty spot in the flag log
	for ( flag.currentAddr = FLASH_FLAG_ADDR; flag.currentAddr < FLASH_FLAG_ADDR + FLAG_getFlagLen(); flag.currentAddr += FLAG_LOG_ENTRY_SIZE )
	{
		if ( FLASH_readU16( flag.currentAddr ) == 0xFFFF )
		{
			// We've found an empty entry
			flag.activeSectorAddr = flag.currentAddr & ERASE_MASK1;
			flag.activeSectorNum = ( flag.activeSectorAddr - FLASH_FLAG_ADDR ) / BLOCK_SIZE;
			break;
		}
	}
	if ( flag.currentAddr == FLASH_FLAG_ADDR + FLAG_getFlagLen() )
	{
		// We didn't find an empty entry, so compare the timestamps of the last entries on all the sectors to see which one is earliest
		earliestTime = 0xFFFFFFFFFFFFFFFF;
		for ( flag.activeSectorNum = 0; flag.activeSectorNum < FLASH_NUM_FLAG_BLOCKS; flag.activeSectorNum++ )
		{
			currentTime = FLASH_readU64( FLASH_FLAG_ADDR + ((unsigned long)flag.activeSectorNum+1) * BLOCK_SIZE - FLAG_LOG_ENTRY_SIZE );
			if ( currentTime < earliestTime )
			{
				earliestTime = currentTime;
				earliestSector = flag.activeSectorNum;
			}
		}
		flag.activeSectorNum = earliestSector;
		flag.activeSectorAddr = FLASH_FLAG_ADDR + (unsigned long)flag.activeSectorNum * BLOCK_SIZE;
		flag.currentAddr = flag.activeSectorAddr;
		if ( FLASH_eraseBlockBusy( flag.currentAddr ) < 0 ) return;
	}

	flag.isReadyToWrite = 1;
}

void FLAG_initTrigs(void)
{
	// Init Iq versions of all the trigger and reset values
	///not now flagStates.lowOutVoltWarnFlag.triggerVal	= IQ_cnst( CFG_remoteCfg.lowOutVoltWarnFlag.triggerVal / MEAS_OUTVOLT_BASE );
	///not now flagStates.lowOutVoltWarnFlag.resetVal		= IQ_cnst( CFG_remoteCfg.lowOutVoltWarnFlag.resetVal / MEAS_OUTVOLT_BASE );
	///not now flagStates.lowOutVoltFaultFlag.triggerVal	= IQ_cnst( CFG_remoteCfg.lowOutVoltFaultFlag.triggerVal / MEAS_OUTVOLT_BASE );
	flagStates.lowOutVoltFaultFlag.resetVal		= IQ_cnst( CFG_remoteCfg.lowOutVoltFaultFlag.resetVal / MEAS_OUTVOLT_BASE );
	flagStates.lowOutVoltGensetFlag.triggerVal	= IQ_cnst( CFG_remoteCfg.lowOutVoltGensetFlag.triggerVal / MEAS_OUTVOLT_BASE );
	flagStates.lowOutVoltGensetFlag.resetVal	= IQ_cnst( CFG_remoteCfg.lowOutVoltGensetFlag.resetVal / MEAS_OUTVOLT_BASE );
	flagStates.highOutVoltFaultFlag.triggerVal	= IQ_cnst( CFG_remoteCfg.highOutVoltFaultFlag.triggerVal / MEAS_OUTVOLT_BASE );
	flagStates.highOutVoltFaultFlag.resetVal	= IQ_cnst( CFG_remoteCfg.highOutVoltFaultFlag.resetVal / MEAS_OUTVOLT_BASE );
	///not now flagStates.highOutCurrFaultFlag.triggerVal	= IQ_cnst( CFG_remoteCfg.highOutCurrFaultFlag.triggerVal / MEAS_OUTCURR_BASE );
	///not now flagStates.highOutCurrFaultFlag.resetVal	= IQ_cnst( CFG_remoteCfg.highOutCurrFaultFlag.resetVal / MEAS_OUTCURR_BASE );
	///not now flagStates.highDisCurrFaultFlag.triggerVal	= IQ_cnst( CFG_remoteCfg.highDisCurrFaultFlag.triggerVal / MEAS_OUTCURR_BASE );
	flagStates.highDisCurrFaultFlag.resetVal	= IQ_cnst( CFG_remoteCfg.highDisCurrFaultFlag.resetVal / MEAS_OUTCURR_BASE );
	///not now flagStates.highTempFaultFlag.triggerVal		= IQ_cnst( CFG_remoteCfg.highTempFaultFlag.triggerVal / MEAS_TEMPR_BASE );
	///not now flagStates.highTempFaultFlag.resetVal		= IQ_cnst( CFG_remoteCfg.highTempFaultFlag.resetVal / MEAS_TEMPR_BASE );
	///not now flagStates.panelMissingFlag.triggerVal		= IQ_cnst( CFG_remoteCfg.panelMissingFlag.triggerVal / MEAS_PVVOLT_BASE );
}

void FLAG_timeHasUpdated(void)
{
	flag.isReadyToCheck = 1;
	if ( CFG_remoteCfg.systemInitFlag.mode != FLAG_DISABLED )
	{
		FLAG_logFlag( FLAG_CODE_SYSTEM_INIT, 1 );
	}
}

unsigned char FLAG_getStatus(void)
{
	return ( flag.isReadyToCheck | (flag.isReadyToWrite<<1) | (flag.isErasing<<2) );
}

FlagBitfield FLAG_getFlagBitfield(void)
{
	return flag.flagBitfield;
}

int FLAG_swapActiveSector(void)
{
	unsigned int tempSector = UTIL_modAdd( flag.activeSectorNum, 1, FLASH_NUM_FLAG_BLOCKS );

	if ( FLASH_eraseBlockBusy( FLASH_FLAG_ADDR + (unsigned long)tempSector * BLOCK_SIZE ) < 0 )
	{
		// Erase failed, try again next time
		return -1;
	}
	// Erase succeeded
	flag.activeSectorNum = tempSector;
	flag.activeSectorAddr = FLASH_FLAG_ADDR + (unsigned long)flag.activeSectorNum * BLOCK_SIZE;
	flag.currentAddr = flag.activeSectorAddr;
	return 0;
}

void FLAG_checkAllFlags(void)
{
	int ii;
	//if ( !flag.isReady ) return;

	// Check the state of all flag-triggering quantities, and buffer flags if required
	// NOTE: don't check system init flag, this is called seperately
	if ( CFG_remoteCfg.lowOutVoltWarnFlag.mode != FLAG_DISABLED )
	{
		flagStates.lowOutVoltWarnFlag.currentVal = meas.outVolt.val;
		FLAG_checkFlagTrig( &flagStates.lowOutVoltWarnFlag, &CFG_remoteCfg.lowOutVoltWarnFlag, FLAG_CODE_LOW_OUT_VOLT_WARN );
		//COMMS_sendDebugPacket( flagStates.lowOutVoltWarnFlag.currentVal, flagStates.lowOutVoltWarnFlag.triggerVal );
	}
	if ( CFG_remoteCfg.lowOutVoltFaultFlag.mode != FLAG_DISABLED )
	{
		flagStates.lowOutVoltFaultFlag.currentVal = meas.outVolt.val;
		FLAG_checkFlagTrig( &flagStates.lowOutVoltFaultFlag, &CFG_remoteCfg.lowOutVoltFaultFlag, FLAG_CODE_LOW_OUT_VOLT_FAULT );
	}
	if ( CFG_remoteCfg.lowOutVoltGensetFlag.mode != FLAG_DISABLED )
	{
		flagStates.lowOutVoltGensetFlag.currentVal = meas.outVolt.val;
		FLAG_checkFlagHold( &flagStates.lowOutVoltGensetFlag, &CFG_remoteCfg.lowOutVoltGensetFlag, FLAG_CODE_LOW_OUT_VOLT_GENSET );
	}
	if ( CFG_remoteCfg.highOutVoltFaultFlag.mode != FLAG_DISABLED )
	{
		flagStates.highOutVoltFaultFlag.currentVal = meas.outVolt.val;
		FLAG_checkFlagTrig( &flagStates.highOutVoltFaultFlag, &CFG_remoteCfg.highOutVoltFaultFlag, FLAG_CODE_HIGH_OUT_VOLT_FAULT );
	}
	if ( CFG_remoteCfg.highOutCurrFaultFlag.mode != FLAG_DISABLED )
	{
		flagStates.highOutCurrFaultFlag.currentVal = meas.outCurr.val;
		FLAG_checkFlagTrig( &flagStates.highOutCurrFaultFlag, &CFG_remoteCfg.highOutCurrFaultFlag, FLAG_CODE_HIGH_OUT_CURR_FAULT );
	}
	if ( CFG_remoteCfg.highDisCurrFaultFlag.mode != FLAG_DISABLED )
	{
		flagStates.highDisCurrFaultFlag.currentVal = IQ_cnst(0.0);
		FLAG_checkFlagTrig( &flagStates.highDisCurrFaultFlag, &CFG_remoteCfg.highDisCurrFaultFlag, FLAG_CODE_HIGH_DIS_CURR_FAULT );
	}
	if ( CFG_remoteCfg.highTempFaultFlag.mode != FLAG_DISABLED )
	{
		flagStates.highTempFaultFlag.currentVal = meas.batTempr.val;
		FLAG_checkFlagTrig( &flagStates.highTempFaultFlag, &CFG_remoteCfg.highTempFaultFlag, FLAG_CODE_HIGH_TEMP_FAULT );
	}
	if ( CFG_remoteCfg.inBreakerOpenFlag.mode != FLAG_DISABLED )
	{
		if ( flagStates.inBreakerOpenFlag == FLAG_STATE_INACTIVE && IO_INBREAKER_OPEN )
		{
			flagStates.inBreakerOpenFlag = FLAG_STATE_ACTIVE;
			FLAG_logFlag( FLAG_CODE_IN_BREAKER_OPEN, 1 );
		}
		else if ( flagStates.inBreakerOpenFlag == FLAG_STATE_ACTIVE && !IO_INBREAKER_OPEN )
		{
			flagStates.inBreakerOpenFlag = FLAG_STATE_INACTIVE;
			FLAG_logFlag( FLAG_CODE_IN_BREAKER_OPEN, 0 );
		}
		if ( flagStates.inBreakerOpenFlag == FLAG_STATE_ACTIVE ) flag.flagBitfield |= (1ull<<FLAG_CODE_IN_BREAKER_OPEN);
		else flag.flagBitfield &= ~(1ull<<FLAG_CODE_IN_BREAKER_OPEN);
		if ( flagStates.inBreakerOpenFlag == FLAG_STATE_ACTIVE && CFG_remoteCfg.inBreakerOpenFlag.mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<FLAG_CODE_IN_BREAKER_OPEN);
		else flag.flagBitfieldRelay &= ~(1ull<<FLAG_CODE_IN_BREAKER_OPEN);
	}
	if ( CFG_remoteCfg.outBreakerOpenFlag.mode != FLAG_DISABLED )
	{
		if ( flagStates.outBreakerOpenFlag == FLAG_STATE_INACTIVE && IO_OUTBREAKER_OPEN )
		{
			flagStates.outBreakerOpenFlag = FLAG_STATE_ACTIVE;
			FLAG_logFlag( FLAG_CODE_OUT_BREAKER_OPEN, 1 );
		}
		else if ( flagStates.outBreakerOpenFlag == FLAG_STATE_ACTIVE && !IO_OUTBREAKER_OPEN )
		{
			flagStates.outBreakerOpenFlag = FLAG_STATE_INACTIVE;
			FLAG_logFlag( FLAG_CODE_OUT_BREAKER_OPEN, 0 );
		}
		if ( flagStates.outBreakerOpenFlag == FLAG_STATE_ACTIVE ) flag.flagBitfield |= (1ull<<FLAG_CODE_OUT_BREAKER_OPEN);
		else flag.flagBitfield &= ~(1ull<<FLAG_CODE_OUT_BREAKER_OPEN);
		if ( flagStates.outBreakerOpenFlag == FLAG_STATE_ACTIVE && CFG_remoteCfg.outBreakerOpenFlag.mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<FLAG_CODE_OUT_BREAKER_OPEN);
		else flag.flagBitfieldRelay &= ~(1ull<<FLAG_CODE_OUT_BREAKER_OPEN);
	}
	if ( CFG_remoteCfg.tempSenseFaultFlag.mode != FLAG_DISABLED )
	{
		if ( flagStates.tempSenseFaultFlag == FLAG_STATE_INACTIVE && meas.batTemprFault )
		{
			flagStates.tempSenseFaultFlag = FLAG_STATE_ACTIVE;
			FLAG_logFlag( FLAG_CODE_TEMP_SENSE_FAULT, 1 );
		}
		else if ( flagStates.tempSenseFaultFlag == FLAG_STATE_ACTIVE && !meas.batTemprFault )
		{
			flagStates.tempSenseFaultFlag = FLAG_STATE_INACTIVE;
			FLAG_logFlag( FLAG_CODE_TEMP_SENSE_FAULT, 0 );
		}
		if ( flagStates.tempSenseFaultFlag == FLAG_STATE_ACTIVE ) flag.flagBitfield |= (1ull<<FLAG_CODE_TEMP_SENSE_FAULT);
		else flag.flagBitfield &= ~(1ull<<FLAG_CODE_TEMP_SENSE_FAULT);
		if ( flagStates.tempSenseFaultFlag == FLAG_STATE_ACTIVE && CFG_remoteCfg.tempSenseFaultFlag.mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<FLAG_CODE_TEMP_SENSE_FAULT);
		else flag.flagBitfieldRelay &= ~(1ull<<FLAG_CODE_TEMP_SENSE_FAULT);
	}
	for ( ii = 0; ii < SAFETY_NUM_SD_REASONS; ii++ )		
	{
		if ( (ii > SAFETY_SD_BIT_OUTVOLT) || (CFG_remoteCfg.sdFlags[ii].mode != FLAG_DISABLED) ) //The last 2 SAFETY_SD_REASONs, CASETMP and FLAG, is not in the cfg as of revision 133, so don't check if its enabled/disabled.
		{
			if ( SAFETY_isShutdown( ii ) )
			{
				if ( flagStates.sdFlags[ii] == FLAG_STATE_INACTIVE )
				{
					FLAG_logFlag( FLAG_CODE_FIRST_SD + ii, 1 );
				}
				flagStates.sdFlags[ii] = FLAG_STATE_ACTIVE;
			}
			else
			{
				if ( flagStates.sdFlags[ii] == FLAG_STATE_ACTIVE )
				{
					FLAG_logFlag( FLAG_CODE_FIRST_SD + ii, 0 );
				}
				flagStates.sdFlags[ii] = FLAG_STATE_INACTIVE;
			}
			if ( flagStates.sdFlags[ii] == FLAG_STATE_ACTIVE ) flag.flagBitfield |= (1ull<<(FLAG_CODE_FIRST_SD+ii));
			else flag.flagBitfield &= ~(1ull<<(FLAG_CODE_FIRST_SD+ii));
			if ( ii <= SAFETY_SD_BIT_OUTVOLT && flagStates.sdFlags[ii] == FLAG_STATE_ACTIVE && CFG_remoteCfg.sdFlags[ii].mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<(FLAG_CODE_FIRST_SD+ii));
			else flag.flagBitfieldRelay &= ~(1ull<<(FLAG_CODE_FIRST_SD+ii));
		}
	}

	if ( CFG_remoteCfg.logFullFlag.mode != FLAG_DISABLED )
	{
		/// RDDtemp flagStates.logFullFlag = ( TELEM_isFull() ) ? FLAG_STATE_ACTIVE : FLAG_STATE_INACTIVE;
		if ( flagStates.logFullFlag == FLAG_STATE_ACTIVE ) flag.flagBitfield |= (1ull<<FLAG_CODE_LOG_FULL);
		else flag.flagBitfield &= ~(1ull<<FLAG_CODE_LOG_FULL);
		if ( flagStates.logFullFlag == FLAG_STATE_ACTIVE && CFG_remoteCfg.logFullFlag.mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<FLAG_CODE_LOG_FULL);
		else flag.flagBitfieldRelay &= ~(1ull<<FLAG_CODE_LOG_FULL);
	}
	
	//if ( CFG_remoteCfg.panelMissingFlag.mode != FLAG_DISABLED ) //Removed for code space.
	//{
	//	flagStates.panelMissingFlag.currentVal = meas.pvOcVolt.val;
	//	FLAG_checkFlagSched( &flagStates.panelMissingFlag, &CFG_remoteCfg.panelMissingFlag, FLAG_CODE_PANEL_MISSING );
	//}
	
	if ( 1 ) //( CFG_remoteCfg.badCfgRange.mode != FLAG_DISABLED )
	{
		if ( flagStates.badCfgRangeFlag == FLAG_STATE_INACTIVE && !CFG_configRangesOk() )
		{
			flagStates.badCfgRangeFlag = FLAG_STATE_ACTIVE;
			FLAG_logFlag( FLAG_CODE_BAD_CFG_RANGE, 1 );
		}
		else if ( flagStates.badCfgRangeFlag == FLAG_STATE_ACTIVE && CFG_configRangesOk() )
		{
			flagStates.badCfgRangeFlag = FLAG_STATE_INACTIVE;
			FLAG_logFlag( FLAG_CODE_BAD_CFG_RANGE, 0 );
		}
		if ( flagStates.badCfgRangeFlag == FLAG_STATE_ACTIVE ) flag.flagBitfield |= (1ull<<FLAG_CODE_BAD_CFG_RANGE);
		else flag.flagBitfield &= ~(1ull<<FLAG_CODE_BAD_CFG_RANGE);
		if ( flagStates.badCfgRangeFlag == FLAG_STATE_ACTIVE && CFG_remoteCfg.outBreakerOpenFlag.mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<FLAG_CODE_BAD_CFG_RANGE);
		else flag.flagBitfieldRelay &= ~(1ull<<FLAG_CODE_BAD_CFG_RANGE);
	}

#ifndef DEBUG_LED_SAMPLE
#ifdef DBG_HARDCODED_VIN_SETPOINT
	// Ignore relay events for turbine. Turn relay on when in bulk mode.
	IO_setRelay( CTRL_setpointIsBulk() && !CTRL_isTurbineLoadEnabled() );
#else
	/// RDDtemp IO_setRelay( flag.flagBitfieldRelay );
#endif
#endif
}

// Returns > 0 if state changed as a result of this check
int FLAG_checkFlagTrig( FlagState * pState, FlagCfgTrig * pCfg, FLAG_Code code )
{
	Time currentTime = TIME_get();
	int isPosTrig;
	int didChange = 0;

	isPosTrig = ( pState->triggerVal >= pState->resetVal );
	
	if ( ( ( pState->currentVal >= pState->triggerVal ) && isPosTrig ) || ( ( pState->currentVal <= pState->triggerVal ) && !isPosTrig ) )
	{
		if ( pState->currentState == FLAG_STATE_INACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST )
		{
			pState->currentState = FLAG_STATE_ACTIVE_HYST;
			pState->hystStartTime = currentTime;
		}
		else if ( pState->currentState == FLAG_STATE_ACTIVE_HYST && currentTime >= ( pState->hystStartTime + pCfg->hystTime ) )
		{
			pState->currentState = FLAG_STATE_ACTIVE;
			FLAG_logFlag( code, 1 );
			didChange = 1;	
		}
	}
	if ( ( ( pState->currentVal <= pState->resetVal ) && isPosTrig ) || ( ( pState->currentVal >= pState->resetVal ) && !isPosTrig ) )
	{
		if ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_ACTIVE_HYST )
		{
			pState->currentState = FLAG_STATE_INACTIVE_HYST;
			pState->hystStartTime = currentTime;
		}
		else if ( pState->currentState == FLAG_STATE_INACTIVE_HYST && currentTime >= ( pState->hystStartTime + pCfg->hystTime ) )
		{
			pState->currentState = FLAG_STATE_INACTIVE;
			FLAG_logFlag( code, 0 );
			didChange = 1;	
		}
	}

	if ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST ) flag.flagBitfield |= (1ull<<code);
	else flag.flagBitfield &= ~(1ull<<code);

	if ( ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST ) && pCfg->mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<code);
	else flag.flagBitfieldRelay &= ~(1ull<<code);

	return didChange;
}

// Returns > 0 if state changed as a result of this check
int FLAG_checkFlagHold( FlagState * pState, FlagCfgHold * pCfg, FLAG_Code code )
{
	Time currentTime = TIME_get();
	int isPosTrig;
	int didChange = 0;

	// Hold time
	if ( pState->currentState == FLAG_STATE_ACTIVE && currentTime < ( pState->hystStartTime + pCfg->holdTime ) ) return 0;

	isPosTrig = ( pState->triggerVal >= pState->resetVal );
	
	if ( ( ( pState->currentVal >= pState->triggerVal ) && isPosTrig ) || ( ( pState->currentVal <= pState->triggerVal ) && !isPosTrig ) )
	{
		if ( pState->currentState == FLAG_STATE_INACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST )
		{
			pState->currentState = FLAG_STATE_ACTIVE_HYST;
			pState->hystStartTime = currentTime;
		}
		else if ( pState->currentState == FLAG_STATE_ACTIVE_HYST && currentTime >= ( pState->hystStartTime + pCfg->hystTime ) )
		{
			pState->currentState = FLAG_STATE_ACTIVE;
			FLAG_logFlag( code, 1 );
			didChange = 1;	
		}
	}
	if ( ( ( pState->currentVal <= pState->resetVal ) && isPosTrig ) || ( ( pState->currentVal >= pState->resetVal ) && !isPosTrig ) )
	{
		if ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_ACTIVE_HYST )
		{
			pState->currentState = FLAG_STATE_INACTIVE_HYST;
			pState->hystStartTime = currentTime;
		}
		else if ( pState->currentState == FLAG_STATE_INACTIVE_HYST && currentTime >= ( pState->hystStartTime + pCfg->hystTime ) )
		{
			pState->currentState = FLAG_STATE_INACTIVE;
			FLAG_logFlag( code, 0 );
			didChange = 1;	
		}
	}

	if ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST ) flag.flagBitfield |= (1<<code);
	else flag.flagBitfield &= ~(1<<code);

	if ( ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST ) && pCfg->mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<code);
	else flag.flagBitfieldRelay &= ~(1ull<<code);

	return didChange;
}

// Returns > 0 if state changed as a result of this check
int FLAG_checkFlagSched( FlagState * pState, FlagCfgSched * pCfg, FLAG_Code code )
{
	int didChange = 0;
	TimeShort timeSinceToday = TIME_getSinceToday();

	if ( TIME_isToday( pState->hystStartTime ) ) // Use hystStartTime to record when the flag was last checked
	{
		// We've checked this flag today, and would have logged it if active, so make it inactive now no matter what
		//pState->currentState = FLAG_STATE_INACTIVE;
	}
	else if ( ( timeSinceToday >= pCfg->checkTime ) && ( timeSinceToday < ( pCfg->checkTime + 10000 ) ) )
	{
		pState->currentState = ( pState->currentVal < pState->triggerVal ) ? FLAG_STATE_ACTIVE : FLAG_STATE_INACTIVE;
		pState->hystStartTime = TIME_get();
		if ( pState->currentState == FLAG_STATE_ACTIVE )
		{
			FLAG_logFlag( code, 1 );
			didChange = 1;
		}
		//else if ( pState->currentState == FLAG_STATE_INACTIVE )
		//{
		//	FLAG_logFlag( code, 0 );
		//	didChange = 1;
		//}

		//COMMS_sendDebugPacket( timeSinceToday >> 16, timeSinceToday, pCfg->checkTime >> 16, pCfg->checkTime );
		//COMMS_sendDebugPacket( pState->currentState, didChange, pState->currentVal, pState->triggerVal );
	}
	else if ( timeSinceToday > ( pCfg->checkTime + 10000 ) )
	{
		// Time is before check time, so make flag inactive
		pState->currentState = FLAG_STATE_INACTIVE;
	}

	if ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST ) flag.flagBitfield |= (1<<code);
	else flag.flagBitfield &= ~(1<<code);

	if ( ( pState->currentState == FLAG_STATE_ACTIVE || pState->currentState == FLAG_STATE_INACTIVE_HYST ) && pCfg->mode == FLAG_LOG_AND_RELAY ) flag.flagBitfieldRelay |= (1ull<<code);
	else flag.flagBitfieldRelay &= ~(1ull<<code);

	return didChange;
}

void FLAG_logFlag( FLAG_Code code, int isActive )
{
	unsigned int ii;
	if ( flag.isReadyToCheck )
	{
		//Time tm = 0x0123456789ABCDEF;
		Time tm = ( TIME_get() & 0x00007FFFFFFFFFFF );
		
		if ( isActive ) tm |=	 0x0000800000000000; // This indicates that the activation of a flag is being logged, rather than the inactivation

		for ( ii = 0; ii < FLAG_BUFFER_ENTRY_SIZE; ii++ )
		{
			flag.buffer[code*FLAG_BUFFER_ENTRY_SIZE+ii] = *( (unsigned char *)&tm + ii );
		}
	}
}

void FLAG_checkAndWrite()
{
	unsigned char tempFlagLog[FLAG_LOG_ENTRY_SIZE];
	FLAG_Code code;
	unsigned int ii;

	FLAG_checkAllFlags();

	if ( !flag.isReadyToWrite || flag.isErasing ) return;

	// Swap sectors if we've filled this one up
	if ( flag.currentAddr - flag.activeSectorAddr >= BLOCK_SIZE )
	{
		if ( FLAG_swapActiveSector() < 0 )
		{
			// Active sector swap failed, try again next time
			return;
		}
	}

	//COMMS_sendDebugPacket( *(unsigned int *)(flag.buffer + FLAG_CODE_POWERON*FLAG_BUFFER_ENTRY_SIZE) );

	// Check all the flag buffer entries and write any active flags to flash
	for ( code = FLAG_CODE_MIN; code <= FLAG_CODE_MAX; code++ )
	{
		if (		*(unsigned int *)(flag.buffer + code*FLAG_BUFFER_ENTRY_SIZE) != 0xFFFF
				||	*(unsigned int *)(flag.buffer + code*FLAG_BUFFER_ENTRY_SIZE + 2) != 0xFFFF
				||	*(unsigned int *)(flag.buffer + code*FLAG_BUFFER_ENTRY_SIZE + 4) != 0xFFFF )
		{
			// This flag is active, so write to flash
			if ( FLASH_startWrite( flag.currentAddr, FLAG_LOG_ENTRY_SIZE ) >= 0 )
			{
				*(FLAG_Code *)tempFlagLog = code;
				for ( ii = 0; ii < FLAG_BUFFER_ENTRY_SIZE; ii++ )
				{
					tempFlagLog[ii+sizeof(FLAG_Code)] = flag.buffer[code*FLAG_BUFFER_ENTRY_SIZE+ii];
				}
				if ( FLASH_writeStr( tempFlagLog, FLAG_LOG_ENTRY_SIZE ) >= 0 )
				{
					// Make the flag inactive again once it has been written
					for ( ii = 0; ii < FLAG_BUFFER_ENTRY_SIZE; ii++ )
					{
						flag.buffer[code*FLAG_BUFFER_ENTRY_SIZE+ii] = 0xFF;
					}
					flag.currentAddr += FLAG_LOG_ENTRY_SIZE;

					//COMMS_sendDebugPacket( code );
				}
			}

			// Only write one flag per call, so return now
			return;
		}
	}
}

void FLAG_eraseStarted()
{
	flag.isErasing = 1;
}

void FLAG_eraseDoneCallback(int retVal)
{
	int ii;

	flag.isErasing = 0;

	if ( retVal >= 0 )
	{
		// Reset to start of first sector
		flag.activeSectorNum = 0;
		flag.activeSectorAddr = FLASH_FLAG_ADDR;
		flag.currentAddr = flag.activeSectorAddr;

		// Re-trigger logging of currently active flags
		if ( flagStates.systemInitFlag == FLAG_STATE_ACTIVE )						FLAG_logFlag( FLAG_CODE_SYSTEM_INIT, 1 );
		if ( flagStates.lowOutVoltWarnFlag.currentState == FLAG_STATE_ACTIVE )		FLAG_logFlag( FLAG_CODE_LOW_OUT_VOLT_WARN, 1 );
		if ( flagStates.lowOutVoltFaultFlag.currentState == FLAG_STATE_ACTIVE )		FLAG_logFlag( FLAG_CODE_LOW_OUT_VOLT_FAULT, 1 );
		if ( flagStates.lowOutVoltGensetFlag.currentState == FLAG_STATE_ACTIVE )	FLAG_logFlag( FLAG_CODE_LOW_OUT_VOLT_GENSET, 1 );
		if ( flagStates.highOutVoltFaultFlag.currentState == FLAG_STATE_ACTIVE )	FLAG_logFlag( FLAG_CODE_HIGH_OUT_VOLT_FAULT, 1 );
		if ( flagStates.highOutCurrFaultFlag.currentState == FLAG_STATE_ACTIVE )	FLAG_logFlag( FLAG_CODE_HIGH_OUT_CURR_FAULT, 1 );
		if ( flagStates.highDisCurrFaultFlag.currentState == FLAG_STATE_ACTIVE )	FLAG_logFlag( FLAG_CODE_HIGH_DIS_CURR_FAULT, 1 );
		if ( flagStates.highTempFaultFlag.currentState == FLAG_STATE_ACTIVE )		FLAG_logFlag( FLAG_CODE_HIGH_TEMP_FAULT, 1 );
		if ( flagStates.inBreakerOpenFlag == FLAG_STATE_ACTIVE )					FLAG_logFlag( FLAG_CODE_IN_BREAKER_OPEN, 1 );
		if ( flagStates.outBreakerOpenFlag == FLAG_STATE_ACTIVE )					FLAG_logFlag( FLAG_CODE_OUT_BREAKER_OPEN, 1 );
		if ( flagStates.tempSenseFaultFlag == FLAG_STATE_ACTIVE )					FLAG_logFlag( FLAG_CODE_TEMP_SENSE_FAULT, 1 );
		for ( ii = 0; ii < SAFETY_NUM_SD_REASONS; ii++ )
		{
			if ( flagStates.sdFlags[ii] == FLAG_STATE_ACTIVE )						FLAG_logFlag( (FLAG_Code)(FLAG_CODE_FIRST_SD + ii), 1 );
		}
		if ( flagStates.logFullFlag == FLAG_STATE_ACTIVE )							FLAG_logFlag( FLAG_CODE_LOG_FULL, 1 );
		if ( flagStates.panelMissingFlag.currentState == FLAG_STATE_ACTIVE )		FLAG_logFlag( FLAG_CODE_PANEL_MISSING, 1 );
		if ( flagStates.badCfgRangeFlag == FLAG_STATE_ACTIVE )						FLAG_logFlag( FLAG_CODE_BAD_CFG_RANGE, 1 );
	}
}

unsigned long FLAG_getFlagLen()
{
	return ( FLASH_NUM_FLAG_BLOCKS * BLOCK_SIZE );
}

