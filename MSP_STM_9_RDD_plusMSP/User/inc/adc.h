/*
 * adc.h
 *
 *  Created on: 22 NOV. 2019 
 *  Author: Yakov Churinov
 */

#ifndef CODE_INC_ADC_H_
#define CODE_INC_ADC_H_

#include "stm32f3xx.h"
//#include "BoardInit.h"

/*  ADC1 channel numbers */
#define VREF_CPU							1
#define I_IN_SENSOR						2
#define I_OUT_SENSOR					3
#define TMP_CMP								4
#define V_LEAK_REF						7
#define TMP_CASE							9

/*  ADC2 channel number */
#define V_LEAK_CHECK					8
#define V12_SENSOR						12
#define I_OUTCOM_SENSOR				13
#define V_IN_SENSOR						14
#define V_OUT_SENSOR					15
#define V_INT_REF							18


#define FOUR_CONVERS					0x03
#define SIX_CONVERS						0x05
#define HRDW_TRIG_RSNG				0x01											// hardware Trigger with detection on the rising edge
#define HRTIM_ADCTRG2					0x09											// HRTIM_ADCTRG2 event
#define ADC_SMPL_7C5					0x03

#define ADC_STARTUP_TIMEOUT		20												// mks

#define ADC_AVERAGE_NUMBER		1
#define ADC_SAMPLE_NUMBER			ADC_AVERAGE_NUMBER * 1.0f

#define VIN_DIVIDER_R1				660.0f 										// kOhm
#define VIN_DIVIDER_R2				5.36f	 										// kOhm
#define REFERENCE_VOLTAGE			3.3f											// V
#define VIN_CONVERCE_COEFF		((VIN_DIVIDER_R1 + VIN_DIVIDER_R2) / VIN_DIVIDER_R2) 

#define VOUT_DIVIDER_R1				499.0f 										// kOhm
#define VOUT_DIVIDER_R2				5.36f	 										// kOhm
#define VOUT_CONVERCE_COEFF		((VOUT_DIVIDER_R1 + VOUT_DIVIDER_R2) / VOUT_DIVIDER_R2)

#define I_DIVIDER_R1					2.2f											// kOhm
#define I_DIVIDER_R2					3.3f	 										// kOhm
#define I_CONVERCE_COEFF			((I_DIVIDER_R1 + I_DIVIDER_R2) / I_DIVIDER_R2)

#define V12_DIVIDER_R1				10.0f 										// Ohm
#define V12_DIVIDER_R2				2.7f	 										// Ohm
#define V12_CONVERCE_COEFF		((V12_DIVIDER_R1 + V12_DIVIDER_R2) / V12_DIVIDER_R2)

#define CPU_VREF_VALUE				3.0f   
#define ZERO_CURR_CODE				1850U
#define FAULT_CURR_CODE				3200U
#define I_SENS_FOR_1A					20.0f / I_CONVERCE_COEFF	// mV

#define TMP_CMP_COEFF					1.0f
#define LEAK_REF_COEFF				1.0f
#define LEAK_CHK_COEFF				1.0f
#define TMP_CASE_COEFF				1.0f
#define INT_REF_COEFF					1.0f

typedef
	__packed struct{
		uint16_t iInSensor;
		uint16_t vInSensor;
		uint16_t iOutSensor;
		uint16_t vOutSensor;
		uint16_t vrefCpu;
		uint16_t v12Sensor;
		uint16_t tmpCmp;
		uint16_t iOutComSensor;
		uint16_t vLeakRef;
		uint16_t vLeakCheck;
		uint16_t tmpCase;
		uint16_t vRefInt;
} regAdcValue_t;

typedef
	__packed struct{
		uint32_t iInSensor;
		uint32_t vInSensor;
		uint32_t iOutSensor;
		uint32_t vOutSensor;
		uint32_t vrefCpu;
		uint32_t v12Sensor;
		uint32_t tmpCmp;
		uint32_t iOutComSensor;
		uint32_t vLeakRef;
		uint32_t vLeakCheck;
		uint32_t tmpCase;
		uint32_t vRefInt;
} wordAdcValue_t;

typedef
	__packed struct{
		float iInSensor;
		float vInSensor;
		float iOutSensor;
		float vOutSensor;
		float vrefCpu;
		float v12Sensor;
		float tmpCmp;
		float iOutComSensor;
		float vLeakRef;
		float vLeakCheck;
		float tmpCase;
		float vRefInt;
} floatValue_t;

#define ADC_STRUCT_MEMBERS_NUM	(sizeof(regAdcValue_t) / sizeof(int16_t))


/** function prototype declarations **/
extern void initAdcToDualRegularSimultaneousMode(void);
extern void initDmaForAdc(uint32_t adcBuffAddr, uint32_t byteCount);
extern void adcGpioConfig(void);
extern void updateSumValue(uint16_t* pMomentVal, uint32_t* pSumValue);
extern void updateAverageValue(float* pAverageValue, uint32_t* pSumValue);
extern void updateCalcValue(floatValue_t* pAverageValue,  floatValue_t* pCalcValue);
extern void setAdcMasterAnalogWatchdogThresholds(uint16_t hiThr, uint16_t loThr);

#endif /* CODE_INC_ADC_H_ */
