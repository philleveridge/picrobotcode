/**
 * Example on writing to the PIC16f628a's EEPROM memory.  This is 
 * tricky.  The exact sequence is:
 *
 * @ Select the correct address, enter data
 * @ Set the WREN flag, if it's not set already
 * @ Execute the correct 5-instruction sequence as given by Microchip
 *
 * That last part is hard to do in C since it needs to be done just 
 * right, so inline assembly is used.
 *
 * This example writes a string into EEPROM memory, starting at position 
 * 0x00.  The string is the name of this file and the date it was 
 * compiled.  To prove it's been written, you can read it back with your 
 * PIC programmer and see the difference.
 */
#define __16f628a
#include "pic/pic16f628a.h"
#include "tsmtypes.h"

// Set the __CONFIG word:
Uint16 at 0x2007  __CONFIG = CONFIG_WORD;

/** This EXACT SEQUENCE of instructions is needed, any deviation will
 *  cause the write to FAIL!  The PIC even counts the number of 
 *  instructions to check if you did this right!  This means we need 
 *  inline ASM, C isn't going to get it Just Right(tm).
 */
#define EEPROM_WRITE()	do {				\
	EECON2=0x00;	/* Get in right bank */		\
	__asm	MOVLW	0x55		__endasm; 	\
	__asm	MOVWF	EECON2		__endasm; 	\
	__asm	MOVLW	0xaa		__endasm; 	\
	__asm	MOVWF	EECON2		__endasm; 	\
	__asm	BSF	EECON1,1;	__endasm;	\
	} while(0)

/** C doesn't have this either, so we make it a macro. */
#define sleep() __asm sleep __endasm

// String containing file name and date of compilation.
//static const char *str=#__FILE__ "@" __DATE__;
// Until SDCC supports proper strings again, this message will be more terse.
static const char str[]={'S', 'D', 'C', 'C', '\0'};

void main(void)
{
	static Uint8 pos;

	PORTA=0x00;	// Output all zero on PORTA.

#ifdef  __16f628a       // Only compile this section for PIC16f628a
	CMCON = 0x07;	/** Disable comparators.  NEEDED FOR NORMAL PORTA
			 *  BEHAVIOR ON PIC16f628a!
			 */
#endif
	TRISA=0x00;	// PORTA all outputs.

	pos=0;		// Start at position 0 in the string and EEPROM.
	do
	{
		EEADR=pos;	// EEPROM position same as string position.
		EEDATA=str[pos];// Set the data to write.
		WREN=1;		// Enable writes.
			EEPROM_WRITE();	// Magic Instruction Sequence Go!
			while(WR);	// Wait for write to finish.
		WREN=0;		// Disable writes.

	} while(str[pos++]);	// Loop while str[pos] != 0x00

	PORTA|=0x01;	// Set A0 high to show the program's done.

	while(1)	// Loop forever:
		sleep();	// Go into sleep mode.
}
