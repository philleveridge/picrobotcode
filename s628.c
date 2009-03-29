// ---------------------------------------------------------------------
// S628.c     Study/experiment with interrupt driven serial
//            communications with a PIC-equiped device as DCE.
//
// Author:    Rob Hamerling.
// Date:      February 2004.
// E-mail:    info@robh.nl 
// homepage:  http://www.robh.nl
// ---------------------------------------------------------------------
//
//  Function: echo incoming datastream from DTE back to DTE
//  Features:
//  - Interrupt driven, high speed, full duplex data flow (57600 bps)
//  - Use of builtin USART in RS232 mode (8 bits, no parity).
//  - With relatively large receive buffer.
//  - Using CTS flow control (PC -> PIC).
//    PC-side should have set CTS output flow control enabled,
//    PC FiFo transmit load count may be set to 16 (max).
//  - While DTE inactive (RTS false) the PIC slumbers. It gives a 'being
//    alive' signal by slowly flashing the RTS light. It is waked-up by
//    RB0 (to which RTS is connected)
//  - Note: no RTS flow control (PIC -> PC)!
//
//  Language support: CC5X compiler version 3.1
//
//  Hardware: PIC 16F628 or similar with UART, and MAX232.
//
//
// ---------------------------------------------------------------------
//
//  Simplified schematics:
//                                         COMx     plug
//     PIC16F628           MAX232          RS232   DB9 DB25
//  +-------------+     +-----------+
//  |             |     |           |
//  |     RA2  (1)|-----|(11)---(14)|--->-- CTS --- 8   5
//  |     RB0  (6)|-----|(12)---(13)|---<-- RTS --- 7   4
//  |             |     |           |
//  |     RB1  (7)|-----|(9)-----(8)|---<-- TxD --- 3   2
//  |     RB2  (8)|-----|(10)----(7)|--->-- RxD --- 2   3
//  |             |     |           |
//  |     (5)     |     |   (15)    |
//  +------|------+     +----|------+
//         +-----------------+----------- GND --- 5   7
//
//                                    +-< DTR --- 4  20
//   Optional cable wraps:            |
//   (maybe required by PC softw.)    +-> DSR --- 6   6
//                                    +-> DCD --- 1   8
//
// ---------------------------------------------------------------------
// Some basic PIC and RS232 knowledge will be needed to fully understand
// the data flow and control signalling used in this program.
//
// The PIC ports use positive logic:
// '1' is positive voltage, '0' is ground.
//
// This program uses positive logic for boolean variables:
// the symbol TRUE for '1', the symbol FALSE for '0'.
//
// In the RS232 standard:
// - Negative voltage ('mark') means OFF for control signals, and
//   indicates 1 (one) for a data signals (start-, data-, stop-bits).
// - Positive voltage ('space') means ON for control signals and
//   0 (zero) for start-, data- and stop-bits.
//
// Since the MAX232 is not only a level convertor (between TTL and RS232)
// but also a signal inverter, you should be aware of the following:
// - The inversion of PIC data-in and data-out by the MAX232 is required
//   to convert data-, start- and stop-bits to/from the corresponding
//   RS232 polarity. So nothing special has to be done in the program.
// - For RTS and CTS the inversion by the MAX232 inversion is NOT desired,
//   and therefore the program uses inverted signaling for RTS and CTS:
//   'FALSE' is used for ON and 'TRUE' for OFF with RTS/CTS signals!
//   As a reminder for this 'inversed' logic the signals are called
//   here CTSinv and RTSinv.
//
// -------------------------------------------------------------------------
//  For other examples and useful learning material 


See also:
 


//    - MicroChip datasheets (for the PIC16F62X: DS30400C).
//    - Tony Kubek's example for the PIC16F876,
//      ASM, interrupt driven, no CTS flow control, single byte buffer.
//    - Fr. Thomas MacGhee's example for the PIC16C74,
//      ASM, not interrupt driven, but contains many educational notes.
// -------------------------------------------------------------------------

