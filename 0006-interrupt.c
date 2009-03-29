/**
 * Basic interrupt-on-B0 example.
 * Derived from http://tams-www.informatik.uni-hamburg.de/applets/hades/webdemos/72-pic/08-counter/count.html
 *
 * On the rising edge of a signal on B0, an interrupt is generated.  
 * This calls Intr(), which increments count(), and displays it on the 
 * upper bits of PORTB.
 */
#define __16f628a
#include "pic/pic16f628a.h"
#include "tsmtypes.h"
 
// Set the __CONFIG word:
// I usually set it to _EXTCLK_OSC&_WDT_OFF&_LVP_OFF&_DATA_CP_OFF&_PWRTE_ON
Uint16 at 0x2007  __CONFIG = CONFIG_WORD;

static Uint8 count;

// No C equivalent, so we make one with inline ASM.
// Pin interrupts can wake a PIC up, timer interrupts will not
#define sleep() __asm sleep __endasm

// Function is called when interrupt happens.
static void Intr(void) interrupt 0
{
	PORTB=count<<4;
	count++;

	INTF=0;		// Clear B0 interrupt so it can happen again

//	GIE=1;		// Globally enable interrupts.
			/** We don't need to do this ourselves since
			 *  the compiler ALWAYS ADDS THIS FOR US
			 *  in interrupt functions!
			 *  If you try and DISable interrupts in an
			 *  interrupt function it WON'T WORK since
			 *  the compiler ALWAYS turns them back ON!
			 */
}

static void main(void)
{
	NOT_RBPU=1;		// Disable Port B pullups...optional
	TRISB=0x0f;		// B0,...,B3=input, B4,...,B7=output
	count=0;

	INTCON=0x00;		// Clear interrupt register completely
	INTE=1;			// Set ONLY PORTB/B0 interrupt
	INTEDG=0;		// Interrupt on falling edge(1 is rising).
	GIE=1;			// Globally enable interrupts

	while(1) sleep();	// Wait for interrupts.  We COULD
				// just make an infinite loop here,
				// but sleeping saves power.
}  
