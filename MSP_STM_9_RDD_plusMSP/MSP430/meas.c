
//-------------------------------------------------------------------
// File: meas.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

//#include <msp430x24x.h>
#include "iqmath.h"
#include "meas.h"
#include "adc_msp.h"
#include "cfg.h"
#include "comms.h"
#include "io.h"
#include "safety.h"
#include "temp.h"
#include "limits.h"

#define CORNERFREQ			0.002
#define CORNERFREQ_FAST		0.2



volatile Meas meas;
//float MEAS_outVoltBase;
int measDoUpdate;

extern floatValue_t calculatedValue;
extern floatValue_t averageValue;
extern floatValue_t momentValue;

void MEAS_init()
{
	meas.outVolt.offset = CFG_localCfg.outVoltAdcCal.offset;
	meas.outVolt.scale = CFG_localCfg.outVoltAdcCal.scale;
	meas.outVolt.val = IQ_cnst( CFG_remoteCfg.floatVolt / MEAS_OUTVOLT_BASE );
	meas.outVolt.base = MEAS_OUTVOLT_IQBASE;

	meas.outCurr.offset = CFG_localCfg.outCurrAdcCal.offset;
	meas.outCurr.scale = CFG_localCfg.outCurrAdcCal.scale;
	meas.outCurr.val = IQ_cnst(0.0);
	meas.outCurr.base = MEAS_OUTCURR_IQBASE;

	meas.pvVolt.offset = CFG_localCfg.pvVoltAdcCal.offset;
	meas.pvVolt.scale = CFG_localCfg.pvVoltAdcCal.scale;
	meas.pvVolt.val = IQ_cnst( CFG_remoteCfg.pvMpVolt / MEAS_PVVOLT_BASE );
	meas.pvVolt.base = MEAS_PVVOLT_IQBASE;

	meas.pvCurr.offset = -CFG_localCfg.pvCurrAdcCal.offset;
	meas.pvCurr.scale = -CFG_localCfg.pvCurrAdcCal.scale;		// Current sensor direction is inverted on PCB
	meas.pvCurr.val = IQ_cnst(0.0);
	meas.pvCurr.base = MEAS_PVCURR_IQBASE;

	meas.rail12Volt.offset = CFG_localCfg.rail12VoltAdcCal.offset;
	meas.rail12Volt.scale = CFG_localCfg.rail12VoltAdcCal.scale;
	meas.rail12Volt.val = IQ_cnst(0.0);
	//meas.rail12Volt.base = MEAS_12V_IQBASE;

	meas.flSetSense.offset = CFG_localCfg.flSetSenseAdcCal.offset;
	meas.flSetSense.scale = CFG_localCfg.flSetSenseAdcCal.scale;
	meas.flSetSense.val = IQ_cnst(0.0);
	//meas.flSetSense.base = MEAS_OUTVOLT_IQBASE;

	meas.caseTmp.offset = CFG_localCfg.caseTmpAdcCal.offset;
	meas.caseTmp.scale = CFG_localCfg.caseTmpAdcCal.scale;
	meas.caseTmp.val = 2500;
	//meas.caseTmp.base = MEAS_PVVOLT_IQBASE;

	meas.tmpCmpSense.offset = CFG_localCfg.tmpCmpSenseAdcCal.offset;
	meas.tmpCmpSense.scale = CFG_localCfg.tmpCmpSenseAdcCal.scale;
	meas.tmpCmpSense.val = IQ_cnst(0.0);
	//meas.tmpCmpSense.base = MEAS_3V3_IQBASE;

	meas.pvPower.val = IQ_cnst(0.0);
	meas.pvPower.base = MEAS_PVPOWER_IQBASE;

	meas.pvOcVolt.val = IQ_cnst(0.0);
	meas.pvOcVolt.base = MEAS_PVVOLT_IQBASE;

	meas.caseTempr.val = IQ_cnst(0.0);
	meas.caseTempr.base = MEAS_CASETEMPR_IQBASE;
	meas.caseTemprFault = 0;

	meas.batTempr.val = IQ_cnst(25.0);
	meas.batTempr.base = MEAS_TEMPR_IQBASE;
	meas.batTemprFault = 0;

	meas.outCharge.val = 0;
	meas.outCharge.base = meas.outCurr.base / CHARGE_UPDATE_RATE_HZ / 3600.0;

	measDoUpdate = 1;
}


