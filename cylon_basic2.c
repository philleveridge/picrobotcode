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

static unsigned char t;
static unsigned char t0;
	
static unsigned char i; // array iterator

static unsigned char  wlc;  //wave length in 0.5us i.e. 1Khz = 0.5us on, 0.5us off -> L=1ms => 1Khz
static unsigned char  wln;  //wave length in 0.5us i.e. 1Khz = 0.5us on, 0.5us off -> L=1ms => 1Khz


// [t/10ms, f]
static unsigned char tune[] = {
	100,2,100,0,100,3,100,0,0
	};
	

static void isr(void) interrupt 0 { 
    
    /*
    Notice the use of 'interrupt 0' keyword in the function above.  This
    is how SDCC knows that this is the interrupt service routine.  We will
    only be using level '0' for PICs, therefore, you can use this same 
    function in all your PIC applications.
    */

    T0IF = 0;               /* Clear timer interrupt flag */     
	
	PORTA ^= 0x80;  		//Flip Bit very -0.5ms = 1kHz
	
	if (Msec >0) Msec--;
	
	if ((PORTA & 0x04) != 0)   //RA2 (pin1)
	{
		Cnt++;
	}
	else
	{
		if (Cnt>0) {
			//
			Mode = Mode +1;
			if (Mode>3) Mode=0;
			Cnt=0;
		}
		
	}
		
	if(t==1)
	{
		wln=tune[t-1];
		wlc=tune[t];
		t+=2;
	}
	
	
	if (t>0 ) //
	{
		if (wlc!=0)
		{
			if (wlc >0) 
			{
				wlc--;
				if (wlc==0) 
				{
					// flip bit A6;
					PORTA ^= 0X40; 
					
					wln--;
					if (wln==0)
					{
						wln=tune[t-1];	
						if (wln != 0) 
						{
							wlc=tune[t];
							t+=2;
						}
						else
						{
							t=0; //finished
						}
					}
					else
						wlc=tune[t-2];
				}
			}
		}
		else
		{
			if (wln>0) {
				wln--;
			}
		}
	}
	
}


void init(void) {
	/* PORTB.1 is an output pin */ 
	TRISB = 0x00; 			// all outputs
	TRISA = 0x04; 			// RA0/1 are outputs RA2 will be input, RA6/RA7 Drive piezo transducer
	
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
#define mask_a (unsigned char)0xFC
#define mask_b (unsigned char)0x00

/* 10 bits */
const unsigned char cylon_bits_a[] = { 2,3,  1,  0,  0, 0, 0,0,0,0 };
const unsigned char cylon_bits_b[] = { 0,0,128,192,112,56,12,6,3,1 };

void cylon() {


	
//	const unsigned char mask_a = 0xfc;
//const unsigned char mask_b = 0x00;

    t=0;
	
	 
 	for (i=0; i<30; i++)
	{
		delay(50);
		if (i%2==0) {
			PORTA = 1;
			PORTB = 206;  //00 10000100
		}
		else {
			PORTA = 1;
			PORTB = 74;  //00 10000100
		}
	}
	


	while(1) {
	
		
		t=0;
	
		while (Mode==0) // wait until mode set
		{
			PORTA = 0;
			PORTB = 132;  //00 10000100
			delay(CYLON_SCAN_DELAY);		
		}	
		
		t=1;
	

		if(Mode==1) {

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

		} else if(Mode==2) {

			// single direction scan

			for(i = 0; i < sizeof(cylon_bits_a); i++) {
				PORTA = cylon_bits_a[i];
				PORTB = cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}


		} else if(Mode==3) {

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

 
 cylon();
 
}
