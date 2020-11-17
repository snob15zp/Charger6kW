
//-------------------------------------------------------------------
// File: flash.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-07: original
//-------------------------------------------------------------------

#ifndef FLASH_H
#define FLASH_H

#include "debug.h"

// The bits required to specify the start of a 4kb, 32kb or 64kb erase
#define ADDR_SPACE_SIZE 0x01000000ul
#define ERASE_SIZE1		0x00001000ul
#define ERASE_MASK1		( ADDR_SPACE_SIZE - ERASE_SIZE1 )
#define ERASE_SIZE2		0x00008000ul
#define ERASE_MASK2		( ADDR_SPACE_SIZE - ERASE_SIZE2 )
#define ERASE_SIZE3		0x00010000ul
#define ERASE_MASK3		( ADDR_SPACE_SIZE - ERASE_SIZE3 )

#define BLOCK_SIZE ERASE_SIZE1

/*	Flash address allocation
	------------------------
	Total size: 512 x 4Kb blocks
	Local Config:	1 block
	Remote Config:	1 block
	Stats:			12 blocks (should be enough for >365 days, should check against final struct size)
	Flags:			2 blocks (200 alarms, 6 bytes timestamp in ms since 1970 plus 2 byte alarm code gives 8 bytes per alarm, easily fits.  Need min two blocks as only have one active at a time.)
	Telemetry:		496 blocks

	Block Address	|	Allocation
	000 000h		|	Local Config
	001 000h		|	Remote Config
	002 000h		|	Stats
*/
#define FLASH_NUM_LOCAL_CFG_BLOCKS		1
#define FLASH_NUM_REMOTE_CFG_BLOCKS		1
#define FLASH_NUM_STATS_BLOCKS			14
#define FLASH_NUM_FLAG_BLOCKS			2

#define FLASH_NUM_USER_CFG_BLOCKS		1
#define FLASH_NUM_FACT_CFG_BLOCKS		1
#define FLASH_NUM_EVNT_CFG_BLOCKS		1

#define FLASH_NUM_CALIB_BLOCKS			1
#define FLASH_NUM_MISC_STATE_BLOCKS		1
#define FLASH_NUM_PERSISTENT_BLOCKS		1

#define FLASH_ORIGIN			0x00000000ul
#define FLASH_LOCAL_CFG_ADDR	FLASH_ORIGIN
#define FLASH_REMOTE_CFG_ADDR	( FLASH_LOCAL_CFG_ADDR + FLASH_NUM_LOCAL_CFG_BLOCKS*BLOCK_SIZE )
#define FLASH_STATS_ADDR		( FLASH_REMOTE_CFG_ADDR + FLASH_NUM_REMOTE_CFG_BLOCKS*BLOCK_SIZE )
#define FLASH_FLAG_ADDR			( FLASH_STATS_ADDR + FLASH_NUM_STATS_BLOCKS*BLOCK_SIZE )
#define FLASH_USER_CFG_ADDR		( FLASH_FLAG_ADDR + FLASH_NUM_FLAG_BLOCKS*BLOCK_SIZE )
#define FLASH_FACT_CFG_ADDR		( FLASH_USER_CFG_ADDR + FLASH_NUM_USER_CFG_BLOCKS*BLOCK_SIZE )
#define FLASH_EVNT_CFG_ADDR		( FLASH_FACT_CFG_ADDR + FLASH_NUM_FACT_CFG_BLOCKS*BLOCK_SIZE )
#define FLASH_CALIB_ADDR		( FLASH_EVNT_CFG_ADDR + FLASH_NUM_EVNT_CFG_BLOCKS*BLOCK_SIZE )
#define FLASH_MISC_STATE_ADDR	( FLASH_CALIB_ADDR + FLASH_NUM_CALIB_BLOCKS*BLOCK_SIZE )
#define FLASH_PERSISTENT_ADDR	( FLASH_MISC_STATE_ADDR + FLASH_NUM_MISC_STATE_BLOCKS*BLOCK_SIZE )
#define FLASH_TELEM_START_ADDR	( FLASH_PERSISTENT_ADDR + FLASH_NUM_PERSISTENT_BLOCKS*BLOCK_SIZE )
#define FLASH_TELEM_END_ADDR	0x00200000ul

#define FLASH_NUM_TELEM_BLOCKS			( ( FLASH_TELEM_END_ADDR - FLASH_TELEM_START_ADDR ) / BLOCK_SIZE )

#define FLASH_BUFFER_SIZE 192

typedef enum FlashMode_
{
	FLASH_IDLE = 0,
	FLASH_ERASE_ACTIVE,
	FLASH_WRITE_ACTIVE,
	FLASH_READ_ACTIVE,
	FLASH_ERASEBUSY_ACTIVE,
	FLASH_WRITEBUSY_ACTIVE
} FlashMode;

void FLASH_init(void);

unsigned char FLASH_getStatus(void);

int FLASH_erase( unsigned long dstAddr, unsigned long len, void (*pCall)(int) );

FlashMode FLASH_getMode(void);

int FLASH_eraseBlockBusy( unsigned long blockAddr );
int FLASH_writeByteBusy( unsigned long addr, unsigned char byte );

int FLASH_startWrite( unsigned long dstAddr, unsigned long len );
int FLASH_writeStr( unsigned char * pSrc, unsigned long len );
void FLASH_endWriteData(void);
void FLASH_endWrite(void);

int FLASH_getFreeBufferSpace(void);

void FLASH_tick(void);

int				FLASH_readStr( unsigned char * pDst, unsigned long srcAddr, unsigned long len, int leaveInReadMode );
// The below all busy wait on flash being free, so should only be called from init functions or similar situations
// without real-time concerns
unsigned char		FLASH_readU08( unsigned long addr );
unsigned int		FLASH_readU16( unsigned long addr );
int					FLASH_readS16( unsigned long addr );
#define				FLASH_readIq( a ) FLASH_readS16( a )
unsigned long		FLASH_readU32( unsigned long addr );
unsigned long long	FLASH_readU64( unsigned long addr );
float				FLASH_readF32( unsigned long addr );




#endif // FLASH_H
