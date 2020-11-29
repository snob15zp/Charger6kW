/*!

\file 

\brief CAN


 * Tritium MCP2515 CAN Interface
 * Copyright (c) 2009, Tritium Pty Ltd.  All rights reserved.
 *
 * Last Modified: J.Kennedy, Tritium Pty Ltd, 30 September 2009
 *
 * - Implements the following CAN interface functions
 *	- CAN_init
 *	- CAN_transmit
 *	- CAN_receive
 *
 
 
 mainMSP.c:mainMSPloop()->          can.h:CAN_transmit()
 
 
 mainMSP.c:MAIN_resetAllAndStart()->can.h:CAN_init()
 mainMSP.c:MAIN_resetRemoteCfg    ->can.h:CAN_init()
 
 comms.c:COMMS_receive()          ->can.h:CAN_init()
 
 
 comms.c:COMMS_receive()->          can.h:CAN_receive()
 
 
 
*/





// Include files
//#include <msp430x24x.h>
#include "Stm32f3xx.h"
#include "can.h"
#include "usci.h"
//#include "mcp.h"
#include "io.h"
#include "cfg.h"
#include "main.h"

#define ALT_FUNC_CAN 0x09

unsigned long CAN_txBufferAddr[CAN_TxBufferInd_NUM];

// Public variables
//can_variables	can;
CAN_TxBuffer	CAN_txBuffer[CAN_TxBufferInd_NUM];
CAN_Rx			CAN_rx;

// Private variables
unsigned char 			buffer[16];
unsigned long			CAN_baseId;

// Private function prototypes
void 					CAN_transmit( void );
void 					CAN_receive( void );
void 					CAN_reset( void );
void 					CAN_read( unsigned char address, unsigned char *ptr, unsigned char bytes );
void 					CAN_read_rx( unsigned char address, unsigned char *ptr );
void 					CAN_write( unsigned char address, unsigned char *ptr, unsigned char bytes );
void 					CAN_write_tx( unsigned char address, unsigned char *ptr );
void 					CAN_rts( unsigned char address );
unsigned char 			CAN_read_status( void );
unsigned char 			CAN_read_filter( void );
void 					CAN_mod( unsigned char address, unsigned char mask, unsigned char data );

//unsigned long CAN_mpptCanBase;

/**************************************************************************************************
 * PUBLIC FUNCTIONS
 *************************************************************************************************/

unsigned long CAN_getBaseId( void )
{
	return CAN_baseId;
}

