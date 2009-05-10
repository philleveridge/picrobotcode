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
static unsigned int  Cnt;
static unsigned char Mode;


unsigned char s_mask;
char *s_bytes, *s_bytes_;
unsigned char c_byte;
unsigned char t_byte;

unsigned char  wlc;  //wave length in 0.5us i.e. 1Khz = 0.5us on, 0.5us off -> L=1ms => 1Khz
unsigned char  wln;  //wave length in 0.5us i.e. 1Khz = 0.5us on, 0.5us off -> L=1ms => 1Khz


// [t/10ms, f]
unsigned char tune[] = {
	100,2,100,0,100,3,100,0,0
	};
	
unsigned char t;
unsigned char t0;

#define TMS 20;


static void isr(void) interrupt 0 { 

    T0IF = 0; 
    Cnt++;
	
	if (Cnt%2==2)
	{
		if (Msec >0) Msec--;
	}
	
	if (Cnt%8==8)  // every 4ms = 250 B/s sample input
	{
		c_byte<<2;
		c_byte |= ((PORTA & 0x04)>>2);
		
		if (cbyte==0xAA) // start received
		{
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
					PORTA ^= 0X40; 		// flip bit A6;
					
					wln--;
					if (wln==0)
					{
						wln=tune[t-1];	
						if (wln != 0) 
							wlc=tune[t];
						t+=2;
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
	t=0;
	Mode=0;
	Cnt=0;
	c_byte=0;
}



///////////////////////////////////////////////////////////////////////////////
// cylon() - simulate cylon scanner
///////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------
// a simple delay function

void delay(unsigned char ms)
{
	Msec=ms;
	
	while (Msec) 
	{
	}  
}

void play_tone()
{
	t=1;
}

void readMode()
{


}


#define CYLON_SCAN_DELAY 25

/*

  A1 A0 A7 A6 V+ B7 B6 B5 B4 
  |  |  |  |  |  |  |  |  |
 ---------------------------
 |      PIC 16F648         |
 -o-------------------------
  |  |  |  |  |  |  |  |  |
  A2 A3 A4 A5 G  B0 B1 B2 B3
 
 
 B0-B7 A0-A1 Are connected to LED
 A2 control inut
 A6/7 Sound output
 
*/ 

#define MASK_A (unsigned char)0xFC
#define MASK_B (unsigned char)0x00



void start() {

	unsigned char i; // array iterator
/* 10 bits */
unsigned char cylon_bits_a[] = { 2,3,  1,  0,  0, 0, 0,0,0,0 };
unsigned char cylon_bits_b[] = { 0,0,128,192,112,56,12,6,3,1 };

unsigned char mask_a = 0xfc;
unsigned char mask_b = 0x00;

	while(1) {
	
		play_tone();

	
		while (Mode==0) // wait until mode set
		{
			PORTA = 0;
			PORTB = 132;  //00 10000100
			delay(CYLON_SCAN_DELAY);		
		}
		
		play_tone();
	

		if(Mode == 1) {

			// traditional (back & forth) cylon scanner

			for(i = 1; i < sizeof(cylon_bits_a); i++) {
			
			
				PORTA &= MASK_A;
				PORTA |= cylon_bits_a[i];
				PORTB &= MASK_B;
				PORTB |= cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}

			for(i = sizeof(cylon_bits_a) - 2; i > 1; i--) {
				PORTA &= MASK_A;
				PORTA |= cylon_bits_a[i];
				PORTB &= MASK_B;
				PORTB |= cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}

		} else if(Mode == 2) {

			// single direction scan

			for(i = 0; i < sizeof(cylon_bits_a); i++) {
				PORTA &= MASK_A;
				PORTA |= cylon_bits_a[i];
				PORTB &= MASK_B;
				PORTB |= cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}


		} else if(Mode == 3) {

			// other direction scan

			for(i = sizeof(cylon_bits_a); i > 0; i--) {
				PORTA &= MASK_A;
				PORTA |= cylon_bits_a[i];
				PORTB &= MASK_B;
				PORTB |= cylon_bits_b[i];
				delay(CYLON_SCAN_DELAY);
			}
		}
	}
}



//__code __at (0x300) unsigned char sound_bytes_0[] = { 0xAA, 0xAA, 0xAA};
//__code __at (0x310) unsigned char sound_bytes_1[] = { 0xCC, 0xCC, 0xCC};

/*

void play_sound(unsigned char sound_id, int ptime) {

unsigned char sound_bytes_0[] = { 0xAA, 0xAA, 0xAA};
unsigned char sound_bytes_1[] = { 0xCC, 0xCC, 0xCC};

		
	// play sound from array
	int sz;
	char *sb;
	
	switch(sound_id) {
	case 0:
		sz = sizeof(sound_bytes_0);
		sb = &sound_bytes_0[0];
		break;
	case 1:
		sz = sizeof(sound_bytes_1);
		sb = &sound_bytes_1[0];
		break;
	}
	
	s_mask=1;
		
	while (ptime-- > 0)
	{
		if (s_mask==0)
		{
			s_mask=1;
			s_bytes++;
			if ((s_bytes-sb) >= sz)
				s_bytes=sb;
		}
		if ((s_mask & *s_bytes) != 0)
		{
			PORTA |= 0x80;  		//Bit 7 On
			PORTA &= 0xBF;  		//Bit 6 Off
		}
		else
		{
			PORTA &= 0x7F;  		//Bit 7 Off
			PORTA |= 0x40;  		//Bit 6 On
		}

		s_mask <<= 1;	
		
		delay(1);
	}
}

*/


void main(void) {
	int i=0; 
	init();
	
	Mode=0;
	Cnt=0;
	
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
	Mode=0;
	Cnt=0;
 
	start();

}
