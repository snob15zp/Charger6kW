
#include "Stm32f3xx.h"
#include "BoardInit.h"

#include "sch.h"
#include "pwm.h"

#include "dcdc.h"

#include "mainMSP.h"

#include "meas.h"
#include "io.h"
#include "usci.h"


volatile uint32_t sysTickCounter = 0;

uint32_t SystemCoreClock = 8000000;

float ftemp;  // for debug 

//FSM for debug 
static uint16_t debugFSM_calc=0;
void debugFSM()       //call from TIM3_IRQHandler
{ 
	switch (debugFSM_calc)
	{	
	  case 1:
			DCDC_Start_Stop(0); break;
	  case 4000:
			setVin(0);DCDC_Start_Stop(1);DCDC_Enable_Disable(1); break;
	  case 8000:
			setVin(100);break;
		case 12000: 
			setVin(25);break;	
		case 16000: 
			debugFSM_calc = 0; break;	
		default: ;	
  };
	
	debugFSM_calc++;
	
}
	


int main(void)
{ 
	__disable_irq();		
	
	SystemCoreClock = setSystemClock();
	SysTick_Config(SystemCoreClock / 1000);																		// set ssystem tick = 1 ms
	DCDC_Init();
	initTim3();
	
	IO_init();// is in  MAIN_resetAllAndStart()
	uart_init();// is in  MAIN_resetAllAndStart()
	mainMSPinit( );

	
	
	__enable_irq();
 	
	do 
	{ //ftemp=2.3*	((float)debugFSM_calc) ;
		MEAS_update();  //ToDo: link with time or measurement
  	DCDC_Loop(0);
		mainMSPloop(0);
	}
	while(1);
}

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/
void SysTick_Handler(void){ 
	sysTickCounter--;//do not used
	SCH_incrMs();
}

void TIM3_IRQHandler(void)
{
	TIM3->SR = ~TIM_SR_UIF;
	PWM_isr();   //function from MSP430 every 512 ms
	//RDD DEBUG debugFSM(); //RDD DEBUG
	
//	if(TIM3->SR & TIM_SR_UIF){
//		TIM3->SR = ~TIM_SR_UIF;
//		switch(tooglPin){
//			case 0:
//				GPIOB->BSRR = GPIO_BSRR_BS_1;
//				tooglPin=1;
//				break;
//			case 1:
//				GPIOB->BSRR = GPIO_BSRR_BR_1;
//				tooglPin=0;
//				break;
//		}
//	}
}

//void delay_ms(uint32_t delayTime){
//	sysTickCounter = delayTime;
//	while(sysTickCounter){}
//}


