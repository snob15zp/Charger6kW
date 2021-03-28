
//-------------------------------------------------------------------
// File: flash.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

//#include <msp430x24x.h>
#include "Stm32f3xx.h"
#include "flash.h"
#include "spi.h"
#include "io.h"
#include "util.h"
///#include "comms.h"

#define CONT_READ_25MHZ				0x03
//#define CONT_READ_80MHZ				0x0B
#define STATUS_REG_READ				0x05
#define	SECTOR_ERASE					0x20
#define	BLOCK_ERASE_32				0x52
#define	BLOCK_ERASE_64				0xD8
#define SINGLE_BYTE_PROGRAM		0x02
//		#define SEQ_BYTE_PROGRAM			0xAD
#define WRITE_ENABLE					0x06
#define WRITE_DISABLE					0x04
#define ENABLE_SREG_WRITE			0x50
#define SREG_WRITE						0x01

#define CSHIGH							(GPIOA->BSRR = GPIO_BSRR_BS_4)
#define CSLOW								(GPIOA->BSRR = GPIO_BSRR_BR_4)


typedef struct Flash_
{
	volatile FlashMode mode;
	unsigned long currentAddr;
	unsigned long lenRem;
	int hasWritten;
	CircBuffer buffer;
	void (*pCallback)(int);		// This will be called when an erase is complete, but in the tick() function, so make sure it's small
	unsigned char bufferStore[FLASH_BUFFER_SIZE];
} Flash;

Flash flash;

int FLASH_isBusy(void);

void FLASH_init()
{
	flash.mode = FLASH_IDLE;
	flash.buffer.putInd = 0;
	flash.buffer.getInd = 0;
	flash.buffer.pBuffer = flash.bufferStore;
	flash.buffer.size = FLASH_BUFFER_SIZE;

	SPI_writeRead(WRITE_DISABLE);
}

unsigned char FLASH_getStatus(void)
{
	return flash.mode;
}

// Return the current flash mode
FlashMode FLASH_getMode()
{
	return flash.mode;
}

int FLASH_isBusy(void)
{
	int busy;
	
	CSLOW;
	SPI_writeRead(STATUS_REG_READ);
	busy = ( SPI_writeRead(0) & 0x01 );
	CSHIGH;
	return (busy);
}

// Erase a single block, and busy-wait before returning
int FLASH_eraseBlockBusy( unsigned long blockAddr )
{
	if ( flash.mode ) return -1;

	flash.mode = FLASH_ERASEBUSY_ACTIVE;

	CSLOW;
	SPI_writeRead(ENABLE_SREG_WRITE);
	CSHIGH;
	
	CSLOW;
	SPI_writeRead(SREG_WRITE);
	SPI_writeRead(0x00);
	CSHIGH;
	
	CSLOW;
	SPI_writeRead(WRITE_ENABLE);
	CSHIGH;
	
	CSLOW;
	SPI_writeRead(SECTOR_ERASE);
	SPI_writeRead( (blockAddr>>16) & 0xFF );
	SPI_writeRead( (blockAddr>>8) & 0xFF );
	SPI_writeRead( (blockAddr) & 0xFF );
	CSHIGH;
	
	while ( FLASH_isBusy() )	{}
//	CSLOW;
//	SPI_writeRead(WRITE_DISABLE);
//	CSHIGH;

	flash.mode = FLASH_IDLE;

	return 0;
}

// Write a single byte, and busy-wait before returning
int FLASH_writeByteBusy( unsigned long addr, unsigned char byte )
{
	if ( flash.mode ) return -1;

	flash.mode = FLASH_WRITEBUSY_ACTIVE;

	CSLOW;
	SPI_writeRead(ENABLE_SREG_WRITE);
	CSHIGH;
	
	CSLOW;
	SPI_writeRead(SREG_WRITE);
	SPI_writeRead(0x00);
	CSHIGH;
	
	CSLOW;
	SPI_writeRead(WRITE_ENABLE);
	CSHIGH;
	CSLOW;
	SPI_writeRead(SINGLE_BYTE_PROGRAM);
	SPI_writeRead( (addr>>16) & 0xFF );
	SPI_writeRead( (addr>>8) & 0xFF );
	SPI_writeRead( (addr) & 0xFF );
	SPI_writeRead( byte );
	CSHIGH;
	while ( FLASH_isBusy() ) {}
//	CSLOW;
//	SPI_writeRead(WRITE_DISABLE);
//	CSHIGH;

	flash.mode = FLASH_IDLE;

	return 0;
}

