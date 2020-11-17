/*******************************************************************************
 * termo.h
 *
 *  Created on: 22 NOV. 2019 
 *  Author: Yakov Churinov
 *
 ********************************************************************************/

#ifndef CODE_INC_TERMO_H_
#define CODE_INC_TERMO_H_

#include <stdint.h>

#define TERMO_TABLE_STEP	5.0f
#define TEMPERATURE_MIN		-20.0f
#define TEMPERATURE_MAX		140.0f

extern float getTemperatureValue(uint32_t adcTemperatureCode);

#endif /* CODE_INC_TERMO_H_ */