#pragma  chip PIC16F628                         // target PIC

#include <int16cxx.h>                           // interrupt support

#pragma  config  &= ~0b11.1111.1111.1111        // all OFF
#pragma  config  |=  0b11.1111.0110.0110
//                                x   xx        FOSC = HS
//                                   x          WDT enabled
//                              x   x           BOD enabled (forces /PWRTE)
//                             x                LVP disabled (makes RB4 free)
//                     xxxxxxx                  no memory protection

#pragma  config ID = 0x6281                     // firmware ID (optional)

#pragma  bit CTSinv  @ PORTA.2                  // CTS signal to DTE (PC)
#pragma  bit RTSinv  @ PORTB.0                  // RTS signal from DTE (PC)
#pragma  bit RTSled  @ PORTB.3                  // visual RTS signal

typedef  bit         BOOL, BOOLEAN;             // boolean variable type(s)
#define  FALSE       0                          // PIC: off, low
#define  TRUE        1                          // PIC: on, high

#define  OSCFREQ     20000000                   // oscillator frequency

#define  TMR1COUNT   (OSCFREQ/4/1000)           // 16-bits count for 1 ms
                                                // (prescaler 1:1)

#define  BPSRATE     57600                      // desired speed
#define  BPSCLASS    TRUE                       // BRGH setting (high)
#define  BPSCOUNT    ((10*OSCFREQ/16/BPSRATE-5)/10)  // SPBRG (BRGH=1)
                                                // closest integer value

#define  XMTBUFSIZE  32                         // output buffer size
#define  RCVBUFSIZE  64                         // input buffer size
#define  DELTA       17                         // minimum free rcv buffer ..
                                                // .. space (PC UARTFiFo + 1)


int8     xmtoffset;                             // offset next byte to xmit
int8     putoffset;                             // offset last appl. out byte
int8     rcvoffset;                             // offset next byte to receive
int8     getoffset;                             // offset last appl. in byte

char     xmtbuf[XMTBUFSIZE];                    // circular output buffer

bank1 char rcvbuf[RCVBUFSIZE];                  // circular input buffer
                                                // located in RAM bank1!


// ----------------------------
//  Interrupt service routine
// ----------------------------
#pragma origin 4                                // hardware requirement
extern interrupt isr(void) {

  char  save_FSR;                               // FSR save byte
  char  x;                                      // intermediate byte value

  int_save_registers                            // save registers
  save_FSR = FSR;                               // save FSR

  if (TXIF == TRUE && TXIE == TRUE) {           // RS232 transmit interrupt
    if (xmtoffset != putoffset) {               // still data in xmit buffer
      x = xmtbuf[xmtoffset];                    // next char to xmit
      if (++xmtoffset >= XMTBUFSIZE)            // update offset
        xmtoffset = 0;                          // wrap
      if (xmtoffset == putoffset)               // was this last byte?
        TXIE = FALSE;                           // disable xmit interrupts
      TXREG = x;                                // now actually xmit char
      }
    }

  if (RCIF == TRUE && RCIE == TRUE) {           // RS232 receive interrupt
    if (OERR == TRUE) {                         // overrun, reset UART
      CREN = FALSE;                             // disable UART
      CREN = TRUE;                              // re-enable UART
      }                                         // discard pending bytes
    else if (FERR == TRUE) {                    // framing error (break?)
      getoffset = 0;                            // flush buffers
      xmtoffset = 0;
      putoffset = 0;
      rcvoffset = 1;
      rcvbuf[0] = RCREG;                        // move byte to rcv buffer
      CTSinv = TRUE;                            // ensure CTS is true
      }
    else {                                      // data without errors
      rcvbuf[rcvoffset] = RCREG;                // move byte to rcv buffer
      x = rcvoffset + 1;                        // offset next byte
      if (x >= RCVBUFSIZE)                      // beyond buffer boundary
        x = 0;                                  // wrap to begin
      if (x != getoffset)                       // buffer not yet full
        rcvoffset = x;                          // update offset,
                                                // (else discard byte,
                                                //  CTS flow control failed)

      if (CTSinv == FALSE) {                    // CTS is TRUE
        x = getoffset - rcvoffset;              // offset difference
        if (x <= 0)                             // wrapping effect
          x += RCVBUFSIZE;                      // wrapping correction
        if (x < DELTA)                          // buffer reaches 'full'
          CTSinv = TRUE;                        // make CTS FALSE
        }
      }
    }

  if (INTE == TRUE && INTF == TRUE) {           // RB0 change interrupt
                                                // nothing to do, just ..
    INTF = FALSE;                               // .. wake-up from sleep
    }

  /* Note: Other interrupts disabled, so no further checks needed */

  FSR = save_FSR;                               // restore FSR
  int_restore_registers                         // restore other

  }


