/**
 * Interrupt Flasher.  The program sits there and does nothing while 
 * interrupts fire periodically and increment PORTA.  Based on code by 
 * Mac Cody on this thread:
 * https://sourceforge.net/forum/forum.php?thread_id=1604002&forum_id=1864
 */
#define __16f628a
#include "pic/pic16f628a.h"
#include "tsmtypes.h"
#include "tsmpic.h"
 
// Set the __CONFIG word:
// I usually set it to _EXTCLK_OSC&_WDT_OFF&_LVP_OFF&_DATA_CP_OFF&_PWRTE_ON
Uint16 at 0x2007  __CONFIG = CONFIG_WORD;
 
static void Intr(void) interrupt 0
{
	T0IF = 0;	// Clear the Timer 0 interrupt.
	PORTA++;	// Toggle the state of the LSB of the port bits

//	GIE=1;		// Globally enable interrupts.
			/** We don't need to do this ourselves since
			 *  the compiler ALWAYS ADDS THIS FOR US
			 *  in interrupt functions!
			 *  If you try and DISable interrupts in an
			 *  interrupt function it WON'T WORK since
			 *  the compiler ALWAYS turns them back ON!
			 */
}

void main(void)
{
	TRISA = 0x00;	// All Port A latch outputs are enabled.

#ifdef  __16f628a	// Only compile this section for PIC16f628a
	CMCON = 0x07;	/** Disable comparators.  NEEDED FOR NORMAL PORTA
			 *  BEHAVIOR ON PIC16f628a!
			 */
#endif

	T0CS = 0;	// Clear to enable timer mode.
	PSA = 0;	// Clear to assign prescaler to Timer 0.

	PS2 = 1;	// Set up prescaler to 1:256.  
	PS1 = 1;
	PS0 = 1;

	INTCON = 0;	// Clear interrupt flag bits.
	GIE = 1;	// Enable all interrupts.

	T0IE = 1;	// Set Timer 0 to 0.  
	TMR0 = 0;	// Enable peripheral interrupts.

	// Loop forever.  
	while(1);
}  