void CAN_init( unsigned long newBaseId )
{
	CAN_TxBufferInd txInd; 
	int ii;

	CAN_baseId = newBaseId;

	// Setup TX addresses
	ii = 0;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_HB_ID;
	CAN_txBufferAddr[ii++] = CAN_BC_BASE + CAN_BC_REQID_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_DEBUG_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_P2P_MISO_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_P2P_REPLY_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_FLAG_SEND_ID;
	CAN_txBufferAddr[ii++] = CAN_BC_BASE + CAN_BC_OUTVOLT_CMD_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_PV_MEAS_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_OUT_MEAS_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_OC_Q_MEAS_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_POW_TEMPR_MEAS_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_TIME_SEND_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_STATUS_ID;
	CAN_txBufferAddr[ii++] = CAN_baseId + CAN_RESET_ID;

	// Set up transmit buffers
	for ( txInd = CAN_TxBufferInd_MIN; txInd <= CAN_TxBufferInd_MAX; txInd++ )
	{
		CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
	}
	
	RCC->APB1ENR |= RCC_APB1ENR_CANEN;
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	
	GPIOB->MODER &= ~(GPIO_MODER_MODER8_0 | GPIO_MODER_MODER8_1);
	GPIOB->MODER &= ~(GPIO_MODER_MODER9_0 | GPIO_MODER_MODER9_1);
	GPIOB->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1;
	GPIOB->OTYPER &= ~GPIO_OTYPER_OT_9;
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR8_0 | GPIO_PUPDR_PUPDR8_1);
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR8_1;	//?????? 10 - pull-down

	GPIOB->AFR[1] |= ALT_FUNC_CAN | (ALT_FUNC_CAN<<GPIO_AFRH_AFRH1_Pos);
	
	CAN->MCR |= CAN_MCR_INRQ;	//initial mode
	while((CAN->MSR & CAN_MSR_INAK)==0){
	}																			//wait for initial mode
		
	//CAN->MCR |= CAN_MCR_ABOM | CAN_MCR_TTCM;
	
	// Set up reset and clocking
	//CAN_reset();
	
	// Need to do this before issueing first command to CAN controller.  Not sure why exactly.
	//brief_pause( 100 );

	//CAN_mod( CANCTRL, 0x03, 0x00 );			// CANCTRL register, modify lower 2 bits, CLK = /1 = 16 MHz
	
	
	//CAN_mpptCanBase = CFG_canBaseId();

	// Set up bit timing & interrupts // ATTN: 125kbps atm, for 500kps change buffer[2] to 0x00, for 250 0x01
	//buffer[0] = 0x05;						// CNF3 register: PHSEG2 = 6Tq, No wakeup, CLKOUT = CLK
	//buffer[1] = 0xF8;						// CNF2 register: set PHSEG2 in CNF3, Triple sample, PHSEG1= 8Tq, PROP = 1Tq
	//buffer[2] = 0x03;						// CNF1 register: SJW = 1Tq, BRP = 0
	switch ( CFG_localCfg.canBaud )
	{
		case BAUD_50:
			CAN->BTR = 0x001c002c;
//			buffer[0] = 0x05;
//			buffer[1] = 0xF8;
//			buffer[2] = 0x09;
			break;
		case BAUD_100:
			CAN->BTR = 0x0005002c;
//			buffer[0] = 0x05;
//			buffer[1] = 0xF8;
//			buffer[2] = 0x04;
			break;
		case BAUD_125:
			CAN->BTR = 0x001c0011;
//			buffer[0] = 0x05;
//			buffer[1] = 0xF8;
//			buffer[2] = 0x03;
			break;
		case BAUD_250:
			CAN->BTR = 0x001c0008;
//			buffer[0] = 0x05;
//			buffer[1] = 0xF1;
//			buffer[2] = 0x01;
			break;
		case BAUD_500:
			CAN->BTR = 0x00050008;
//			buffer[0] = 0x45;
//			buffer[1] = 0xF8;
//			buffer[2] = 0x00;
			break;
		case BAUD_1000:
			CAN->BTR = 0x001e0001;
//			buffer[0] = 0x02;
//			buffer[1] = 0xC9;
//			buffer[2] = 0x00;
	}
//	buffer[3] = 0x23;						// CANINTE register: enable ERROR, RX0 & RX1 interrupts on IRQ pin
//	buffer[4] = 0x00;						// CANINTF register: clear all IRQ flags
//	buffer[5] = 0x00;						// EFLG register: clear all user-changable error flags
	//CAN_write( CNF3, &buffer[0], 6);		// Write to registers
	
	CAN->FMR |= CAN_FMR_FINIT;
	
//	// Set up receive filtering & masks
//	// RXF0 - Buffer 0
//	buffer[ 0] = 0x00;
//	buffer[ 1] = 0x00;
//	buffer[ 2] = 0x00;
//	buffer[ 3] = 0x00;
//	// RXF1 - Buffer 0
//	buffer[ 4] = 0x00;
//	buffer[ 5] = 0x00;
//	buffer[ 6] = 0x00;
//	buffer[ 7] = 0x00;
//	// RXF2 - Buffer 1
//	buffer[ 8] = (unsigned char)(CAN_baseId >> 3);
//	buffer[ 9] = (unsigned char)(CAN_baseId << 5);
//	buffer[10] = 0x00;
//	buffer[11] = 0x00;
//	//CAN_write( RXF0SIDH, &buffer[0], 12 );
//	
//	// RXF3 - Buffer 1
//	buffer[ 0] = (unsigned char)(CAN_BC_BASE >> 3);
//	buffer[ 1] = (unsigned char)(CAN_BC_BASE << 5);
//	buffer[ 2] = 0x00;
//	buffer[ 3] = 0x00;
//	// RXF4 - Buffer 1
//	buffer[ 4] = 0x00;
//	buffer[ 5] = 0x00;
//	buffer[ 6] = 0x00;
//	buffer[ 7] = 0x00;
//	// RXF5 - Buffer 1
//	buffer[ 8] = 0x00;
//	buffer[ 9] = 0x00;
//	buffer[10] = 0x00;
//	buffer[11] = 0x00;
////	CAN_write( RXF3SIDH, &buffer[0], 12 );

