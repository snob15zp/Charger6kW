
//-------------------------------------------------------------------
// File: cfg.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef CONFIG_H
#define CONFIG_H

#include "debug.h"
#include "iqmath.h"
///#include "flag.h"
#include "time.h"
#include "telem.h"
#include "safety.h"

#define CONFIG_CRC_OK 0x80

// Structure for holding the product code
typedef struct ProductCode_
{
	char chars[4];
} ProductCode;

// Structure for configuring the linear transformation of a signal, e.g. an adc reading
typedef struct Calib_
{
	Iq scale;
	Iq offset;
} Calib;

typedef struct Cfg_
{
	unsigned char status;
} Cfg;


extern Cfg cfg;

void CFG_init(void);

unsigned char CFG_getStatus(void);
int CFG_configRangesOk(void);
int CFG_checkRanges(void);

unsigned int CFG_getLocalCfgLen(void);
unsigned int CFG_getRemoteCfgLen(void);
unsigned int CFG_getReadonlyCfgLen(void);
unsigned char * CFG_getReadonlyCfgAddr(void);
unsigned char * CFG_getLocalCfgAddr(void);
unsigned char * CFG_getRemoteCfgAddr(void);

void				CFG_productCode(unsigned char * pDst);
unsigned long		CFG_serialNumber(void);
unsigned long		CFG_canBaseId(void);
unsigned long		CFG_canBaud(void);
unsigned int		CFG_configVersion(void);
Calib				CFG_outVoltAdcCal(void);
Calib				CFG_outCurrAdcCal(void);
Calib				CFG_pvVoltAdcCal(void);
Calib				CFG_pvCurrAdcCal(void);
Calib				CFG_rail12VoltAdcCal(void);
Calib				CFG_flSetSenseAdcCal(void);
Calib				CFG_caseTmpAdcCal(void);
Calib				CFG_tmpCmpSenseAdcCal(void);
Calib				CFG_mpptSamplePtDacCal(void);
Calib				CFG_vinLimDacCal(void);
Calib				CFG_flTrimDacCal(void);
float				CFG_thermRinf(void);
float 				CFG_thermBeta(void);
float 				CFG_maxVinOverVoc(void);
float 				CFG_overCurrSpSw(void);

TELEM_SamplePeriod	CFG_telemSamplePeriod(void);
unsigned int		CFG_telemEnableBitfield(void);

float 				CFG_mpOcVoltRatio(void);
float				CFG_bulkVolt(void);
float				CFG_bulkTime(void);
float				CFG_floatVolt(void);
float				CFG_tmpCmp(void);

// For each copy and paste into other sources
typedef unsigned int UInt16;
typedef unsigned long UInt32;

// Structure to hold configuration values that are not settable via the web interface, e.g. scale factors and offsets, CAN settings, etc.
typedef struct LocalCfg_
{
	// These first entries should exist on all Tritium MSP430 projects
	// ===============================================================
	UInt16 checksum;
	UInt16 size;				// in bytes
	ProductCode productCode;
	UInt32 serialNumber;
	UInt32 canBaseId;
	UInt32 canBaud;
	// The rest of the structure is application specific
	// =================================================
	UInt16 configVersion;
	// ADC Calibration
	Calib outVoltAdcCal;
	Calib outCurrAdcCal;
	Calib pvVoltAdcCal;
	Calib pvCurrAdcCal;
	Calib rail12VoltAdcCal;
	Calib flSetSenseAdcCal;
	Calib caseTmpAdcCal;
	Calib tmpCmpSenseAdcCal;
	// PWM DAC Calibration
	Calib mpptSamplePtDacCal;
	Calib vinLimDacCal;
	Calib flTrimDacCal;
	// Thermistor parameters
	float thermRinf;
	float thermBeta;
	// Safety parameters
	//float maxVinOverVoc;	// Ignored now, left in for backwards compatibility of local config
	float isSlave; //Hijacked a float for backwards compatibility. Any non zero value means a master.
	float overCurrSpSw;
} LocalCfg;

// Structure to hold configuration values that can be set via the web interface, and may change regularly
typedef struct RemoteCfg_
{
	UInt16 checksum;
	UInt16 size;				// in bytes
	// Flag configuration
	FlagCfgSimple systemInitFlag;
	FlagCfgTrig	lowOutVoltWarnFlag;
	FlagCfgTrig	lowOutVoltFaultFlag;
	FlagCfgHold	lowOutVoltGensetFlag;
	FlagCfgTrig	highOutVoltFaultFlag;
	FlagCfgTrig highOutCurrFaultFlag;
	FlagCfgTrig highDisCurrFaultFlag;
	FlagCfgTrig highTempFaultFlag;
	FlagCfgSimple inBreakerOpenFlag;
	FlagCfgSimple outBreakerOpenFlag;
	FlagCfgSimple tempSenseFaultFlag;
	FlagCfgSimple sdFlags[SAFETY_NUM_SD_REASONS - 2]; //Casetmp & fan flags are omitted from CFG to keep aerltalk revisions to a minimum
	FlagCfgSimple logFullFlag;
	FlagCfgSched panelMissingFlag;
	// Telemetry configuration
	TELEM_SamplePeriod telemSamplePeriod;
	UInt16 telemEnableBitfield;
	// Control configuration
	float pvOcVolt;
	float pvMpVolt;
	float floatVolt;
	float bulkVolt;
	float bulkTime;
	float bulkResetVolt;
	float tmpCmp;			// Units?  mV / C for whole pack, NOT per cell
} RemoteCfg;

extern LocalCfg		CFG_localCfg;
extern RemoteCfg	CFG_remoteCfg;

extern UInt16 CFG_toggleSwitchMode;
extern float CFG_outVoltCutoffOffset;
extern float CFG_outVoltCutoffScale;

#endif // CONFIG_H

