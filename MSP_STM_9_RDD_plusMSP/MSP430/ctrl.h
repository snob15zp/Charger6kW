
//-------------------------------------------------------------------
// File: ctrl.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef CONTROL_H
#define CONTROL_H

#include "debug.h"

#define CTRL_SLOW_PERIOD_MS		100
#define CTRL_BULK_HYSTERISIS_MS 2000

void CTRL_init();

void CTRL_setFltrimParams_old();
void CTRL_setFltrimParams_new();

void CTRL_setOutVoltCmd( float outVoltCmd );
float CTRL_getOutVoltCmd();

void CTRL_calcOutVoltSetpoints();

void CTRL_enableOutput(int enable);
int CTRL_isOutputEnabled();
#ifdef DBG_HARDCODED_VIN_SETPOINT
int CTRL_isTurbineLoadEnabled();
#endif

void CTRL_tick();

void CTRL_checkBulkFloat();
int CTRL_setpointIsBulk();
void CTRL_controlSyncRect();

void CTRL_vmpHigh();


unsigned char CTRL_getStatus();

void CTRL_temp();

#endif // CONTROL_H