// Erase all sectors touched by the given address range.
// This call starts erasing; FLASH_tick() continues it.
int FLASH_erase( unsigned long dstAddr, unsigned long len, void (*pCall)(int) )
{
	// Return if flash activity in progress already
	if ( flash.mode ) return -1;

	flash.mode = FLASH_ERASE_ACTIVE;

	// Round address down to start of smallest erasable block containing start address
	flash.currentAddr = dstAddr & ERASE_MASK1;
	// Round length up to end of smallest erasable block containing end address
	flash.lenRem = ( ( len + ( dstAddr - flash.currentAddr ) - 1 ) & ERASE_MASK1 ) + ERASE_SIZE1;

	flash.pCallback = pCall;

	return 0;
}


int FLASH_startWrite( unsigned long dstAddr, unsigned long len )
{
	// Return if flash activity in progress already
	if ( flash.mode ) return -1;

	flash.mode = FLASH_WRITE_ACTIVE;

	flash.hasWritten = FALSE;
	flash.currentAddr = dstAddr;
	flash.lenRem = len;

	return 0;
}

// Copy data to be written into circular write buffer, and start write process
// Return amount of data added to buffer
int FLASH_writeStr( unsigned char * pSrc, unsigned long len )
{
	unsigned int ii, bytesToWriteThis;

	if ( flash.mode == FLASH_WRITE_ACTIVE )
	{
		bytesToWriteThis = ( flash.lenRem < len ) ? flash.lenRem : len;

		for ( ii = 0; ii < bytesToWriteThis; ii++ )
		{
			if ( UTIL_putToCircBuffer( &flash.buffer, pSrc[ii] ) < 0 )
			{
				break;
			}
		}

		flash.lenRem -= ii;

		return ii;
	}

	return -1;
}

// Called to indicate that no more data is expected in this write, so just keep writing until buffer is empty
void FLASH_endWriteData()
{
	if ( flash.mode == FLASH_WRITE_ACTIVE )
	{
		flash.lenRem = 0;
	}
}

void FLASH_endWrite()
{
	if ( flash.mode == FLASH_WRITE_ACTIVE )
	{
		CSLOW;
		SPI_writeRead(WRITE_DISABLE);
		CSHIGH;
		flash.buffer.getInd = 0;
		flash.buffer.putInd = 0;
	}
	flash.mode = FLASH_IDLE;
	flash.lenRem = 0;
}

int FLASH_getFreeBufferSpace()
{
	return UTIL_getCircBufferSpace( &flash.buffer );
}

int FLASH_readStr( unsigned char * pDst, unsigned long srcAddr, unsigned long len, int leaveInReadMode )
{
	unsigned int ii;
	if ( flash.mode != FLASH_IDLE && flash.mode != FLASH_READ_ACTIVE ) return -1;
	if ( FLASH_isBusy() ) return -2;

	if ( flash.mode == FLASH_IDLE && len > 0 )
	{
		CSLOW;
		SPI_writeRead(CONT_READ_25MHZ);
		SPI_writeRead((srcAddr>>16)&0xFF);
		SPI_writeRead((srcAddr>>8)&0xFF);
		SPI_writeRead((srcAddr>>0)&0xFF);
		//SPI_writeRead(0);
		flash.mode = FLASH_READ_ACTIVE;
	}

	for ( ii = 0; ii < len; ii++ ) 
	{
		pDst[ii] = SPI_writeRead(0);	
	}

	if ( !leaveInReadMode )
	{
		CSHIGH;
		flash.mode = FLASH_IDLE;
	}

	return 0;
}


