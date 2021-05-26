/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */


#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "Stm32f3xx.h"

#define USART3_FCLK 			(36000000UL)
#define USART3_DEFBAUDRATE 			(9600UL)

unsigned long Time_Cycle = 0;
static USART_TypeDef* usart = USART3;

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );


/* ----------------------- Start implementation -----------------------------*/
void vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
    if( xRxEnable )
    {
			usart->CR1 |= USART_CR1_RXNEIE;
    }
    else
    {
			usart->CR1 &= ~USART_CR1_RXNEIE;			
    }

    if ( xTxEnable )
    {
			usart->CR1 |= USART_CR1_TXEIE;
    }
    else
    {
			usart->CR1 &= ~USART_CR1_TXEIE;			
    }
}


void txd_RS485_EN(void)
{
	GPIOC->BSRR = GPIO_BSRR_BS_12;	
}

void txd_RS485_DIS(void)
{
	GPIOC->BSRR = GPIO_BSRR_BR_12;	
}

void check_txd_RS485(void)
{
    if ( USART3->ISR & USART_ISR_TC_Msk )
    {
			txd_RS485_DIS();
    }
    
    MB_CopyStructToDiscreteInputs();
    MB_CopyStructToInputRegs();
}

			
		
		
BOOL xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;	
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;	
	NVIC_DisableIRQ(USART3_IRQn);	
	
//PB10_USART3_TX
	GPIOB->MODER &= ~GPIO_MODER_MODER10_Msk;
	GPIOB->MODER |= GPIO_MODER_MODER10_1;

	GPIOB->OTYPER &= ~GPIO_OTYPER_OT_10;	
	GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR10_Msk;
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR10_Msk;	
	
	GPIOB->AFR[1] &= ~GPIO_AFRH_AFRH2_Msk;
	GPIOB->AFR[1] |= (0x7UL << GPIO_AFRH_AFRH2_Pos);

//PB11	USART3_RX
	GPIOB->MODER &= ~GPIO_MODER_MODER11_Msk;
	GPIOB->MODER |= GPIO_MODER_MODER11_1;	

	GPIOB->OTYPER |= GPIO_OTYPER_OT_11; 
	GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR11_Msk;
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR11_Msk;	
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR11_1;	

	GPIOB->AFR[1] &= ~GPIO_AFRH_AFRH3_Msk;
	GPIOB->AFR[1] |= (0x7UL << GPIO_AFRH_AFRH3_Pos);
	
//PC12	RS485_EN	
	GPIOC->MODER &= ~GPIO_MODER_MODER12_Msk;
	GPIOC->MODER |= GPIO_MODER_MODER12_0;
	GPIOC->OTYPER &= ~GPIO_OTYPER_OT_12;	
	GPIOC->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR12_Msk;
	GPIOC->PUPDR &= ~GPIO_PUPDR_PUPDR12_Msk;
	GPIOC->BSRR = GPIO_BSRR_BR_12;	
	
	USART3->CR1 &= ~USART_CR1_UE;
//	USART3->BRR = USART3_PCLK/ulBaudRate;	
	USART3->BRR = USART3_FCLK/USART3_DEFBAUDRATE;
	


	switch ( eParity )
	{
		case MB_PAR_NONE:		
			USART3->CR1 &= ~USART_CR1_PCE;
			break;
		case MB_PAR_ODD:
			USART3->CR1 |= USART_CR1_PCE;
			USART3->CR1 |= USART_CR1_PS;		
			break;
		case MB_PAR_EVEN:
			USART3->CR1 |= USART_CR1_PCE;						
			USART3->CR1 &= ~USART_CR1_PS;
			break;
	}
	USART3->CR2 &= USART_CR2_STOP_Msk;
	USART3->CR1|= USART_CR1_TE | USART_CR1_RE; 	
	
	NVIC_SetPriority( USART3_IRQn, 7 );
	NVIC_EnableIRQ( USART3_IRQn );
	USART3->CR1 |= USART_CR1_UE;

	return TRUE;	
}

BOOL xMBPortSerialPutByte( CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
		usart->TDR = ucByte;
    return TRUE;
}

BOOL xMBPortSerialGetByte( CHAR *pucByte )
{
    CHAR data;	
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
    
    data = 	usart->RDR;
    *pucByte = data;
    return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty();
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived();
}


void USART3_IRQHandler(void)
{
  	if ( USART3->ISR & USART_ISR_RXNE )
    {
        USART3->ICR |= USART_ICR_ORECF;
        pxMBFrameCBByteReceived();			
        USART3->RQR |= USART_RQR_RXFRQ;
        usart = USART3;
    }
		
    if ( USART3->ISR & USART_ISR_TXE )
    {
        USART3->RQR |= USART_RQR_TXFRQ;		
        pxMBFrameCBTransmitterEmpty();
        usart = USART3;        
    }
}



#ifdef MODBUS
void USART1_IRQHandler(void)
{
    if ( (USART1->ISR & USART_ISR_TXE))
    {
        USART1->RQR |= USART_RQR_TXFRQ;
        pxMBFrameCBTransmitterEmpty();
        usart = USART1;
    }	
	
	if( (USART1->ISR & USART_ISR_RXNE))
	{
		USART1->ICR |= USART_ICR_ORECF;
		USART1->RQR |= USART_RQR_RXFRQ;
		pxMBFrameCBByteReceived();
        usart = USART1;
	}
}
#endif
