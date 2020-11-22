
//-------------------------------------------------------------------
// File: comms.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------
/*

Protocol over CAN

mainMSP.c:MAIN_resetAllAndStart-> comms.c:COMMS_init();

status.c:statusFuncs ->comms.c:COMMS_getStatus();
lcd.c:lcd_loadTelemetry()->comms.c:COMMS_getStatus();

sch.c:tasks->comms.c:COMMS_sendHeartbeat();->CAN_txBuffer[]

sch.c:tasks->comms.c:COMMS_sendStatus();->CAN_txBufferp[]

sch.c:tasks->comms.c:COMMS_sendPvMeas();->CAN_txBuffer[]

sch.c:tasks->comms.c:COMMS_sendOutMeas()->CAN_txBuffer[]

sch.c:tasks->comms.c:COMMS_sendOcQMeas()->CAN_txBuffer[]

sch.c:tasks->comms.c:COMMS_sendPowTemprMeas();->CAN_txBuffer[]

sch.c:tasks->comms.c:COMMS_sendTime();->CAN_txBuffer[]

sch.c:tasks->comms.c:COMMS_sendFlag();->CAN_txBuffer[]

sch.c:tasks->comms.c:COMMS_sendOutVoltCmd();->CAN_txBuffer[]

mainMSP:mainMSPloop->comms.c:COMMS_receive();->CAN_receive(),lcd_canRecved(),IO_disablePwmCtrl(),MAIN_resetRemoteCfg(),// Force watchdog reset;
                                               TIME_recFromBc();CTRL_setOutVoltCmd(),CAN_init()

sch.c:tasks->comms.c:COMMS_sendP2pPacket();->COMMS_endFlashSend(),COMMS_endFlashSend();->CAN_txBuffer[]

comms.c:COMMS_startFlashRec()->comms.c:COMMS_sendDebugPacket();->CAN_txBuffer[]

//void COMMS_sendDebugPacketU64( unsigned long long val );

//void COMMS_sendDebugPacketFloat( float val1, float val2 );

*/


//#include <msp430x24x.h>
#include "variant.h"
#include "comms.h"
#include "can.h"
#include "io.h"
#include "meas.h"
#include "iqmath.h"
#include "pwm.h"
#include "flash.h"
#include "cfg.h"
#include "stats.h"
#include "flag.h"
#include "telem.h"
#include "main.h"
#include "time.h"
#include "status.h"
#include "util.h"
#include "ctrl.h"
#include "lcd.h"


typedef enum P2pMode_
{
	P2P_IDLE = 0,
	P2P_FLASH_SEND_ACTIVE,
	P2P_MEM_SEND_ACTIVE,
	P2P_FLASH_REC_PAUSED,
	P2P_FLASH_REC_ACTIVE
} P2pMode;

typedef struct P2p_
{
	P2pMode mode;
	unsigned char reqChars[2];
	unsigned long currentAddr;
	unsigned long lenRem;
	unsigned long packetCount;
} P2p;

typedef struct Comms_
{
	P2p p2p;
} Comms;

// Variable local to the comms module
Comms comms;

int COMMS_startFlashSend( unsigned long addr, unsigned long len );
int COMMS_startMemSend( unsigned char * addr, unsigned int len );
int COMMS_startFlashRec( unsigned long addr, unsigned long len );
void COMMS_endFlashSend();
void COMMS_endMemSend();
void COMMS_endFlashRec();

void COMMS_init()
{
	comms.p2p.mode = P2P_IDLE;
}

unsigned char COMMS_getStatus()
{
	return comms.p2p.mode;
}

