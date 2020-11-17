/*
 * HiResTim.h
 *
 *  Created on: 17 NOV. 2019
 *  Author: Yakov Churinov
 */

#ifndef CODE_INC_HIRESTIM_H_
#define CODE_INC_HIRESTIM_H_

#include "stm32f3xx.h"

#define BUCK_CLK_MIN					18000U														// 18 kHz
#define BUCK_CLK							20000U														// 20 kHz
#define BUCK_CLK_MAX					22000U														// 22 kHz
#define BUCK_PERIOD_MIN				52200
#define BUCK_PERIOD_MAX				64000
#define BUCK_PERIOD						(uint16_t)(72000000U * 16 / BUCK_CLK) 
#define HRTIM_AUTOCLBR_14us		(HRTIM_DLLCR_CALRTE_1 | HRTIM_DLLCR_CALRTE_0)
#define DUTY_MIN							BUCK_PERIOD / 20 									// 5%							
#define DUTY_MAX							((BUCK_PERIOD / 10)*6)						// 60%
#define ADC_OFFSET						1152U						//		
#define ADC_TRG								((BUCK_PERIOD /10) * 9)						// 90%							
#define MAX_DUTY_STEP_POS			1000
#define MAX_DUTY_STEP_NEG			(-1 * MAX_DUTY_STEP_POS)
#define PERIOD_STEP						200
#define PERIOD_STEP_NEG				(-1 * PERIOD_STEP)

typedef
	enum {
		TIM_A =	0,
		TIM_B,
		TIM_C,
		TIM_D,
		TIM_E,
	} hrTim_ent;

typedef
	enum{
		MODE_VOLTAGE_STAB,
		MODE_CURRENT_LIMIT
	} workMode_ent;



/** function prototype declarations **/
extern void initHighResolutionTimer(void);
extern uint16_t hrtimerUpdateDuty(uint16_t dutycycle);
extern void hrtimersGpioInit(void);
extern uint16_t hrtimersOutEnable(uint16_t duty);
extern void hrtimersOutDisable(void);


#endif /* CODE_INC_HIRESTIM_H_ */
