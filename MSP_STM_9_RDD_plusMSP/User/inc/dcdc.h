#ifndef DCDC_H
#define DCDC_H
#include <stdint.h>
#include "adc.h"


extern floatValue_t averageValue;
extern floatValue_t calculatedValue;
extern volatile regAdcValue_t momentValue;

int setVin(float Vin);

extern int DCDC_Start_Stop(uint8_t SS);     //RDD 1-Start; 0-Stop: Not work HRtim
extern int DCDC_Enable_Disable(uint8_t ED); //RDD 1-Enable: DCDC work in stop mode; 0-Disable :  Not control DCDC, not regulator, but adc work

extern int	DCDC_Init(void);
extern int 	DCDC_Loop(char l);

#endif
