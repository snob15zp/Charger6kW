
#include "Stm32f3xx.h"
#include "BoardInit.h"
#include "HiResTim.h"
#include "adc.h"


#include "dcdc.h"

/*
*
*   Interface Variable
*
*/

//typedef
//	__packed struct{
//		float iInSensor;
//		float vInSensor;
//		float iOutSensor;
//		float vOutSensor;
//		float vrefCpu;
//		float v12Sensor;
//		float tmpCmp;
//		float iOutComSensor;
//		float vLeakRef;
//		float vLeakCheck;
//		float tmpCase;
//		float vRefInt;
//} floatValue_t;

floatValue_t averageValue;
floatValue_t calculatedValue;

/*
*
*  Internal variable
*
*/

volatile struct DCDC_Flags{
	uint8_t FAULT_DETECT					;  
	uint8_t ADC_CONVERS_COMPLIT   ;
	uint8_t WORK_CYCLE_END				;
	uint8_t CONTROL_ENABLE				;
	uint8_t CONTROL_START				  ;
	uint8_t CONTROL_STOP					;
	uint8_t CURRENT_LIMIT				  ;
	uint8_t MAX_DUTY_LIMIT			  ;
	uint8_t MIN_DUTY_LIMIT				;
	uint8_t PERIOD_STEP_UP				;
};

 


#define VOUT_STAB				12U			// V
#define IOUT_STAB				70U			// A

/**  Global variables declarations start **/

static volatile  struct  DCDC_Flags statusFlags; 


volatile __align(4)  regAdcValue_t momentValue = {0,0,0,0,0,0,0,0,0,0,0,0}; //for DMA

__align(4) wordAdcValue_t sumValueBank_1 = {0,0,0,0,0,0,0,0,0,0,0,0}; //temporary summ 1
__align(4) wordAdcValue_t sumValueBank_2 = {0,0,0,0,0,0,0,0,0,0,0,0}; //temporary summ 2
 uint32_t *pSumValueNow = (uint32_t*)&sumValueBank_1;
 uint32_t *pSumValueStored = (uint32_t*)&sumValueBank_2;


 float adcVoutStab = VOUT_STAB;   //RD Target max output voltage  in Volts 
 float adcIoutStab = IOUT_STAB;   //RD Target max output current 
 uint16_t dutyCycle = DUTY_MIN;
 uint16_t buckPeriod = BUCK_PERIOD_MAX;  // ????



 int16_t delta;   //??????
// int16_t delta_duty;   //spectr
 int32_t delta_duty;   //spectr
 uint16_t offset = 500;  //????

 float efficiency; //??????

 float Vin_Target=0; //target input voltage for MPPT

//extern Ctrl ctrl;

/**  Global variables declarations end **/

/*************************************************************************************************************************
*
*          Interface functions
*
**************************************************************************************************************************/

int setVin(float Vin)
{
	Vin_Target=Vin;
	return 0;
};

int DCDC_Start_Stop(uint8_t SS)
{
	 if (SS) 
    {statusFlags.CONTROL_START=1;}
		 else 
    {statusFlags.CONTROL_STOP=1;};
	return 0;
};     //RDD 1-Start; 0-Stop: Not work HRtim

int DCDC_Enable_Disable(uint8_t ED)
{
	statusFlags.CONTROL_ENABLE=ED;
}; //RDD 1-Enable: DCDC work in stop mode; 0-Disable :  Not control DCDC, not regulator, but adc work



/*************************************************************************************************************************
*
*         Interrupt Handlers
*
**************************************************************************************************************************/
void DMA1_Channel1_IRQHandler(void)
{
//	GPIOB->BSRR = GPIO_BSRR_BR_1;
	
	DMA1->IFCR = DMA_IFCR_CGIF1;
	statusFlags.ADC_CONVERS_COMPLIT = 1;
	//EXTI->SWIER=EXTI_IMR_MR0;
	
	
	
	//measureExecute(); 
	
//	GPIOB->BSRR = GPIO_BSRR_BS_1;
	
}


