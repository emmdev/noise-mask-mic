
#include "io430.h"

#define LED_PIN         P1OUT_bit.P0

#define CS_PIN          P1OUT_bit.P4
#define SCK_PIN         P1OUT_bit.P5
#define MOSI_PIN        P1OUT_bit.P6
#define LDAC_PIN        P1OUT_bit.P2

void write_DAC( unsigned int );
unsigned int read_ADC( void );

unsigned int simul_ADCDAC( unsigned int );

unsigned int timerCount = 0;
char toggle = 0;

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  P1DIR |= (1 << 0) + (1 << 2) + (1 << 4) + (1 << 5) + (1 << 6); // Set P1.0, etc to output direction
  P1SEL &= ~( (1 << 0) + (1 << 2) + (1 << 4) + (1 << 5) + (1 << 6) ); //Select pins as IOs
  P1SEL |= (1 << 1); //Set P1.1 as peripheral (for ADC channel A1)

  //start
  CS_PIN = 1;
  SCK_PIN = 0;
  MOSI_PIN = 0;
  LDAC_PIN = 1;
  
  TACCTL0 = CCIE;
  TACTL = TASSEL_2 + MC_1; // Set the timer A to SMCLCK, Up to CCR0
  TACCR0 = 125 ; // 8kHz -> 1MHz/8kHz = 125
  
  __bis_SR_register(LPM0_bits + GIE); // Enter LPM0 w/ interrupts
  
  unsigned int  test;
  
  for (;;)
  {
    volatile unsigned int i;            // volatile to prevent optimization

    toggle ^= 1;
    
    i = 20000;                          // SW Delay
    do i--;
    while (i != 0);
/*    
    //new DAC update
    test = read_ADC();
    test <<= 2;
    write_DAC( (0x7 << 12) + test );*/
    
    if (toggle == 1) {
//      write_DAC( (0x7 << 12) + 0xfff );
//      test = read_ADC();

      test = simul_ADCDAC( (0x7 << 12) + 0xfff );
      
      LED_PIN = (test > 1000) ? 1 : 0;
    } else {
      write_DAC( (0x7 << 12) + 0x000 );
    }
  }

}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
//  timerCount = (timerCount + 1) % 8;
//  if(timerCount == 0) {
    toggle ^= 1;
    LED_PIN = (toggle == 1) ? 1 : 0;
//  }

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

unsigned int read_ADC()
{
  ADC10CTL0 &= ~ENC;				// Disable ADC
  ADC10CTL0 = ADC10SHT_2 + ADC10ON;     	// 16 clock ticks, ADC On
//  ADC10CTL0 = ADC10SHT_3 + ADC10ON;     	// 64 clock ticks, ADC On
  ADC10CTL1 = ADC10SSEL_3 + INCH_1;		// Set chan to INCH_1, SMCLK
  ADC10CTL0 |= ENC + ADC10SC;             	// Enable and start conversion

  while (ADC10CTL0 & ADC10IFG == 0) {
    __delay_cycles(1);
  }
  
  return ADC10MEM;			// Saves measured value.
}

unsigned int simul_ADCDAC( unsigned int data_out )
{
  //init DAC
  CS_PIN = 0;    
  for (unsigned int mask = 0x8000; mask > 0; mask >>= 1) {
    MOSI_PIN = (data_out & mask) ? 1 : 0;
    SCK_PIN = 1;
    __delay_cycles(10);
    SCK_PIN = 0;
  }
  CS_PIN = 1;
  
  //init ADC
  ADC10CTL0 &= ~ENC;				// Disable ADC
  ADC10CTL0 = ADC10SHT_2 + ADC10ON;     	// 16 clock ticks, ADC On
//  ADC10CTL0 = ADC10SHT_3 + ADC10ON;     	// 64 clock ticks, ADC On
  ADC10CTL1 = ADC10SSEL_3 + INCH_1;		// Set chan to INCH_1, SMCLK

  //trigger ADC
  ADC10CTL0 |= ENC + ADC10SC;             	// Enable and start conversion
  
  /* delay of 5 chosen so that ADC samples after DAC settles fully
  test for DAC settling: DAC was set to 0x000, now set to 0xfff, waiting until
  ADC measures > 1000 (out of 1023)
  */
  __delay_cycles(5);
  
  //trigger DAC
  LDAC_PIN = 0;
  
  //get ADC result
  while (ADC10CTL0 & ADC10IFG == 0) {
    __delay_cycles(1);
  }
  
  LDAC_PIN = 1;
  
  //return ADC result
  return ADC10MEM;
}