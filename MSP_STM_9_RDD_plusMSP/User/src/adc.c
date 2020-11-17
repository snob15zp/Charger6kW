/*
 * adc.c
 *
 *  Created on: 22 NOV. 2019 .
 *  Author: Yakov Churinov
 */


#include "adc.h"

/*
static const float conversCoeffArray[] = { I_CONVERCE_COEFF, VIN_CONVERCE_COEFF, I_CONVERCE_COEFF, VOUT_CONVERCE_COEFF, 
					                                 REFERENCE_VOLTAGE, V12_CONVERCE_COEFF, TMP_CMP_COEFF, I_CONVERCE_COEFF,
																					 LEAK_REF_COEFF, LEAK_CHK_COEFF, TMP_CASE_COEFF, INT_REF_COEFF };
*/

/******************************************************************************************
 *  ADC1 channel 1:  VREF_CPU
 *  ADC1 channel 2:  I_OUT_SENSOR
 *  ADC1 channel 3:  I_IN_SENSOR
 *  ADC1 channel 4:  TMP_CMP
 *  ADC1 channel 7:  V_LEAK_REF
 *  ADC1 channel 9:	 TMP_CASE  
 *
 *  ADC2 channel 8:  V_LEAK_CHECK 
 *  ADC2 channel 12: V12_SENSOR
 *  ADC2 channel 13: I_OUTCOM_SENSOR
 *  ADC2 channel 14: V_IN_SENSOR
 *  ADC2 channel 15: V_OUT_SENSOR
 *  ADC2 channel 18: V_INT_REF
 *
 *  Measurements are carried out in the mode of joint work of two ADCs.
 *  Sequence of parallel conversion:
 *  1: (ADC1_3, ADC2_14) - I_IN_SENSOR, V_IN_SENSOR
 *  2: (ADC1_2, ADC2_15) - I_OUT_SENSOR, V_OUT_SENSOR
 *  3: (ADC1_1, ADC2_12) - VREF_CPU, V12_SENSOR
 *  4: (ADC1_4, ADC2_13) - TMP_CMP, I_OUTCOM_SENSOR
 *  5: (ADC1_7, ADC2_8)  - V_LEAK_REF, V_LEAK_CHECK
 *  5: (ADC1_9, ADC2_18) - TMP_CASE, V_INT_REF
 ******************************************************************************************/
