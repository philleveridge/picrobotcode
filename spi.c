void init_spi_communication ( void )
 {
  TRISC4 = 1;   //SPI Slave In
  TRISC5 = 0;   //SPI Slave Out
  TRISC3 = 1;   //SPI Slave CLK In
  TRISA5 = 1;   //SPI SS Enabled In, used for other purpose then SPI
     SMP = 0;   //SPI slave mode
   SSPEN = 1;   //SPI port pins enabled
   SSPM0 = 0;   //Slave Mode SS pin Disabled
   SSPM1 = 0;
   SSPM2 = 1;
   SSPM3 = 0;
     CKP = 1;   //Clock idle is high
     CKE = 0;   //Data on rising edge
 }

void main ( void )
 {
  hardware_init();
  initialize ();
  init_spi_communication ();    // Initialization of SPI
  SSPBUF = 0x00;                        // Clear SPI receive buffer
  SSPIE = 1;                      // Enable interrupt on SPI receiving
  PEIE = 1;                             // Enable peripherial interrupt(For SPI)
  ei();                         // Global interrupt enable
  asm("sleep");                 // Going to sleep
  while (1)
   ;
 }
