/*
 * Tritium MSP430 2xx USCI SPI interface
 * Copyright (c) 2009, Tritium Pty Ltd.  All rights reserved.
 *
 * Last Modified: J.Kennedy, Tritium Pty Ltd, 30 September 2009
 *
 * - Implements the following SPI interface functions
 *	- init
 *	- transmit
 *	- exchange
 *
 *
 */

// Include files
///#include <msp430x24x.h>
#include "Stm32f3xx.h"
#include "usci.h"
#include <signal.h>
///#include "io.h"
///#include "comms.h"
#include "lcd.h"
#include "time.h"
#include "crc16.h"
#include "ctrl.h"

/*
 * Initialise SPI port
 * 	- Master, 8 bits, mode 0:0, max speed, 3 wire
 * 	- Clock = 0: SMCLK /2 (works with no external clock input, DCO only)
 *	- Clock = 1: ACLK  /2 (fastest possible, from external clock input)
 */

#define DLE 0x10
#define ETX 0x03
#define STX 0x02

#define ALT_FUNC_USART 0x07
#define USART1_BAUD 1200		//b/s
#define USART1_FCLK 72000000	//Hz

typedef enum uart_State_
{
	UART_STATE_IDLE = 0,
	UART_STATE_RECEIVING_PACKET,
	UART_STATE_RECEIVED_PACKET,
	UART_STATE_SENDING_PACKET
} uart_State;

#define FOOTER_LENGTH 6

// Global variables
unsigned char tx_buffer[PACKET_LENGTH];
unsigned char rx_buffer[PACKET_LENGTH];
unsigned char tx_count = 0;
unsigned char rx_count = 0;
unsigned char expected_rx_count = 0;
unsigned char prev_rx = 0xFF;
uart_State uart_state = UART_STATE_IDLE;
int sent_startup = 0;
Time prev_time = 0;
unsigned int tx_buf_len = 0;

telemetry_t telemetry_R;
factoryConfig_t factoryConfig_R;
userConfig_t userConfig_R;
eventConfig_t eventConfig_R;
sysInfo_t sysInfo_R;
command_t command_R;
password_t password_R;
miscState_t miscState_R;

factoryConfig_t factoryConfig_W;
userConfig_t userConfig_W;
eventConfig_t eventConfig_W;
command_t command_W;
setTime_t setTime_W;
miscState_t miscState_W;

persistentStorage_t persistentStorage;

//void usci_init( unsigned char clock )
//{
///// remove old hardware
//}

///*
// * Transmits data on SPI connection
// *	- Busy waits until entire shift is complete
// *	- On devices with hardware SPI support, this function is identical to spi_exchange,
// *	  with the execption of not returning a value
// *	- On devices with software (bit-bashed) SPI support, this function can run faster
// *	  because it does not require data reception code
// */
//void usci_transmit( unsigned char data )
//{
//	/// remove old hardware
//}

///*
// * Exchanges data on SPI connection
// *	- Busy waits until entire shift is complete
// *	- This function is safe to use to control hardware lines that rely on shifting being finalised
// */
//unsigned char usci_exchange( unsigned char data )
//{
//	/// remove old hardware
//	return 0;
//}

void uart_init( void )
{
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  //USART1 clock enable
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;     //GPIOC clock enable
	
	GPIOC->MODER &= ~(GPIO_MODER_MODER4_0 | GPIO_MODER_MODER4_1);		
	GPIOC->MODER &= ~(GPIO_MODER_MODER5_0 | GPIO_MODER_MODER5_1);
	GPIOC->MODER |= GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1;
	GPIOC->OTYPER &= ~GPIO_OTYPER_OT_4;
//	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR5_0 | GPIO_PUPDR_PUPDR5_1);
//	GPIOC->PUPDR |= GPIO_PUPDR_PUPDR5_1;
	
	GPIOC->AFR[0] |= (ALT_FUNC_USART<<GPIO_AFRL_AFRL4_Pos) | (ALT_FUNC_USART<<GPIO_AFRL_AFRL5_Pos);
	
	USART1->BRR=USART1_FCLK/USART1_BAUD;
	USART1->CR1 = USART_CR1_UE;
	USART1->CR1 |= USART_CR1_TE;
	USART1->CR1 |= USART_CR1_RE;
	USART1->CR1 |= USART_CR1_RXNEIE;
	NVIC_EnableIRQ(USART1_IRQn);
}

/*
 * Transmit data 
 *	- Begins transmission of the data buffer
 *	- Transmits one byte of data, the rest are handled by the Tx IRQ
 */