void initAdcToDualRegularSimultaneousMode(void){

	adcGpioConfig();																													// init adc GPIO

	RCC->AHBENR |= RCC_AHBENR_ADC12EN;																				// enable clock for ADC1 and ADC2
	RCC->CFGR2 	&= ~RCC_CFGR2_ADCPRE12_Msk;																		// select ADC_CLK use AHB clock = 72MHz

	ADC1->CR &= ~ADC_CR_ADVREGEN_Msk;																					// enable ADC1 voltage regulators
	ADC1->CR |=  ADC_CR_ADVREGEN_0;
	
	ADC2->CR &= ~ADC_CR_ADVREGEN_Msk;																					// enable ADC2 voltage regulators
	ADC2->CR |=  ADC_CR_ADVREGEN_0;

	uint32_t adcStartTime = (ADC_STARTUP_TIMEOUT * (SystemCoreClock / 1000000));
	while(--adcStartTime);																										// wait till ADC1 and ADC2 voltage regulators starts

	ADC12_COMMON->CCR = ADC12_CCR_VREFEN |																		// VREFINT channel enabled
											ADC12_CCR_MDMA_1 |																		// MDMA mode enabled (A single DMA channel is used)
											ADC12_CCR_DMACFG |																		// DMA Circular Mode selected
											ADC12_CCR_CKMODE_1 |																	// synchronous clock mode ADC_clk = HCLK/2 = 36 MHz
											ADC12_CCR_MULTI_2 | ADC12_CCR_MULTI_1;  							// dual ADC mode selection as Regular simultaneous mode only
	
	ADC1->CR |= ADC_CR_ADCAL;																									// Calibrate the ADC1 in single-ended input mode
	while(ADC1->CR & ADC_CR_ADCAL);

	ADC2->CR |= ADC_CR_ADCAL;																									// Calibrate the ADC2 in single-ended input mode
	while(ADC2->CR & ADC_CR_ADCAL);

	ADC1->CR |= ADC_CR_ADEN ;																									// enable ADC1
	while((ADC1->ISR & ADC_ISR_ADRDY) == 0);
	ADC2->CR |= ADC_CR_ADEN;																									// enable ADC2
	while((ADC2->ISR & ADC_ISR_ADRDY) == 0);

	ADC1->CFGR |= ( I_OUT_SENSOR << ADC_CFGR_AWD1CH_Pos )|										// enable analog watchdog for I OUT
								ADC_CFGR_AWD1EN | ADC_CFGR_AWD1SGL |
								ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_1 | ADC_CFGR_EXTSEL_0 |	// HRTIM_ADCTRG1 event Internal signal from on chip timers EXTSEL[3:0] = 0111
								ADC_CFGR_EXTEN_0; 																					// 01: Hardware trigger detection on the rising edge

	ADC1->SQR1 = 	(SIX_CONVERS << ADC_SQR1_L_Pos) |														// sequence from 6 conversions 
								( I_IN_SENSOR << ADC_SQR1_SQ1_Pos ) |
								( I_OUT_SENSOR << ADC_SQR1_SQ2_Pos ) |
								( VREF_CPU << ADC_SQR1_SQ3_Pos ) |
								( TMP_CMP << ADC_SQR1_SQ4_Pos );

	ADC1->SQR2 = 	( V_LEAK_REF << ADC_SQR2_SQ5_Pos ) |
								( TMP_CASE << ADC_SQR2_SQ6_Pos );

	ADC1->SMPR1 |= ( ADC_SMPL_7C5 << ADC_SMPR1_SMP1_Pos ) |										// Tconv = (7.5 + 12.5) ADC clock cycles = 20 ADC clock cycles = 20/36 = 0.55 µs 
								 ( ADC_SMPL_7C5 << ADC_SMPR1_SMP2_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR1_SMP3_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR1_SMP4_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR1_SMP7_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR1_SMP9_Pos );

	ADC2->SQR1 =	( SIX_CONVERS << ADC_SQR1_L_Pos) |													// sequence from 6 conversions 
								( V_IN_SENSOR << ADC_SQR1_SQ1_Pos ) |
								( V_OUT_SENSOR << ADC_SQR1_SQ2_Pos ) |
								( V12_SENSOR << ADC_SQR1_SQ3_Pos ) |
								( I_OUTCOM_SENSOR << ADC_SQR1_SQ4_Pos );

	ADC2->SQR2 = 	( V_LEAK_CHECK << ADC_SQR2_SQ5_Pos ) |
								( V_INT_REF << ADC_SQR2_SQ6_Pos );

	ADC2->SMPR1 |= ( ADC_SMPL_7C5 << ADC_SMPR1_SMP8_Pos );

	ADC2->SMPR2 |= ( ADC_SMPL_7C5 << ADC_SMPR2_SMP12_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR2_SMP13_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR2_SMP14_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR2_SMP15_Pos ) |
								 ( ADC_SMPL_7C5 << ADC_SMPR2_SMP18_Pos );

	ADC1->ISR = ADC_ISR_EOS | ADC_ISR_AWD1;	
	ADC1->IER = ADC_IER_AWD1IE;
	ADC1->CR |= ADC_CR_ADSTART;

	NVIC_SetPriority(ADC1_2_IRQn, 1);
	NVIC_EnableIRQ(ADC1_2_IRQn);
}

/***********************************************************************************************************************************
 * 
 *
 ***********************************************************************************************************************************/
