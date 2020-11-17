#ifndef _PROTOCOL_H
#define _PROTOCOL_H
#include <inttypes.h>

#pragma anon_unions

typedef union {
	struct
	{
		uint32_t systemPower : 1;
		uint32_t lowOutVoltWarn : 1;
		uint32_t lowOutVoltFault : 1;
		uint32_t lowOutVoltGenSet : 1;
		uint32_t highOutVoltFault : 1;
		uint32_t highOutCurrentFault : 1;
		uint32_t highDischargeCurrentFault : 1;
		uint32_t highBatteryTempFault : 1;
		uint32_t inputBreakerOpen : 1; // Not used
		uint32_t outputBreakerOpen : 1; // Not used
		uint32_t tempSensorFault : 1;
		uint32_t negativePVCurrentSutdown : 1;
		uint32_t highPVCurrentShutdown : 1;
		uint32_t highPVVoltShutdown : 1;
		uint32_t highOutCurrentShutdown : 1;
		uint32_t highOutVoltShutdown : 1;
		uint32_t highHeatSinkTemp : 1;
		uint32_t fanFault : 1;
		uint32_t logFull : 1;
		uint32_t solarPanelMissing : 1;
		uint32_t configValueOutOfRange : 1;
		uint32_t : 11; // Reserved
	};
	uint32_t flags;

} eventFlags_t;

typedef union {
	struct
	{
		uint64_t config_factoryCRC : 1;
		uint64_t config_factoryCheck : 1;
		uint64_t config_userCRC : 1;
		uint64_t config_userCheck : 1;
		uint64_t config_eventCRC : 1;
		uint64_t config_eventCheck : 1;
		uint64_t config_miscCRC : 1;
		uint64_t config_miscCheck : 1;
		uint64_t statistics : 8; // Not used
		uint64_t flags : 8; // Not used
		uint64_t telemetry : 8; // Not used
		uint64_t communications : 8; // Not used
		uint64_t flash : 8; // Not used
		uint64_t control_isSlave : 1;
		uint64_t control_usingBulk : 1;
		uint64_t control_mode : 2;
		uint64_t control_outputEnabled : 1;
		uint64_t control_groundFaultShutdown : 1;	// Was smart shutdown for G2
		uint64_t control_autoOn : 1;
		uint64_t control_remoteShutdown : 1;
		uint64_t safety_operation : 1;
		uint64_t : 7;
	};
	uint64_t status;
	unsigned char bytes[1];

} status_t;

typedef union {
	struct
	{
		char productID[8];
		char serialNumber[8];
		float pvVoltage;
		float pvCurrent;
		float outputVoltage;
		float outputCurrent;
		float ocVoltage;
		float outputCharge;
		float pvPower;
		float batteryTemp;
		eventFlags_t eventFlags;
		status_t status;
		uint64_t time;
	};
	unsigned char bytes[1];
} telemetry_t;

typedef struct {
	float scale;
	float offset;
} calibration_t;

typedef union {
	struct {
		char productID[8];
		char serialNumber[8];
		calibration_t outVoltAdcCal;
		calibration_t outCurrentAdcCal;
		calibration_t pvVoltAdcCal;
		calibration_t pvCurrentAdcCal;
		calibration_t rail12VoltAdcCal;
		calibration_t floatSetSenseAdcCal;
		calibration_t pwmErrorMinusAdcCal;
		calibration_t tmpCmpSenseAdcCal;
		calibration_t mpptSamplePointDacCal;
		calibration_t vinLimDacCal;
		calibration_t floatTrimDacCal;
		float thermRinf;
		float thermBeta;
		float overCurrentSetPoint;
	};
	unsigned char bytes[1];
} factoryConfig_t;

typedef struct {
	uint16_t mode;
	uint16_t : 16;	// aligned to 32bit boundary
	float triggerVal;
	float resetVal;
	uint32_t hystTime; // ms
} eventParams_t;

typedef struct {
	uint16_t mode;
} eventParams_enable_t;

typedef struct {
	uint16_t mode;
	uint16_t : 16;	// aligned to 32bit boundary
	float triggerVal;
	float resetVal;
	uint32_t hystTime; // ms
	uint32_t holdTime; // ms
} eventParams_genset_t;

typedef struct {
	uint16_t mode;
	uint16_t : 16;	// aligned to 32bit boundary
	float triggerVal;
	uint32_t checkTime; // ms
} eventParams_missing_t;

