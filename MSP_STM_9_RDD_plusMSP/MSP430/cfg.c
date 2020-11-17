
//-------------------------------------------------------------------
// File: cfg.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#include "variant.h"
#include "cfg.h"
#include "can.h"
#include "debug.h"
#include "meas.h"
#include "flash.h"
//#include "status.h"
#include "buildnum.h"


//#include <math.h>

LocalCfg CFG_localCfg = 
{
	0,
	sizeof(LocalCfg),
	{ { 'A', '0', '0', '1' } },
	0,
	0x600,
	(UInt32)BAUD_500,
	100,
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	{ IQ_cnst(1.0), IQ_cnst(0.0) },
	IQ_cnst(0.0),
	IQ_cnst(0.0),
	IQ_cnst(0.0),
	IQ_cnst(50.0)
};

// Control defaults (define here as we use them in checking code also)
#define PV_OC_VOLT_DFT		120
#define PV_MP_VOLT_DFT		96
#define FLOAT_VOLT_DFT		55.2
#define BULK_VOLT_DFT		57.6
#define BULK_TIME_DFT		120.0
#define BULK_RESET_VOLT		50.4
#define TMP_CMP_DFT			0.0
#define OUT_VOLT_CUTOFF_OFFSET_DFT	5.0
#define OUT_VOLT_CUTOFF_SCALE_DFT	0.185

RemoteCfg CFG_remoteCfg = 
{
	0,
	sizeof(RemoteCfg),
	// Flag configuration
	{ FLAG_LOG },
	{ FLAG_LOG, 42.0,	44.0,	10000l		},
	{ FLAG_LOG, 40.0,	42.0,	10000l		},
	{ FLAG_LOG, 38.0,	55.0,	3600000l	},
	{ FLAG_LOG, 65.0,	63.0,	10000l		},
	{ FLAG_LOG, 60.0,	59.0,	10000l		},
	{ FLAG_LOG, 500.0,	495.0,	10000l		},
	{ FLAG_LOG, 100.0,	95.0,	60000l		},
	{ FLAG_LOG },
	{ FLAG_LOG },
	{ FLAG_LOG },
	{ { FLAG_LOG }, { FLAG_LOG }, { FLAG_LOG }, { FLAG_LOG }, { FLAG_LOG } },
	{ FLAG_LOG },
	{ FLAG_LOG, 20.0, 12l * 3600l * 1000l },
	// Telemetry configuration
	TELEM_PERIOD_200MS,
	TELEM_OUTVOLT_MASK | TELEM_OUTCURR_MASK | TELEM_OUTCHARGE_MASK | TELEM_PVVOLT_MASK
		| TELEM_PVCURR_MASK | TELEM_PVPOWER_MASK | TELEM_PVOCVOLT_MASK,
	// Control configuration
	PV_OC_VOLT_DFT,
	PV_MP_VOLT_DFT,
	FLOAT_VOLT_DFT,
	BULK_VOLT_DFT,
	BULK_TIME_DFT,
	BULK_RESET_VOLT,
	TMP_CMP_DFT
};

// Structure to hold information that cannot be overwritten
typedef struct ReadonlyCfg_
{
	UInt16 checksum;
	UInt16 size;				// in bytes
	UInt16 hardwareVersion;
	UInt16 firmwareVersion;
	UInt32 buildNumber;
} ReadonlyCfg;

ReadonlyCfg readonlyCfg =
{
	0,
	sizeof(ReadonlyCfg),
	1,
	VAR_VERSION_NUMBER,
	BUILD_NUMBER
};

const Calib calibDft = { IQ_cnst(0.0), IQ_cnst(1.0) };

Cfg cfg;

UInt16 CFG_toggleSwitchMode;
float CFG_outVoltCutoffOffset = OUT_VOLT_CUTOFF_OFFSET_DFT;
float CFG_outVoltCutoffScale = OUT_VOLT_CUTOFF_SCALE_DFT;