void MEAS_setDoUpdate( int doUpd )
{
	measDoUpdate = doUpd;
}

//#define ADC_READANDFILTER( cMeas, adcConvNum, doUpdate ) \
//	if ( ADC12IFG & (1<<adcConvNum) ) \
//	{ \
//		cMeas.valPreFilter = IQ_mpy( ADC12MEM##adcConvNum + cMeas.offset, cMeas.scale ); \
//		if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter ); \
//	}

#define ADC_READANDFILTER_STM( cMeas, doUpdate ) \
		cMeas.valPreFilter = cMeas.valReal / cMeas.base; \
		if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter ); 



void MEAS_update()
{
	// Read and filter all ADC channels
//	ADC_READANDFILTER( meas.outVolt, 0, measDoUpdate );
//	ADC_READANDFILTER_BIDIR( meas.outCurr, 1, measDoUpdate );
//	ADC_READANDFILTER( meas.pvVolt, 2, measDoUpdate );
//	ADC_READANDFILTER_BIDIR( meas.pvCurr, 3, measDoUpdate );
//	ADC_READANDFILTER( meas.rail12Volt, 4, measDoUpdate );
//	ADC_READANDFILTER( meas.flSetSense, 5, measDoUpdate );
//	ADC_READANDFILTER( meas.caseTmp, 6, measDoUpdate );
		
//#ifndef EXTERNAL_TEMP
//	ADC_READANDFILTER( meas.tmpCmpSense, momentValue.tmpCmp, measDoUpdate );
//#else
//	ADC_READANDFILTER_PROPER(meas.tmpCmpSense, 7, measDoUpdate);
//#endif
	
	meas.outVolt.valReal = calculatedValue.vOutSensor;
  meas.outVolt.valPreFilter = meas.outVolt.valReal / meas.outVolt.base;
	meas.outVolt.val = meas.outVolt.valPreFilter;
	

	meas.outCurr.valReal = calculatedValue.iOutSensor;
  meas.outCurr.valPreFilter = meas.outCurr.valReal / meas.outCurr.base;
	meas.outCurr.val = meas.outCurr.valPreFilter;
	
	
	
	meas.pvVolt.valReal = calculatedValue.vInSensor;
  ADC_READANDFILTER_STM( meas.pvVolt, measDoUpdate );
	

	meas.pvCurr.valReal = calculatedValue.iInSensor;
	meas.pvCurr.valPreFilter = meas.pvCurr.valReal / meas.pvCurr.base;
	meas.pvCurr.val = meas.pvCurr.valPreFilter;

	meas.rail12Volt.valPreFilter = averageValue.v12Sensor;
	meas.rail12Volt.val = calculatedValue.v12Sensor;
	
	meas.flSetSense.val = 0;	//?????
	
	meas.caseTmp.val = 0;	//?????
	
	meas.tmpCmpSense.val = 0;	//?????

	meas.pvOcVolt.valReal = 0; //?????
	
	meas.batTemprFault = 0;	//????
	
	meas.caseTempr.val = averageValue.tmpCase;
	meas.caseTempr.valReal = calculatedValue.tmpCase;
	
	meas.caseTemprFault = 0;	//?????

	// Filter pv power
	if ( measDoUpdate )
	{
		meas.pvPower.val = MEAS_filter( meas.pvPower.val, IQ_mpy( meas.pvVolt.valPreFilter, meas.pvCurr.valPreFilter ) );
	}
}

