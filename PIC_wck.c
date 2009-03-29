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

void delay_ms(int ms)
{
	while (ms > 100) {
		delay(100);
		ms = ms - 100;
	}
	delay(ms);
}


void SendByte(char Data1)
{
	static unsigned char i;

	TRISB|=TX_BIT|RX_BIT;	// These need to be 1 for USART to work
	SPBRG=SPBRG_VALUE;	// Baud Rate register, calculated by macro
	BRGH=BAUD_HI;

	SYNC=0;			// Disable Synchronous/Enable Asynchronous
	SPEN=1;			// Enable serial port
	TXEN=1;			// Enable transmission mode

	//for(i=0; str[i] != '\0'; i++)
	{
		TXREG=Data1;	// Add a character to the output buffer
		while(!TXIF);	// Wait while the output buffer is full
	}

	//while(1);	// Loop forever
}


static unsigned char GetByte(int TimeOut)
{
	static unsigned char i;

	TRISB=TX_BIT|RX_BIT;	// These need to be 1 for USART to work

	SPBRG=SPBRG_VALUE;	// Baud Rate register, calculated by macro
	BRGH=BAUD_HI;

	SYNC=0;			// Disable Synchronous/Enable Asynchronous
	SPEN=1;			// Enable serial port
	TXEN=1;			// Enable transmission mode
	CREN=1;			// Enable reception mode

	//while(1)
	{
		while(!RCIF);	// Wait until data recieved
		i=RCREG;	// Store for later

		//while(!TRMT);	// Wait until we're free to transmit
		//TXREG=i;	// Transmit
	}
	return i;
}


#define HEADER 		0xFF
#define NULL 		0
#define ROTATE_CCW 	3
#define ROTATE_CW 	4
#define TIME_OUT1 	100
#define TIME_OUT2 	100

/******************************************************************************/
/* Function that sends Operation Command Packet(4 Byte) to wCK module */
/* Input : Data1, Data2 */
/* Output : None */
/******************************************************************************/
void SendOperCommand(char Data1, char Data2)
{
	char CheckSum;
	CheckSum = (Data1^Data2)&0x7f;
	SendByte(HEADER);
	SendByte(Data1);
	SendByte(Data2);
	SendByte(CheckSum);
}

/******************************************************************************/
/* Function that sends Passive wCK Command to wCK module */
/* Input : ServoID */
/* Output : Position */
/******************************************************************************/
char ActDown(char ServoID)
{
	char Position;
	SendOperCommand(0xc0|ServoID, 0x10);
	GetByte(TIME_OUT1);
	Position = GetByte(TIME_OUT1);
	return Position;
}

/******************************************************************/
/* Function that sends 360 degree Wheel wCK Command */
/* Input : ServoID, SpeedLevel, RotationDir */
/* Return : Rotation Number */
/*****************************************************************/
char Rotation360(char ServoID, char SpeedLevel, char RotationDir)
{
	char RotNum;
	if(RotationDir==ROTATE_CCW) 
	{
		SendOperCommand((6<<5)|ServoID, (ROTATE_CCW<<4)|SpeedLevel);
	}
	else if(RotationDir==ROTATE_CW) 
	{
		SendOperCommand((6<<5)|ServoID, (ROTATE_CW<<4)|SpeedLevel);
	}
	RotNum = GetByte(TIME_OUT1);
	GetByte(TIME_OUT1);
	return RotNum;
}


void main(void)
{
	char id, old_position, now_position;
	init(); // Initialize peripheral devices(prepare for serial port)
	id = 0;
	old_position = ActDown(id); // Read the initial position of a wCK with ID 0
	while(1) {
		now_position = ActDown(id); // Read current position
		// If position value decreased, rotate to ccw direction for 1 second and turn to passive mode for 1 second
		if(now_position<old_position) {
			Rotation360(id, 10, ROTATE_CCW);
			delay_ms(1000);
			ActDown(id);
			delay_ms(1000);
		}
		// If position value increased, rotate to cw direction for 1 second and turn to passive mode for 1 second
		else if(now_position>old_position) {
			Rotation360(id, 10, ROTATE_CW);
			delay_ms(1000);
			ActDown(id);
			delay_ms(1000);
		}
		old_position = ActDown(id); // Read current position and save it
		delay_ms(300);
	}
}