typedef struct {
	unsigned char modBusAddress;
	unsigned char canBaudRate;
	uint16_t canBusID;
	uint16_t isSlave;
	uint16_t : 16;	// aligned to 32bit boundary
} commsConfig_t;

typedef struct {
	float pvOcVolt;
	float pvMpVolt;
	float floatVolt;
	float bulkVolt;
	uint32_t bulkTime;
	float bulkResetVolt;
	float tempCompensation;
	float nominalVolt;
} setPointsConfig_t;

typedef union {
	struct {
		eventParams_genset_t lowOutVoltGenset;
		commsConfig_t commsConfig;
		setPointsConfig_t setPointsConfig;
	};
	unsigned char bytes[1];
} userConfig_t;

typedef union {
	struct {
		eventParams_t lowOutVoltWarn;
		eventParams_t lowOutVoltFault;
		eventParams_t highOutVoltFault;
		eventParams_t highOutCurrentFault;
		eventParams_t highDischargeCurrentFault;
		eventParams_t highBatteryTempFault;
		eventParams_enable_t inputBreakerOpen; // Not used
		eventParams_enable_t outputBreakerOpen; // Not used
		eventParams_enable_t tempSensorFault;
		eventParams_enable_t pvNegativeCurrentSutdown;
		eventParams_enable_t pvHighCurrentShutdown;
		eventParams_enable_t pvHighVoltShutdown;
		eventParams_enable_t highOutCurrentShutdown;
		eventParams_enable_t highOutVoltShutdown;
		eventParams_enable_t logFull;
		uint16_t : 16; // eventParams_enable_t is 16bit so add some padding to align with 32bit.
		eventParams_missing_t solarPanelMissing;
	};
	unsigned char bytes[1];
} eventConfig_t;

typedef union {
	struct {
		char hwVersion[8];
		char fwVersion[8];
		char modelType[4];
		char productID[8];
		char serialNumber[8];
	};
	unsigned char bytes[1];
} sysInfo_t;

typedef union {
	struct {
		uint16_t commandCode;
		uint16_t arg;
	};
	unsigned char bytes[1];
} command_t;

typedef union {
	struct {
		unsigned char password[32];
	};
	unsigned char bytes[1];
} password_t;

typedef union {
	struct {
		uint64_t time;
	};
	unsigned char bytes[1];
} setTime_t;

typedef union  {
	struct	{
		uint16_t toggleSwitchMode;
		uint16_t _enableSmartShutdown;	// Not used anymore
		float outVoltCutoffOffset;
		float outVoltCutoffScale;
		uint16_t : 16;
		uint16_t : 16;
		uint16_t : 16;
		uint16_t : 16;
	};
	unsigned char bytes[1];
} miscState_t;

#define VERSION_TELEMETRY 1
#define VERSION_FACTORY 1
#define VERSION_USER 1
#define VERSION_EVENTS 1
#define VERSION_SYS_INFO 1
#define VERSION_COMMAND 1
#define VERSION_PASSWORD 1
#define VERSION_SET_TIME 1
#define VERSION_MISC_STATE 1

typedef enum packet_Type_
{
	TYPE_TELEMETRY = 0x00,
	TYPE_FACTORY = 0x01,
	TYPE_USER = 0x02,
	TYPE_EVENTS = 0x03,
	TYPE_SYS_INFO = 0x04,
	TYPE_COMMAND = 0x05,
	TYPE_PASSWORD = 0x06,
	TYPE_SET_TIME = 0x07,
	TYPE_MISC_STATE = 0x08
} packet_Type;

typedef enum command_Code_
{
	// Command codes used by command_t
	COMMAND_RESET = 0x0000,
	COMMAND_ENABLE_OUTPUT = 0x0001,

	// Response codes used by command_t
	RESPONSE_STARTUP = 0xFF00,
	RESPONSE_ERROR = 0xFF01
} command_Code;

// Error args for RESPONSE_ERROR
typedef enum error_Code_
{
	ERROR_PACKET_TYPE = 0x00,
	ERROR_VERSION = 0x01
} error_Code;

typedef union {
	struct {
		unsigned char type : 4;
		unsigned char request : 1;
		unsigned char version : 3;
	};
	unsigned char byte;
} packetIdentifier_t;


typedef struct {
	unsigned int autoOn;
} persistentStorage_t;

#endif
