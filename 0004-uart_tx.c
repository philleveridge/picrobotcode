/**
 * This example for the PIC16f628a outputs a string through the serial 
 * port at a given baud.  It has been tested on a PIC16f628a operating 
 * at 4MHz outputting at 9600 baud.  See the PIC16f628a manual for 
 * tables of allowable clock rate and baud rate combinations.
 *
 * NOTE:  The PIC's serial port has the same timings as a normal PC 
 * serial port, but the wrong voltages.  A PIC only has 0V and 5V, while 
 * a serial port produces, and expects +12V/-12V!  You can't hook it up 
 * directly, a translator chip is needed.
 *
 * The MAX232 IC and its clones are among the most popular for this.  
 * They have built-in voltage inverters and doublers allowing them to 
 * run off the same 5V supply as your PIC, and are really simple to use.
 * See http://burningsmell.org/biollante/max232.gif for an outline.
 */
#define __16f628a
#include "pic/pic16f628a.h"
#include "tsmtypes.h"

// Set the __CONFIG word:
Uint16 at 0x2007  __CONFIG = CONFIG_WORD;

// If KHZ is not specified by the makefile, assume it to be 4 MHZ
#ifndef KHZ
#define KHZ	4000
#endif

// These are fixed.  The 16f628a can only use these as transmit and recieve.
#define TX_PORT	2
#define RX_PORT	1
#define TX_BIT	(1<<TX_PORT)
#define RX_BIT	(1<<RX_PORT)


// Twiddle these as you like BUT remember that not all values work right!
// See the datasheet for what values can work with what clock frequencies.
#define	BAUD	9600
#define BAUD_HI	1

// This section calculates the proper value for SPBRG from the given
// values of BAUD and BAUD_HI.  Derived from Microchip's datasheet.
#if	(BAUD_HI == 1)
#define	BAUD_FACTOR	(16L*BAUD)
#else
#define	BAUD_FACTOR	(64L*BAUD)
#endif
#define SPBRG_VALUE	(unsigned char)(((KHZ*1000L)-BAUD_FACTOR)/BAUD_FACTOR)

// Pop culture reference go!
//static const char str[]="\aQ! Q! Q! Q! US! US! US! US!\r\n";

// Until SDCC supports strings again the message will be far more terse.
static const char str[]={'\a', 's', 'd', 'c', 'c', '\r', '\n', '\0'};

void main(void)
{
	static unsigned char i;

	TRISB|=TX_BIT|RX_BIT;	// These need to be 1 for USART to work
	SPBRG=SPBRG_VALUE;	// Baud Rate register, calculated by macro
	BRGH=BAUD_HI;

	SYNC=0;			// Disable Synchronous/Enable Asynchronous
	SPEN=1;			// Enable serial port
	TXEN=1;			// Enable transmission mode

	/**
	 * Loop through each character in the array.  The '\0' marks 
	 * the end of the array.
	 *
	 * NOTE:  It does not send repeatedly!  It sends each character 
	 * ONCE then breaks the loop, stopping in the infinite loop 
	 * below it.
	 */
	for(i=0; str[i] != '\0'; i++)
	{
		TXREG=str[i];	// Add a character to the output buffer
		while(!TXIF);	// Wait while the output buffer is full
	}

	while(1);	// Loop forever
}
