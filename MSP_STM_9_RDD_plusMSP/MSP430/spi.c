//-------------------------------------------------------------------
//-------------------------------------------------------------------

#include "Stm32f3xx.h"
#include "spi.h"

//-------------------------------------------------------------------
//-------------------------------------------------------------------

 void init_SPI1(void)
{
	NVIC_DisableIRQ(SPI1_IRQn);	
	
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	
	SPI1->CR1 &= ~(0x1UL << SPI_CR1_SPE_Pos);
	
//PA5	SCK	
	GPIOA->MODER &= ~GPIO_MODER_MODER5_Msk;
	GPIOA->MODER |= GPIO_MODER_MODER5_1;
	
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR5_Msk;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR5_0;

	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_5;	
	
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR5_Msk;	
	
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL5_Msk;
	GPIOA->AFR[0] |= (0x5UL << GPIO_AFRL_AFRL5_Pos);
	
//PA7	MOSI	
	GPIOA->MODER &= ~GPIO_MODER_MODER7_Msk;
	GPIOA->MODER |= GPIO_MODER_MODER7_1;
	
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR7_Msk;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR7_0;

	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_7;	
	
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR7_Msk;	
	
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL7_Msk;
	GPIOA->AFR[0] |= (0x5UL << GPIO_AFRL_AFRL7_Pos);
	
//PA4	CS	
	GPIOA->MODER &= ~GPIO_MODER_MODER4_Msk;
	GPIOA->MODER |= GPIO_MODER_MODER4_0;
	
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR4_Msk;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4_0;

	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_4;	
	
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR4_Msk;
  GPIOA->PUPDR |= GPIO_PUPDR_PUPDR4_0;
	
//PA6	MISO	
	GPIOA->MODER &= ~GPIO_MODER_MODER6_Msk;
		GPIOA->MODER |= /*GPIO_MODER_MODER6_0 | */GPIO_MODER_MODER6_1;
		
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR6_Msk;
	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6_0;
		
	GPIOA->OTYPER |= GPIO_OTYPER_OT_6;
		
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR6_Msk;	
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR6_0;	

	GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL6_Msk;
	GPIOA->AFR[0] |= (0x5UL << GPIO_AFRL_AFRL6_Pos);

	SPI1->CR1 =   (0x1UL << SPI_CR1_SSM_Pos)
							| (0x1UL << SPI_CR1_SSI_Pos)
							| (0x3UL << SPI_CR1_BR_Pos) 
							| (0x1UL << SPI_CR1_MSTR_Pos)
						  | (0x0UL << SPI_CR1_CPOL_Pos) | (0x0UL << SPI_CR1_CPHA_Pos);

	SPI1->CR2  = 0x1740;

	SPI1->CR1 |= (0x1UL << SPI_CR1_SPE_Pos);
}

unsigned char SPI_writeRead(unsigned char byte)
{
unsigned char dummy; 	
	while( !(SPI1->SR & SPI_SR_TXE) ) {}				
	*(unsigned char *)&SPI1->DR = byte;
	while( !(SPI1->SR & SPI_SR_TXE) ) {}				
	while( (SPI1->SR & SPI_SR_RXNE) == 0) {}				
	dummy = *(unsigned char *)&SPI1->DR;
	return dummy;
}
