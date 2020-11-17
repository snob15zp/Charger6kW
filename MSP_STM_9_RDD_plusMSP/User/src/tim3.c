#include "Stm32f3xx.h"
#include "tim3.h"

void initTim3(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->PSC = 71;
	TIM3->ARR = 512;
	TIM3->DIER = TIM_DIER_UIE;
	NVIC_SetPriority(TIM3_IRQn,15);
	NVIC_EnableIRQ(TIM3_IRQn);
	TIM3->CR1 = TIM_CR1_CEN;
}
