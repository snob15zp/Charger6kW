//--------------------------------------------------------------------
// File: lcd.h
// Devices: MSP430F247
// Author: David Finn, Tritium Pty Ltd.
// History:
//  28/06/11 - original
// Required hardware:
// Notes:
//--------------------------------------------------------------------
#ifndef __LCD_H__
#define __LCD_H__

#ifndef DSP28_DATA_TYPES
#define DSP28_DATA_TYPES
typedef char				int8;
typedef int             	int16;
typedef long            	int32;
typedef long long			int64;
typedef unsigned char		Uint8;
typedef unsigned int    	Uint16;
typedef unsigned long   	Uint32;
typedef unsigned long long	Uint64;
typedef float           	float32;
typedef long double     	float64;
#endif


#define LCD_PERIOD_MS	500
#define COMMSTIMEOUT	5		// LCD_PERIOD_MS periods (refer to sch.c)

typedef enum lcd_CfgState_
{
	LCDCFG_STATE_CFG_IDLE = 0,
	LCDCFG_STATE_ERASE,
	LCDCFG_STATE_WAIT_FOR_ERASE,
	LCDCFG_STATE_WRITE,
	LCDCFG_STATE_CHECK_CFG,
	LCDCFG_STATE_SEND_CFG
} lcd_CfgState;


/*------------------------------------------------------------------------------
Prototypes for the functions in lcd.c
------------------------------------------------------------------------------*/
void lcd_init(void);
void lcd_update(void);
void lcd_canRecved(void);

void lcd_eraseDoneCallback(int retval);
void lcd_writeFlVoltToCfg(float f);
void lcd_runCfgStateMachine(void);

void lcd_loadTelemetry(void);
int lcd_queueWrite(int type);
int lcd_startWrite(void);
void lcd_checkPersistentUpdate(void);

#endif // __LCD_H__
