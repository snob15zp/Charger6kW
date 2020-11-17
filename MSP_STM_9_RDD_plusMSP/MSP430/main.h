
//-------------------------------------------------------------------
// File: main.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef MAIN_H
#define MAIN_H

#include "debug.h"
#include "variant.h"


void MAIN_resetAllAndStart();
void MAIN_resetRemoteCfg();

//void __inline__ brief_pause(register unsigned int n);

#endif // MAIN_H