void MEAS_updateCharge()
{
	meas.outCharge.val += meas.outCurr.val;
}

void MEAS_resetCharge()
{
	meas.outCharge.val = 0;
}

int MEAS_isPvActive()
{
	return ( meas.pvOcVolt.val > IQ_mpy( IQ_cnst( 1.1 ), IQ_mpy( IQ_cnst( MEAS_OUTVOLT_TO_PVVOLT ), meas.outVolt.val ) ) );
}

#ifndef EXTERNAL_TEMP
#define BAT_TEMPR_RAW_MIN	1795
#define BAT_TEMPR_RAW_MAX	3004
#define BAT_TEMPR_RAW_FAULT 3100	// 0.78 * 4096.  This equates to -15C.  With thermistor missing, expect to measure 0.81
#define BAT_TEMPR_RAW_STEP	39		//This is for 100K beta 25/100 = 4540K
#define BAT_TEMPR_TABLE_LEN 32
// Temperature lookup table, in Iq
const Iq BAT_TEMPR_TABLE[] = 
{
    1639, 1601, 1564, 1527, 1490, 1453, 1416, 1379, 1342, 1306, 1269, 1232, 1195, 1157, 1120, 1082, 1044, 1006, 967, 928, 888, 847, 806, 765, 722, 679, 635, 589, 543, 495, 445, 394, 
};
#endif

#define CASE_TEMPR_RAW_MIN	85
#define CASE_TEMPR_RAW_MAX	2837
#define CASE_TEMPR_RAW_FAULT 3100	// 0.78 * 4096.  This equates to -15C.  With thermistor missing, expect to measure 0.81
#define CASE_TEMPR_RAW_STEP	64		//This is for 100K beta 25/100 = 4540K
#define CASE_TEMPR_TABLE_LEN 44
// Temperature lookup table, in Iq
const Iq CASE_TEMPR_TABLE[] = 
{
    3948,3380,3042,2802,2617,2465,2337,2226,2127,2038,1956,1882,1812,1747,1685,1626,1570,1517,1465,1415,1367,1319,1273,1228,1183,1139,1095,1052,1008,965,922,878,834,789,744,698,650,601,551,498,442,384,321,254,
};