//	// RXM0 - Buffer 0
//	buffer[ 0] = 0xFF;						// Match entire 11 bit ID (ID is left-justified in 32-bit mask register)
//	buffer[ 1] = 0xE0;
//	buffer[ 2] = 0x00;
//	buffer[ 3] = 0x00;
//	// RXM1 - Buffer 1
//	buffer[ 4] = 0xFC;						// Match upper 6 bits
//	buffer[ 5] = 0x00;
//	buffer[ 6] = 0x00;
//	buffer[ 7] = 0x00;
////	CAN_write( RXM0SIDH, &buffer[0], 8 );

	CAN->FM1R &= ~CAN_FM1R_FBM0;
	CAN->FS1R |= CAN_FS1R_FSC0;
	CAN->FFA1R &= ~CAN_FFA1R_FFA0;
	CAN->sFilterRegister[0].FR1 = 0x00;
	CAN->sFilterRegister[0].FR2 = 0x00;
 	CAN->FA1R |= CAN_FA1R_FACT0;
	
	CAN->FM1R &= ~CAN_FM1R_FBM1;
	CAN->FS1R |= CAN_FS1R_FSC1;
	CAN->FFA1R &= ~CAN_FFA1R_FFA1;
	CAN->sFilterRegister[1].FR1 = 0x00;
	CAN->sFilterRegister[1].FR2 = 0x00;
 	CAN->FA1R |= CAN_FA1R_FACT1;
	
	// Switch out of config mode into normal operating mode
	CAN->FMR &= ~CAN_FMR_FINIT;
	CAN->MCR |= CAN_MCR_NART;
	CAN->MCR &= ~CAN_MCR_INRQ;	//initial mode exit
	CAN->MCR &= ~CAN_MCR_SLEEP;	//sleep mode exit
	while((CAN->MSR & CAN_MSR_INAK)!=0){
	}																			//wait for initial mode exit
//	CAN_mod( CANCTRL, 0xE0, 0x00 );			// CANCTRL register, modify upper 3 bits, mode = Normal
}

/*
 * Receives a CAN message from the MCP2515
 *	- Run this routine when an IRQ is received
 *	- Query the controller to identify the source of the IRQ
 *		- If it was an ERROR IRQ, read & clear the Error Flag register, and return it
 *		- If it was an RX IRQ, read the message and address, and return them
 *		- If both, handle the error preferentially to the message
 *	- Clear the appropriate IRQ flag bits
 */

