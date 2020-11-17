/******************************************************************************************************
 * 	BoardInit.h
 *
 *  Created on: 16 NOV 2019 ã.
 *  Author: Yakov Churinov
 *
 *******************************************************************************************************/

#ifndef CODE_INC_BOARDINIT_H_
#define CODE_INC_BOARDINIT_H_

#include <stdint.h>

#define HSE_STARTUP_TIMEOUT    	0x0500U   // Time out for HSE start up
#define ADC_AVERAGE_NUM					8U


/** function prototype declarations **/

extern uint32_t setSystemClock(void);
extern void initCoreIoPins(void);
extern uint16_t getNewDytyValue(void);
extern void delay_ms(uint32_t delayTime);
extern void measureExecute(void);
extern void endOfCycleExecute(void);
extern void initTim3(void);

#endif /* CODE_INC_BOARDINIT_H_ */
