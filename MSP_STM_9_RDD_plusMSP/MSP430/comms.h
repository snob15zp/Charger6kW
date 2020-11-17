
//-------------------------------------------------------------------
// File: adc.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef COMMS_H
#define COMMS_H

#include "debug.h"

void COMMS_init();

unsigned char COMMS_getStatus();

void COMMS_sendHeartbeat();
void COMMS_sendStatus();
void COMMS_sendPvMeas();
void COMMS_sendOutMeas();
void COMMS_sendOcQMeas();
void COMMS_sendPowTemprMeas();
void COMMS_sendTime();
void COMMS_sendFlag();
void COMMS_sendOutVoltCmd();

void COMMS_receive();

void COMMS_sendP2pPacket();

void COMMS_sendDebugPacket( unsigned int val1, unsigned int val2, unsigned int val3, unsigned int val4 );
void COMMS_sendDebugPacketU64( unsigned long long val );
void COMMS_sendDebugPacketFloat( float val1, float val2 );

#endif // COMMS_H

