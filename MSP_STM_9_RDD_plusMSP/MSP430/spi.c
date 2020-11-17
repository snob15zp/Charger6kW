
//-------------------------------------------------------------------
// File: spi.c
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-27: original
//-------------------------------------------------------------------

///#include <msp430x24x.h>
#include "spi.h"

void SPI_init()
{
	/// RDD OLD hardware UCB1CTL1 |= UCSWRST; // Set UCSWRST

	/// RDD OLD hardware UCB1CTL0 = UCCKPH | UCMSB | UCMST;
	/// RDD OLD hardware UCB1CTL1 |= UCSSEL_2;
	/// RDD OLD hardware UCB1BR0 = 16;		// 16 Mhz / 16 = 1 MHz?  Doesn't seem to work at faster speeds??
	
	/// RDD OLD hardware UCB1CTL1 &= ~( UCSWRST ); // Clear UCSWRST to enable SPI operation
}

unsigned char SPI_writeRead( unsigned char byte )
{
	/// RDD OLD hardware UCB1TXBUF = byte;							// write the byte to be sent into the TXBUF
	/// RDD OLD hardware while ( ( UC1IFG & UCB1TXIFG ) == 0 );			// wait for the byte to be written out
	/// RDD OLD hardware while ( ( UC1IFG & UCB1RXIFG ) == 0 );			// wait for a byte to be read in
	/// RDD OLD hardware return ( UCB1RXBUF );						// return the recieved byte
	return 0;
}