//*************************************************************************************************************************
void ADC1_2_IRQHandler(void){    //RDD fault
	
	if ( ADC1->ISR & ADC_ISR_AWD1){
		ADC1->ISR = ADC_ISR_AWD1;
		statusFlags.FAULT_DETECT = 1;
		hrtimersOutDisable();
	}
}
//*************************************************************************************************************************
void EXTI0_IRQHandler(void)   //software interrupt for regulator
{
	GPIOB->BSRR = GPIO_BSRR_BR_1;
		EXTI->PR=EXTI_IMR_MR0;
	GPIOB->BSRR = GPIO_BSRR_BS_1;
//static	uint16_t tooglPin=0;
//		switch(tooglPin)
//			{
//			
//			case 0:
//				GPIOB->BSRR = GPIO_BSRR_BS_1;
//				tooglPin=1;
//				break;
//			case 1:
//				GPIOB->BSRR = GPIO_BSRR_BR_1;
//				tooglPin=0;
//				break;
//		  };

}

//*************************************************************************************************************************
uint16_t  Regulator(float Vin);

void HRTIM1_TIMA_IRQHandler(void)  
{
	GPIOB->BSRR = GPIO_BSRR_BR_1;
		HRTIM1->sTimerxRegs[TIM_A].TIMxICR=HRTIM_TIMICR_REPC;
	//  EXTI->SWIER=EXTI_IMR_MR0; //software interrupt for regulator
	
	  measureExecute(); 
	  
	  endOfCycleExecute();
	
	 	if(statusFlags.CONTROL_ENABLE)
		{ 
			dutyCycle= Regulator(Vin_Target);
			hrtimerUpdateDuty(dutyCycle);
/*
			efficiency = (calculatedValue.vOutSensor * calculatedValue.iOutSensor) /
									 (calculatedValue.vInSensor * calculatedValue.iInSensor) * 100;
*/
		}
	
	GPIOB->BSRR = GPIO_BSRR_BS_1;
//static	uint16_t tooglPin=0;
//		switch(tooglPin)
//			{
//			
//			case 0:
//				GPIOB->BSRR = GPIO_BSRR_BS_1;
//				tooglPin=1;
//				break;
//			case 1:
//				GPIOB->BSRR = GPIO_BSRR_BR_1;
//				tooglPin=0;
//				break;
//		  };

}



/**************************************************************************************************************************
***************************************************************************************************************************
***************************************************************************************************************************
*
***************************************************************************************************************************
***************************************************************************************************************************
***************************************************************************************************************************/


int DCDC_Init()
{
	initCoreIoPins();
	initAdcToDualRegularSimultaneousMode();
	initDmaForAdc( (uint32_t)&momentValue,  (sizeof(momentValue)/sizeof(uint32_t)) );
	setAdcMasterAnalogWatchdogThresholds( FAULT_CURR_CODE, 0);
	initHighResolutionTimer();


	//delay_ms(100);

	
	statusFlags.CONTROL_ENABLE = 0;
	statusFlags.CONTROL_STOP = 1;

//	GPIOD->BSRR |= GPIO_BSRR_BS_2;
	
  //software interrupt for regulator
	EXTI->IMR=EXTI_IMR_MR0;                	//EXTI_IMR_IM
	NVIC_SetPriority(EXTI0_IRQn, 2); //
  NVIC_EnableIRQ(EXTI0_IRQn);
	
	return 1;
}

int DCDC_Loop(char l)
{
	do 
	{
		//measureExecute();
		//endOfCycleExecute();
	}
	while(l);
	return 1;
}

/******************************************************************************************
* For V-I stabilizator, dont use for MPPT
*
*******************************************************************************************/
//uint16_t getNewDytyValue(void){

//	static workMode_ent workMode = MODE_VOLTAGE_STAB;
////RDD Spread spectrum begin
//	if(statusFlags.PERIOD_STEP_UP == 0)   //RDD Spread spectrum
//	{                               
//		buckPeriod -= PERIOD_STEP;                                       // PERIOD_STEP_UP only there 
//		if(buckPeriod == BUCK_PERIOD_MIN) {statusFlags.PERIOD_STEP_UP = 1;}  //RD Replace to <= ?
//	} else 
//	{
//		buckPeriod += PERIOD_STEP;
//		if(buckPeriod == BUCK_PERIOD_MAX) {statusFlags.PERIOD_STEP_UP = 0;}
//	}

//	HRTIM1->sTimerxRegs[TIM_A].PERxR = buckPeriod;														// next period for timer
////RDD Spread spectrum end	
//	switch (workMode)
//  {
//  	case MODE_VOLTAGE_STAB:

