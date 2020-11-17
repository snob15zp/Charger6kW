/*******************************************************************
 *
 * DESCRIPTION:	CRC routines and variables
 *
 * AUTHOR:	Adrian Edmonds
 *		Pierre D'Souza - (PD)
 *
 *  _HISTORY:	n.nn	-07
 *			File Creation
 *
 *			12-Feb-08 (PD)
 *			Added file/function headers
 *******************************************************************/

#include "crc16.h"

/* 
 * CRC16 Lookup tables (High and Low Byte) for 4 bits per iteration. 
 */
unsigned short CRC16_LookupHigh[16] = {
        0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
        0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1
};
unsigned short CRC16_LookupLow[16] = {
        0x00, 0x21, 0x42, 0x63, 0x84, 0xA5, 0xC6, 0xE7,
        0x08, 0x29, 0x4A, 0x6B, 0x8C, 0xAD, 0xCE, 0xEF
};


/*
 * CRC16 "Register". This is implemented as two 8bit values
 */
unsigned char CRC16_High, CRC16_Low;


/********************************************************************
 *************P R I V A T E    F U N C T I O N S*********************
 ********************************************************************/

/********************************************************************
 *	Name		: CRC16_Update4Bits
 *	Description	: A helper function to the main CRC calculation.
 *			Process 4 bits of the message to update the CRC Value.
 *	Inputs		: val - data whose CRC is to be calculated
 *			Note that the data must be in the low nibble of val.
 *	Outputs		: None
 *	Return Value: None
 *	Side Effects: Global "CRC16_High","CRC16_Low" are modified. (hardcoded)
 *	History		: 12-Feb-2008 (PD)
 *			Function header and comments.
 ********************************************************************/
void CRC16_Update4Bits( unsigned char val )
{
	unsigned char	t;

	// Step one, extract the Most significant 4 bits of the CRC register
	t = CRC16_High >> 4;

	// XOR in the Message Data into the extracted bits
	t = t ^ val;

	// Shift the CRC Register left 4 bits
	CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4);
	CRC16_Low = CRC16_Low << 4;

	// Do the table lookups and XOR the result into the CRC Tables
	CRC16_High = CRC16_High ^ CRC16_LookupHigh[t];
	CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t];
}


/********************************************************************
 ***************P U B L I C    F U N C T I O N S*********************
 ********************************************************************/

/********************************************************************
 *	Name		: CalculateCRC16
 *	Description	: Calculate the CRC of the given data.
 *	Inputs		: cp - pointer to data whose CRC is to be calculated
 *			  len - data length
 *			  match - boolean value indicating whether function should check the validity of the data in cp of length (len-2) by calculating the
				   checksum of the data and comparing it with the last two bytes (len-1),(len-2) of data array (cp)
 *			  blankState - The value that indicates blank (erased) data in flash/fram memory.
 *	Outputs	: if match is false Crc16 Values CRC16_High, CRC16_low are written in last two bytes (len-1),(len-2) of data array (cp)
 *	Return Value: Status of the CRC calculation
 *			2: Success  Crc Matched
 *			1: Success  (Crc Calculated / Partially Blank Page so CRC not present as yet to match)
 *			0: Fully Blank Page
 *			-1: CRC Mis-Match Found
 *			-2: Page too short (len <= 2)
 *	Side Effects:  Global "CRC16_High","CRC16_Low" are modified. (hardcoded)
 *	History		: 12-Feb-2008 (PD)
 *			Function header and comments.
 ********************************************************************/
int CalculateCRC16(unsigned char *cp, int len,int match,unsigned char blankState)
{
	int i;
	int AllBlank;
	
	CRC16_High = 0xFF;
	CRC16_Low = 0xFF;
	
	
	if (len > 2)
	{
		AllBlank = 1;
		for(i=0;i<len-2;i++)
		{
			CRC16_Update4Bits( cp[i]>> 4 );	
			CRC16_Update4Bits( cp[i] & 0x0F);			
			AllBlank &= (cp[i] == blankState);
		}
								
		if (CRC16_Low == blankState)
		{
			CRC16_Low++;
		}
		
		if (CRC16_High == blankState)
		{
			CRC16_High++;
		}
		
		if ((AllBlank) && (cp[len-1] == blankState) && (cp[len-2] == blankState))
		{
			return 0;	//Blank Page
		}
							
		if (match)
		{
			if  ((cp[len-1] == blankState) && (cp[len-2] == blankState))
			{
				return 1;  //Partially written page
			}
			else
			{
				if ((CRC16_Low == cp[len-1]) && (CRC16_High == cp[len-2]))
				{
					return 2;
				}
				else
				{

					return -1;
				}		
			}
		}
		else
		{
			cp[len-1] = CRC16_Low;
			cp[len-2] = CRC16_High;		
			return 1;
		}
	}
	
	return -2;
	
	
	
}
