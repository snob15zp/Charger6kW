
//-------------------------------------------------------------------
// File: sch.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#include "main.h"
#include "sch.h"
#include "comms.h"
#include "meas.h"
#include "time.h"
#include "telem.h"
#include "io.h"
#include "ctrl.h"
#include "can.h"
#include "flag.h"
#include "stats.h"
#include "safety.h"
#include "usci.h"
#include "lcd.h"
#include "temp.h"


typedef struct TaskDef_
{
	int period_ms;
	int counter_ms;			// Inialise to > 0 for offset
	void (*pFunc)(void);
} TaskDef;

#if (AER_PRODUCT_ID == AER05_RACK)
TaskDef tasks[] = 
{	
	{	(int)(1000.0 / CHARGE_UPDATE_RATE_HZ),	20,		MEAS_updateCharge },
	{	2000,									10,		MEAS_updateTempr },
	{	2000,									1010,	CTRL_calcOutVoltSetpoints },
	{	100,									60,		STATS_updateAll	},
	{	200,									0,		FLAG_checkAndWrite },
	{	TELEM_BASE_PERIOD_MS,					100,	TELEM_logIfPeriodElapsed },
	{	CTRL_SLOW_PERIOD_MS,					80,		CTRL_checkBulkFloat },
	{	1000,									140,	COMMS_sendHeartbeat },
	{	1000,									240,	COMMS_sendStatus },
	{	1000,									340,	COMMS_sendPvMeas },
	{	1000,									440,	COMMS_sendOutMeas },
	{	1000,									540,	COMMS_sendOcQMeas },
	{	1000,									640,	COMMS_sendPowTemprMeas },
	{	1000,									740,	COMMS_sendFlag },
	{	1000,									840,	COMMS_sendTime },
	{	1000,									940,	COMMS_sendOutVoltCmd },
	{	2,										0,		COMMS_sendP2pPacket },
	{	300,									0,		SAFETY_toggleRedLed },
	{	500,									0,		IO_toggleGreenLed }
};
#elif (AER_PRODUCT_ID == AER07_WALL)
TaskDef tasks[] = 
{	
	{	(int)(1000.0 / CHARGE_UPDATE_RATE_HZ),	20,		MEAS_updateCharge },
	{	2000,									10,		MEAS_updateTempr },
	{	2000,									1010,	CTRL_calcOutVoltSetpoints },
	{	100,									60,		STATS_updateAll	},
	{	200,									0,		FLAG_checkAndWrite },
	{	TELEM_BASE_PERIOD_MS,					100,	TELEM_logIfPeriodElapsed },
	{	CTRL_SLOW_PERIOD_MS,					80,		CTRL_checkBulkFloat },
	{	1000,									140,	COMMS_sendHeartbeat },
	{	1000,									240,	COMMS_sendStatus },
	{	2000,									340,	COMMS_sendPvMeas },
	{	2000,									440,	COMMS_sendOutMeas },
	{	2000,									540,	COMMS_sendOcQMeas },
	{	2000,									640,	COMMS_sendPowTemprMeas },
	{	2000,									740,	COMMS_sendFlag },
	{	2000,									840,	COMMS_sendTime },
	{	2000,									940,	COMMS_sendOutVoltCmd },
	{	LCD_PERIOD_MS,							0,		lcd_update },
	{	2,										0,		COMMS_sendP2pPacket },
	{	1,										1000,	IO_fanDrvPWM },
	{	2000,									2000,	IO_fanSetSpeed }
};
#endif


unsigned int numTasks;
volatile unsigned long timeSinceLastRun_ms;

void SCH_init()
{
	timeSinceLastRun_ms = 0;
	numTasks = sizeof( tasks ) / sizeof( tasks[0] );
}

void SCH_incrMs() //RDD ToDo link with systimer
{
	timeSinceLastRun_ms++;
}

void SCH_runActiveTasks()
{
	unsigned long timeSinceLastRunLocal_ms;
	int ii;
	
	// Store the time since last run, then zero it
	timeSinceLastRunLocal_ms = timeSinceLastRun_ms;
	timeSinceLastRun_ms = 0;
	
	// Run all events that should be run given elapsed time
	for ( ii = 0; ii < numTasks; ii++ )
	{
		tasks[ii].counter_ms -= timeSinceLastRunLocal_ms;
		if ( tasks[ii].counter_ms <= 0 )
		{
			tasks[ii].pFunc();
			while( tasks[ii].counter_ms <= 0 ) tasks[ii].counter_ms += tasks[ii].period_ms;
		}
	}
}
