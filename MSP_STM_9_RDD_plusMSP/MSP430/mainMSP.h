
//-------------------------------------------------------------------
// File: main.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef MAINMSP_H
#define MAINMSP_H

#include "debug.h"
#include "variant.h"


void MAIN_resetAllAndStart(void);
void MAIN_resetRemoteCfg(void);

///void __inline__
void brief_pause(register unsigned int n);
extern int mainMSPinit( void );
extern int mainMSPloop( char l );
	
#endif // MAINMSP_H
