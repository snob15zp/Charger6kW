/*
 * BoardInit.c
 *
 *  Created on: 16 NOV 2019 г.
 *  Author: Yakov Churinov
 
    SystemClock & DCDC control pin
 */

#include "stm32f3xx.h"
#include "BoardInit.h"


/******************************************************************************************
*	PLL (clocked by HSE) used as System clock source
*
*******************************************************************************************/
uint32_t setSystemClock(void){

	uint32_t startUpTimeout = HSE_STARTUP_TIMEOUT;

	RCC->CFGR &= (uint32_t)(~RCC_CFGR_SW);																		// select HSI as system clock
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI){}									// wait for it to switch

	RCC->CR |= ((uint32_t)RCC_CR_HSEON);																			// Enable HSE
	while(((RCC->CR & RCC_CR_HSERDY) == 0) && (startUpTimeout > 0)) {					// Wait until HSE is ready and if Time out is reached exit
		startUpTimeout--;
	}

	FLASH->ACR |= FLASH_ACR_LATENCY_1;																				//  set ratio of the HCLK period to the Flash access time for two wait sates, if 48 < HCLK ≤ 72 MHz

	RCC->CR &= ~RCC_CR_PLLON;																									// disable PLL
	while((RCC->CR & RCC_CR_PLLRDY) != 0){}																		// wait for it to switch
		
		RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV2;																		// HSE input to PLL divided by 2 = 16 / 2 = 8MHz

    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 |																				// HCLK  = SYSCLK div  1
								 RCC_CFGR_PPRE2_DIV1 |																			// PCLK2 = HCLK   div  1
								 RCC_CFGR_PPRE1_DIV2;																				// PCLK1 = HCLK   div  2

    RCC->CFGR |= (startUpTimeout > 0) ? ( RCC_CFGR_PLLMUL9 | RCC_CFGR_PLLSRC_HSE_PREDIV ) : // HSE/PREDIV clock selected as PLL entry clock source
																				( RCC_CFGR_PLLMUL16 | RCC_CFGR_PLLSRC_HSI_DIV2 ); // HSI clock divided by 2 selected as PLL entry clock source

    RCC->CR |= RCC_CR_PLLON;																								// Enable the main PLL
    while((RCC->CR & RCC_CR_PLLRDY) == 0){}	    														// Wait till the main PLL is ready

    RCC->CFGR |= RCC_CFGR_SW_PLL;																						// Select the main PLL as system clock source
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL){} 		// Wait till the main PLL is used as system clock source

		RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;

    return (startUpTimeout > 0) ? 72000000 : 64000000;											// if HSE enabled then return 72 MHz else return 64 MHz
}

/******************************************************************************************
*
*
*******************************************************************************************/
#define MODE_OUTPUT		0x01UL
#define LOW_SPEED_OUT	0x03

void initCoreIoPins(void){

	GPIOC->MODER &= ~(GPIO_MODER_MODER7_Msk | GPIO_MODER_MODER6_Msk);
	GPIOC->MODER |=	(MODE_OUTPUT << GPIO_MODER_MODER7_Pos) | (MODE_OUTPUT << GPIO_MODER_MODER6_Pos);
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_7 | GPIO_OTYPER_OT_6);
	GPIOC->OSPEEDR &= ~( GPIO_OSPEEDER_OSPEEDR7_Msk | GPIO_OSPEEDER_OSPEEDR6_Msk ); 	// set low speed out for the pins PC7, PC6
	GPIOC->BRR = GPIO_BRR_BR_7 | GPIO_BRR_BR_6;
	GPIOB->MODER &= ~(GPIO_MODER_MODER1_1 | GPIO_MODER_MODER1_0);
	GPIOB->MODER |= GPIO_MODER_MODER1_0;
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT_1);
	
}





