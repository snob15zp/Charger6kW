/*******************************************************************
 *
 * DESCRIPTION:	CRC routines and variables
 *
 * AUTHOR:	Adrian Edmonds
 *		Pierre D'Souza - (PD)
 *
 * HISTORY:	n.nn	-07
 *			File Creation
 *
 *			12-Feb-08 (PD)
 *			Added file/function headers
 *******************************************************************/

#ifndef CRC16_H
#define CRC16_H
  

/********************************************************************
 *	Name		: CalculateCRC16
 *	Description	: Calculate or Check the CRC of the given data.
 *	Inputs		: cp - pointer to data array (page) whose CRC is to be calculated (last two bytes contain or will contain the two byte crc16 checksum)
 *			  len - data length
 *			  match - boolean value indicating whether function should check the validity of the data in cp of length (len-2) by calculating the
				   checksum of the data and comparing it with the last two bytes (len-1),(len-2) of data array (cp)
 *			  blankState - The value that indicates blank (erased) data in the data array (i,e flash/fram memory page erased data value)
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
int CalculateCRC16(unsigned char *cp, int len,int match,unsigned char blankState);


/*
 * CRC16 "Register". This is implemented as two 8bit values
 */
extern unsigned char CRC16_High, CRC16_Low;

#endif