// -----------------------------------------------
//  copy output bytes of caller
//    from: application buffer
//      to: interrupt controlled transmit buffer
//
//  returns nothing
//
//  notes: - initiates transmission (interrupt handler)
//           when not currently transmitting
//         - spin when transmission buffer full
//           (wait for free buffer space)
// -----------------------------------------------
static void putdata(char *buffer,
                    char bytesout) {
  char  i;                                      // counter(s)
  char  x;                                      // intermediate byte value

  for (i=0; i<bytesout; i++) {                  // all user data
    x = buffer[i];                              // copy char
    xmtbuf[putoffset] = x;                      // .. to buffer
    x = putoffset + 1;                          // next char
    if (x >= XMTBUFSIZE)                        // beyond buffer boundary
      x = 0;
    while (x == xmtoffset)                      // buffer full!
      ;                                         // spin until something xmit'd
    putoffset = x;                              // update offset
    TXIE = TRUE;                                // (re-)enable xmit interrupts
    }
  }


// ----------------------------------------------------------------
//  copy input bytes to callers buffer
//     from: interrupt controlled receive buffer
//       to: application buffer
//  returns: number of bytes actually stored in application buffer
//
//   notes: - rise CTS when receive buffer has more than <DELTA>>
//            bytes free space after delivering data to caller.
// ----------------------------------------------------------------
static char getdata(char *buffer,               // application buffer
                    char bufsize) {             // size of appl. buffer

  char  i, x;

  for (i=0; i<bufsize; i++) {                   // fill user buffer (max)
    if (getoffset == rcvoffset)                 // no more data
      break;
    x = rcvbuf[getoffset];                      // copy char
    buffer[i] = x;                              // .. to user buffer
    if (++getoffset >= RCVBUFSIZE)              // update offset
      getoffset = 0;
    }

  if (CTSinv == TRUE) {                         // (CTS is FALSE)
    x = getoffset - rcvoffset;                  // offset difference
    if (x <= 0)                                 // wrapping effect
      x += RCVBUFSIZE;                          // wrapping correction
    if (x >= DELTA)                             // enough free space now
      CTSinv = FALSE;                           // (make CTS TRUE)
    }

  return i;                                     // number of bytes returned
  }


// -------------------------------------------------------
//  Perform all required initial PIC setup
// -------------------------------------------------------
static void setup() {

  CMCON   = 0b0000.0111;                        // Comparator off
  CCP1CON = 0b0000.0000;                        // Capt/Comp/PWM off

  OPTION  = 0b0000.1111;                        // WDT prescaler 1:128
  T1CON   = 0b0000.0001;                        // Timer1 enabled, presc 1:1
  INTCON  = 0;                                  // all interrupt bits off
  PIR1    = 0;                                  //  ..
  INTEDG  = 0;                                  // int at falling edge RB0
                                                // (= rising of RTS!)

  PORTA   = 0;                                  // all ports zero
  PORTB   = 0;                                  //  ..
  TRISA   = 0b0010.0000;                        // IN: RA5/MCLR
  TRISB   = 0b0001.0011;                        // IN: RB0,1,4

  PIE1    = 0;                                  // disable all ext. interrupts

  BRGH    = BPSCLASS;                           // baudrate class
  SPBRG   = BPSCOUNT;                           // baudrate clock divisor
  TXEN    = TRUE;                               // enable UART transmit
  SYNC    = FALSE;                              // async mode
  RCIE    = TRUE;                               // enable receive interrupts
  SPEN    = TRUE;                               // enable UART
  CREN    = TRUE;                               // enable UART receive

  PEIE    = TRUE;                               // enable external interrupts
  INTE    = TRUE;                               // RB0 (RTSinv) change
  GIE     = TRUE;                               // globally enable interrupts

  }


