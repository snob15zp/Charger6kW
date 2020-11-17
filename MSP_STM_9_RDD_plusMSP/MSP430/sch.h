
//-------------------------------------------------------------------
// File: sch.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef SCH_H
#define SCH_H

#include "debug.h"

void SCH_init();
void SCH_incrMs();
void SCH_runActiveTasks();

#endif // SCH_H