void CAN_receive( void )
{
	unsigned char flags = 0;
	unsigned char err_flags;
	unsigned char tec;
	unsigned char rec;
	unsigned int can_esr;
	unsigned int can_rdlr;
	unsigned int can_rdhr;
	unsigned int can_rir;
	
	// Read out the interrupt flags register
	//CAN_read( CANINTF, &flags, 1 );
	if(CAN->RF0R & CAN_RF0R_FULL0)
		flags=1;	//
	if(CAN->RF1R & CAN_RF1R_FULL1)
		flags |= (1<<1);
	if(CAN->TSR & CAN_TSR_RQCP0)
		flags |= (1<<2);
	if(CAN->TSR & CAN_TSR_RQCP1)
		flags |= (1<<3);
	if(CAN->TSR & CAN_TSR_RQCP2)
		flags |= (1<<4);
	if(CAN->MSR & CAN_MSR_ERRI)
		flags |= (1<<5);
	if(CAN->MSR & CAN_MSR_WKUI)
		flags |= (1<<6);
	
	// Check for errors
	if(CAN->MSR & CAN_MSR_ERRI){
		// Read error flags and counters
		//CAN_read( EFLAG, &buffer[0], 1 );
		//CAN_read( TEC, &buffer[1], 2 );
		can_esr = CAN->ESR;
		rec = can_esr>>24;
		can_esr = can_esr<<8;
		tec = can_esr>>24;
		if(CAN->ESR & CAN_ESR_EWGF)
			err_flags = 1;
		if(rec>=96)
			err_flags |= (1<<1);
		if(tec>=96)
			err_flags |= (1<<2);
		if(rec>=128)
			err_flags |= (1<<3);
		if(tec>=128)
			err_flags |= (1<<4);
		if(CAN->ESR & CAN_ESR_BOFF)
			err_flags |= (1<<5);
		if(CAN->RF0R & CAN_RF0R_FOVR0)
			err_flags |= (1<<6);
		if(CAN->RF1R & CAN_RF1R_FOVR1)
			err_flags |= (1<<7);
		// Clear error flags
		//CAN_mod( EFLAG, buffer[0], 0x00 );	// Modify (to '0') all bits that were set
		CAN->ESR &= ~(CAN_ESR_EWGF | CAN_ESR_BOFF);
		CAN->RF0R &= ~CAN_RF0R_FOVR0;
		CAN->RF1R &= ~CAN_RF1R_FOVR1;
		// Return error code, a blank address field, and error registers in data field
		CAN_rx.status = CAN_ERROR;
		CAN_rx.address = 0x0000;
		CAN_rx.data.data_u8[0] = flags;		// CANINTF
		CAN_rx.data.data_u8[1] = err_flags;	// EFLG
		CAN_rx.data.data_u8[2] = tec;	// TEC
		CAN_rx.data.data_u8[3] = rec;	// REC
		// Clear the IRQ flag
		//CAN_mod( CANINTF, MCP_IRQ_ERR, 0x00 );
		CAN->MSR &= CAN_MSR_ERRI;
	}	
	
	// No error, check for received messages, buffer 0
	else if((CAN->RF0R & 0x03) != 0x00){
		// Read in the info, address & message data
		//CAN_read( RXB0CTRL, &buffer[0], 14 );
		// Fill out return structure
		// check for Remote Frame requests and indicate the status correctly
		//if(( buffer[0] & MCP_RXB0_RTR ) == 0x00 ){
		if((CAN->sFIFOMailBox[0].RIR & CAN_RI0R_RTR) == 0x00 ){
			// We've received a standard data packet
			CAN_rx.status = CAN_OK;
			// Fill in the data
			can_rdlr = CAN->sFIFOMailBox[0].RDLR;
			can_rdhr = CAN->sFIFOMailBox[0].RDHR;
			CAN_rx.data.data_u8[0] = (unsigned char)(can_rdlr & 0xFF);
			CAN_rx.data.data_u8[1] = (unsigned char)((can_rdlr>>8) & 0xFF);
			CAN_rx.data.data_u8[2] = (unsigned char)((can_rdlr>>16) & 0xFF);
			CAN_rx.data.data_u8[3] = (unsigned char)(can_rdlr>>24);
			CAN_rx.data.data_u8[4] = (unsigned char)(can_rdhr & 0xFF);
			CAN_rx.data.data_u8[5] = (unsigned char)((can_rdhr>>8) & 0xFF);
			CAN_rx.data.data_u8[6] = (unsigned char)((can_rdhr>>16) & 0xFF);
			CAN_rx.data.data_u8[7] = (unsigned char)(can_rdhr>>24);
		}
		else{
			// We've received a remote frame request
			// Data is irrelevant with an RTR
			CAN_rx.status = CAN_RTR;
		}
		// Fill in the address
//		CAN_rx.address = buffer[1];
//		CAN_rx.address = CAN_rx.address << 3;
//		buffer[2] = buffer[2] >> 5;
//		CAN_rx.address = CAN_rx.address | buffer[2];
		can_rir = CAN->sFIFOMailBox[0].RIR;
		CAN_rx.address = (unsigned char)((can_rir>>21) & 0xFF);
		// Clear the IRQ flag
//		CAN_mod( CANINTF, MCP_IRQ_RXB0, 0x00 );
		CAN->RF0R |= CAN_RF0R_RFOM0;
	}
	
	// No error, check for received messages, buffer 1
	else if((CAN->RF1R & 0x03) != 0x00){
		// Read in the info, address & message data
//		CAN_read( RXB1CTRL, &buffer[0], 14 );
		// Fill out return structure
		// check for Remote Frame requests and indicate the status correctly
//		if(( buffer[0] & MCP_RXB1_RTR ) == 0x00 ){
		if((CAN->sFIFOMailBox[1].RIR & CAN_RI0R_RTR) == 0x00 ){
			// We've received a standard data packet
			CAN_rx.status = CAN_OK;
			// Fill in the data
			can_rdlr = CAN->sFIFOMailBox[1].RDLR;
			can_rdhr = CAN->sFIFOMailBox[1].RDHR;
			CAN_rx.data.data_u8[0] = (unsigned char)(can_rdlr & 0xFF);
			CAN_rx.data.data_u8[1] = (unsigned char)((can_rdlr>>8) & 0xFF);
			CAN_rx.data.data_u8[2] = (unsigned char)((can_rdlr>>16) & 0xFF);
			CAN_rx.data.data_u8[3] = (unsigned char)(can_rdlr>>24);
			CAN_rx.data.data_u8[4] = (unsigned char)(can_rdhr & 0xFF);
			CAN_rx.data.data_u8[5] = (unsigned char)((can_rdhr>>8) & 0xFF);
			CAN_rx.data.data_u8[6] = (unsigned char)((can_rdhr>>16) & 0xFF);
			CAN_rx.data.data_u8[7] = (unsigned char)(can_rdhr>>24);
		}
		else{
			// We've received a remote frame request
			// Data is irrelevant with an RTR
			CAN_rx.status = CAN_RTR;
		}
		// Fill in the address
//		CAN_rx.address = buffer[1];
//		CAN_rx.address = CAN_rx.address << 3;
//		buffer[2] = buffer[2] >> 5;
//		CAN_rx.address = CAN_rx.address | buffer[2];
		can_rir = CAN->sFIFOMailBox[1].RIR;
		CAN_rx.address = (unsigned char)((can_rir>>21) & 0xFF);
		// Clear the IRQ flag
//		CAN_mod( CANINTF, MCP_IRQ_RXB1, 0x00 );
		CAN->RF1R |= CAN_RF1R_RFOM1;
	}
	else{
		CAN_rx.status = CAN_ERROR;
		CAN_rx.address = 0x0001;
		CAN_rx.data.data_u8[0] = flags;		// CANINTF
	}
}

