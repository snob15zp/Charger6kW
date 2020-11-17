//-------------------------------------------------------------------
// File: cfg.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Calem Walsh, Tritium Pty Ltd.
// Description: This module redirects function calls to the appropriate functions for different models and hardware versions
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef VARIANT_H
#define VARIANT_H

// PRODUCT ID - must be selected at compile time - switches compiled code between wall mount and rack mount
#define AER05_RACK 0
#define AER07_WALL 1

#define AER_PRODUCT_ID		AER07_WALL
#define VAR_VERSION_NUMBER	303
#define VAR_VERSION_NUMBER_STR	"V3.03"
//#define DEBUG_LED_SAMPLE

// MODEL ID - retrieved from bootloader flash at runtime. 
#define MODEL_ID_HV 1
#define MODEL_ID_MV 0

// DEVICE ID - stored in bootloader flash, retrieved to ensure that bootloader hware_info is correct in flash
#define DEVICE_ID 0x41303031ul

// Don't use this if any of the resistors on P2.5 are fitted
//#define ENABLE_PWM_TEST

//#define EXTERNAL_TEMP
#define DIGITAL_TEMP

typedef struct hware_info_			//*$* TODO merge this struct with Monte's readonly cfg struct, make it accessible from PC interface
{
	unsigned long device_id;			//4 bytes
	unsigned char hardware_version;		//1 byte
	unsigned char model_id;				//1 byte
	unsigned char bootloader_version;	//1 byte
	unsigned char unallocated;			//1 bytes
} hware_info;

extern hware_info hware;

void VAR_retreive_hware(void);

void VAR_SAFETY_setLimits(void);		//called from safety init, varies safety setpoints depending on HV or MV.

void VAR_CTRL_setFltrimParams(void);	//called from ctrl init, varies fltrim stuff depending on hardware version (gain resistors changed for hardware version 2)

#endif // VARIANT_H