//			if(calculatedValue.iOutSensor > adcIoutStab) { workMode = MODE_CURRENT_LIMIT;}
//			else {
//				uint16_t targetDuty = adcVoutStab * buckPeriod / calculatedValue.vInSensor;
//				offset += (adcVoutStab > calculatedValue.vOutSensor) ? 10 : -10;
//				delta = targetDuty - dutyCycle + offset;

//				if(delta > MAX_DUTY_STEP_POS){ delta = MAX_DUTY_STEP_POS;}
//					else if (delta < MAX_DUTY_STEP_NEG){delta = MAX_DUTY_STEP_NEG;}
//				dutyCycle += delta;	
//			}

//  		break;

//  	case MODE_CURRENT_LIMIT:
//			if(calculatedValue.vOutSensor > adcVoutStab) { workMode = MODE_VOLTAGE_STAB;}
//			else {
//				if(calculatedValue.iOutSensor > adcIoutStab){ dutyCycle -= 10; }
//					else { dutyCycle += 10; }
//			}
//		break;
//  }

//	statusFlags.MIN_DUTY_LIMIT = 0;
//	statusFlags.MAX_DUTY_LIMIT = 0;

//	if( dutyCycle >= DUTY_MAX){
//		dutyCycle = DUTY_MAX;
//		statusFlags.MAX_DUTY_LIMIT = 1;
//	} else if (dutyCycle <= DUTY_MIN){
//			dutyCycle = DUTY_MIN;
//			statusFlags.MIN_DUTY_LIMIT = 1;		
//		}
//	return dutyCycle;	
//}
/*****************************************************************************************
* Go to state with target Vin voltage.
*/////////////////////////////////////////////////////////////////////////////////////////
uint16_t  Regulator(float Vin)// Regulator like PID
{

//	static workMode_ent workMode = MODE_VOLTAGE_STAB;
//RDD Spread spectrum begin

//	delta_duty = (PERIOD_STEP * 100) / (buckPeriod / (dutyCycle / 100));  //spectr
	delta_duty = (buckPeriod *1000 / dutyCycle);  //koef-t spectr 
	delta_duty = PERIOD_STEP*1000 / delta_duty  ;  //delta_duty spectr
	
	if(statusFlags.PERIOD_STEP_UP == 0)   //RDD Spread spectrum
	{                               
		buckPeriod -= PERIOD_STEP;                                       // PERIOD_STEP_UP only there 
//		delta_duty = -100;              //spectr
		delta_duty = -1*delta_duty;              //spectr
		if(buckPeriod == BUCK_PERIOD_MIN) {statusFlags.PERIOD_STEP_UP = 1;}  //RD Replace to <= ?
	} else 
	{
		buckPeriod += PERIOD_STEP;
//		delta_duty = 100;              //spectr
		if(buckPeriod == BUCK_PERIOD_MAX) {statusFlags.PERIOD_STEP_UP = 0;}
	}

	HRTIM1->sTimerxRegs[TIM_A].PERxR = buckPeriod;														// next period for timer

//RDD Spread spectrum end	

				delta = (Vin> calculatedValue.vInSensor) ?  - 10 : +10;
//				if(delta > MAX_DUTY_STEP_POS){ delta = MAX_DUTY_STEP_POS;}
//					else if (delta < MAX_DUTY_STEP_NEG){delta = MAX_DUTY_STEP_NEG;}
//				dutyCycle = dutyCycle + delta;	
				dutyCycle = dutyCycle + delta + delta_duty;	//spectr
			

	statusFlags.MIN_DUTY_LIMIT = 0;
	statusFlags.MAX_DUTY_LIMIT = 0;

	if( dutyCycle >= DUTY_MAX){
		dutyCycle = DUTY_MAX;
		statusFlags.MAX_DUTY_LIMIT = 1;
	} else if (dutyCycle <= DUTY_MIN){
			dutyCycle = DUTY_MIN;
			statusFlags.MIN_DUTY_LIMIT = 1;		
		}
	return dutyCycle;	
};	
/*****************************************************************************************
* LimutDuty is in Regulator inside
******************************************************************************************/

//uint16_t LimutDuty( uint16_t Duty)
//{
//if( Duty >= DUTY_MAX){
//		Duty = DUTY_MAX;
//		statusFlags.MAX_DUTY_LIMIT = 1;
//	} else if (Duty <= DUTY_MIN){
//			Duty = DUTY_MIN;
//			statusFlags.MIN_DUTY_LIMIT = 1;		
//		}
//	dutyCycle=Duty;
//	return dutyCycle;
//}