/*
 * Transmits a CAN message to the bus
 *	- finds the first tx buffer with a message waiting to send
 *	- sends this message if there is a free mailbox
 *  - if a free mailbox exists and the id matches, sends on this mailbox preferentially
 */
void CAN_transmit( void )
{
	unsigned int txInd;
	unsigned int address;
	static unsigned int buf_addr[3] = {0xFFFF, 0xFFFF, 0xFFFF};
	unsigned int can_tdlr;
	unsigned int can_tdhr;
	unsigned int can_tir;

	// Find the first active transmit buffer
	for ( txInd = 0; txInd < CAN_TxBufferInd_NUM; txInd++ )
	{
		if ( CAN_txBuffer[txInd].status == CAN_TXBUFFER_WAITING )
		{
			break;
		}
	}

	// If we didn't find any active tx buffers, return without sending anything
	if ( txInd == CAN_TxBufferInd_NUM )
	{
		return;
	}

	// Get address
	address = CAN_txBufferAddr[txInd];
	
	// Fill data into buffer
	// Allow room at the start of the buffer for the address info if needed
//	buffer[ 5] = CAN_txBuffer[txInd].data.data_u8[0];
//	buffer[ 6] = CAN_txBuffer[txInd].data.data_u8[1];
//	buffer[ 7] = CAN_txBuffer[txInd].data.data_u8[2];
//	buffer[ 8] = CAN_txBuffer[txInd].data.data_u8[3];
//	buffer[ 9] = CAN_txBuffer[txInd].data.data_u8[4];
//	buffer[10] = CAN_txBuffer[txInd].data.data_u8[5];
//	buffer[11] = CAN_txBuffer[txInd].data.data_u8[6];
//	buffer[12] = CAN_txBuffer[txInd].data.data_u8[7];
	can_tdlr = (CAN_txBuffer[txInd].data.data_u8[3]<<24) | (CAN_txBuffer[txInd].data.data_u8[2]<<16) |
							(CAN_txBuffer[txInd].data.data_u8[1]<<8) | CAN_txBuffer[txInd].data.data_u8[0];
	can_tdhr = (CAN_txBuffer[txInd].data.data_u8[7]<<24) | (CAN_txBuffer[txInd].data.data_u8[6]<<16) |
							(CAN_txBuffer[txInd].data.data_u8[5]<<8) | CAN_txBuffer[txInd].data.data_u8[4];

	// Check if the incoming address has already been configured in a mailbox
	if( address == buf_addr[0]){
		// Mailbox 0 setup matches our new message, so write if we ready, otherwise do nothing while we wait for the existing message to be sent
//		if ( ( CAN_read_status() & 0x04 ) == 0x00 )
		if (CAN->TSR & CAN_TSR_TME0)
		{
			// Write to TX Buffer 0, start at data registers, and initiate transmission
//			CAN_write_tx( 0x01, &buffer[5] );	
			CAN->sTxMailBox[0].TDLR = can_tdlr;
			CAN->sTxMailBox[0].TDHR = can_tdhr;
//			CAN_rts( 0 );
			CAN->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
			CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
		}
	}
	else if( address == buf_addr[1] ){
		// Mailbox 1 setup matches our new message, so write if we ready, otherwise do nothing while we wait for the existing message to be sent
//		if ( ( CAN_read_status() & 0x10 ) == 0x00 )
		if (CAN->TSR & CAN_TSR_TME1) 
		{
			// Write to TX Buffer 1, start at data registers, and initiate transmission
//			CAN_write_tx( 0x03, &buffer[5] );
			CAN->sTxMailBox[1].TDLR = can_tdlr;
			CAN->sTxMailBox[1].TDHR = can_tdhr;
//			CAN_rts( 1 );
			CAN->sTxMailBox[1].TIR |= CAN_TI1R_TXRQ;
			CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
		}
	}
	else if( address == buf_addr[2] ){
		// Mailbox 2 setup matches our new message, so write if we ready, otherwise do nothing while we wait for the existing message to be sent
//		if ( ( CAN_read_status() & 0x40 ) == 0x00 )
		if (CAN->TSR & CAN_TSR_TME2) 
		{
			// Write to TX Buffer 2, start at data registers, and initiate transmission
//			CAN_write_tx( 0x05, &buffer[5] );	
			CAN->sTxMailBox[2].TDLR = can_tdlr;
			CAN->sTxMailBox[2].TDHR = can_tdhr;
//			CAN_rts( 2 );
			CAN->sTxMailBox[2].TIR |= CAN_TI1R_TXRQ;
			CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
		}
	}
	else{
		// No matches in existing mailboxes
		// No mailboxes already configured, so we'll need to load an address - set it up
//		buffer[0] = (unsigned char)(address >> 3);
//		buffer[1] = (unsigned char)(address << 5);
//		buffer[2] = 0x00;						// EID8
//		buffer[3] = 0x00;						// EID0
//		buffer[4] = 0x08;						// DLC = 8 bytes
		
		// Check if we've got any un-setup mailboxes free and use them
		// Otherwise, find a non-busy mailbox and set it up with our new address
		if( buf_addr[0] == 0xFFFF ){			// Mailbox 0 is free
			// Write to TX Buffer 0, start at address registers, and initiate transmission
//			CAN_write_tx( 0x00, &buffer[0] );
			CAN->sTxMailBox[0].TIR = 0x00;
			CAN->sTxMailBox[0].TIR = address<<21;
			CAN->sTxMailBox[0].TDTR = 0x08;
			CAN->sTxMailBox[0].TDLR = can_tdlr;
			CAN->sTxMailBox[0].TDHR = can_tdhr;
//			CAN_rts( 0 );
			CAN->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
			buf_addr[0] = address;
			CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
		}									
		else if( buf_addr[1] == 0xFFFF ){		// Mailbox 1 is free
			// Write to TX Buffer 1, start at address registers, and initiate transmission
//			CAN_write_tx( 0x02, &buffer[0] );
			CAN->sTxMailBox[1].TIR = 0x00;
			CAN->sTxMailBox[1].TIR = address<<21;
			CAN->sTxMailBox[1].TDTR = 0x08;
			CAN->sTxMailBox[1].TDLR = can_tdlr;
			CAN->sTxMailBox[1].TDHR = can_tdhr;
//			CAN_rts( 1 );
			CAN->sTxMailBox[1].TIR |= CAN_TI0R_TXRQ;
			buf_addr[1] = address;
			CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
		}
		else if( buf_addr[2] == 0xFFFF ){		// Mailbox 2 is free
			// Write to TX Buffer 2, start at address registers, and initiate transmission
//			CAN_write_tx( 0x04, &buffer[0] );
			CAN->sTxMailBox[2].TIR = 0x00;
			CAN->sTxMailBox[2].TIR = address<<21;
			CAN->sTxMailBox[2].TDTR = 0x08;
			CAN->sTxMailBox[2].TDLR = can_tdlr;
			CAN->sTxMailBox[2].TDHR = can_tdhr;
//			CAN_rts( 2 );
			CAN->sTxMailBox[2].TIR |= CAN_TI0R_TXRQ;
			buf_addr[2] = address;
			CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
		}
		else {					
			// No mailboxes unused, so find one that isn't currently transmitting and change the address
			//while(( CAN_read_status() & 0x54 ) == 0x54);
			// Is mailbox 0 free?
//			if(( CAN_read_status() & 0x04 ) == 0x00) {
			if (CAN->TSR & CAN_TSR_TME0){
				// Setup mailbox 0 and send the message
//				CAN_write_tx( 0x00, &buffer[0] );
				CAN->sTxMailBox[0].TIR = 0x00;	//clear TI0R
				CAN->sTxMailBox[0].TIR = address<<21;	//standard ID = address
				CAN->sTxMailBox[0].TDTR = 0x08;	//number of bytes
				CAN->sTxMailBox[0].TDLR = can_tdlr;
				CAN->sTxMailBox[0].TDHR = can_tdhr;
//				CAN_rts( 0 );
				CAN->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
				buf_addr[0] = address;
				CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
			}
			// Is mailbox 1 free?
//			else if(( CAN_read_status() & 0x10 ) == 0x00) {
			else if (CAN->TSR & CAN_TSR_TME1){
				// Setup mailbox 1 and send the message
//				CAN_write_tx( 0x02, &buffer[0] );
				CAN->sTxMailBox[1].TIR = 0x00;
				CAN->sTxMailBox[1].TIR = address<<21;
				CAN->sTxMailBox[1].TDTR = 0x08;
				CAN->sTxMailBox[1].TDLR = can_tdlr;
				CAN->sTxMailBox[1].TDHR = can_tdhr;
//				CAN_rts( 1 );
				CAN->sTxMailBox[1].TIR |= CAN_TI0R_TXRQ;
				buf_addr[1] = address;
				CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
			}
			// Is mailbox 2 free?
//			else if(( CAN_read_status() & 0x40 ) == 0x00) {
			else if (CAN->TSR & CAN_TSR_TME2){
				// Setup mailbox 2 and send the message
//				CAN_write_tx( 0x04, &buffer[0] );
				CAN->sTxMailBox[2].TIR = 0x00;
				CAN->sTxMailBox[2].TIR = address<<21;
				CAN->sTxMailBox[2].TDTR = 0x08;
				CAN->sTxMailBox[2].TDLR = can_tdlr;
				CAN->sTxMailBox[2].TDHR = can_tdhr;
//				CAN_rts( 2 );
				CAN->sTxMailBox[2].TIR |= CAN_TI0R_TXRQ;
				buf_addr[2] = address;
				CAN_txBuffer[txInd].status = CAN_TXBUFFER_EMPTY;
			}
		}			
	}
}

