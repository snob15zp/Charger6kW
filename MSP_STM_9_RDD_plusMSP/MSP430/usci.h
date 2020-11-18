/*
 * Tritium MSP430 2xx USCI SPI interface header file
 * Copyright (c) 2009, Tritium Pty Ltd.  All rights reserved.
 *
 * Last Modified: C.Walsh 15 June 2011
 *
 */

#ifndef USCIV_H
#define USCIV_H

#include "protocol.h"

#include "debug.h"

#define PACKET_LENGTH		254
 
#define PACKET_HEARTBEAT 0x01
#define PACKET_NEW_FLOAT 0x02

extern telemetry_t telemetry_R;
extern factoryConfig_t factoryConfig_R;
extern userConfig_t userConfig_R;
extern eventConfig_t eventConfig_R;
extern sysInfo_t sysInfo_R;
extern command_t command_R;
extern miscState_t miscState_R;

extern factoryConfig_t factoryConfig_W;
extern userConfig_t userConfig_W;
extern eventConfig_t eventConfig_W;
extern setTime_t setTime_W;
extern miscState_t miscState_W;

extern persistentStorage_t persistentStorage;

// Public Function prototypes
//extern 	void 			usci_init( unsigned char clock );
//extern 	void			usci_transmit( unsigned char data );  //RDD CAN
//extern 	unsigned char 	usci_exchange( unsigned char data ); //RDD CAN
extern void uart_tx(void);  //Transmit data low level

extern	void			uart_init(void);
extern	int				uart_send(int type); //high level 
extern	void			uart_receive(void); // high level

//struct rxPacket {
//	unsigned char packet_type;
//	unsigned long time;
//	unsigned char checksum;	
//};

//typedef union _group_rxPacket {
//	unsigned char data_u8[RX_PACKET_LENGTH];
//	struct rxPacket p;
//} group_rxPacket;

// Private Function prototypes

#endif
