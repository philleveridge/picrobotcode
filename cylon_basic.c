/*
 
 Compile: sdcc --debug -mpic14 -p16f627 toggle_led.c
 Simulate: gpsim -pp16f627 -s toggle_led.cod toggle_led.asm
 
*/
 

#include <pic16f627.h>
 
/* Setup chip configuration */
typedef unsigned int config;
config at 0x2007 __CONFIG = 
	_CP_OFF &
	_WDT_OFF &
	_BODEN_OFF &
	_PWRTE_OFF &
	_INTRC_OSC_NOCLKOUT &
	_MCLRE_ON &
	_LVP_OFF;
 
#define b1 0x02 		/* pin 1 on PORTB */
#define B_OUTPUTS 0xFD 	/* value used to setup TRISB */

#define TIMER1 0x20 	// Used in delay routine
#define TIMER2 0x21 	// "	"	"	
#define PATERN 0x22 	// Pattern data for effect's

///////////////////////////////////////////////////////////////////////////////
// init() - initialize everything
///////////////////////////////////////////////////////////////////////////////
static unsigned char Msec;
static unsigned char Cnt;
static unsigned char Mode;

static void isr(void) interrupt 0 { 
    
    /*
    Notice the use of 'interrupt 0' keyword in the function above.  This
    is how SDCC knows that this is the interrupt service routine.  We will
    only be using level '0' for PICs, therefore, you can use this same 
    function in all your PIC applications.
    */

    T0IF = 0;               /* Clear timer interrupt flag */     
	
	PORTA ^= 0x80;  		//Flip Bit
	
	if (Msec >0) Msec--;
	
	if ((PORTA & 0x04) != 0)
	{
		Cnt++;
	}
	else
	{
		if (Cnt>0) {
			//
			Mode = !Mode;
			Cnt=0;
		}
		
	}
	
}


void init(void) {
	/* PORTB.1 is an output pin */ 
	TRISB = 0x00; 			// all outputs
	TRISA = 0x34; 			// RA0/1 are outputs RA2 will be input, RA6/RA7 Drive piezo transducer
	
	CMCON = 0x07;           /* disable comparators */
    T0CS = 0;               /* clear to enable timer mode */
    PSA = 0;                /* clear to assign prescaller to TMRO */
    
    /*
    The TMR0 interupt will occur when TMR0 overflows from 0xFF to
    0x00.  Without a prescaler, TMR0 will increment every clock
    cycle resulting in an interrupt every 256 cycles.  However, 
    using a prescaler, we can force that interrupt to occure at
    less frequent intervals.
    
    Each clock cycle is 1/4 the external clock.  Using that, and
    knowing the prescaler, we can determine the time interval for
    our interrupt.  
    
    PS2 PS1 PS0 Ratio   Cycles  4MHz        10MHz
    0   0   0   1:2     512      512.0 uS    204.8 uS    
    0   0   1   1:4     1024     1.024 mS    409.6 uS
    0   1   0   1:8     2048     2.048 mS    819.2 uS
    0   1   1   1:16    4096     4.096 mS    1.638 mS
    1   0   0   1:32    8192     8.192 mS    3.276 mS
    1   0   1   1:64    16384   16.384 mS    6.553 mS
    1   1   0   1:128   32768   32.768 mS   13.107 mS
    1   1   1   1:256   65536   65.536 mS   26.214 mS 
    */
    
    PS2 = 0;                /* 011 @ 4Mhz = 1.638 mS */
    PS1 = 0;  
    PS0 = 0;  

    INTCON = 0;             /* clear interrupt flag bits */
    GIE = 1;                /* global interrupt enable */
    T0IE = 1;               /* TMR0 overflow interrupt enable */
      
        
    TMR0 = 0;               /* clear the value in TMR0 */
 
}



///////////////////////////////////////////////////////////////////////////////
// cylon() - simulate cylon scanner
///////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------
// a simple delay function

void delay(unsigned char ms)
{
	Msec=ms<<2;
	
	while (Msec) 
	{
		PORTA ^= 0x40;  		//Flip Bit
	}  		//Flip Bit
}


#define CYLON_SCAN_DELAY 20

void cylon(unsigned char cylon_style) {

	/* 10 bits */
	const unsigned char cylon_bits_a[] = { 2,3,  1,  0,  0, 0, 0,0,0,0 };
	const unsigned char cylon_bits_b[] = { 0,0,128,192,112,56,12,6,3,1 };
	
	const unsigned char mask_a = 0xfc;
	const unsigned char mask_b = 0x00;
	
	
	
	unsigned char i; // array iterator

	while(1) {
	
		while (Mode==0) // wait until mode set
		{
			PORTA = 0;
			PORTB = 132;
			delay(CYLON_SCAN_DELAY);		
		}
		

		if(cylon_style == 0) {

			// traditional (back & forth) cylon scanner

			for(i = 1; i < sizeof(cylon_bits_a); i++) {
			
			
				PORTA &= mask_a;
				PORTA |= cylon_bits_a[i];
				PORTB &= mask_b;
				PORTB |= cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}

			for(i = sizeof(cylon_bits_a) - 2; i > 1; i--) {
				PORTA &= mask_a;
				PORTA |= cylon_bits_a[i];
				PORTB &= mask_b;
				PORTB |= cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}

		} else if(cylon_style == 1) {

			// single direction scan

			for(i = 0; i < sizeof(cylon_bits_a); i++) {
				PORTA = cylon_bits_a[i];
				PORTB = cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}


		} else if(cylon_style == 2) {

			// other direction scan

			for(i = sizeof(cylon_bits_a); i > 0; i--) {
				PORTA = cylon_bits_a[i];
				PORTB = cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}
		}
	}
}

 
void main(void) {
 
 init();

 Mode=0;
 
 cylon(0);
 
}
