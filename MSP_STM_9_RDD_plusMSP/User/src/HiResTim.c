/*
 * HiResTim.c
 *
 *  Created on: 17 NOV. 2019 ã.
 *  Author: Yakov Churinov
 */

#include "HiResTim.h"


/******************************************************************************************
*
*
*******************************************************************************************/
void initHighResolutionTimer(void){

	RCC->CFGR3 |= RCC_CFGR3_HRTIM1SW_PLL;																			// use the PLLx2 clock for HRTIM
	RCC->APB2ENR |= RCC_APB2ENR_HRTIM1EN;																			// enable HRTIM clock

	HRTIM1->sCommonRegs.DLLCR |= HRTIM_AUTOCLBR_14us | HRTIM_DLLCR_CALEN;			// periodic calibration enabled, with the lowest calibration period (2048 x tHRTIM)
	while ((HRTIM1->sCommonRegs.ISR & HRTIM_ISR_DLLRDY) == 0);

	HRTIM1->sTimerxRegs[TIM_A].TIMxCR = HRTIM_TIMCR_CONT |										// timer operates in continuous mode and rolls over to zero when it reaches TIMxPER value
																			HRTIM_TIMCR_PREEN |										// preload enabled
																			//HRTIM_TIMCR_TREPU |										// update on repetition enabled
	                                    HRTIM_TIMCR_TRSTU |            // update is triggered by Timerx counter reset or roll-over to 0 after reaching the period value  
																			HRTIM_TIMCR_CK_PSC_1;									// Fhrck equivalent frequency (144 x 8)MHz = 1.152 GHz
	
	HRTIM1->sTimerxRegs[TIM_A].REPxR = 0;
	HRTIM1->sTimerxRegs[TIM_A].TIMxDIER = HRTIM_TIMDIER_REPIE;								// enable REP interrupts
//	HRTIM1->sTimerxRegs[TIM_A].TIMxDIER = HRTIM_TIMDIER_RSTIE;                // enable Reset/roll-over Interrupt Enable interrupts   


	HRTIM1->sTimerxRegs[TIM_A].PERxR = (uint16_t)BUCK_PERIOD;									// period for timer
	HRTIM1->sTimerxRegs[TIM_A].CMP1xR = DUTY_MIN;															// CMP1 event for duty cycle regulation
	
	HRTIM1->sTimerxRegs[TIM_A].SETx1R = HRTIM_SET1R_PER;											// out TA1 set on PER
	HRTIM1->sTimerxRegs[TIM_A].RSTx1R = HRTIM_RST1R_CMP1;											// out TA1 reset on CMP1

	HRTIM1->sTimerxRegs[TIM_A].CMP2xR = ADC_TRG;															// RDD Duty CMP2-> event for ADC trigger
	HRTIM1->sCommonRegs.CR1 = HRTIM_CR1_ADC1USRC_0; 													// ADC trigger update: Timer A 
	HRTIM1->sCommonRegs.ADC1R = HRTIM_ADC1R_AD1TAC2; 													// ADC trigger event: Timer A compare 2

	HRTIM1->sTimerxRegs[TIM_A].OUTxR &= ~( HRTIM_OUTR_POL1 |									// output 1 active polarity is high
																				 HRTIM_OUTR_POL2 |									// output 2 active polarity is high
																				 HRTIM_OUTR_IDLES1 |								// output 1 idle state inactive
																				 HRTIM_OUTR_IDLES2 );								// output 2 idle state inactive

	HRTIM1->sTimerxRegs[TIM_A].OUTxR |= HRTIM_OUTR_DTEN;											// dead time enable

	HRTIM1->sTimerxRegs[TIM_A].DTxR |= (HRTIM_DTR_DTPRSC_1 | HRTIM_DTR_DTPRSC_0) | 	// select Tdtg = 6.944 ns
																		 (HRTIM_DTR_DTR_6 | HRTIM_DTR_DTF_6) |		 		// Dead time falling and rising  = 16 * Tdtg = 111.1 ns
																		 (HRTIM_DTR_DTFSLK | HRTIM_DTR_DTRSLK);

	HRTIM1->sCommonRegs.ODISR = HRTIM_ODISR_TA1ODIS | HRTIM_ODISR_TA2ODIS;		// TA1 TA2 outputs disable
	
	HRTIM1->sCommonRegs.CR2 = HRTIM_CR2_TASWU;

	HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_TACEN;


	NVIC_SetPriority(HRTIM1_TIMA_IRQn, 0);
	NVIC_EnableIRQ(HRTIM1_TIMA_IRQn);

	hrtimersGpioInit();

}


/******************************************************************************************
*
*
*******************************************************************************************/
#define MODE_AF					0x02
#define ALT_FUNC_13			0x0D
#define HIGH_SPEED_OUT	0x03

void hrtimersGpioInit(void){

	GPIOA->MODER &= ~(GPIO_MODER_MODER9_Msk | GPIO_MODER_MODER8_Msk);					// set ALT function mode for the pins PA8, PA9
	GPIOA->MODER |= (MODE_AF << GPIO_MODER_MODER9_Pos) | (MODE_AF << GPIO_MODER_MODER8_Pos);

	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_8);									// set output push-pull for the pins PA8, PA9

	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR9_Msk | GPIO_PUPDR_PUPDR8_Msk);					// set no pull-up, pull-down  for the pins PA8, PA9

	GPIOA->OSPEEDR |= (HIGH_SPEED_OUT << GPIO_OSPEEDER_OSPEEDR9_Pos) |
										(HIGH_SPEED_OUT << GPIO_OSPEEDER_OSPEEDR8_Pos); 				// set high speed out for the pins PA8, PA9

	GPIOA->AFR[1] &= ~(GPIO_AFRH_AFRH1_Msk | GPIO_AFRH_AFRH0_Msk);						// enable alternate function for the pins PA8, PA9
	GPIOA->AFR[1] |= (ALT_FUNC_13 << GPIO_AFRH_AFRH1_Pos) | (ALT_FUNC_13 << GPIO_AFRH_AFRH0_Pos);

}

/******************************************************************************************
*
*
*******************************************************************************************/
uint16_t hrtimerUpdateDuty(uint16_t dutycycle){
	HRTIM1->sTimerxRegs[TIM_A].CMP1xR = dutycycle;														// CMP1 for duty cycle regulation
	HRTIM1->sTimerxRegs[TIM_A].CMP2xR = dutycycle + ADC_OFFSET;
	return 0;
}

/******************************************************************************************
*
* 
*******************************************************************************************/
uint16_t hrtimersOutEnable(uint16_t duty){
	HRTIM1->sTimerxRegs[TIM_A].CMP1xR = duty;																	// CMP1 event for duty cycle regulation
	HRTIM1->sTimerxRegs[TIM_A].CMP2xR = duty + ADC_OFFSET;
	HRTIM1->sCommonRegs.OENR = HRTIM_OENR_TA1OEN | HRTIM_OENR_TA2OEN;
	return duty;
}

/******************************************************************************************
*
*
*******************************************************************************************/
void hrtimersOutDisable(void){
	HRTIM1->sCommonRegs.ODISR = HRTIM_ODISR_TA1ODIS | HRTIM_ODISR_TA2ODIS;		// TA1 TA2 outputs disable
}
