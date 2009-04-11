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
 
#define TIMER1 0x20 	// Used in delay routine
#define TIMER2 0x21 	// "	"	"	
#define PATERN 0x22 	// Pattern data for effect's

///////////////////////////////////////////////////////////////////////////////
// init() - initialize everything
///////////////////////////////////////////////////////////////////////////////
/*

const unsigned char sound[] = {
	32,128,48,198,24,99,140,49,198,24,
	99,140,49,198,24,99,140,49,198,56,
	99,140,115,206,24,227,140,57,231,
	24,99,204,49,198,24,115,142,49,231,
	156,115,206,57,231,156,51,198,24,
	99,204,115,206,24,115,142,57,179,
	57,115,166,49,99,142,49,198,25,51,
	102,50,99,198,204,68,204,156,57,179,
	25,51,51,99,102,102,102,204,136,152,
	49,206,24,99,206,57,99,24,115,102,
	114,230,140,198,156,115,156,49,230,
	24,99,206,57,231,156,115,206,57,231,
	156,99,204,57,198,24,115,206,57,103,
	140,113,206,25,227,156,49,198,56,231,
	140,115,206,57,99,156,51,198,24,103,
	140,49,206,24,198,57,103,140,113,206,
	24,227,156,51,198,60,230,140,57,143,
	49,231,156,99,140,49,198,57,231,28,99,
	140,49,198,24,227,28,99,206,24,227,
	156,49,198,24,99,140,49,198,57,231,
	156,115,206,115,140,49,198,24,99,140,
	49,198,24,231,156,115,206,24,231,156,
	51,198,156,99,140,115,142,49,198,24,
	231,156,49,206,57,231,140,113,140,57,
	231,24,99,140,49,198,24,99,140,51,198,
	24,227,25,99,156,115,206,57,199,156,
	115,140,49,198,24,199,140,25,227,24,
	99,140,49,198,152,99,140,49,198,24,231,
	156,51,206,49,198,24,99,140,49,198,57,
	103,156,99,140,57,199,24,99,206,24,206,
	24,231,28,99,140,113,206,25,99,156,115,
	206,25,231,156,51,142,49,198,24,199,
	140,49,67,24,99,204,24,195,152,51,142,
	49,198,56,198,156,115,140,49,198,24,
	198,136,24,195,24,99,12
	
};
*/

/*const unsigned char sound[] = {
	0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,8,0,0,2,32,0,
	0,0,0,0,128,0,0,0,0,2,0,
	0,0,0,0,1,0,16,80,2,160,0,
	0,30,0,188,0,126,0,124,1,240,2,
	237,1,222,7,248,15,248,60,240,115,192,
	207,129,159,7,127,14,252,56,240,241,226,
	199,133,143,23,63,110,252,188,240,113,227,
	199,248,184,241,227,194,207,137,31,19,126,
	78,252,24,240,115,224,198,128,159,3,55,
	6,220,24,184,49,224,198,204,141,17,51,
	102,78,216,152,49,99,66,206,140,25,51,
	118,100,204,152,17,35,103,204,136,145,35,
	2,70,140,24,17,50,98,196,136,128,1,
	35,70,12,24,24,49,96,192,128,24,1,
	3,6,64,12,24,48,0,2,128,12,0,
	1,0,6,128,8,0,1,0,12,128,0,
	1,48
};
*/



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
 


unsigned char Msec;
unsigned char s_mask;
char *s_bytes, *s_bytes_;
unsigned char c_byte;

unsigned char  wlc;  //wave length in 0.5us i.e. 1Khz = 0.5us on, 0.5us off -> L=1ms => 1Khz

static void isr(void) interrupt 0 { 
   /*
    Notice the use of 'interrupt 0' keyword in the function above.  This
    is how SDCC knows that this is the interrupt service routine.  We will
    only be using level '0' for PICs, therefore, you can use this same 
    function in all your PIC applications.
    */

    T0IF = 0;               /* Clear timer interrupt flag */     
	if (Msec >0) Msec--;
	
	PORTA ^= 0X80;  // 1khz

	
	if (wlc!=0)
	{
		if (wlc >0) 
		{
			wlc--;
			if (wlc==0) 
			{
				// flip bit A6;
				PORTA ^= 0X40; 
			}
		}
	}
	

}


void init(void) {
	TRISB = 0x00; // all outputs
	TRISA = 0x00; // all outputs
	
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

	Msec=0;
}

// ------------------------------------------------
// a simple delay function




void delay(unsigned char ms)
{
	Msec=ms<<2;
	
	while (Msec) 
	{
	}  
}

void play_tone()
{
	// two tomes
	// 2 sec of 500Hz (B above middle C)
	int i;
	
	for (i=0; i<2000; i++) // 2 sec (1000 x 2 ms)
	{
		wlc=2;
		while (wlc!=0) ;
	}
		
	// 1 sec silence
	
	for (i=0; i<20; i++)  //  20 x 50ms = 1 sec
	{
		delay(50);
	}
	

	// 2 sec of 330Hz (G above middle C)
		
	for (i=0; i<2000; i++) // 2 sec (1000 x 2 ms)
	{
		wlc=3;		
		while (wlc!=0) ;
	}	
	
	// 1 sec silence
	
	for (i=0; i<20; i++)  //  20 x 50ms = 1 sec
	{
		delay(50);
	}
	

	// output on A4/A5 = PORTA = 0011 0000 
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
 
	init();
	
	while (1) {	
		//play_sound(0, 500);
		//play_sound(1, 500);
		
		play_tone();
	}

}