/******************************************************************************************
* 
*
*******************************************************************************************/
void measureExecute(void){

	static uint16_t index = 0;
	
	if (!statusFlags.ADC_CONVERS_COMPLIT){ return; }

	statusFlags.ADC_CONVERS_COMPLIT = 0;

	//updateSumValue((uint16_t*)&momentValue, pSumValueNow);
{
	uint16_t* pMomentVal=(uint16_t*)&momentValue;
	uint32_t* pSumValue=pSumValueNow;
	uint16_t i = 0; ;
	while(i < ADC_STRUCT_MEMBERS_NUM){
		*pSumValue += *pMomentVal;																							// add the new value to the sum	
		pSumValue++;
		pMomentVal++;
		i++;
	}
}	
	
//	MEAS_update();

	index++;
	if (index == ADC_AVERAGE_NUMBER)   //#define ADC_AVERAGE_NUMBER		1
		{
		uint32_t *ptrTmp = pSumValueNow;  //swap the buffers
		pSumValueNow = pSumValueStored;
		pSumValueStored = ptrTmp;
		//updateAverageValue((float*)&averageValue, pSumValueStored);	
      float* pAverageValue=(float*)&averageValue;
  		uint32_t* pSumValue=pSumValueStored;		
      uint16_t i = 0; 
    	while(i < ADC_STRUCT_MEMBERS_NUM)
				{
	  	   *pAverageValue = *pSumValue * 1.0f / ADC_SAMPLE_NUMBER ;								// calc average value
		     *pSumValue = 0;																													// erase sum value
	   	    pSumValue++;
	  	    pAverageValue++;
	  	    i++;
	      }

		//
		
		index = 0;
		statusFlags.WORK_CYCLE_END = 1;  // 

	  }	

}

/******************************************************************************************
*  Calculated normalized value
*
*******************************************************************************************/
void endOfCycleExecute(void){

	if(!statusFlags.WORK_CYCLE_END) { return; } // no average value

	//updateCalcValue((floatValue_t*)&averageValue, (floatValue_t*)&calculatedValue);

	floatValue_t* pAverageValue=(floatValue_t*)&averageValue;
	floatValue_t* pCalcValue=(floatValue_t*)&calculatedValue;
	
	float adcMultipler = CPU_VREF_VALUE / pAverageValue->vrefCpu;  //correction results according extern ref 
	float adcCurrMultipler = adcMultipler * 50 * I_CONVERCE_COEFF;

	pCalcValue->vInSensor = pAverageValue->vInSensor * adcMultipler * VIN_CONVERCE_COEFF;
	pCalcValue->vOutSensor = pAverageValue->vOutSensor * adcMultipler * VOUT_CONVERCE_COEFF;

	float delta = (pAverageValue->iInSensor - ZERO_CURR_CODE);  //local delta
	pCalcValue->iInSensor = (delta >= 0) ? (delta * adcCurrMultipler) : 0.0f;  //only positive value

	delta = (pAverageValue->iOutSensor - ZERO_CURR_CODE);
 	pCalcValue->iOutSensor = (delta >= 0) ? (delta * adcCurrMultipler) : 0.0f; //only positive value

	delta = (pAverageValue->iOutComSensor - ZERO_CURR_CODE);
	pCalcValue->iOutComSensor = (delta >= 0) ? (delta * adcCurrMultipler) : 0.0f; //only positive value

	pCalcValue->v12Sensor = pAverageValue->v12Sensor * adcMultipler * V12_CONVERCE_COEFF; //12V on the board
	pCalcValue->vrefCpu = pAverageValue->vrefCpu * adcMultipler;	//External Ref?
	
	
  //set control bits, stop/start HR timers
		
		if(statusFlags.CONTROL_STOP)
			{
			statusFlags.CONTROL_STOP = 0;
			statusFlags.CONTROL_START = 0;
			statusFlags.CONTROL_ENABLE = 0;
			offset = 500;
			hrtimersOutDisable();
		  }
			else
			{
	      if(statusFlags.CONTROL_START)
					{
	      	 statusFlags.CONTROL_ENABLE = 1;
		       statusFlags.CONTROL_START = 0;
					 dutyCycle = hrtimersOutEnable(DUTY_MIN);
					}
		  }
		       
//	calculatedValue.tmpCase = getTemperatureValue((uint32_t)averageValue.tmpCase );
	
	statusFlags.WORK_CYCLE_END = 0;

}