void CFG_init()
{
	readonlyCfg.hardwareVersion = hware.hardware_version;

	//UInt16 tempChecksum;

	// Update checksums in default config structures
	CFG_localCfg.checksum = UTIL_calcChecksum(	(unsigned char *)&CFG_localCfg + sizeof(CFG_localCfg.checksum), 
												sizeof(CFG_localCfg) - sizeof(CFG_localCfg.checksum) );
	CFG_remoteCfg.checksum = UTIL_calcChecksum(	(unsigned char *)&CFG_remoteCfg + sizeof(CFG_remoteCfg.checksum), 
												sizeof(CFG_remoteCfg) - sizeof(CFG_remoteCfg.checksum) );

	//cfg.status = 0;

	// Check local config checksum
	//tempChecksum = UTIL_calcChecksumFlash(	FLASH_LOCAL_CFG_ADDR + sizeof(CFG_localCfg.checksum), 
	//											sizeof(CFG_localCfg) - sizeof(CFG_localCfg.checksum) );
	//if ( tempChecksum != FLASH_readU16(FLASH_LOCAL_CFG_ADDR) )
	//{
		//cfg.status &= ~(1);
	//}
	//else
	//{
		// Load local config from flash
	//	FLASH_readStr( (unsigned char *)&CFG_localCfg, FLASH_LOCAL_CFG_ADDR, sizeof(LocalCfg), 0 );
	//}

	// Check remote config checksum
	//tempChecksum = UTIL_calcChecksumFlash(	FLASH_REMOTE_CFG_ADDR + sizeof(CFG_remoteCfg.checksum), 
	//											sizeof(CFG_remoteCfg) - sizeof(CFG_remoteCfg.checksum) );
	//if ( tempChecksum != FLASH_readU16(FLASH_REMOTE_CFG_ADDR) )
	//{
		//cfg.status &= ~(2);
	//}
	//else
	//{
		// Load remote config from flash
	//	FLASH_readStr( (unsigned char *)&CFG_remoteCfg, FLASH_REMOTE_CFG_ADDR, sizeof(RemoteCfg), 0 );
	//}

	// Calculate readonly config checksum
	//readonlyCfg.checksum = UTIL_calcChecksum(	(unsigned char *)(&readonlyCfg) + sizeof(readonlyCfg.checksum), 
	//											sizeof(readonlyCfg) - sizeof(readonlyCfg.checksum) );

	// Check config values for range limits
	if ( CFG_checkRanges() )
	{
		cfg.status |= CONFIG_CRC_OK;
	}
	else
	{
		cfg.status &= ~(CONFIG_CRC_OK);
	}

	//cfg.status |= 8;
	//CFG_localCfg.canBaseId = 0X600;
	//CFG_localCfg.canBaud = 4;
}

