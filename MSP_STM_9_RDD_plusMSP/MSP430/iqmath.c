
//-------------------------------------------------------------------
// File: iqmath.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

///#include <msp430x24x.h>
#include <signal.h>
#include "iqmath.h"


Iq IQ_mpy( Iq a, Iq b )
{
	long temp;

	// Uses hardware multiplier
	temp = (long)a * (long)b;
	// Rounding: mid values are rounded up
	temp += IQ_K;
	// Divide by base
	return (Iq)( temp >> IQ_Q );

	/*long mpyResult;

	// Disable interrupts
	dint();

	// Load operands
	MPYS = a;
	OP2 = b;
	
	// One extra clock cycle to ensure multiple is complete regardless of addressing mode
	//__asm__ __volatile__ ( "nop" );

	// Enable interrupts again
	eint();

	// Retrieve 32-bit results
	mpyResult = ( (long)(RESHI) << 16 ) + RESLO;

	// Rounding
	mpyResult += IQ_K;

	// Adjust for IQ format and return
	return (Iq)( mpyResult >> IQ_Q );*/
}

Iq24Long IQ_mpyTo24( Iq a, Iq b )
{
	long temp;

	// Uses hardware multiplier
	temp = (long)a * (long)b;
	// Rounding: mid values are rounded up
	//temp += IQ_K;

	return temp;
}

Iq IQ_div( Iq a, Iq b )
{
	long temp;

	// Multiply by base
	temp = ((long)a) << IQ_Q;
	// Do division
	return (Iq)( temp / (long)b );
}

