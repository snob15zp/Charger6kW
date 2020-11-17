
//-------------------------------------------------------------------
// File: adc.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef ADC_H
#define ADC_H

#include "debug.h"
#include "meas.h"
//#include "main.h"
//#include "stm32f3xx.h"

void ADC_init();

// Macro to read and filter ADC values
//#define ADC_READANDFILTER( cMeas, adcConvNum, doUpdate ) \
//	if ( ADC12IFG & (1<<adcConvNum) )\
//	{ 
//		cMeas.valPreFilter = IQ_mpy( ADC12MEM##adcConvNum + cMeas.offset, cMeas.scale ); \
//		if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter ); \
//	}

#define ADC_READANDFILTER( cMeas, cAdcValue, doUpdate ) \
	cMeas.valPreFilter = IQ_mpy( cAdcValue + cMeas.offset, cMeas.scale ); \
	if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter );

//#define ADC_READANDFILTER_PROPER( cMeas, adcConvNum, doUpdate ) \
//	if ( ADC12IFG & (1<<adcConvNum) ) \
//	{ \
//		cMeas.valPreFilter = IQ_mpy( ADC12MEM##adcConvNum, cMeas.scale )  + cMeas.offset; \
//		if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter ); \
//	}

// Input current sensor on AER07_WALL faces other way
#if (AER_PRODUCT_ID == AER05_RACK)
#define ADC_READANDFILTER_BIDIR( cMeas, adcConvNum, doUpdate ) \
	if ( ADC12IFG & (1<<adcConvNum) ) \
	{ \
		cMeas.valPreFilter = IQ_mpy( ADC12MEM##adcConvNum - (1<<11) + cMeas.offset, cMeas.scale ); \
		if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter ); \
	}


#elif (AER_PRODUCT_ID == AER07_WALL)
//#define ADC_READANDFILTER_BIDIR( cMeas, adcConvNum, doUpdate ) \
//	if ( ADC12IFG & (1<<adcConvNum) ) \
//	{ \
//		if(adcConvNum == 3)	\
//			cMeas.valPreFilter = IQ_mpy( (1<<11) - ADC12MEM##adcConvNum + cMeas.offset, cMeas.scale ); \
//		else cMeas.valPreFilter = IQ_mpy( ADC12MEM##adcConvNum - (1<<11) + cMeas.offset, cMeas.scale ); \
//		if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter ); \
//	}

#define ADC_READANDFILTER_BIDIR( cMeas, cAdcValue, doUpdate ) \
	{ \
		if(cAdcValue == momentValue.iInSensor)	\
			cMeas.valPreFilter = IQ_mpy( (1<<11) - cAdcValue + cMeas.offset, cMeas.scale ); \
		else cMeas.valPreFilter = IQ_mpy( cAdcValue - (1<<11) + cMeas.offset, cMeas.scale ); \
		if ( doUpdate ) cMeas.val = MEAS_filter( cMeas.val, cMeas.valPreFilter ); \
	}

#endif

// Macro to start a new conversion if not busy
#define ADC_STARTIFDONE		if ( !( ADC12CTL1 & ADC12BUSY ) ) ADC12CTL0 |= ADC12SC

#endif // ADC_H