///**************************************************************************************************
// * PRIVATE FUNCTIONS
// *************************************************************************************************/

///*
// * Resets MCP2515 CAN controller via SPI port
// *	- SPI port must be already initialised
// */
//void CAN_reset( void )
//{
////	can_select;
////	usci_transmit( MCP_RESET );
////	can_deselect;
//	CAN->MCR |= CAN_MCR_RESET;
//}
// 
///*
// * Reads data bytes from the MCP2515
// *	- Pass in starting address, pointer to array of bytes for return data, and number of bytes to read
// */
//void CAN_read( unsigned char address, unsigned char *ptr, unsigned char bytes )
//{
//	unsigned char i;
//	
//	can_select;
////	usci_transmit( MCP_READ );
//	usci_transmit( address );
//	for( i = 0; i < bytes; i++ ) *ptr++ = usci_exchange( 0x00 );
//	can_deselect;
//}

///*
// * Reads data bytes from receive buffers
// *	- Pass in buffer number and start position as defined in MCP2515 datasheet
// *		- For starting at data, returns 8 bytes
// *		- For starting at address, returns 13 bytes
// */
///*void CAN_read_rx( unsigned char address, unsigned char *ptr )
//{
//	unsigned char i;
//	
//	address &= 0x03;						// Force upper bits of address to be zero (they're invalid)
//	address <<= 1;							// Shift input bits to correct location in command byte
//	address |= MCP_READ_RX;					// Construct command byte for MCP2515
//	
//	can_select;
//	usci_transmit( address );
//	
//	if(( address & 0x02 ) == 0x00 ){		// Start at address registers
//		for( i = 0; i < 13; i++ ){
//			*ptr++ = usci_exchange( 0x00 );
//		}
//	}
//	else{									// Start at data registers
//		for( i = 0; i < 8; i++ ){
//			*ptr++ = usci_exchange( 0x00 );
//		}
//	}
//	can_deselect;
//}*/

