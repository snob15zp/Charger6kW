
//-------------------------------------------------------------------
// File: time.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#include "variant.h"
///#include "lcd.h"
///#include "meas.h"
#include "ctrl.h"
#include "safety.h"

hware_info hware =								//DEFAULTS - used when no hware info is found stored in bootloader
		{
			DEVICE_ID,							//invalid hardware constants from bootloader
			3,									//assume hware version 3
#if (MODEL_ID_MV == 1)
			MODEL_ID_MV,						//MV
#else
#if (MODEL_ID_HV == 1)
			MODEL_ID_HV,						//MV
#else
			Neither MV or HV defined !!!!!
#endif
#endif
			1,									//all version 1 had bootloader version 1
			0									//
		};	

inline void VAR_retreive_hware(void)
{
	//hware_info *hwareFlashPtr = (hware_info *)0xffd8;	
	//if(hwareFlashPtr->device_id == DEVICE_ID)	// make sure that the hware info is valid
	//{
	//	hware = *hwareFlashPtr;
	//} 
	
}

inline void VAR_SAFETY_setLimits(void)
{
	if( hware.model_id == MODEL_ID_MV )
	{
		SAFETY_setLimitsMV();
	}
	else
	{
		SAFETY_setLimitsHV();
	}
}

inline void VAR_CTRL_setFltrimParams(void)
{
	if( hware.hardware_version < 3 )
	{
		CTRL_setFltrimParams_old();
	}
	else
	{
		CTRL_setFltrimParams_new();
	}
}
