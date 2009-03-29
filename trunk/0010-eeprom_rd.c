#define __16f628a
#include "pic/pic16f628a.h"
#include "tsmtypes.h"

// Set the __CONFIG word:
Uint16 at 0x2007  __CONFIG = CONFIG_WORD;

/**
 * Correct sequence for reading the EEPROM is:
 * @ Set address
 * @ Set RD bit
 * @ Read value from EEDATA
 *
 * This expression does exactly that, first setting EEADR and RD
 * before returning the value of EEDATA.
 */
#define EEPROM_READ(ADDR) (EEADR=ADDR,RD=1,EEDATA)

/**
 * Inline assembly to call the SLEEP instruction, since there's no 
 * equivalent function in C.
 */
#define sleep() __asm SLEEP __endasm

static void main(void)
{
#ifdef  __16f628a       // Only compile this section for PIC16f628a
	CMCON = 0x07;	/** Disable comparators.  NEEDED FOR NORMAL PORTA
			 *  BEHAVIOR ON PIC16f628a!
			 */
#endif
	TRISB=0x00;			// Set PORTB as all outputs.
	PORTB=EEPROM_READ(0x00);	// Display EEPROM 0x00 on PORTB.
	while(1) sleep();		// Drop into low-power mode.
}