void uart_tx( void )   ///this must be plased to HardWareLevel Group
{
	// Set the byte counter
	tx_count = 0;
//	// Make sure IRQ flag is clear
//	IFG2 &= ~UCA0TXIFG;
	while(!(USART1->ISR & USART_ISR_TC)){
	}
	/// remove old hardware IFG2 &= ~UCA0TXIFG;
	USART1->ICR &= ~USART_ICR_TCCF;
	// Enable Tx IRQ
	USART1->CR1 |= USART_CR1_TCIE;
	/// remove old hardware IE2 |= UCA0TXIE;
	//UCA0CTL1 |= UCTXADDR;	// Set address bit transmission marker to preceed next tx byte
	// Load first byte into buffer and transmit
	USART1->TDR = tx_buffer[tx_count++];
	/// remove old hardware UCA0TXBUF = tx_buffer[tx_count++];
}

void uart_send_response(command_Code cc, uint16_t arg)
{
	command_R.commandCode = cc;
	command_R.arg = arg;
	uart_send(TYPE_COMMAND);
}

void uart_send_startup(void)
{
	uart_send_response(RESPONSE_STARTUP, 0);
}

/*
 * Send an error.
 */
void uart_send_error(error_Code ec)
{
	uart_send_response(RESPONSE_ERROR, ec);
}

void loadPassword()
{
	int i;
	unsigned char* password = (unsigned char*)0xFFE0;

	for(i=0; i<32; i++)
	{
		password_R.password[i] = password[i];
	}
}

/*
 * Process the recevied data.
 * If the received data is a request then send the requested packet otherwise write the data to flash then
 * send it back to the sender so it can validate that the correct data was written.
 */
void uart_receive(void)
{
	unsigned char i;
	unsigned char k;
	unsigned char crc_start;
	packetIdentifier_t packetID;
	unsigned char *bytes = 0;
	unsigned char structSize = 0;
	unsigned char version = 0;

	if(!sent_startup)  ///sent_startup used only here
	{
		sent_startup = 1;
		uart_send_startup();
		return;
	}

	if(uart_state == UART_STATE_RECEIVED_PACKET)
	{
		crc_start = rx_count - FOOTER_LENGTH;
		k = crc_start;
		// Un-stuff crc bytes
		if(rx_buffer[k] == DLE)
		{
			k++;
		}
		k++;
		if(rx_buffer[k] == DLE)
		{
			k++;
		}
		rx_buffer[crc_start+1] = rx_buffer[k];
		if(CalculateCRC16(rx_buffer,crc_start+2,1,0x00) == 2)
		{
			// The CRC was correct so process the packet
			k = 4;
			if(rx_buffer[k] == DLE)
			{
				k++;
			}
			packetID.byte = rx_buffer[k++];
			switch(packetID.type)
			{
			case TYPE_TELEMETRY:
				break;
			case TYPE_FACTORY:
				bytes = factoryConfig_W.bytes;
				structSize = sizeof(factoryConfig_t);
				version = VERSION_FACTORY;
				break;
			case TYPE_USER:
				bytes = userConfig_W.bytes;
				structSize = sizeof(userConfig_t);
				version = VERSION_USER;
				break;
			case TYPE_EVENTS:
				bytes = eventConfig_W.bytes;
				structSize = sizeof(eventConfig_t);
				version = VERSION_EVENTS;
				break;
			case TYPE_SYS_INFO:
				break;
			case TYPE_COMMAND:
				bytes = command_W.bytes;
				structSize = sizeof(command_t);
				version = VERSION_COMMAND;
				break;
			case TYPE_PASSWORD:
				break;
			case TYPE_SET_TIME:
				bytes = setTime_W.bytes;
				structSize = sizeof(setTime_t);
				version = VERSION_SET_TIME;
				break;
			case TYPE_MISC_STATE:
				bytes = miscState_W.bytes;
				structSize = sizeof(miscState_t);
				version = VERSION_MISC_STATE;
				break;
			default:
				// Unknown packet type
				uart_send_error(ERROR_PACKET_TYPE);
				return;
			}

			if(packetID.request)
			{
				if (packetID.type == TYPE_TELEMETRY)
				{
					// Reload telemetry data each time.
					// The other packets are only loaded at startup or when the flash is modified.
					lcd_loadTelemetry();
				}
				else if (packetID.type == TYPE_PASSWORD)
				{
					loadPassword();
				}
				if (packetID.type != TYPE_COMMAND && packetID.type != TYPE_SET_TIME)
				{
					uart_send(packetID.type);
					return;
				}
			}
			else if(bytes)
			{
				if(packetID.version != version)
				{
					uart_send_error(ERROR_VERSION);
					return;
				}

				i = 0;
				// Un-stuff received data and load it into the appropriate struct
				while(k < crc_start)
				{
					if(rx_buffer[k] == DLE)
					{
						k++;
					}
					if (i >= structSize)
					{
						// Too much data
						i = -1;
						break;
					}
					if (k >= crc_start)
					{
						// Bad stuffing - DLE at the end of the data buffer
						i = -1;
						break;
					}
					bytes[i++] = rx_buffer[k++];
				}
				if(i == structSize)
				{
					if(packetID.type == TYPE_COMMAND)
					{
						if(command_W.commandCode == COMMAND_RESET)
						{
							// Force watchdog reset
							/// RDDtemp IO_disablePwmCtrl();
							/// remove old hardware WDTCTL = 0x00;
							return;
						}
						else if (command_W.commandCode == COMMAND_ENABLE_OUTPUT)
						{
							CTRL_enableOutput(command_W.arg ? 1 : 0);
							uart_send_response(COMMAND_ENABLE_OUTPUT, command_W.arg ? 1 : 0);
							return;
						}
					}
					else if(packetID.type == TYPE_SET_TIME)
					{
						TIME_set(setTime_W.time);
					}
					else
					{
						// Successfully received the packet so verify and start writing to flash
						if(lcd_queueWrite(packetID.type))
						{
							return;
						}
					}
				}
			}
		}
		uart_state = UART_STATE_IDLE;
	}
	else
	{
		if (uart_state == UART_STATE_IDLE)
		{
			lcd_checkPersistentUpdate();
		}
	}
}

