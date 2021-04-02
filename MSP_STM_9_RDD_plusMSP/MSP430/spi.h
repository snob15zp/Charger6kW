
//-------------------------------------------------------------------
// File: spi.h
// Project: CY CoolMax MPPT
// Device: MSP430F247
// Author: Monte MacDiarmid, Tritium Pty Ltd.
// Description: 
// History:
//   2010-07-27: original
//-------------------------------------------------------------------

#ifndef __SPI1_H__
#define __SPI1_H__

void init_SPI1(void);
unsigned char SPI_writeRead(unsigned char byte);

void init_TIM2(void);
void task_spi(void);

#endif // __SPI1_H__

