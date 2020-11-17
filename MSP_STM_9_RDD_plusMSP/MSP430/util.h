
//-------------------------------------------------------------------
// File: util.h
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef UTIL_H
#define UTIL_H

#include "debug.h"

// Constant Definitions
#define	TRUE			1
#define FALSE			0

// Typedefs for quickly joining multiple bytes/ints/etc into larger values
// These rely on byte ordering in CPU & memory - i.e. they're not portable across architectures
typedef union _group_64 {
	float data_fp[2];
	unsigned char data_u8[8];
	char data_8[8];
	unsigned int data_u16[4];
	int data_16[4];
	unsigned long data_u32[2];
	long data_32[2];
	unsigned long long data_u64;
	long long data_64;
} group_64;

typedef union _group_32 {
	float data_fp;
	unsigned char data_u8[4];
	char data_8[4];
	unsigned int data_u16[2];
	int data_16[2];
	unsigned long data_u32;
	long data_32;
} group_32;

typedef union _group_16 {
	unsigned char data_u8[2];
	char data_8[2];
	unsigned int data_u16;
	int data_16;
} group_16;


// Circular buffer
typedef struct CircBuffer_
{
	volatile unsigned char * pBuffer;
	volatile int putInd;
	volatile int getInd;
	int size;
} CircBuffer;

int UTIL_modAdd( int x, int y, int M );
int UTIL_getCircBufferSpace( CircBuffer * pCb );
int UTIL_putToCircBuffer( CircBuffer * pCb, unsigned char x );
unsigned char UTIL_getFromCircBuffer( CircBuffer * pCb );

unsigned int UTIL_calcChecksum( unsigned char * pStart, unsigned int len );
unsigned int UTIL_calcChecksumFlash( unsigned long startAddr, unsigned int len );

unsigned int UTIL_decimalToBcd( unsigned int dec );

#endif