int uart_send(int type)
{
	unsigned char *bytes;
	unsigned char structSize;
	unsigned char version;
	int i;
	int k;
	unsigned char c;
	unsigned char d;
	packetIdentifier_t packetID;

	uart_state = UART_STATE_SENDING_PACKET;

	switch(type)
	{
	case TYPE_TELEMETRY:
		bytes = telemetry_R.bytes;
		structSize = sizeof(telemetry_t);
		version = VERSION_TELEMETRY;
		break;
	case TYPE_FACTORY:
		bytes = factoryConfig_R.bytes;
		structSize = sizeof(factoryConfig_t);
		version = VERSION_FACTORY;
		break;
	case TYPE_USER:
		bytes = userConfig_R.bytes;
		structSize = sizeof(userConfig_t);
		version = VERSION_USER;
		break;
	case TYPE_EVENTS:
		bytes = eventConfig_R.bytes;
		structSize = sizeof(eventConfig_t);
		version = VERSION_EVENTS;
		break;
	case TYPE_SYS_INFO:
		bytes = sysInfo_R.bytes;
		structSize = sizeof(sysInfo_t);
		version = VERSION_SYS_INFO;
		break;
	case TYPE_COMMAND:
		bytes = command_R.bytes;
		structSize = sizeof(command_t);
		version = VERSION_COMMAND;
		break;
	case TYPE_PASSWORD:
		bytes = password_R.bytes;
		structSize = sizeof(password_t);
		version = VERSION_PASSWORD;
		break;
	case TYPE_MISC_STATE:
		bytes = miscState_R.bytes;
		structSize = sizeof(miscState_t);
		version = VERSION_MISC_STATE;
		break;
	default:
		uart_state = UART_STATE_IDLE;
		return 0;
	}

	packetID.type = type;
	packetID.request = 0;
	packetID.version = version;

	tx_buffer[0] = DLE;
	tx_buffer[1] = STX;

	// Length is written in index 2 and 3 later

	k = 4;
	if (packetID.byte == DLE)
	{
		tx_buffer[k++] = DLE;
	}
	tx_buffer[k++] = packetID.byte;

	// Copy data from the struct
	for (i=0; i<structSize; i++)
	{
		if (bytes[i] == DLE)
		{
			if(k >= PACKET_LENGTH - FOOTER_LENGTH)
			{
				// Error: Too much data
				return 0;
			}
			tx_buffer[k++] = DLE;
		}
		if(k >= PACKET_LENGTH - FOOTER_LENGTH)
		{
			// Error: Too much data
			return 0;
		}
		tx_buffer[k++] = bytes[i];
	}

	tx_buf_len = k+FOOTER_LENGTH;
	// Write total packet length
	if (tx_buf_len == DLE)
	{
		tx_buffer[2] = DLE;
		tx_buffer[3] = DLE;
	}
	else
	{
		tx_buffer[2] = tx_buf_len;
		tx_buffer[3] = 0;
	}

	CalculateCRC16(tx_buffer,k+2,0,0x00);
	c = tx_buffer[k];
	d = tx_buffer[k+1];
	if (c == DLE)
	{
		tx_buffer[k++] = DLE;
	}
	tx_buffer[k++] = c;
	if (d == DLE)
	{
		tx_buffer[k++] = DLE;
	}
	tx_buffer[k++] = d;
	for(; k<tx_buf_len-2; k++)
	{
		tx_buffer[k] = 0;
	}

	tx_buffer[k++] = DLE;
	tx_buffer[k++] = ETX;

	uart_tx();

	return 1;
}

