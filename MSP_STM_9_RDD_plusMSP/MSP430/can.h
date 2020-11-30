
/*
 * Tritium MCP2515 CAN interface header
 * Copyright (c) 2008, Tritium Pty Ltd.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *	- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
 *	  in the documentation and/or other materials provided with the distribution.
 *	- Neither the name of Tritium Pty Ltd nor the names of its contributors may be used to endorse or promote products 
 *	  derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. 
 *
 * Last Modified: J.Kennedy, Tritium Pty Ltd, 18 November 2008
 *
 * - Implements the following CAN interface functions
 *	- CAN_init
 *	- CAN_transmit
 *	- CAN_receive
 *
 */

#ifndef CAN_H
#define CAN_H

#include "Stm32f3xx.h"
#include "debug.h"
#include "util.h"

// Adding a transmit packet:
//		- Add the id to the list below
//		- Add an entry to the CAN_TxBufferInd enum at the correct priority
//		- Add an entry to the CAN_txBufferAddr array at the same location
// CAN packet offsets									Bytes:	0	1	2	3	4	5	6	7
#define CAN_HB_ID					0		// Transmit			<product id >	<serial #   > (uint32, uint32)
#define CAN_PV_MEAS_ID				1		// Transmit			<PV voltage >	<PV current > (float, float)
#define CAN_OUT_MEAS_ID				2		// Transmit			<out voltage>	<out current> (float, float)
#define CAN_OC_Q_MEAS_ID			3		// Transmit			<PV OC volt >	<out charge > (float, float)
#define CAN_POW_TEMPR_MEAS_ID		4		// Transmit			<PV power	>	<batt tempr > (float, float)
#define CAN_FLAG_SEND_ID			5		// Transmit			< active event bitfield     > (uint64)
#define CAN_STATUS_ID				6		// Transmit			< module status bitfield	> (maybe don't document this, say reserved)
#define CAN_TIME_SEND_ID			7		// Transmit			< time						> (uint64)		
//#define CAN_TEMP_FLSET_ID			13
//#define CAN_TEMP_ENABLE_ID			14
//#define CAN_TEMP_CMD1_ID			15		
//#define CAN_TEMP_CMD2_ID			16
//#define CAN_TEMP_CMD3_ID			17
#define CAN_P2P_REQ_ID				18		// Receive
#define CAN_P2P_REPLY_ID			19		// Transmit
#define CAN_P2P_MOSI_ID				20		// Receive
#define CAN_P2P_MISO_ID				21		// Transmit
#define CAN_BOOTLOAD_ID				22		// Receive			(don't document)
#define CAN_RESET_ID				23		// Receive/Transmit	< ALL/RCO >	-	-	-	-	- (send string 'ALL' for full reset, 'RCO' for remote config reset.  Replies with Y or N in byte 0)
#define CAN_DEBUG_ID				31		// Transmit			(don't document)

// Broadcast address and offsets
#define CAN_BC_BASE				0x760
#define CAN_BC_TIME_ID				1		// Recieve			< time						> (uint64)
#define CAN_BC_OUTVOLT_CMD_ID		2		// Transmit/Receive	<out voltage>	-	-	-	- (float)
#define CAN_BC_REQID_ID				3		// Transmit			<curr base id>	<serial #	> (uint32, uint32)
#define CAN_BC_ISSUEID_ID			4		// Recieve			<new base id >	<serial #	> (uint32, uint32)

// Transmit buffer indices, in order of priority; tx mailboxes on CAN controller will be filled with first ready packet with lowest index
typedef enum CAN_TxBufferInd_
{
	CAN_TxBufferInd_MIN = 0,

	CAN_HB_INDEX = CAN_TxBufferInd_MIN,
	CAN_BC_REQID_INDEX,
	CAN_DEBUG_INDEX,
	CAN_P2P_MISO_INDEX,
	CAN_P2P_REPLY_INDEX,
	CAN_FLAG_SEND_INDEX,
	CAN_BC_OUTVOLT_CMD_INDEX,
	CAN_PV_MEAS_INDEX,
	CAN_OUT_MEAS_INDEX,
	CAN_OC_Q_MEAS_INDEX,
	CAN_POW_TEMPR_MEAS_INDEX,
	CAN_TIME_SEND_INDEX,
	CAN_STATUS_INDEX,
	CAN_RESET_INDEX,

	CAN_TxBufferInd_MAX = CAN_RESET_INDEX
} CAN_TxBufferInd;
enum { CAN_TxBufferInd_NUM = (CAN_TxBufferInd_MAX - CAN_TxBufferInd_MIN) + 1 };

// Address associated with each transmit buffer -- this array should be in the same order as the CAN_TxBufferInd enum
extern unsigned long CAN_txBufferAddr[CAN_TxBufferInd_NUM];


// Baud rates
typedef enum CAN_BaudRate_
{
	BAUD_50 = 0,
	BAUD_100,
	BAUD_125,
	BAUD_250,
	BAUD_500,
	BAUD_1000
} CAN_BaudRate;

typedef enum CAN_TxBufferStatus_
{
	CAN_TXBUFFER_EMPTY = 0,
	CAN_TXBUFFER_WAITING
} CAN_TxBufferStatus;

typedef struct CAN_TxBuffer_
{
	CAN_TxBufferStatus	status;
	group_64			data;
} CAN_TxBuffer;

// Public variables
/*typedef struct _can_variables {
	//void				(*init)(void);
	void				(*transmit)(void);
	void				(*receive)(void);
	unsigned int		status;
	unsigned int 		address;
	group_64			data;
} can_variables;*/
typedef struct CAN_Rx_
{
	unsigned int	status;
	unsigned int	address;
	group_64		data;
} CAN_Rx;

// Status values (for message reception)
#define CAN_ERROR		0xFFFF
#define CAN_MERROR		0xFFFE
#define CAN_WAKE		0xFFFD
#define CAN_RTR			0xFFFC
#define CAN_OK			0x0001

// Public functions
//void CAN_echo( can_variables * src, can_variables * dst );
void CAN_init( unsigned long newBaseId );
void CAN_receive( void );
void CAN_transmit( void );
unsigned long CAN_getBaseId( void );

extern unsigned int 	CAN_status;
extern CAN_TxBuffer		CAN_txBuffer[CAN_TxBufferInd_NUM];
extern CAN_Rx			CAN_rx;

// SPI port interface macros
//#define can_select		P3OUT &= ~CAN_nCS
//#define can_select RCC->APB1ENR |= RCC_APB1ENR_CANEN
//#define can_deselect	P3OUT |= CAN_nCS
//#define can_deselect RCC->APB1ENR &= ~RCC_APB1ENR_CANEN

//#define IS_CAN_INT		( (P2IN & CAN_nINT) == 0x00 )
#define IS_CAN_INT ((CAN->RF0R & CAN_RF0R_FULL0) | (CAN->RF1R & CAN_RF1R_FULL1) | \
										(CAN->TSR & CAN_TSR_RQCP0) | (CAN->TSR & CAN_TSR_RQCP1) | \
										(CAN->TSR & CAN_TSR_RQCP2) | (CAN->MSR & CAN_MSR_ERRI) | \
										(CAN->MSR & CAN_MSR_WKUI))
										
		
#endif