// If erasing, erase next sector if not busy.
// If writing, continue writing from buffer until done.
void FLASH_tick()
{
	unsigned long numErasedBytes;
	unsigned char eraseCmd;
	if ( flash.mode == FLASH_ERASE_ACTIVE )
	{
		if ( !FLASH_isBusy() )
		{
			if (		( flash.currentAddr & ERASE_MASK3 ) == flash.currentAddr	// if we are at the start of large block
					&&	flash.lenRem >= ERASE_SIZE3	)								// and we have a large block left to erase
			{
				eraseCmd = BLOCK_ERASE_64;
				numErasedBytes = ERASE_SIZE3;
			}
			else if (		( flash.currentAddr & ERASE_MASK2 ) == flash.currentAddr	// if we are at the start of medium block
						&&	flash.lenRem >= ERASE_SIZE2	)								// and we have a medium block left to erase
			{
				eraseCmd = BLOCK_ERASE_32;
				numErasedBytes = ERASE_SIZE2;
			}
			else // must only have a small sector left to erase
			{
				eraseCmd = SECTOR_ERASE;
				numErasedBytes = ERASE_SIZE1;
			}
			CSLOW;
			SPI_writeRead(ENABLE_SREG_WRITE);
			CSHIGH;
			CSLOW;
			SPI_writeRead(SREG_WRITE);
			SPI_writeRead(0x00);
			CSHIGH;
			CSLOW;
			SPI_writeRead(WRITE_ENABLE);
			CSHIGH;
			CSLOW;
			SPI_writeRead(eraseCmd);			// issue the block erase command
			SPI_writeRead( (flash.currentAddr>>16) & 0xFF );
			SPI_writeRead( (flash.currentAddr>>8) & 0xFF );
			SPI_writeRead( (flash.currentAddr) & 0xFF );
			CSHIGH;
			if ( flash.lenRem <= numErasedBytes )
			{
				// Done erasing
				flash.lenRem = 0;
				flash.mode = FLASH_IDLE;
				if ( flash.pCallback != 0 )
  			{
					flash.pCallback(0);
				}
			}
			else
			{
				flash.currentAddr += numErasedBytes;
				flash.lenRem -= numErasedBytes;
			}
		}
	}

	else if ( flash.mode == FLASH_WRITE_ACTIVE )
	{
		if (		UTIL_getCircBufferSpace(&flash.buffer) < (flash.buffer.size-2)	// if we have at least two bytes of data in buffer to write
				&&	!FLASH_isBusy() )												// and flash part not busy.
		{

			CSLOW;
			if ( !flash.hasWritten ) 
			{
				SPI_writeRead(ENABLE_SREG_WRITE);
				CSHIGH;
				CSLOW;
				SPI_writeRead(SREG_WRITE);
				SPI_writeRead(0x00);
				CSHIGH;
				CSLOW;
				SPI_writeRead(WRITE_ENABLE);
				CSHIGH;
				CSLOW;
//				SPI_writeRead( SEQ_BYTE_PROGRAM );
				SPI_writeRead( SINGLE_BYTE_PROGRAM );
				SPI_writeRead( (flash.currentAddr>>16) & 0xFF);
				SPI_writeRead( (flash.currentAddr>>8) & 0xFF);
				SPI_writeRead( (flash.currentAddr>>0) & 0xFF);
			}
			else
			{
//				SPI_writeRead( SEQ_BYTE_PROGRAM );				
				SPI_writeRead( SINGLE_BYTE_PROGRAM );
			}
// Sequential programming takes two bytes -- assume we always program an even number of bytes
// so don't bother checking lengths, etc.
			SPI_writeRead( UTIL_getFromCircBuffer( &flash.buffer ) );
			SPI_writeRead( UTIL_getFromCircBuffer( &flash.buffer ) );
			CSHIGH;
			// Don't do this here -- this happens when data is added to the buffer for writing;
			// here we just write until the buffer is empty and we're not waiting for anything more.
//			//flash.currentAddr += 2;
//			//flash.lenRem -= 2
			flash.hasWritten = TRUE;
		}

		if ( ( flash.lenRem == 0 ) && ( UTIL_getCircBufferSpace(&flash.buffer) == (flash.buffer.size-1) ) ) FLASH_endWrite(); // nothing left to write
	}
}

unsigned char FLASH_readU08( unsigned long addr )
{
	unsigned char val;
	FLASH_readStr( (unsigned char *)&val, addr, sizeof(val), 0 );
	return val;
}

unsigned int FLASH_readU16( unsigned long addr )
{
	unsigned int val;
	FLASH_readStr( (unsigned char *)&val, addr, sizeof(val), 0 );
	return val;
}

int FLASH_readS16( unsigned long addr )
{
	int val;
	FLASH_readStr( (unsigned char *)&val, addr, sizeof(val), 0 );
	return val;
}

unsigned long FLASH_readU32( unsigned long addr )
{
	unsigned long val;
	FLASH_readStr( (unsigned char *)&val, addr, sizeof(val), 0 );
	return val;
}

float FLASH_readF32( unsigned long addr )
{
	float val;
	FLASH_readStr( (unsigned char *)&val, addr, sizeof(val), 0 );
	return val;
}

unsigned long long FLASH_readU64( unsigned long addr )
{
	unsigned long long val;
	FLASH_readStr( (unsigned char *)&val, addr, sizeof(val), 0 );
	return val;
}