void MEAS_updateTempr()
{
	Iq rawCaseTmpVal;

	int ind1, ind2;

#ifdef DIGITAL_TEMP
	unsigned int kelvin = TEMP_getValue();
	if (kelvin == UINT_MAX)
	{
		// Invalid temp
		meas.batTempr.val = IQ_cnst(25.0 / MEAS_TEMPR_BASE);
		meas.batTemprFault = 1;
	}
	else
	{
		long celcius = TEMP_getValue() - ((long)(273.15f * 64.0f));		// deg C * 64
		meas.batTempr.val = (Iq)((celcius * 4096) / 64 / 100);			// Scale to 12 bit decimal with 100 deg C base

		meas.batTemprFault = 0;
	}

#else

	Iq rawTmpCmpVal;

	//Check for tmpCmp sensor fault - do tmpCmp measurement from lookup table
	rawTmpCmpVal = meas.tmpCmpSense.val;
	

#ifndef EXTERNAL_TEMP
	if ( rawTmpCmpVal > BAT_TEMPR_RAW_FAULT )
	{
		meas.batTempr.val = IQ_cnst( 25.0 / MEAS_TEMPR_BASE );
		meas.batTemprFault = 1;
	}
	else
	{
		meas.batTemprFault = 0;

		if ( rawTmpCmpVal < BAT_TEMPR_RAW_MIN ) rawTmpCmpVal = BAT_TEMPR_RAW_MIN;
		else if ( rawTmpCmpVal > ( BAT_TEMPR_RAW_MAX - BAT_TEMPR_RAW_STEP ) ) rawTmpCmpVal = BAT_TEMPR_RAW_MAX - BAT_TEMPR_RAW_STEP;
	
		ind1 = ( rawTmpCmpVal - BAT_TEMPR_RAW_MIN ) / BAT_TEMPR_RAW_STEP;
		ind2 = ind1 + 1;

		if ( CFG_remoteCfg.tmpCmp != 0.0 ) //tmpcmp value can only be negative, setting it positive will make it be changed to zero silently by sanity checks in  CFG_checkRanges().
		{
			meas.batTempr.val = BAT_TEMPR_TABLE[ind1] 
				+ (Iq)( ( (long)( BAT_TEMPR_TABLE[ind2] - BAT_TEMPR_TABLE[ind1] ) ) * ( (long)( rawTmpCmpVal - BAT_TEMPR_RAW_MIN - ind1 * BAT_TEMPR_RAW_STEP ) ) / ( (long) BAT_TEMPR_RAW_STEP ) );
		}
		else
		{
			meas.batTempr.val = 0; 
		}
		//meas.tempr.val = IQ_cnst( 100.0 / MEAS_TEMPR_BASE );
	}
#else
	meas.batTempr.val = rawTmpCmpVal;
#endif
#endif

	meas.batTempr.valReal = meas.batTempr.val * meas.batTempr.base;


	//Check for CaseTmp sensor fault - do casetmp measurement from lookup table
	rawCaseTmpVal = meas.caseTmp.val;
	if ( rawCaseTmpVal > CASE_TEMPR_RAW_FAULT )
	{
		meas.caseTempr.val = IQ_cnst( 25.0 / MEAS_CASETEMPR_BASE );
		meas.caseTemprFault = 1;
	}
	else
	{
		meas.caseTemprFault = 0;
		
		if ( rawCaseTmpVal < CASE_TEMPR_RAW_MIN ) rawCaseTmpVal = CASE_TEMPR_RAW_MIN;
		else if ( rawCaseTmpVal > ( CASE_TEMPR_RAW_MAX - CASE_TEMPR_RAW_STEP ) ) rawCaseTmpVal = CASE_TEMPR_RAW_MAX - CASE_TEMPR_RAW_STEP;
	
		ind1 = ( rawCaseTmpVal - CASE_TEMPR_RAW_MIN ) / CASE_TEMPR_RAW_STEP;
		ind2 = ind1 + 1;

		meas.caseTempr.val = CASE_TEMPR_TABLE[ind1] 
				+ (Iq)( ( (long)( CASE_TEMPR_TABLE[ind2] - CASE_TEMPR_TABLE[ind1] ) ) * ( (long)( rawCaseTmpVal - CASE_TEMPR_RAW_MIN - ind1 * CASE_TEMPR_RAW_STEP ) ) / ( (long) CASE_TEMPR_RAW_STEP ) );
		
	}

//	COMMS_sendDebugPacket( (unsigned int) meas.batTempr.val, 0,0, 0 );
	
	meas.caseTempr.valReal = meas.caseTempr.val * meas.caseTempr.base;
}

Iq MEAS_filter( Iq valNow, Iq valIn )
{
	valIn -= valNow;
	if (IQ_abs(valIn) <= ((int)(1.0/(float)CORNERFREQ)+1)) 
	{
		if (valIn < 0) valNow--;
		else if (valIn > 0) valNow++;
	}
	else valNow += IQ_mpy( IQ_cnst(CORNERFREQ), valIn );
	return valNow;
	//return valIn;
}

Iq MEAS_filterFast( Iq valNow, Iq valIn )
{
	valIn -= valNow;
	if (IQ_abs(valIn) <= ((int)(1.0/(float)CORNERFREQ_FAST)+1)) 
	{
		if (valIn < 0) valNow--;
		else if (valIn > 0) valNow++;
	}
	else valNow += IQ_mpy( IQ_cnst(CORNERFREQ_FAST), valIn );
	return valNow;
	//return valIn;
}

IqLong	MEAS_integrate( IqLong valNow, Iq valIn )
{
	return 0;
}



