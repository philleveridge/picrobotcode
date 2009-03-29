/*
 * PWM registers configuration
 * Fosc = 4000000 Hz
 * Fpwm = 4000.00 Hz (Requested : 4000 Hz)
 * Duty Cycle = 60 %
 * Resolution is 9 bits
 * Prescaler is 1
 * Ensure that your PWM pin is configured as digital output
 * see more details on http://www.micro-examples.com/
 * this source code is provided 'as is',
 * use it at your own risks
 *
 * this example smoothly blinks LEDs on RC1 and RC2 alternatvely
 * using PIC CCP module configured as PWM output
 *
 * source code example for mikroC
 * feel free to use this code at your own risks
 *
 * Author : Bruno Gavand, September 2007
 * see more details on http://www.micro-examples.com/
 *
 *******************************************************************************
 */
#include <pic16f627.h>


void main()
        {
        unsigned char   dc ;

        TRISC = 0 ;                     // set PORTC as output
        PORTC = 0 ;                     // clear PORTC

        /*
         * configure CCP module as 4000 Hz PWM output
         */
        PR2 =  0x07c;   //b01111100 ;
        T2CON = 0x05;   //0b00000101 ;
        CCP1CON = 0x0c; //0b00001100 ;
        CCP2CON = 0x3c; //0b00111100 ;

        for(;;)                         // forever
                {
                /*
                 * PWM resolution is 10 bits
                 * don't use last 2 less significant bits CCPxCON,
                 * so only CCPRxL have to be touched to change duty cycle
                 */
                for(dc = 0 ; dc < 128 ; dc++)
                        {
                        CCPR1L = dc ;
                        CCPR2L = 128 - dc ;
                        Delay_ms(10) ;
                        }
                for(dc = 127 ; dc > 0 ; dc--)
                        {
                        CCPR1L = dc ;
                        CCPR2L = 128 - dc ;
                        Delay_ms(10) ;
                        }
                }
        }


 



