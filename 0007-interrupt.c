/**
 * Simple Interrupt-On-B4...B7-Changed example.
 * Derived from http://tams-www.informatik.uni-hamburg.de/applets/hades/webdemos/72-pic/08-counter/count.html
 *
 * The manual says this interrupt occurs when any pin from B4 through B7
 * "changes", but this isn't exactly true.  The interrupt occurs when
 * the value WRITTEN TO B4...B7 doesn't match the value READ FROM B4...B7.
 *
 * So we set PORTB as all inputs, enable the pullup resistors to pull
 * them all high, and write 0xff to PORTB so the output matches the input.
 * Then we wait for the interrupt signalling something's pulled one down.
 *
 * When the interrupt occurs, we read the value from PORTB and write it 
 * back so that the outputs match the inputs again.  If we didn't do 
 * this, the interrupt would fire CONSTANTLY  instead of just ONCE since
 * the pin has ALWAYS CHANGED.
 */
#define __16f628a
#include "pic/pic16f628a.h"
#include "tsmtypes.h"
 
// Set the __CONFIG word:
// I usually set it to _EXTCLK_OSC&_WDT_OFF&_LVP_OFF&_DATA_CP_OFF&_PWRTE_ON
Uint16 at 0x2007  __CONFIG = CONFIG_WORD;

static Uint8 count;
static Uint8 breg;

// No C equivalent, so we make one with inline ASM.
// Pin interrupts can wake a PIC up, timer interrupts will not
#define sleep() __asm sleep __endasm

static void Intr(void) interrupt 0
{
	PORTA=count;	// PORTA0...A3 is count value.
	count++;	// Increment count

	breg=PORTB;	// Read PORTB inputs into breg.
	PORTB=breg;	/** Write that value back into PORTB latch values.
			 * The interrupt compares the inputs to the PORTB
			 * latch values, so if we don't update the latch 
			 * values, the interrupt will fire constantly!
			 */

	RBIF=0;		// Clear PORTB4...7 interrupt flag

//	GIE=1;		// Globally enable interrupts.
			/**  We don't need to do this ourselves since
			 *  the compiler ALWAYS ADDS THIS FOR US
			 *  in interrupt functions!
			 *  If you try and DISable interrupts in an 
			 *  interrupt function it WON'T WORK since
			 *  the compiler ALWAYS turns them back ON!
			 */
}

static void main(void)
{
	NOT_RBPU=0;		// Enable PORTB pullups

#ifdef __16f628a	// Only compile this section for PIC16f628a
        CMCON = 0x07;   /** Disable comparators.  NEEDED FOR NORMAL PORTA
                         *  BEHAVIOR ON PIC16f628a!
                         */
#endif

	TRISB=0xff;		// PORTB is all inputs
	TRISA=0x00;		// PORTA is all outputs
	PORTB=0xff;		/** The interrupt compares current
				 *  inputs to PORTB latch values.
				 *  We set latch values to all 1 to start.
				 */
	count=0;
	PORTA=count;		// Turn PORTA outputs low

	INTCON=0x00;		// Clear interrupt register completely.
	RBIE=1;			// Enable ONLY PORTB/B4...B7 interrupt.
	GIE=1;			// Globally enable interrupts.

	while(1) sleep();	/** Wait for interrupts.  We COULD
				 *  just make an infinite loop here,
				 *  but sleeping saves power.
				 */
}
