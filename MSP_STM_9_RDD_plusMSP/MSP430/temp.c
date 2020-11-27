
// Include files
//#include <msp430x24x.h>
#include "temp.h"
#include "time.h"
#include "io.h"
#include "pwm.h"
#include "limits.h"

/*
 * Digital temp sensor
 */

static const unsigned int oneSecond = 1000000ul / PWM_PERIOD_US;	// Number of ticks in a second

static unsigned int tickCount;			// Number of ticks since the start of the second
static unsigned int pulseCount;
static unsigned int edgeTickCount;		// The last time a rising edge was detected
static int currentState;				// The current state of the digital temp input

static unsigned int temperature;		// Temp in kelvin * 64

/**
 *  @ingroup groupDigitalTemp
 *  @brief temperature = (273.15f + 25) * 64;
*/
void TEMP_init()
{
	tickCount = 0;
	pulseCount = 0;
	edgeTickCount = 0;
	currentState = IO_getDigitalTemp();
	
	temperature = (273.15f + 25) * 64;
}


/**
 *  @ingroup groupDigitalTemp
 *  @brief Just returns the state of the input

 * Count the number of pulses in about a second and measure the time
 * Then temperature in kelvin is pulses/time
   Get temperature in Kelvin * 64
 
 */
void TEMP_tick()
{
	int newState = IO_getDigitalTemp();
	int risingEdge = newState && !currentState;
	currentState = newState;

	tickCount++;
	if (risingEdge)
	{
		edgeTickCount = tickCount;
		pulseCount++;
	}

	if (tickCount >= oneSecond)
	{
		// One second has elapsed so calculate temp
		if (pulseCount == 0 || pulseCount >= 400)
		{
			// No pulses or too many pulses
			temperature = UINT_MAX;
		}
		else if (pulseCount != 0)
		{
			unsigned long pulses = (pulseCount * 10000ul) << 6;			// Scale up by 10000 because time is us / 100. 7 bits worth of decimal.
			unsigned long time = edgeTickCount * PWM_PERIOD_US / 100;	// Total time for all the pulses in us / 100
			temperature = pulses / time;
		}

		// Reset
		tickCount = 0;
		pulseCount = 0;
		edgeTickCount = 0;
	}
}

/**
 *  @ingroup groupDigitalTemp
 *  @brief Just returns the state of the temperature in Kelvin * 64
*/
unsigned int TEMP_getValue()
{
	return temperature;
}