int CFG_checkRanges()
{
	int rangesOk = 1;

	if ( CFG_remoteCfg.pvOcVolt != CFG_remoteCfg.pvOcVolt ) { CFG_remoteCfg.pvOcVolt = PV_OC_VOLT_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_remoteCfg.pvOcVolt < 12.0 ) { CFG_remoteCfg.pvOcVolt = 12.0; rangesOk = 0; }
	else if ( CFG_remoteCfg.pvOcVolt > 300 ) { CFG_remoteCfg.pvOcVolt = 300.0; rangesOk = 0; }

	if ( CFG_remoteCfg.pvMpVolt != CFG_remoteCfg.pvMpVolt ) { CFG_remoteCfg.pvMpVolt = PV_MP_VOLT_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_remoteCfg.pvMpVolt < 12.0 ) { CFG_remoteCfg.pvMpVolt = 12.0; rangesOk = 0; }
	else if ( CFG_remoteCfg.pvMpVolt > 300.0 ) { CFG_remoteCfg.pvMpVolt = 300.0; rangesOk = 0; }

	if ( CFG_remoteCfg.floatVolt != CFG_remoteCfg.floatVolt ) { CFG_remoteCfg.floatVolt = FLOAT_VOLT_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_remoteCfg.floatVolt < 12.0 ) { CFG_remoteCfg.floatVolt = 12.0; rangesOk = 0; }
	else if ( CFG_remoteCfg.floatVolt > 200.0 ) { CFG_remoteCfg.floatVolt = 200.0; rangesOk = 0; }
	
	if ( CFG_remoteCfg.bulkVolt != CFG_remoteCfg.bulkVolt ) { CFG_remoteCfg.bulkVolt = BULK_VOLT_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_remoteCfg.bulkVolt < 12.0 ) { CFG_remoteCfg.bulkVolt = 12.0; rangesOk = 0; }
	else if ( CFG_remoteCfg.bulkVolt > 200.0 ) { CFG_remoteCfg.bulkVolt = 200.0; rangesOk = 0; }
	
	if ( CFG_remoteCfg.bulkTime != CFG_remoteCfg.bulkTime ) { CFG_remoteCfg.bulkTime = BULK_TIME_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_remoteCfg.bulkTime < 0.0 ) { CFG_remoteCfg.bulkTime = 0.0; rangesOk = 0; }
	else if ( CFG_remoteCfg.bulkTime > 86400.0 ) { CFG_remoteCfg.bulkTime = 86400.0; rangesOk = 0; }
	
	if ( CFG_remoteCfg.bulkResetVolt != CFG_remoteCfg.bulkResetVolt ) { CFG_remoteCfg.bulkResetVolt = BULK_VOLT_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_remoteCfg.bulkResetVolt < 12.0 ) { CFG_remoteCfg.bulkResetVolt = 12.0; rangesOk = 0; }
	else if ( CFG_remoteCfg.bulkResetVolt > 200.0 ) { CFG_remoteCfg.bulkResetVolt = 200.0; rangesOk = 0; }
	
	if ( CFG_remoteCfg.tmpCmp != CFG_remoteCfg.tmpCmp ) { CFG_remoteCfg.tmpCmp = BULK_VOLT_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_remoteCfg.tmpCmp < -200.0 ) { CFG_remoteCfg.tmpCmp = -200.0; rangesOk = 0; }
	else if ( CFG_remoteCfg.tmpCmp > 0 ) { CFG_remoteCfg.tmpCmp = 0; rangesOk = 0; }

	if ( CFG_outVoltCutoffOffset != CFG_outVoltCutoffOffset ) { CFG_outVoltCutoffOffset = OUT_VOLT_CUTOFF_OFFSET_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_outVoltCutoffOffset < -50.0 ) { CFG_outVoltCutoffOffset = -50.0; rangesOk = 0; }
	else if ( CFG_outVoltCutoffOffset > 50.0 ) { CFG_outVoltCutoffOffset = 50.0; rangesOk = 0; }

	if ( CFG_outVoltCutoffScale != CFG_outVoltCutoffScale ) { CFG_outVoltCutoffScale = OUT_VOLT_CUTOFF_SCALE_DFT; rangesOk = 0; }	// This should check for NaN
	else if ( CFG_outVoltCutoffScale < 0.0 ) { CFG_outVoltCutoffScale = 0.0; rangesOk = 0; }
	else if ( CFG_outVoltCutoffScale > 1.0 ) { CFG_outVoltCutoffScale = 1.0; rangesOk = 0; }

	return rangesOk;
}

unsigned char CFG_getStatus()
{
	return cfg.status;
}

int CFG_configRangesOk()
{
	return ( cfg.status & CONFIG_CRC_OK );
}

UInt16 CFG_getLocalCfgLen()
{
	return sizeof( LocalCfg );
}

UInt16 CFG_getRemoteCfgLen()
{
	return sizeof( RemoteCfg );
}

UInt16 CFG_getReadonlyCfgLen()
{
	return sizeof( ReadonlyCfg );
}

unsigned char * CFG_getLocalCfgAddr()
{
	return (unsigned char *)&CFG_localCfg;
}

unsigned char * CFG_getRemoteCfgAddr()
{
	return (unsigned char *)&CFG_remoteCfg;
}

unsigned char * CFG_getReadonlyCfgAddr()
{
	return (unsigned char *)&readonlyCfg;
}