// -------------------------------------------------------------------
//  Milliseconds delay by using TMR1 as 16-bits counter
//
//  See top of source for the value of TMR1COUNT
// -------------------------------------------------------------------
static void msdelay(char millisec) {

  uns16 usTimeCount;                            // value of Timer1

  do  {
    TMR1H = 0;                                  // restart Timer1 ..
    TMR1L = 0;                                  // .. counting
    do {
      usTimeCount = (uns16)TMR1H << 8;          // take high byte value
      usTimeCount += TMR1L;                     // aad low byte value
      } while (usTimeCount < TMR1COUNT);        // pause 1 millisecond
    } while (--millisec > 0);                   // number of milliseconds
  }


// -------------------------------------------------------------------
//  Keep PIC in slumbering state: sleep most of the time
//
//  Flash a LED as visual 'being alive' signal
//
// -------------------------------------------------------------------
static  void  waitforRTS(void) {

  char  ucOptionSave;                           // option reg at entry

  ucOptionSave = OPTION;                        // save OPTION register
  while (RTSinv == TRUE) {                      // waiting for RTS
    OPTION = 0b0000.1111;                       // WDT postscaler 1:128
    sleep();                                    // wait for RB0 or Watchdog
    RTSled = TRUE;                              // RTS LED on
    OPTION = 0b0000.1001;                       // WDT postscaler 1:2
    sleep();                                    // duration of RTS LED flash
    RTSled = FALSE;                             // RTS LED off
    }
  OPTION = ucOptionSave;                        // restore OPTION to original
  }


// ===============================================================
//
//   M A I N L I N E
//
//   Initially CTS is set false and the program waits for
//   RTS to become true before activating the echo loop.
//   When RTS become true, CTS follows, which allows the
//   DTE to send data. The echo loop remains active as
//   long as RTS remains true. When RTS becomes false the
//   echo-loop is terminated and the PIC reset to its initial
//   state, waiting for RTS.
//
// ===============================================================
extern void main(void) {

  char   i, k, l;                               // counter(s)
  char   buffer[20];                            // local I/O buffer
  const  char *welcome = "Echo from S628\n\r";  // welcome!

  setup();                                      // init PIC

  for (;;) {                                    // forever
    RTSled = FALSE;                             // assume RTS false
    CTSinv = TRUE;                              // CTS FALSE

    waitforRTS();                               // in slumbering state

    RTSled = TRUE;                              // show RTS status
    CTSinv = FALSE;                             // CTS TRUE

    xmtoffset = 0;                              // (re-)init ..
    putoffset = 0;                              //  .. input and ..
    rcvoffset = 0;                              //   .. output ..
    getoffset = 0;                              //    .. buffer offsets

    for (i=0; welcome[i] != '\0'; i++) {        // copy msg to I/O buffer
      k = welcome[i];
      buffer[i] = k;
      }
    putdata(buffer, i);                         // send msg to DTE

    while (RTSinv == FALSE) {                   // RTS true
      l = getdata(buffer, sizeof(buffer));      // get input
      if (l > 0)                                // something received
        putdata(buffer, l);                     // echo the input
      else                                      // nothing received
        msdelay(25);                            // do 'low priority' work
      clrwdt();                                 // reset watchdog
      }

    }

  }