///*
// * Writes data bytes to the MCP2515
// *	- Pass in starting address, pointer to array of bytes, and number of bytes to write
// */
//void CAN_write( unsigned char address, unsigned char *ptr, unsigned char bytes )
//{
//	unsigned char i;
//	
//	can_select;
////	usci_transmit( MCP_WRITE );
//	usci_transmit( address );
//	for( i = 0; i < (bytes-1); i++ ){
//		usci_transmit( *ptr++ );
//	}
//	usci_transmit( *ptr );
//	can_deselect;
//}

///*
// * Writes data bytes to transmit buffers
// *	- Pass in buffer number and start position as defined in MCP2515 datasheet
// *		- For starting at data, accepts 8 bytes
// *		- For starting at address, accepts 13 bytes
// */
//void CAN_write_tx( unsigned char address, unsigned char *ptr )
//{
//	unsigned char i;
//	
//	address &= 0x07;						// Force upper bits of address to be zero (they're invalid)
////	address |= MCP_WRITE_TX;				// Construct command byte for MCP2515
//	
//	can_select;
//	usci_transmit( address );
//	
//	if(( address & 0x01 ) == 0x00 ){		// Start at address registers
//		for( i = 0; i < 13; i++ ){
//			usci_transmit( *ptr++ );
//		}
//	}
//	else{									// Start at data registers
//		for( i = 0; i < 8; i++ ){
//			usci_transmit( *ptr++ );
//		}
//	}
//	can_deselect;
//}

