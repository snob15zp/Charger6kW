#ifndef __EfcumDef_H
#define __EfcumDef_H

#include <stdint.h>

#define ITM_Port8(n)    		(*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   		(*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   		(*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR           		(*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          		0x01000000

#define TIM4_ON							(TIM_CR1_ARPE | TIM_CR1_CEN)
#define TIM4_OFF						TIM_CR1_ARPE
#define TIM1_ON							(TIM_CR1_ARPE | TIM_CR1_OPM | TIM_CR1_CEN)

#define MIN_FREQUENCY				((uint32_t)340000)
#define MAX_FREQUENCY				((uint32_t)370000)

#define RELAY1_ON						GPIOD->BSRR = GPIO_BSRR_BS_2
#define RELAY1_OFF					GPIOD->BSRR = GPIO_BSRR_BR_2
#define RELAY2_ON						GPIOC->BSRR = GPIO_BSRR_BS_12
#define RELAY2_OFF					GPIOC->BSRR = GPIO_BSRR_BR_12

#define TX_REQUEST_ON				GPIOC->BSRR = GPIO_BSRR_BS_6
#define TX_REQUEST_OFF			GPIOC->BSRR = GPIO_BSRR_BR_6

#define CONTACTOR_ON				(uint16_t)0x01
#define HVA_SENSORS_OK			(uint16_t)0xFE

#define MAX_ANGLE						37888												// максимальное значение для controlAngle
#define MIN_ANGLE 					220													// минимальное значение для controlAngle

#define	ONE_DEGREE					220
#define	MIN_CURRENT					1
#define MAX_POWER_FACTOR 		controlParamArray[0]
#define SPARK_SPEED 				controlParamArray[1]
#define SPEED_COEFF 				controlParamArray[2]
#define REGULATION_DEPTH 		controlParamArray[3]				// глубина отработки искры относительно угла зажигания до возникновения искрового пробоя
#define TIME_WITOUT_SPARK 	controlParamArray[4]				// промежуток времени без искрений до быстрого нарастания I DC
#define VOLTAGE_RISE_COEFF 	controlParamArray[5]				// коэффициент нарастания напряжения до 0,8*U DC (после искрового пробоя)
#define DISCHARGE_TIME 			controlParamArray[6]				// время гашения дугового разряда (количество периодов пропуска управления тиристорами)
#define REPAIR_TIME 				controlParamArray[7]				// количество полупериодов, за которое восстановится напряжение до 0,8*U DC после дугового пробоя
#define NOM_HI_CURRENT	 		controlParamArray[8]				// номинальное значение выходного среднего тока ВАП, мА
#define NOM_HI_VOLTAGE	 		controlParamArray[9]				// номинальное значение пикового выходного напряжения ВАП (без нагрузки), кВ.
#define LOW_VOLTAGE_LIM	 		controlParamArray[10]
#define HI_VOLTAGE_LIM	 		controlParamArray[11]

/*************************************************************************************************
 * Признаки ошибок процесса регулирования
 * ***********************************************************************************************/
#define  NO_ERROR							((uint16_t)0x0000)
#define  SPARK_ERROR					((uint16_t)0x0001)
#define  FLASH_ERROR					((uint16_t)0x0002)
#define  HVA_STATE_ERROR			((uint16_t)0x0004)
#define  STOP_ERROR						((uint16_t)0x0010)
#define  VOLTAGE_ERROR				((uint16_t)0x0020)
#define  CURRENT_ERROR				((uint16_t)0x0040)
#define  FREQUENCY_ERROR			((uint16_t)0x0080)
#define  SYNCHRO_ERROR				((uint16_t)0x0100)
#define  TIMEOUT_ERROR				((uint16_t)0xFFFF)

//#define  CHECK_INPUT_STATE		((uint16_t)0xFFFF)
//#define  NO_CHECK_INPUT_STATE	((uint16_t)0x0000)

/*************************************************************************************************
* Определения типов данных и переменных для работы с
*
* ***********************************************************************************************/
typedef 
	enum {
		WM_WAIT_SYNCHRO,
		WM_WAIT_START,
		WM_STOP,
		WM_AUTO_CONTROL,
		WM_AUTO_START,
		WM_HAHD_CONTROL
	} TWorkMode;

typedef 
	struct{
		uint16_t  SparkRate;										// темп искрения (количество периодов между искрами)
		uint16_t  SpeedFactor;									// прогрессивный фактор скорости искрения
		uint16_t  TimeWithoutSpark;							// промежуток времени без искрений до быстрого нарастания I DC
		uint16_t  RepairTime;
		uint16_t  DischargeTime;
		uint16_t  LowVoltageLimit;							// установка для аварийной сигнализации при пониженном напряжении и обнаружения дугового разряда
		uint16_t  HighVoltageLimit;							// установка для аварийной сигнализации при перенапряжении (холостой ход)
		uint16_t  HighCurrentLimit;							// установка для аварийной сигнализации при перегрузке (ток перегрузки)
		uint16_t  AngleLimit;										// ограничение угла зажигания тиристоров (симистора)
	} TControlParam;

typedef 
	struct{
		uint16_t  PrimaryVoltage;
		uint16_t  PrimaryCurrent;
		uint16_t  HighVoltage;
		uint16_t  HighCurrent;
		uint16_t  ZeroLevel;
	} TMomentValues;

typedef 
	struct{
		uint32_t  PrimaryVoltage;
		uint32_t  PrimaryCurrent;
		uint32_t  HighVoltage;
		uint32_t  HighCurrent;
	} TSummValues;

typedef 
	struct{
		uint16_t   StartWord;
		uint16_t   PrimaryVoltage;
		uint16_t   PrimaryCurrent;
		uint16_t   HighVoltage;
		uint16_t   HighCurrent;
		uint16_t   Frequency;
		uint16_t   InputState;
		uint16_t   ControlAngle;												// угол открытия тиристоров
		uint16_t   ErrorCode;
		uint16_t   StopWord;
	} TOutputValues;																	// структура для передаваемых значений

typedef 
	struct{
		uint16_t  Command;
		uint16_t  Data;
	} TRxData;

typedef  void (*type_MyFunct)(void);


/******* Объявления функций **********************************************************************/
extern void updateMeasuredValue(void);
extern uint16_t checkForSparkError(void);
extern uint16_t executeOneCycle(uint16_t inputsCheckStatus);
extern void stopHighVoltageUnit(void);
extern TWorkMode checkSynchroPulse(void);
extern TWorkMode powerOnHighVoltageUnit(void);
extern TWorkMode softStartHighVoltageUnit(void);
extern TWorkMode handControlHighVoltageUnit(void);
extern void adjustHighVoltageUnit(void);
extern void setControlAngle(void);
extern void initControlParameters(void);
extern void startTransmitData(void);
extern void enableThyristorControl(void);
extern void controlAngleHahdSet(void);
extern void setStopMode(void);
extern void setStartMode(void);
extern void nill(void);

#endif
