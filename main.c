
#include "io430.h"

#define LED_PIN         P1OUT_bit.P0

#define CS_PIN          P1OUT_bit.P4
#define SCK_PIN         P1OUT_bit.P5
#define MOSI_PIN        P1OUT_bit.P6
#define LDAC_PIN        P1OUT_bit.P2

void write_DAC(unsigned int);

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  P1DIR |= (1 << 0) + (1 << 2) + (1 << 4) + (1 << 5) + (1 << 6); // Set P1.0, etc to output direction
  P1SEL &= ~( (1 << 0) + (1 << 2) + (1 << 4) + (1 << 5) + (1 << 6) ); //Select pins as IOs

  //start
  CS_PIN = 1;
  SCK_PIN = 0;
  MOSI_PIN = 0;
  LDAC_PIN = 1;
    
  for (;;)
  {
    volatile unsigned int i;            // volatile to prevent optimization

//    P1OUT ^= 0x01;                      // Toggle P1.0 using exclusive-OR
    LED_PIN ^= 1;
    
    i = 20000;                          // SW Delay
    do i--;
    while (i != 0);
    
    //new DAC update
    write_DAC( (0x7 << 12) + 2000 );
  }

}

void write_DAC( unsigned int data_out )
{
  CS_PIN = 0;    
  for (unsigned int mask = 0x8000; mask > 0; mask >>= 1) {
    MOSI_PIN = (data_out & mask) ? 1 : 0;
    SCK_PIN = 1;
    __delay_cycles(10);
    SCK_PIN = 0;
  }
  CS_PIN = 1;
  LDAC_PIN = 0;
  __delay_cycles(10);
  LDAC_PIN = 1;
}
