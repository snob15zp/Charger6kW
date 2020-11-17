
//-------------------------------------------------------------------
// File: mult.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

/*#include <msp430x24x.h>
#include "mult.h"

int MULT_s16( int a, int b )
{
	// Disable interrupts
	dint();

	// Load operands
	MPYS = a;
	OP2 = b;
	
	// One extra clock cycle to ensure multiple is complete regardless of addressing mode
	//__asm__ __volatile__ ( "nop" );

	// Enable interrupts again
	eint();

	// Return high word of result

}*/