/*
ProductCode CFG_productCode();
{
	ProductCode code;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.poductCode;
	else 
	{
		// Read from flash
		FLASH_readStr( (unsigned char *)&code, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.productCode) - &(CFG_localCfg) ), sizeof(ProductCode), 0 );
		return code;
	}
}

UInt32 CFG_serialNumber()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.serialNumber;
	// Read from flash
	else 
	{
		return FLASH_readU32( FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.serialNumber) - &(CFG_localCfg) ) );
	}
}

UInt32 CFG_canBaseId()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.canBaseId;
	// Read from flash
	else 
	{
		return FLASH_readU32( FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.canBaseId) - &(CFG_localCfg) ) );
	}
}

UInt32 CFG_canBaud()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.canBaud;
	// Read from flash
	else 
	{
		return FLASH_readU32( FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.canBaud) - &(CFG_localCfg) ) );
	}
}

UInt16 CFG_configVersion()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.configVersion;
	// Read from flash
	else 
	{
		return FLASH_readU16( FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.configVersion) - &(CFG_localCfg) ) );
	}
}

Calib CFG_outVoltAdcCal()
{
	Calib cal;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.outVoltAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.outVoltAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_outCurrAdcCal()
{
	Calib cal;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.outCurrAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.outCurrAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_pvVoltAdcCal()
{
	Calib cal;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.pvVoltAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.pvVoltAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_pvCurrAdcCal()
{
	Calib cal;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.pvCurrAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.pvCurrAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_rail12VoltAdcCal()
{
	Calib cal;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.rail12VoltAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.rail12VoltAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_flSetSenseAdcCal()
{
	Calib cal;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.flSetSenseAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.flSetSenseAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_pwmErrMinusAdcCal()
{
	Calib cal;
	if ( !cfg.cfgOk ) return CFG_localCfgDft.pwmErrMinusAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.pwmErrMinusAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_tmpCmpSenseAdcCal()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.tmpCmpSenseAdcCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.tmpCmpSenseAdcCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_mpptSamplePtDacCal()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.mpptSamplePtDacCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.mpptSamplePtDacCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_vinLimDacCal()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.vinLimDacCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.vinLimDacCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

Calib CFG_flTrimDacCal()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.flTrimDacCal;
	// Read from flash
	else 
	{
		FLASH_readStr( (unsigned char)&cal, FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.flTrimDacCal) - &(CFG_localCfg) ), sizeof(Calib), 0 );
		return cal;
	}
}

float CFG_thermRinf()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.thermRinf;
	// Read from flash
	else 
	{
		return FLASH_readF32( FLASH_LOCAL_CFG_ADDR + (unsigned long)( &(CFG_localCfgDft.configVersion) - &(CFG_localCfg) ) );
	}
}

float CFG_thermBeta()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.thermBeta;
	// Read from flash
	else 
	{
		return 0;
	}
}

float CFG_maxVinOverVoc()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.maxVinOverVoc;
	// Read from flash
	else 
	{
		return 0;
	}
}

float CFG_overCurrSpSw()
{
	if ( !cfg.cfgOk ) return CFG_localCfgDft.overCurrSpSw;
	// Read from flash
	else 
	{
		return 0;
	}
}

TELEM_SamplePeriod CFG_telemSamplePeriod()
{
	if ( !cfg.cfgOk ) return CFG_remoteCfgDft.telemSamplePeriod;
	// Read from flash
	else 
	{
		return 0;
	}
}

UInt16 CFG_telemEnableBitfield()
{
	if ( !cfg.cfgOk ) return CFG_remoteCfgDft.telemEnableBitfield;
	// Read from flash
	else 
	{
		return 0;
	}
}

float CFG_mpOcVoltRatio()
{
	if ( !cfg.cfgOk ) return CFG_remoteCfgDft.mpOcVoltRatio;
	// Read from flash
	else 
	{
		return 0;
	}
}

float CFG_bulkVolt()
{
	if ( !cfg.cfgOk ) return CFG_remoteCfgDft.bulkVolt;
	// Read from flash
	else 
	{
		return 0;
	}
}

float CFG_bulkTime()
{
	if ( !cfg.cfgOk ) return CFG_remoteCfgDft.bulkTime;
	// Read from flash
	else 
	{
		return 0;
	}
}

float CFG_floatVolt()
{
	if ( !cfg.cfgOk ) return CFG_remoteCfgDft.floatVolt;
	// Read from flash
	else 
	{
		return 0;
	}
}

float CFG_tmpCmp()
{
	if ( !cfg.cfgOk ) return CFG_remoteCfgDft.tmpCmp;
	// Read from flash
	else 
	{
		return 0;
	}
}
*/





