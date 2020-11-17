
//-------------------------------------------------------------------
// File: meas.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef MEAS_H
#define MEAS_H

#include "debug.h"
#include "variant.h"
#include "adc.h"

// Measurement bases
// =================
/*	
	// Old setup with 430k in Pv sense
	//pvVolt meas:		Vpvadc		= 10 / ( 430 + 390 + 10 ) * 1 / ( 1 + 1 ) * Vpv
	//				=>	Vpvmax		= 3.3 * 2 * ( 430 + 390 + 10 ) / 10 / 1
	//								= 547.8v
	//				=>	Vpvbase		= 550v
	pvVolt meas:		Vpvadc		= 10 / ( 390 + 10 ) * 1 / ( 1 + 1 ) * Vpv
					=>	Vpvmax		= 3.3 * 2 * ( 390 + 10 ) / 10 / 1
									= 264V
					=>	Vpvbase		= 270v
	HV setup with 220k & 220k
	//pvVolt meas:		Vpvadc		= 10 / ( 220 + 220 + 10 ) * 1 / ( 1 + 1 ) * Vpv
	//				=>	Vpvmax		= 3.3 * 2 * ( 220 + 220 + 10 ) / 10 / 1
	//								= 297V
	//				=>	Vpvbase		= 300v
	outVolt meas:		Voutadc		= 27 / ( 330 + 330 + 27 ) * 1 / ( 1 + 2.2 ) * Vout
					=>	Voutmax		= 3.3 * 3.2 * ( 330 + 330 + 27 ) / 27 / 1
									= 268.69
					=>	Voutbase	= 270v
	curr meas			Viadc		= 0.02 * 3.3/5 * I + 1.65v
					=>	Imax		= (3.3-1.65) / ( 0.02 * 3.3/5 )
									= 125A
					half max ADC counts will be subtracted automatically, so 0.5 pu adc reading should map to Imax, i.e
					=>	Ibase		= 2 * 125A
									= 250A
*/

#define MEAS_PVVOLT_BASE		300.0 //270.0//550.0
#define MEAS_OUTVOLT_BASE		255.0 //320 SR
#define MEAS_PVCURR_BASE		265.0 //125.0 needed for 50A sensors (250.0 for 100A sensors)
#define MEAS_OUTCURR_BASE		265.0 
#define MEAS_PVPOWER_BASE		( MEAS_PVVOLT_BASE * MEAS_PVCURR_BASE )
#define MEAS_TEMPR_BASE			100.0
#define MEAS_CASETEMPR_BASE		160.0
#define MEAS_12V_BASE			12.0
#define MEAS_3V3_BASE			3.3

#define MEAS_PVVOLT_IQBASE	( MEAS_PVVOLT_BASE / 4096.0 )
#define MEAS_OUTVOLT_IQBASE	( MEAS_OUTVOLT_BASE / 4096.0 )
#define MEAS_PVCURR_IQBASE	( MEAS_PVCURR_BASE / 4096.0 )
#define MEAS_OUTCURR_IQBASE	( MEAS_OUTCURR_BASE / 4096.0 )
#define MEAS_PVPOWER_IQBASE	( MEAS_PVPOWER_BASE / 4096.0 )
#define MEAS_TEMPR_IQBASE	( MEAS_TEMPR_BASE / 4096.0 )
#define MEAS_CASETEMPR_IQBASE	( MEAS_CASETEMPR_BASE / 4096.0 )
#define MEAS_12V_IQBASE		( MEAS_12V_BASE / 4096.0 )
#define MEAS_3V3_IQBASE		( MEAS_3V3_BASE / 4096.0 )

#define CHARGE_UPDATE_RATE_HZ	2

#define MEAS_TEMPR_NOM		( 25.0 )

#define MEAS_OUTVOLT_TO_PVVOLT	( MEAS_OUTVOLT_BASE / MEAS_PVVOLT_BASE )

#include "iqmath.h"

//extern float MEAS_outVoltBase;

typedef struct CommsMeas_
{
	Iq		val;
	float	base;
	float	valReal;
} CommsMeas;

typedef struct AdcMeas_ 
{
	Iq		offset;
	Iq		scale;
	Iq		valPreFilter;
	Iq		val;
} AdcMeas;

typedef struct AdcCommsMeas_ 
{
	Iq		offset;
	Iq		scale;
	Iq		valPreFilter;
	Iq		val;
	float	base;
	float	valReal;
} AdcCommsMeas;

typedef struct IntegralMeas_
{
	IqLong	val;
	float	base;
	float	valReal;
} IntegralMeas;

typedef struct Meas_
{
	// Measurements from ADC channels
	AdcCommsMeas	outVolt;
	AdcCommsMeas	outCurr;
	AdcCommsMeas	pvVolt;
	AdcCommsMeas	pvCurr;
	AdcMeas			rail12Volt;
	AdcMeas			flSetSense;
	AdcMeas			caseTmp;
	AdcMeas			tmpCmpSense;
	// Pv power (i.e. instantaneous pv current times pv voltage.  Seperate measurement so it can be filtered seperately, allowing for the case
	// where e.g. current and voltage both oscillate, but out of phase, so power is always low.  This would not be seen if filtered current and voltage
	// were used to generate power
	CommsMeas		pvPower;
	// Pv open circuit voltage
	CommsMeas		pvOcVolt;	
	// Tmp Cmp Temperature
	CommsMeas		batTempr;
	int				batTemprFault;

	// Case tmp Temperature
	CommsMeas		caseTempr;
	int				caseTemprFault;

	// Total daily charge integration
	IntegralMeas	outCharge;
} Meas;


extern volatile Meas meas;

void MEAS_init();
void MEAS_setDoUpdate( int doUpd );
void MEAS_update();
void MEAS_updateCharge();
void MEAS_resetCharge();
void MEAS_updateTempr();

int MEAS_isPvActive();

Iq MEAS_temprLookup( Iq rawval );
Iq MEAS_filter( Iq valNow, Iq valIn );
Iq MEAS_filterFast( Iq valNow, Iq valIn );

#endif // MEAS_H