void COMMS_sendHeartbeat()
{
	// Normal heartbeat
	CAN_txBuffer[CAN_HB_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId;
	CAN_txBuffer[CAN_HB_INDEX].data.data_u8[3] = CFG_localCfg.productCode.chars[0];
	CAN_txBuffer[CAN_HB_INDEX].data.data_u8[2] = CFG_localCfg.productCode.chars[1];
	CAN_txBuffer[CAN_HB_INDEX].data.data_u8[1] = CFG_localCfg.productCode.chars[2];
	CAN_txBuffer[CAN_HB_INDEX].data.data_u8[0] = CFG_localCfg.productCode.chars[3];
	CAN_txBuffer[CAN_HB_INDEX].data.data_u32[1] = CFG_localCfg.serialNumber;
	//CAN_txBuffer[CAN_HB_INDEX].data.data_u32[1] = IQ_mpyTo24( IQ_cnst(0.25), IQ_cnst(0.1) );
	//can.transmit();
	CAN_txBuffer[CAN_HB_INDEX].status = CAN_TXBUFFER_WAITING;

	// Global heartbeat / address request
	CAN_txBuffer[CAN_BC_REQID_INDEX].status = CAN_TXBUFFER_EMPTY;
	CAN_txBuffer[CAN_BC_REQID_INDEX].data.data_u32[0] = CAN_getBaseId();
	CAN_txBuffer[CAN_BC_REQID_INDEX].data.data_u32[1] = CFG_localCfg.serialNumber;
	CAN_txBuffer[CAN_BC_REQID_INDEX].status = CAN_TXBUFFER_WAITING;

	
}

void COMMS_sendStatus()
{
	CAN_txBuffer[CAN_STATUS_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_STATUS_ID;
	STATUS_update();
	CAN_txBuffer[CAN_STATUS_INDEX].data.data_u64 = STATUS_getStatus();
	//can.transmit();
	CAN_txBuffer[CAN_STATUS_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendTime()
{
	CAN_txBuffer[CAN_TIME_SEND_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_TIME_SEND_ID;
	CAN_txBuffer[CAN_TIME_SEND_INDEX].data.data_u64 = TIME_get();
	//can.transmit();
	CAN_txBuffer[CAN_TIME_SEND_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendPvMeas()
{
	CAN_txBuffer[CAN_PV_MEAS_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_PV_MEAS_ID;
//	CAN_txBuffer[CAN_PV_MEAS_INDEX].data.data_fp[0] = meas.pvVolt.valReal = meas.pvVolt.val * meas.pvVolt.base;
//	CAN_txBuffer[CAN_PV_MEAS_INDEX].data.data_fp[1] = meas.pvCurr.valReal = meas.pvCurr.val * meas.pvCurr.base;
	CAN_txBuffer[CAN_PV_MEAS_INDEX].data.data_fp[0] = meas.pvVolt.valReal;
	CAN_txBuffer[CAN_PV_MEAS_INDEX].data.data_fp[1] = meas.pvCurr.valReal;
	//can.transmit();                                                    //RDD why?
	CAN_txBuffer[CAN_PV_MEAS_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendOutMeas()
{
	CAN_txBuffer[CAN_OUT_MEAS_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_OUT_MEAS_ID;
//	CAN_txBuffer[CAN_OUT_MEAS_INDEX].data.data_fp[0] = meas.outVolt.valReal = meas.outVolt.val * meas.outVolt.base;
//	CAN_txBuffer[CAN_OUT_MEAS_INDEX].data.data_fp[1] = meas.outCurr.valReal = meas.outCurr.val * meas.outCurr.base;
	CAN_txBuffer[CAN_OUT_MEAS_INDEX].data.data_fp[0] = meas.outVolt.valReal;
	CAN_txBuffer[CAN_OUT_MEAS_INDEX].data.data_fp[1] = meas.outCurr.valReal;
	//can.transmit();
	CAN_txBuffer[CAN_OUT_MEAS_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendOcQMeas()
{
	CAN_txBuffer[CAN_OC_Q_MEAS_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_OC_Q_MEAS_ID;
//	CAN_txBuffer[CAN_OC_Q_MEAS_INDEX].data.data_fp[0] = meas.pvOcVolt.valReal = meas.pvOcVolt.val * meas.pvOcVolt.base;
//	CAN_txBuffer[CAN_OC_Q_MEAS_INDEX].data.data_fp[1] = meas.outCharge.valReal = meas.outCharge.val * meas.outCharge.base;
	CAN_txBuffer[CAN_OC_Q_MEAS_INDEX].data.data_fp[0] = meas.pvOcVolt.valReal;
	CAN_txBuffer[CAN_OC_Q_MEAS_INDEX].data.data_fp[1] = meas.outCharge.valReal;
	//can.transmit();
	CAN_txBuffer[CAN_OC_Q_MEAS_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendPowTemprMeas()
{
	CAN_txBuffer[CAN_POW_TEMPR_MEAS_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_POW_MEAS_ID;
//	CAN_txBuffer[CAN_POW_TEMPR_MEAS_INDEX].data.data_fp[0] = meas.pvPower.valReal =  meas.pvPower.val * meas.pvPower.base;
	CAN_txBuffer[CAN_POW_TEMPR_MEAS_INDEX].data.data_fp[0] = meas.pvPower.valReal;
	CAN_txBuffer[CAN_POW_TEMPR_MEAS_INDEX].data.data_fp[1] = meas.batTempr.valReal; // tempr.valReal gets updated in MEAS_updateTempr()
	//can.transmit();
	CAN_txBuffer[CAN_POW_TEMPR_MEAS_INDEX].status = CAN_TXBUFFER_WAITING;

	/*unsigned long long *Flash_ptrA;                         // Segment A pointer
	unsigned long long Flash_data = 0;
	unsigned long long version_number = 0;

	Flash_ptrA = (unsigned long long *)0xffd8;              // Initialize Flash segment A ptr
	Flash_data = *Flash_ptrA;		 */				
		
}

void COMMS_sendFlag()
{
	CAN_txBuffer[CAN_FLAG_SEND_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_OC_Q_MEAS_ID;
	CAN_txBuffer[CAN_FLAG_SEND_INDEX].data.data_u64 = FLAG_getFlagBitfield();
	//can.transmit();
	CAN_txBuffer[CAN_FLAG_SEND_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendOutVoltCmd()
{
	CAN_txBuffer[CAN_BC_OUTVOLT_CMD_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_OUTVOLT_CMD_ID;
	CAN_txBuffer[CAN_BC_OUTVOLT_CMD_INDEX].data.data_fp[0] = CTRL_getOutVoltCmd();
	CAN_txBuffer[CAN_BC_OUTVOLT_CMD_INDEX].data.data_fp[1] = 0.0f;
	//can.transmit();
	CAN_txBuffer[CAN_BC_OUTVOLT_CMD_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendDebugPacket( unsigned int val1, unsigned int val2, unsigned int val3, unsigned int val4 )
{
	CAN_txBuffer[CAN_DEBUG_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_DEBUG_ID;
	CAN_txBuffer[CAN_DEBUG_INDEX].data.data_u16[0] = val1;
	CAN_txBuffer[CAN_DEBUG_INDEX].data.data_u16[1] = val2;

	CAN_txBuffer[CAN_DEBUG_INDEX].data.data_u16[2] = val3;
	CAN_txBuffer[CAN_DEBUG_INDEX].data.data_u16[3] = val4;
	//can.transmit();
	CAN_txBuffer[CAN_DEBUG_INDEX].status = CAN_TXBUFFER_WAITING;
}

/*
void COMMS_sendDebugPacketU64( unsigned long long val )
{
	CAN_txBuffer[CAN_DEBUG_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_DEBUG_ID;
	CAN_txBuffer[CAN_DEBUG_INDEX].data.data_u64 = val;
	//can.transmit();
	CAN_txBuffer[CAN_DEBUG_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_sendDebugPacketFloat( float val1, float val2 )
{
	CAN_txBuffer[CAN_DEBUG_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_DEBUG_ID;
	CAN_txBuffer[CAN_DEBUG_INDEX].data.data_fp[0] = val1;
	CAN_txBuffer[CAN_DEBUG_INDEX].data.data_fp[1] = val2;
	//can.transmit();
	CAN_txBuffer[CAN_DEBUG_INDEX].status = CAN_TXBUFFER_WAITING;
}
*/
/* NOTE: Point to point (P2P) comms commands for communicating streams of data
	u08[0]	u08[1]	cmd
	-------------------------------------
	R		L		read local config
	R		R		read remote config
	R		S		read stats
	R		A		read flags
	R		T		read stored telemetry
	W		L		write local config
	W		R		write remote config
	W		S		erase stats
	W		A		erase flags
	W		T		erase stored telemetry
*/

void COMMS_receive()
{
	int retVal = -1;
	unsigned long tmpAddr;
	unsigned long p2pReply = 0;
	Time tmMin;
	if ( IS_CAN_INT )
	{
		// IRQ flag is set, so run the receive routine to either get the message, or the error
		CAN_receive();
		
		// Process the CAN message 
		if ( CAN_rx.status == CAN_OK )
		{
#if (AER_PRODUCT_ID == AER05_RACK)
				IO_toggleOrangeLed();
#elif (AER_PRODUCT_ID == AER07_WALL)
				lcd_canRecved();
#endif
			if ( CAN_rx.address == CAN_getBaseId() + CAN_RESET_ID )
			{
				if ( CAN_rx.data.data_u8[0] == 'A' && CAN_rx.data.data_u8[1] == 'L' && CAN_rx.data.data_u8[2] == 'L' )
				{
					// Make safe
					IO_disablePwmCtrl();
					//PWM_setMpptSamplePt( IQ_cnst(0.1) );    //RDD why?
					//PWM_setVinLim( IQ_cnst(0.1) );
					// Reset
					//MAIN_resetAllAndStart();
					//WDTCTL = 0x00;	// Force watchdog reset		//****** Can't change this back without it breaking??
					retVal = 0;
					
				}
				else if ( CAN_rx.data.data_u8[0] == 'R' && CAN_rx.data.data_u8[1] == 'C' && CAN_rx.data.data_u8[2] == 'O' )
				{
					MAIN_resetRemoteCfg();
					retVal = 0;
				}
				else
				{
					retVal = -1;
				}
				CAN_txBuffer[CAN_RESET_INDEX].status = CAN_TXBUFFER_EMPTY;
				CAN_txBuffer[CAN_RESET_INDEX].data.data_u64 = 0;
				CAN_txBuffer[CAN_RESET_INDEX].data.data_u8[0] = (retVal<0)?'N':'Y';
				CAN_txBuffer[CAN_RESET_INDEX].status = CAN_TXBUFFER_WAITING;
				//WDTCTL = 0x00;	// Force watchdog reset
				//can.transmit();
			}
			else if ( CAN_rx.address == CAN_BC_BASE + CAN_BC_TIME_ID )
			{
				TIME_recFromBc( CAN_rx.data.data_u64 );
			}
			else if ( CAN_rx.address == CAN_BC_BASE + CAN_BC_OUTVOLT_CMD_ID )
			{
				CTRL_setOutVoltCmd( CAN_rx.data.data_fp[0] );
			}
			else if ( CAN_rx.address == CAN_BC_BASE + CAN_BC_ISSUEID_ID )
			{
				if ( CAN_rx.data.data_u32[1] == CFG_localCfg.serialNumber && CAN_rx.data.data_u32[0] != CAN_getBaseId() )
				{
					// Serial number matches and base ID is different, so change our base ID
					CAN_init( CAN_rx.data.data_u32[0] );
				}
			}
			/*else if ( CAN_rx.address == CAN_getBaseId() + CAN_TEMP_FLSET_ID )
			{
				//IO_flset( CAN_rx.data.data_u16[0] );
			}
			else if ( CAN_rx.address == CAN_getBaseId() + CAN_TEMP_ENABLE_ID )
			{
				if ( CAN_rx.data.data_u8[0] )
				{
					P2OUT |= ENABLE;                //RDD why
				}
				else
				{
					P2OUT &= ~(ENABLE);
				}
			}
			else if ( CAN_rx.address == CAN_getBaseId() + CAN_TEMP_CMD1_ID )
			{
				PWM_setMpptSamplePt( IQ_cnst( CAN_rx.data.data_fp[0] ) );
			}
			else if ( CAN_rx.address == CAN_getBaseId() + CAN_TEMP_CMD2_ID )
			{
				PWM_setVinLim( IQ_cnst( CAN_rx.data.data_fp[0] ) );
			}
			else if ( CAN_rx.address == CAN_getBaseId() + CAN_TEMP_CMD3_ID )
			{
				PWM_setFlTrim( IQ_cnst( CAN_rx.data.data_fp[0] ) );
			}*/
			else if ( CAN_rx.address == CAN_getBaseId() + CAN_BOOTLOAD_ID )
			{
				if (		CAN_rx.data.data_u8[0] == 'B' && CAN_rx.data.data_u8[1] == 'O' && CAN_rx.data.data_u8[2] == 'O' && CAN_rx.data.data_u8[3] == 'T'
						&&	CAN_rx.data.data_u8[4] == 'L' && CAN_rx.data.data_u8[5] == 'O' && CAN_rx.data.data_u8[6] == 'A' && CAN_rx.data.data_u8[7] == 'D' )
				{
					//__asm__ __volatile__ ( "br #0xf000" ); // jump to start of bootloader
//					WDTCTL = 0x00;	// Force watchdog reset		//****** Can't change this back without it breaking??
				}
			}
			else if ( CAN_rx.address == CAN_getBaseId() + CAN_P2P_REQ_ID )
			{
			
				comms.p2p.reqChars[0] = CAN_rx.data.data_u8[0];
				comms.p2p.reqChars[1] = CAN_rx.data.data_u8[1];
				if ( comms.p2p.reqChars[0] == 'R' )
				{
					// Master wants to read some data
					switch ( comms.p2p.reqChars[1] ) 
					{
						//case 'L': p2pReply = CFG_getLocalCfgLen(); retVal = COMMS_startFlashSend( FLASH_LOCAL_CFG_ADDR, p2pReply ); break;
						//case 'R': p2pReply = CFG_getRemoteCfgLen(); retVal = COMMS_startFlashSend( FLASH_REMOTE_CFG_ADDR, p2pReply ); break;
						case 'L': p2pReply = CFG_getLocalCfgLen(); retVal = COMMS_startMemSend( CFG_getLocalCfgAddr(), p2pReply ); break;
						case 'R': p2pReply = CFG_getRemoteCfgLen(); retVal = COMMS_startMemSend( CFG_getRemoteCfgAddr(), p2pReply ); break;
						case 'O': p2pReply = CFG_getReadonlyCfgLen(); retVal = COMMS_startMemSend( CFG_getReadonlyCfgAddr(), p2pReply ); break;
						case 'S': 
							tmpAddr = FLASH_STATS_ADDR + IQ_min(CAN_rx.data.data_u16[1],FLASH_NUM_STATS_BLOCKS-1)*BLOCK_SIZE;
							p2pReply = BLOCK_SIZE; //STATS_getStatsLen( tmpAddr ); 
							retVal = COMMS_startFlashSend( tmpAddr, p2pReply ); 
							break;
						//case 'S': p2pReply = STATS_getStatsLen(); retVal = COMMS_startMemSend( STATS_getStatsMemAddr(), p2pReply ); break;
						case 'F': 
							tmpAddr = FLASH_FLAG_ADDR + IQ_min(CAN_rx.data.data_u16[1],FLASH_NUM_FLAG_BLOCKS-1)*BLOCK_SIZE;
							p2pReply = BLOCK_SIZE; //FLAG_getFlagLen(); 
							retVal = COMMS_startFlashSend( tmpAddr, p2pReply ); 
							break;
						case 'T': 
							tmpAddr = FLASH_TELEM_START_ADDR + IQ_min(CAN_rx.data.data_u16[1],FLASH_NUM_TELEM_BLOCKS-1)*BLOCK_SIZE;
							p2pReply = BLOCK_SIZE; //TELEM_getTelemLen( tmpAddr ); 
							retVal = COMMS_startFlashSend( tmpAddr, p2pReply ); 
							break;
					}
				}
				else if ( comms.p2p.reqChars[0] == 'W' ) 
				{
					
					// Master wants to write some data
					switch ( comms.p2p.reqChars[1] ) 
					{
						case 'L': p2pReply = CFG_getLocalCfgLen(); retVal = COMMS_startFlashRec( FLASH_LOCAL_CFG_ADDR, p2pReply ); break;
						case 'R': p2pReply = CFG_getRemoteCfgLen(); retVal = COMMS_startFlashRec( FLASH_REMOTE_CFG_ADDR, p2pReply ); break;
						case 'S': 
							/*
							p2pReply = STATS_getStatsLenTotal(); 
							STATS_eraseStarted(); 
							retVal = FLASH_erase( FLASH_STATS_ADDR, p2pReply, STATS_eraseDoneCallback ); 
							if ( retVal < 0 ) STATS_eraseDoneCallback(retVal);
							else STATS_resetAddr();	// The erase has begun, so reset the addr to start of stats flash
							*/
							p2pReply = 0; 
							break;
						case 'F': 
							p2pReply = FLAG_getFlagLen(); 
							FLAG_eraseStarted();
							retVal = FLASH_erase( FLASH_FLAG_ADDR, p2pReply, FLAG_eraseDoneCallback ); 
							if ( retVal < 0 ) FLAG_eraseDoneCallback(retVal);
							p2pReply = 0; 
							break;
						//case 'T': p2pReply = TELEM_getTelemLen(); retVal = FLASH_erase( FLASH_TELEM_START_ADDR, p2pReply ); p2pReply = 0; break;
						case 'T': 
							/*
							p2pReply = TELEM_getNumBlocks() * BLOCK_SIZE; 
							TELEM_eraseStarted(); 
							retVal = FLASH_erase( FLASH_TELEM_START_ADDR, p2pReply, TELEM_eraseDoneCallback ); 
							if ( retVal < 0 ) TELEM_eraseDoneCallback(retVal);
							//else STATS_resetAddr();	// The erase has begun, so reset the addr to start of stats flash
							*/
							p2pReply = 0; 
							break;
						case 'E': retVal = 0; COMMS_endFlashRec(); break;
					}
				}
				else if ( comms.p2p.reqChars[0] == 'I' )
				{
					// Master is requesting a specific peice of information
					switch( comms.p2p.reqChars[1] )
					{
						case 'S': 
							retVal = 0; //(STATS_getIsReady()?0:-1); 
							/*
							p2pReply = ((unsigned long)STATS_getCurrentBlock()) | (((unsigned long)STATS_getNumBlocks())<<16); 
							*/
							break;
						case 'T': 
							retVal = 0; //(TELEM_getIsReady()?0:-1); 
							/*
							tmMin = (Time)CAN_rx.data.data_16[1] + ( (Time)CAN_rx.data.data_32[1] << 16 );
							if ( tmMin == 0 )
								p2pReply = ((unsigned long)TELEM_getMostRecentBlock()) | (((unsigned long)TELEM_getNumBlocks())<<16); 
							else
							{
								p2pReply = ((unsigned long)TELEM_getNumBlocksByTime(tmMin)) | (((unsigned long)TELEM_getNumBlocks())<<16); 
							}
							*/
							break;
					}
				}
				
				CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_EMPTY;
				//can.address = CAN_getBaseId() + CAN_P2P_REPLY_ID;
				CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u64 = CAN_rx.data.data_u64;
				if ( retVal == -1 ) 
				{
					CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[2] = 'N';			// Refused
					CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[1] = p2pReply;
				}
				else if ( retVal == -2 )
				{
					CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[2] = 'P';			// Accepted, but pause before transmission
					CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[1] = 0;
				}
				else 
				{
					CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[2] = 'Y';			// Accepted, active
					CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[1] = p2pReply;
				}
				CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_WAITING;
				//can.transmit();
			}
			else if ( CAN_rx.address == CAN_getBaseId() + CAN_P2P_MOSI_ID )
			{
				if ( comms.p2p.mode == P2P_FLASH_REC_ACTIVE )
				{
					FLASH_writeStr(CAN_rx.data.data_u8, 8);
				}
			}
		}

		// Process a remote request
		if ( CAN_rx.status == CAN_RTR )
		{

		}
	}
}

int COMMS_startFlashSend( unsigned long addr, unsigned long len )
{
	if ( comms.p2p.mode ) return -1;

	comms.p2p.mode = P2P_FLASH_SEND_ACTIVE;

	comms.p2p.currentAddr = addr;
	comms.p2p.lenRem = len;

	comms.p2p.packetCount = 0;

	return 0;
}

int COMMS_startMemSend( unsigned char * addr, unsigned int len )
{
	if ( comms.p2p.mode ) return -1;

	comms.p2p.mode = P2P_MEM_SEND_ACTIVE;

	comms.p2p.currentAddr = (unsigned long)addr;
	comms.p2p.lenRem = (unsigned long)len;

	comms.p2p.packetCount = 0;

	return 0;
}

int COMMS_startFlashRec( unsigned long addr, unsigned long len )
{
	COMMS_sendDebugPacket(1,2,3,4);

	static unsigned int triedOnce = 0;
	if ( comms.p2p.mode ) 
	{
		if(!triedOnce)
		{
			triedOnce = 1;
			return -1;
			
		} else {
			triedOnce = 0;
			//had more than one request since starting the write, try resetting the flash controller and erasing again
			FLASH_init();
		}
	}

	// Need to erase first
	if ( FLASH_erase( addr, len, 0 ) < 0 ) return -1;

	comms.p2p.mode = P2P_FLASH_REC_PAUSED;

	comms.p2p.currentAddr = addr;
	comms.p2p.lenRem = len;

	return -2;	// so that 'pause for erasing' ack will be sent
}

void COMMS_sendP2pPacket()
{
	unsigned int bytesToSendThis, ii;
	int retVal;
	if (		comms.p2p.mode == P2P_FLASH_SEND_ACTIVE 
			&&	CAN_txBuffer[CAN_P2P_MISO_INDEX].status == CAN_TXBUFFER_EMPTY 
			&&	CAN_txBuffer[CAN_P2P_REPLY_INDEX].status == CAN_TXBUFFER_EMPTY )
	{
		bytesToSendThis = ( comms.p2p.lenRem < 6 ) ? comms.p2p.lenRem : 6;

		CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u16[0] = comms.p2p.packetCount;
		CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u16[1] = 0;
		CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u32[1] = 0;
		if ( ( retVal = FLASH_readStr( CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u8 + 2, comms.p2p.currentAddr, bytesToSendThis, FALSE ) ) == 0 )
		{
			//can.address = CFG_localCfg.canBaseId + CAN_P2P_MISO_ID;
			//can.transmit();
			CAN_txBuffer[CAN_P2P_MISO_INDEX].status = CAN_TXBUFFER_WAITING;
			comms.p2p.lenRem -= bytesToSendThis;
			comms.p2p.currentAddr += bytesToSendThis;
			comms.p2p.packetCount++;
		}
		//else
		//{
		//	// TEMP: Flash must be busy, signal this
		//	can.address = CAN_mpptCanBase + CAN_P2P_REPLY_ID;
		//	can.data.data_u8[0] = 'R';
		//	can.data.data_u8[1] = 'F';
		//	can.data.data_16[1] = retVal;
		//	can.transmit();
		//}

		if ( comms.p2p.lenRem == 0 )
		{
			COMMS_endFlashSend();
		}
	}
	else if (		comms.p2p.mode == P2P_MEM_SEND_ACTIVE
				&&	CAN_txBuffer[CAN_P2P_MISO_INDEX].status == CAN_TXBUFFER_EMPTY 
				&&	CAN_txBuffer[CAN_P2P_REPLY_INDEX].status == CAN_TXBUFFER_EMPTY )
	{
		bytesToSendThis = ( comms.p2p.lenRem < 6 ) ? comms.p2p.lenRem : 6;

		CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u16[0] = comms.p2p.packetCount;
		CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u16[1] = 0;
		CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u32[1] = 0;
		for ( ii = 0; ii < bytesToSendThis; ii++ )
		{
			CAN_txBuffer[CAN_P2P_MISO_INDEX].data.data_u8[ii+2] = *(unsigned char *)(comms.p2p.currentAddr++);
			comms.p2p.lenRem--;
		}
		//can.transmit();
		CAN_txBuffer[CAN_P2P_MISO_INDEX].status = CAN_TXBUFFER_WAITING;
		comms.p2p.packetCount++;

		if ( comms.p2p.lenRem == 0 )
		{
			COMMS_endMemSend();
		}
	}
	else if ( comms.p2p.mode == P2P_FLASH_REC_PAUSED && CAN_txBuffer[CAN_P2P_REPLY_INDEX].status == CAN_TXBUFFER_EMPTY )
	{
		// Try to start writing to flash
		CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_EMPTY;
		CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[1] = comms.p2p.lenRem;
		if ( FLASH_startWrite( comms.p2p.currentAddr, comms.p2p.lenRem ) == 0 )
		{
			// Erasing is complete, so signal P2P master if we are waiting to recieve data to be written
			//can.address = CFG_localCfg.canBaseId + CAN_P2P_REPLY_ID;
			CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[0] = 0;
			CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[0] = 'W';
			CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[1] = comms.p2p.reqChars[1];
			CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[2] = 'Y';
			//can.data.data_u32[1] = comms.p2p.lenRem; // happens above
			//can.transmit();
			CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_WAITING;

			comms.p2p.mode = P2P_FLASH_REC_ACTIVE;
		}
	}
}

void COMMS_endFlashSend()
{
	FLASH_readStr( 0, 0, 0, FALSE );	// Take flash module out of read mode

	comms.p2p.mode = P2P_IDLE;

	CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_P2P_REPLY_ID;
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[0] = 0;
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[1] = 0;
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[0] = 'R';
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[1] = 'E';
	//can.transmit();
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_endMemSend()
{
	comms.p2p.mode = P2P_IDLE;

	CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_EMPTY;
	//can.address = CFG_localCfg.canBaseId + CAN_P2P_REPLY_ID;
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[0] = 0;
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u32[1] = 0;
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[0] = 'R';
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].data.data_u8[1] = 'E';
	//can.transmit();
	CAN_txBuffer[CAN_P2P_REPLY_INDEX].status = CAN_TXBUFFER_WAITING;
}

void COMMS_endFlashRec()
{
	FLASH_endWriteData();

	comms.p2p.mode = P2P_IDLE;
}


