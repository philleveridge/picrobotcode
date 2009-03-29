//===================================================================//
//                         SERVOTST.C                                //
//===================================================================//
//	By:		Dale Botkin <dale@botkin.org>					    //
//	Date:	3/15/2000                                             //
//-------------------------------------------------------------------//
// Simple C program for the PIC MPU, pretty much any variety.  Will  //
// exercise up to four R/C servos connected to pins RB0:3.  I tested //
// this with three, but it's pretty obvious you need to power the    //
// servos separately from the PIC to cut down on glitches.  Very     //
// easy to modify for up to 8 servos, though every servo you add     //
// sucks up another 10% of available CPU time.  Since all we're      //
// doing is exercising them, though, using 80% of CPU time running   //
// the pulses is no biggie.                                          //
//-------------------------------------------------------------------//
// Copyright (C)2000, Dale Botkin, all rights reserved.  You may use //
// this code freely for personal, non-commercial use.  In the highly //
// unlikely event that you want to use this code for commercial use, //
// email me to make arrangements.                                    //
//-------------------------------------------------------------------//


#include "16C711.H"					// Change for your CPU
#include "int16CXX.h"

#pragma config WDTE=off, FOSC=XT, BODEN=on
#pragma config |= 0x3FB0  			// Code protect off

char	servo_pin;					// Variable for port pins
char	servo[4];						// Servo position values
char	servo_num;					// Pointer to current servo position value
char	position;						// Working servo position value
char	window;						// 2ms - position value
char	boogies;						// Interrupt counter
char	loops;						// Loop counter
char	current;						// same as servo_num, but outside of interrupt
bit	direction;					// direction of travel flag

#pragma	origin 4

		// With .1ms resolution, we can control the servo in 10% steps:
		// 10 = 0 degrees (or 100% backward motion)		1.0ms
		// 11 = 18 degrees (or 80% backward motion)		1.1ms
		// 12 = 36 degrees (or 60% backward motion)		1.2ms
		// 13 = 54 degrees (or 40% backward motion)		1.3ms
		// 14 = 72 degrees (or 20% backward motion)		1.4ms
		// 15 = 90 degrees (or stopped)				1.5ms
		// 16 = 108 degrees (or 20% forward motion)		1.6ms
		// 17 = 126 degrees (or 40% forward motion)		1.7ms
		// 18 = 144 degrees (or 60% forward motion)		1.8ms
		// 19 = 162 degrees (or 80% forward motion)		1.9ms
		// 20 = 180 degrees (or 100% forward motion)		2.0ms
		// So our pulse routine needs to run 1ms + x tenths.
		// Next version will (maybe) use hundredths, from 100 to 200.

interrupt scan_servos() {
	PORTA = 2;									// indicate where we are
	T0IF = 0;										// Clear TMR0 interrupt flag
	int_save_registers								// Save status & W reg
	char fsr; fsr = FSR;							// Save FSR
	OPTION = 8;									// No TMR0 prescaler
	servo_pin = 1;									// Set up for first servo 
	for(servo_num = 0; servo_num <=3; servo_num++) {
		position = servo[servo_num];					// get pulse width value
		window = 30 - position;						// Figure out remainder of 2ms window
		PORTB = servo_pin;							// Turn on servo pin
	   	TMR0 = 0;									// Clear TMR0
		do {
			TMR0 = 11;							// Adjust for execution time
			while ( TMR0 < 100 );					// wait 100 uS
		} while( --position > 0 );					// for as many as we need
		PORTB = 0;								// Turn off all servo pins
			// Now we have to burn up the remainder of the 2ms total pulse
			// window...
		if(window > 0) {
			do {
					TMR0 = 11;					// Adjust for execution time
				while ( TMR0 < 100 );				// wait 100 uS
			} while(--window > 0);					// for as many as we need
		}
			// And set us up for the next servo in line...
		servo_pin = servo_pin*2;						// Set up for next servo in line
	}
		
		// Now we set up for an 8ms delay until the ext interrupt.
		// We just spent 8ms pulsing the servos, so in another 12 we
		// need to do it again.
	int_restore_registers							// Retrieve the registers
	FSR = fsr;									// Restore FSR
	OPTION = 5;									// 256 prescaler for TMR0
	TMR0 = 187;									// Set up for 12ms interrupt
	boogies++;									// Increment boogie counter
	PORTA = 0;
}
	

void main(){
	T0CS = 0;										// T0 on instruction cycle
	T0SE = 1;										// rising edge
	clearRAM();									// start with clear RAM
	PORTA = 0;						
	PORTB = 0;									// All outputs off
	TRISA = 0;
	TRISB = 0;									// Port B is all output
	OPTION = 5;									// Prescaler set to 256
	direction = 0;										
	
		// Set all servo position registers to the 90 degree mid point.
	for(current = 0; current < 4; current++) {
		servo[current] = 15;
	} 
	TMR0 = 131;									// Set up timer for 8ms
	T0IE = 1;										// Enable TMR0 interrupt
	GIE = 1;										// Duh.

		// The boogie counter gets updated every 20ms, so 50 boogies
		// equals one second.  Tip of the hat to bogomips.

		// First wait 2 seconds to let us see how the servos do at deadband
	while(boogies < 100);							// Loop for 2 seconds

		// Now do 60 loops of 10% steps once a second, full motion
		// range from 10 to 20 and back.  Should take 1 minute.
	direction = 1;
	loops = 0;
	PORTA = 1;
	do {
		boogies = 0;								// clear boogie counter
		while( boogies < 10 );						// Wait 1/5 second	
		for(current=0; current < 4; current++) {
			if(servo[current] == 25) direction = 0;
			if(servo[current] == 5) direction = 1;
			if(direction == 0) servo[current]--;
			if(direction == 1) servo[current]++;
		}
	} while(++loops < 60);

		// Now recenter the servos for 2 seconds 
	for(current=0; current < 4; current++) {
		servo[current] = 15;
	}		
	boogies = 0;
	while(boogies < 100);

		// Now we finish up with 10 full-travel swings, 10 to 20,
		// with a 2-second delay in between. 

	loops = 0;
	PORTA = 0xD;
	do {
		boogies = 0;
		while(boogies < 100);						// 2 second delay
		for(current=0; current < 4; current++) {
			if(servo[current] == 25) direction = 0;
			else if(servo[current] == 5) direction = 1;
			else if(servo[current] > 5 && servo[current] < 25) direction = 0;
			if(direction == 0) servo[current] = 5;
			if(direction == 1) servo[current] = 25;
		}
	} while(++loops < 10);

		// And loop forever.
	main();
}
