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

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "Stm32f3xx.h"
/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR( void );

/* ----------------------- Start implementation -----------------------------*/
//BOOL xMBPortTimersInit( USHORT usTim1Timerout50us )
//{
//    return FALSE;
//}
BOOL xMBPortTimersInit( USHORT usTim1Timerout50us )
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->SR = 0;
	TIM2->PSC = 3600 - 1;
	TIM2->ARR = usTim1Timerout50us - 1;		
//	TIM2->ARR = 1000 - 1;			
//	TIM2->PSC = 63;
//	TIM2->ARR = 300;		
	
	TIM2->DIER = TIM_DIER_UIE;
	NVIC_SetPriority(TIM2_IRQn,14);
	NVIC_EnableIRQ(TIM2_IRQn);
	TIM2->CR1 |= TIM_CR1_CEN;
  return TRUE;
}

inline void vMBPortTimersEnable(  )
{
	TIM2->CNT = 0;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
}

inline void vMBPortTimersDisable(  )
{
	RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
}

inline void vMBPortTimersDelay( USHORT usTimeOutMS )
{
};

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */


void TIM2_IRQHandler(void)
{
	if(TIM2->SR & TIM_SR_UIF)
	{
		TIM2->SR = ~TIM_SR_UIF;
//		USART3->TDR = 0x47;
    pxMBPortCBTimerExpired();		
	}
}









