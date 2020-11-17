
//-------------------------------------------------------------------
// File: iqmath.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef IQMATH_H
#define IQMATH_H

#include "debug.h"

#define IQ_Q	12
#define IQ_K	(1 << (IQ_Q-1))

typedef int Iq;
typedef long int IqLong;
typedef long int Iq24Long;
typedef long long int Iq24LongLong;

#define IQ_MIN 0x8000 	
#define IQ_MAX 0x7FFF

#define IQ_cnst(A) ( (Iq) ((A) * 4096.0) )

#define IQ_abs(A) ( ((A)<0)?(-(A)):(A) )
#define IQ_sign(A) ( ((A)==0)?(0):(((A)<0)?(-1):(1)) )
#define IQ_min(A,B) ( ((A)<(B))?(A):(B) ) 
#define IQ_max(A,B) ( ((A)>(B))?(A):(B) ) 

Iq IQ_mpy( Iq a, Iq b );
Iq24Long IQ_mpyTo24( Iq a, Iq b );

Iq IQ_div( Iq a, Iq b );

#endif // IQMATH_H

