
//-------------------------------------------------------------------
// File: flag.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef FLAG_H
#define FLAG_H

#include "debug.h"
#include "iqmath.h"
#include "time.h"

typedef unsigned long long FlagBitfield;

typedef enum FLAG_Mode_
{
	FLAG_DISABLED = 0,
	FLAG_LOG,
	FLAG_LOG_AND_RELAY
} FLAG_Mode;

// Structure for configuring a simple flag
typedef struct FlagCfgSimple_
{
	FLAG_Mode mode;
} FlagCfgSimple;

// Structure for configuring a triggered flag with both value and time hysterysis
typedef struct FlagCfgTrig_
{
	FLAG_Mode mode;
	float triggerVal;
	float resetVal;
	TimeShort hystTime;
} FlagCfgTrig;

// Structure for configuring a triggered flag with value hysterysis and a minimum hold time
typedef struct FlagCfgHold_
{
	FLAG_Mode mode;
	float triggerVal;
	float resetVal;
	TimeShort hystTime;
	TimeShort holdTime;
} FlagCfgHold;

// Structure for configuring a scheduled one-check-per-day flag
typedef struct FlagCfgSched_
{
	FLAG_Mode mode;
	float triggerVal;
	TimeShort checkTime;
} FlagCfgSched;

typedef enum FLAG_Code_
{
	FLAG_CODE_MIN = 0,
	FLAG_CODE_SYSTEM_INIT = FLAG_CODE_MIN,
	FLAG_CODE_LOW_OUT_VOLT_WARN,
	FLAG_CODE_LOW_OUT_VOLT_FAULT,
	FLAG_CODE_LOW_OUT_VOLT_GENSET,
	FLAG_CODE_HIGH_OUT_VOLT_FAULT,
	FLAG_CODE_HIGH_OUT_CURR_FAULT,
	FLAG_CODE_HIGH_DIS_CURR_FAULT,
	FLAG_CODE_HIGH_TEMP_FAULT,
	FLAG_CODE_IN_BREAKER_OPEN,
	FLAG_CODE_OUT_BREAKER_OPEN,
	FLAG_CODE_TEMP_SENSE_FAULT,
	FLAG_CODE_PVCURR_NEG_SD,
	FLAG_CODE_PVCURR_POS_SD,
	FLAG_CODE_PVVOLT_SD,
	FLAG_CODE_OUTCURR_POS_SD,
	FLAG_CODE_OUTVOLT_SD,
	FLAG_CODE_CASETMP_SD,
	FLAG_CODE_FAN_SD,
	FLAG_CODE_LOG_FULL,
	FLAG_CODE_PANEL_MISSING,
	FLAG_CODE_BAD_CFG_RANGE,
	FLAG_CODE_MAX = FLAG_CODE_BAD_CFG_RANGE
} FLAG_Code;
enum { FLAG_CODE_NUM = (FLAG_CODE_MAX - FLAG_CODE_MIN) + 1 };

#define FLAG_CODE_FIRST_SD FLAG_CODE_PVCURR_NEG_SD

typedef enum FLAG_State_
{
	FLAG_STATE_INACTIVE,
	FLAG_STATE_ACTIVE,
	FLAG_STATE_INACTIVE_HYST,
	FLAG_STATE_ACTIVE_HYST
} FLAG_State;

typedef struct FlagState_
{
	FLAG_State currentState;
	Iq currentVal;
	Iq triggerVal;
	Iq resetVal;
	Time hystStartTime;
} FlagState;


void FLAG_init(void);
void FLAG_initTrigs(void);
void FLAG_timeHasUpdated(void);

unsigned char FLAG_getStatus(void);

FlagBitfield FLAG_getFlagBitfield(void);

void FLAG_checkAllFlags(void); 

int FLAG_checkFlagTrig( FlagState * pState, FlagCfgTrig * pCfg, FLAG_Code code );
int FLAG_checkFlagHold( FlagState * pState, FlagCfgHold * pCfg, FLAG_Code code );
int FLAG_checkFlagSched( FlagState * pState, FlagCfgSched * pCfg, FLAG_Code code );

void FLAG_logFlag( FLAG_Code code, int isActive );

void FLAG_checkAndWrite(void);

void FLAG_eraseStarted(void);
void FLAG_eraseDoneCallback(int retVal);

unsigned long FLAG_getFlagLen(void);

#endif // FLAG_H

