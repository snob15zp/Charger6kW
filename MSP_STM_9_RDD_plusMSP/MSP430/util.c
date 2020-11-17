
//-------------------------------------------------------------------
// File: util.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-27: original
//-------------------------------------------------------------------

#include "util.h"
#include "flash.h"

int UTIL_modAdd( int x, int y, int M )
{
	x += y;
	while ( x >= M ) x -= M;
	while ( x < 0 ) x += M;
	return x;
}

int UTIL_getCircBufferSpace( CircBuffer * pCb )
{
	if ( pCb->getInd <= pCb->putInd ) return ( (pCb->size-1) - (pCb->putInd-pCb->getInd) );
	else return ( pCb->getInd - pCb->putInd - 1 );
}

int UTIL_putToCircBuffer( CircBuffer * pCb, unsigned char x )
{
//	if ( UTIL_getCircBufferSpace( pCb ) )
//	{
//		pCb->pBuffer[pCb->putInd] = x;
//		pCb->putInd = UTIL_modAdd( pCb->putInd, 1, pCb->size );
//		return 0;
//	}
	return -1;
}

unsigned char UTIL_getFromCircBuffer( CircBuffer * pCb )
{
	unsigned char x = 0x55;
	if ( UTIL_getCircBufferSpace( pCb ) < (pCb->size-1) )
	{
		x = pCb->pBuffer[pCb->getInd];
		pCb->getInd = UTIL_modAdd( pCb->getInd, 1, pCb->size );
	}
	return x;
}

unsigned int UTIL_calcChecksum( unsigned char * pStart, unsigned int len )
{
	unsigned int checksum = 0, ii;
    for (ii = 0; ii < len; ii++)
    {
        checksum += pStart[ii];
    }
	return checksum;
}

unsigned int UTIL_calcChecksumFlash( unsigned long startAddr, unsigned int len )
{
	unsigned int checksum = 0, ii;
    for (ii = 0; ii < len; ii++)
    {
        checksum += FLASH_readU08(startAddr + ii);
    }
	return checksum;
}

unsigned int UTIL_decimalToBcd( unsigned int dec )
{
	unsigned int bcd, base, temp;
	int digit;
	base = 10000;
	bcd = 0;
	for ( digit = 4; digit >= 0; digit-- )
	{
		if ( dec >= base )
		{
			temp = dec/base;
			bcd += temp << (4*digit);
			dec -= temp * base;
		}
		base /= 10;
	}
	return bcd;
}