///*
// * Request to send selected transmit buffer
// *	- Pass in address of buffer to transmit: 0, 1 or 2
// */
//void CAN_rts( unsigned char address )
//{
//	unsigned char i;
//	
//	// Set up address bits in command byte
//	
//	//i = MCP_RTS;
//	if( address == 0 ) i |= 0x01;
//	else if( address == 1 ) i |= 0x02;
//	else if( address == 2 ) i |= 0x04;
//	
//	// Write command
//	can_select;
//	usci_transmit( i );
//	can_deselect;
//}

///*
// * Reads MCP2515 status register
// */
//unsigned char CAN_read_status( void )
//{
//	unsigned char status;
//	
//	can_select;
////	usci_transmit( MCP_STATUS );
//	status = usci_exchange( 0x00 );
//	can_deselect;
//	return status;
//}

///*
// * Reads MCP2515 RX status (filter match) register
// */
///*unsigned char CAN_read_filter( void )
//{
//	unsigned char status;
//	
//	can_select;
//	usci_transmit( MCP_FILTER );
//	status = usci_exchange( 0x00 );
//	can_deselect;
//	return status;
//}*/
// 
///*
// * Modifies selected register in MCP2515
// *	- Pass in register to be modified, bit mask, and bit data
// */
////void CAN_mod( unsigned char address, unsigned char mask, unsigned char data )
////{
////	can_select;
//////	usci_transmit( MCP_MODIFY );
//////	usci_transmit( address );
//////	usci_transmit( mask );
//////	usci_transmit( data );
////	
////	can_deselect;
////}

