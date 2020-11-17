
//-------------------------------------------------------------------
// File: status.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-08-18: original
//-------------------------------------------------------------------

#ifndef STATUS_H
#define STATUS_H

typedef unsigned char Status;

void STATUS_init();

void STATUS_update();

unsigned long long STATUS_getStatus();


#endif // STATUS_H