//interrupt(USCIAB0TX_VECTOR) enablenested uart_tx_isr(void)

/// remove old hardware interrupt(USCIAB0TX_VECTOR)
//void uart_tx_isr(void ) ///this must be plased to HardWareLevel Group
//{
//	/// remove old hardware IE2 &= ~(UCA0TXIE);
//	/// remove old hardware eint();

//	if (uart_state != UART_STATE_SENDING_PACKET)
//	{
//		/// remove old hardware IE2 &= ~(UCA0TXIE);
//	}
//	else if (tx_count < tx_buf_len)
//	{
//		/// remove old hardware UCA0TXBUF = tx_buffer[tx_count++];
//		/// remove old hardware IE2 |= UCA0TXIE;
//	}
//	else
//	{
//		// Finished sending so start listening for new requests.
//		uart_state = UART_STATE_IDLE;
//		/// remove old hardware IE2 &= ~(UCA0TXIE);  //Turn off transmit interrupt until next time we transmit
//	}
//}

//interrupt(USCIAB0RX_VECTOR) enablenested uart_rx_isr(void)
/// remove old hardware interrupt(USCIAB0RX_VECTOR)
//void uart_rx_isr(void)  ///this must be plased to HardWareLevel Group
void USART1_IRQHandler(void)
{
	unsigned char rx;
	Time time;
	
	//IRQ transmit complete
	if(USART1->ISR & USART_ISR_TC){
		/// remove old hardware IE2 &= ~(UCA0TXIE);
		USART1->CR1 &= ~USART_CR1_TCIE;	//tx interrupt disable
		/// remove old hardware eint();
		if (uart_state != UART_STATE_SENDING_PACKET)
		{
			/// remove old hardware IE2 &= ~(UCA0TXIE);
			USART1->CR1 &= ~USART_CR1_TCIE;	//tx interrupt disable
		}
		else if (tx_count < tx_buf_len)
		{
			/// remove old hardware UCA0TXBUF = tx_buffer[tx_count++];
			USART1->CR1 |= USART_CR1_TCIE;	//tx interrupt enable
			USART1->TDR = tx_buffer[tx_count++];
			/// remove old hardware IE2 |= UCA0TXIE;
		}
		else
		{
			// Finished sending so start listening for new requests.
			uart_state = UART_STATE_IDLE;
			/// remove old hardware IE2 &= ~(UCA0TXIE);  //Turn off transmit interrupt until next time we transmit
			USART1->CR1 &= ~USART_CR1_TCIE;	//tx interrupt disable
		}
	}
	
	//IRQ receive complete
	if(USART1->ISR & USART_ISR_RXNE){
		/// remove old hardware IE2 &= ~(UCA0RXIE);
		USART1->CR1 &= ~USART_CR1_RXNEIE;	//rx interrupt disable
		/// remove old hardware eint();
		/// remove old hardware rx = (unsigned char)(UCA0RXBUF);
		rx = (unsigned char)USART1->RDR;
		if((uart_state != UART_STATE_RECEIVED_PACKET) && (uart_state != UART_STATE_SENDING_PACKET))
		{
			// Reset the state if the last byte was received more than 1s ago.
			time = TIME_get();
			if(time > prev_time + 1000 || time < prev_time)
			{
				uart_state = UART_STATE_IDLE;
				prev_rx = 0xFF;
			}
			prev_time = time;

			if((rx == STX) && (prev_rx == DLE))
			{
				// A packet has started
				rx_buffer[0] = DLE;
				rx_buffer[1] = STX;
				rx_count = 2;
				expected_rx_count = 0;
				uart_state = UART_STATE_RECEIVING_PACKET;
			}
			else if(uart_state == UART_STATE_RECEIVING_PACKET)
			{
				rx_buffer[rx_count++] = rx;
				if(rx_count <= 4)
				{
					if (((rx_count == 3) && (rx != DLE)) || ((rx_count == 4) && (prev_rx == DLE)))
					{
						expected_rx_count = rx;
					}
				}
				else if(rx_count <= expected_rx_count)
				{
					if((prev_rx == DLE) && (rx == ETX))
					{
						if (rx_count == expected_rx_count)
						{
							uart_state = UART_STATE_RECEIVED_PACKET;
						}
						else
						{
							// Length doesn't match
							uart_state = UART_STATE_IDLE;
						}
					}
				}
				else
				{
					// Bad packet - reset state
					uart_state = UART_STATE_IDLE;
				}
			}
			else if ((rx == DLE) && (prev_rx == DLE))
			{
				rx = 0xFF;
			}
		}

		prev_rx = rx;
	/// remove old hardware IE2 |= UCA0RXIE;
	USART1->CR1 |= USART_CR1_RXNEIE;
	}
}
