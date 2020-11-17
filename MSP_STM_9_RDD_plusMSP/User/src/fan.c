#include "stm32f3xx.h"
#include "fan.h"

void fanInit(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIODEN;
	GPIOD->MODER &= ~(GPIO_MODER_MODER2_0 | GPIO_MODER_MODER2_1);
	GPIOD->MODER |= GPIO_MODER_MODER2_0;
	GPIOD->OTYPER &= GPIO_OTYPER_OT_2;
	GPIOD->ODR &= ~GPIO_ODR_2;
}

void fanOn(void)
{
	GPIOD->ODR |= GPIO_ODR_2;
}

void fanOff(void)
{
	GPIOD->ODR &= ~GPIO_ODR_2;
}