void initDmaForAdc(uint32_t adcBuffAddr, uint32_t byteCount){

	RCC->AHBENR |= RCC_AHBENR_DMA1EN;																	// enable clock for DMA1

	DMA1_Channel1->CPAR = (uint32_t) (&(ADC12_COMMON->CDR));					// set the peripheral register address in the DMA 
	DMA1_Channel1->CMAR = adcBuffAddr;																// set the memory address in the DMA
	DMA1_Channel1->CNDTR = byteCount; 																// configure the total number of data to be transferred 
	DMA1_Channel1->CCR = DMA_CCR_PL_1 |																// channel priority level  PL[1:0] = 10: High
											 DMA_CCR_MSIZE_1 |														// memory size  MSIZE[1:0] = 10: 32-bits
											 DMA_CCR_PSIZE_1 |														// peripheral size size  PSIZE[1:0] = 10: 32-bits
											 DMA_CCR_MINC	|																// memory increment mode enabled 
										   DMA_CCR_CIRC |																// circular mode enabled
											 DMA_CCR_TCIE |												 				// transfer complete interrupt enable
											 DMA_CCR_EN;																	// channel enable

	NVIC_SetPriority(DMA1_Channel1_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

/******************************************************************************************
 *
 * ADC1 GPIO Configuration
 *  PA0  ------> ADC1_IN1
 *  PA1  ------> ADC1_IN2
 *  PA2  ------> ADC1_IN3
 *  PA3  ------> ADC1_IN4
 *
 * ADC12 GPIO Configuration
 *  PC1  ------> ADC12_IN7
 *  PC2  ------> ADC12_IN8
 *  PC3  ------> ADC12_IN9
 *
 * ADC2 GPIO Configuration
 *	PB2  ------> ADC2_IN12
 *  PB12 ------> ADC2_IN13
 *  PB14 ------> ADC2_IN14
 *  PB15 ------> ADC2_IN15
 *
 ******************************************************************************************/
#define MODE_ANALOG			3UL

void adcGpioConfig(void){

	GPIOA->MODER |= ( MODE_ANALOG << GPIO_MODER_MODER0_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER1_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER2_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER3_Pos );

	GPIOA->PUPDR |= ~( GPIO_PUPDR_PUPDR0_Msk | GPIO_PUPDR_PUPDR1_Msk |		// set no pull-up, no pull-down
										 GPIO_PUPDR_PUPDR2_Msk | GPIO_PUPDR_PUPDR3_Msk );

	GPIOB->MODER |= ( MODE_ANALOG << GPIO_MODER_MODER2_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER12_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER14_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER15_Pos );

	GPIOB->PUPDR |= ~( GPIO_PUPDR_PUPDR2_Msk | GPIO_PUPDR_PUPDR12_Msk |				// set no pull-up, no pull-down
										 GPIO_PUPDR_PUPDR14_Msk | GPIO_PUPDR_PUPDR15_Msk );

	GPIOC->MODER |= ( MODE_ANALOG << GPIO_MODER_MODER1_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER2_Pos ) |
									( MODE_ANALOG << GPIO_MODER_MODER3_Pos );

	GPIOC->PUPDR |= ~( GPIO_PUPDR_PUPDR1_Msk | GPIO_PUPDR_PUPDR2_Msk |				// set no pull-up, no pull-down
										 GPIO_PUPDR_PUPDR3_Msk );
}

/******************************************************************************************************
 *
 *
 ******************************************************************************************************/
void updateSumValue(uint16_t* pMomentVal, uint32_t* pSumValue){
	
	uint16_t i = 0; ;
	while(i < ADC_STRUCT_MEMBERS_NUM){
		*pSumValue += *pMomentVal;																							// add the new value to the sum	
		pSumValue++;
		pMomentVal++;
		i++;
	}
}


/******************************************************************************************************
 *
 *
 ******************************************************************************************************/
void updateAverageValue(float* pAverageValue, uint32_t* pSumValue){
	uint16_t i = 0; ;
	while(i < ADC_STRUCT_MEMBERS_NUM){
		*pAverageValue = *pSumValue * 1.0f / ADC_SAMPLE_NUMBER ;								// calc average value
		*pSumValue = 0;																													// erase sum value
		pSumValue++;
		pAverageValue++;
		i++;
	}
}

/******************************************************************************************************
 *
 *
 ******************************************************************************************************/
void updateCalcValue( floatValue_t* pAverageValue,  floatValue_t* pCalcValue ){	

	float adcMultipler = CPU_VREF_VALUE / pAverageValue->vrefCpu;
	float adcCurrMultipler = adcMultipler * 50 * I_CONVERCE_COEFF;

	pCalcValue->vInSensor = pAverageValue->vInSensor * adcMultipler * VIN_CONVERCE_COEFF;
	pCalcValue->vOutSensor = pAverageValue->vOutSensor * adcMultipler * VOUT_CONVERCE_COEFF;

	float delta = (pAverageValue->iInSensor - ZERO_CURR_CODE);
	pCalcValue->iInSensor = (delta >= 0) ? (delta * adcCurrMultipler) : 0.0f;

	delta = (pAverageValue->iOutSensor - ZERO_CURR_CODE);
 	pCalcValue->iOutSensor = (delta >= 0) ? (delta * adcCurrMultipler) : 0.0f;

	delta = (pAverageValue->iOutComSensor - ZERO_CURR_CODE);
	pCalcValue->iOutComSensor = (delta >= 0) ? (delta * adcCurrMultipler) : 0.0f;

	pCalcValue->v12Sensor = pAverageValue->v12Sensor * adcMultipler * V12_CONVERCE_COEFF;
	pCalcValue->vrefCpu = pAverageValue->vrefCpu * adcMultipler;

}

/******************************************************************************************************
 *
 *
 ******************************************************************************************************/
void setAdcMasterAnalogWatchdogThresholds(uint16_t hiThr, uint16_t loThr){
	ADC1->CR |= ADC_CR_ADSTP;
	while(ADC1->CR & ADC_CR_ADSTP){};
	ADC1->TR1 = (hiThr << ADC_TR1_HT1_Pos) | loThr;
	ADC1->CR |= ADC_CR_ADSTART;
}

