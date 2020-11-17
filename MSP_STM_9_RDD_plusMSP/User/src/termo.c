/*
 * termo.c
 *
 *  Created on: 22 NOV. 2019 
 *  Author: Yakov Churinov
 */


#include "termo.h"


static const uint16_t termoTable[] = { 3216, 3181, 3136, 3079, 3008, 2922, 2818, 2696, 2557, 2402, 2234, 2056, 
																			 1872, 1687, 1507, 1334, 1173, 1024,  890,  770,  664,  572,  492,  423,
																				363,  313,  269,  232,  201,  174,  151,  131,  114 };

#define TERMO_CODE_MIN		termoTable[0]
#define TERMO_CODE_MAX		termoTable[(sizeof(termoTable)/sizeof(termoTable[0])) - 1]


/******************************************************************************************************
 *
 *
 ******************************************************************************************************/
float getTemperatureValue(uint32_t adcTemperatureCode){  //call from endOfCycleExecute
	
	if ( adcTemperatureCode > TERMO_CODE_MIN ) { return TEMPERATURE_MIN; }
	if ( adcTemperatureCode < TERMO_CODE_MAX ) { return TEMPERATURE_MAX; }
 
	/* binary table search */
	uint16_t indexLeft = 0;
	uint16_t indexMiddle = 0;
	uint16_t indexRight = sizeof(termoTable)/sizeof(termoTable[0]) - 1;
	while( (indexRight - indexLeft) > 1 ){
		indexMiddle = (indexLeft + indexRight) >> 1;
		if(adcTemperatureCode > termoTable[indexMiddle]){
			indexRight = indexMiddle;
		} else {indexLeft = indexMiddle;}																						
	}

	/* linear interpolation Y = (( X - X0 )*( Y1 - Y0)/( X1 - X0)) + Y0 */
	uint16_t x0 = termoTable[indexRight];
	float y0 = (float)(TEMPERATURE_MIN + (TERMO_TABLE_STEP * indexRight));
	uint16_t x1 = termoTable[indexLeft]; 
	float y1 = (float)(TEMPERATURE_MIN + (TERMO_TABLE_STEP * indexLeft));

	return ((adcTemperatureCode - x0)*(y1 - y0)/(x1 - x0)) + y0;
}

