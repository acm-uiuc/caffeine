/*
 * Decoder to enable LCDs/Vends.
 *
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#define F_CPU 8000000

#include <util/delay.h>

// Globals
uint8_t timer_counter[9];
uint8_t command_counter;
uint8_t previous_input = 255;

// Function prototypes
void vend_pulse(uint8_t);
void vend_off(uint8_t);
void vend_on(uint8_t);
void lcd_off(uint8_t);
void lcd_on(uint8_t);

ISR(TIMER1_COMPA_vect) {
  uint8_t i;


  for (i = 0; i < 9; i++) {
    if (timer_counter[i] == 45) {
      vend_off(i);
    } else if (timer_counter[i] < 45) {
      timer_counter[i]++;
    }
  }

}


void vend_pulse(uint8_t port)
{
  uint8_t i;

  if (port == 0xE) {
    for (i = 0; i < 9; i++) {
      vend_on(i);
      timer_counter[i] = 0;
    }
  } else if (port < 9) {
    vend_on(port);
    timer_counter[port] = 0;
  }
}

void vend_off(uint8_t port)
{
  uint8_t pb, pc, i;
  switch (port)
  {
    case 0x0:
      pc = PORTC;
      PORTC = pc & ~(1 << 6);
      timer_counter[0] = 255;
      break;
    case 0x1:
      pc = PORTC;
      PORTC = pc & ~(1 << 5);
      timer_counter[1] = 255;
      break;
    case 0x2:
      pc = PORTC;
      PORTC = pc & ~(1 << 4);
      timer_counter[2] = 255;
      break;
    case 0x3:
      pc = PORTC;
      PORTC = pc & ~(1 << 3);
      timer_counter[3] = 255;
      break;
    case 0x4:
      pc = PORTC;
      PORTC = pc & ~(1 << 2);
      timer_counter[4] = 255;
      break;
    case 0x5:
      pc = PORTC;
      PORTC = pc & ~(1 << 1);
      timer_counter[5] = 255;
      break;
    case 0x6:
      pc = PORTC;
      PORTC = pc & ~(1 << 0);
      timer_counter[6] = 255;
      break;
    case 0x7:
      pb = PORTB;
      PORTB = pb & ~(1 << 5);
      timer_counter[7] = 255;
      break;
    case 0x8:
      pb = PORTB;
      PORTB = pb & ~(1 << 4);
      timer_counter[8] = 255;
      break;
    case 0xE:
      for (i = 0; i < 9; i++)
        timer_counter[i] = 255;
      pb = PORTB;
      pc = PORTC;
      PORTB = pb & ~0x30;
      PORTC = pc & ~0x7F;
      break;
  }
}

void lcd_off(uint8_t port)
{
  uint8_t pb, pd;

  switch (port)
  {
    case 0x0:
      pb = PORTB;
      PORTB = pb & ~(1 << 3);
      break;
    case 0x1:
      pb = PORTB;
      PORTB = pb & ~(1 << 2);
      break;
    case 0x2:
      pb = PORTB;
      PORTB = pb & ~(1 << 1);
      break;
    case 0x3:
      pb = PORTB;
      PORTB = pb & ~(1 << 0);
      break;
    case 0x4:
      pd = PORTD;
      PORTD = pd & ~(1 << 7);
      break;
    case 0x5:
      pd = PORTD;
      PORTD = pd & ~(1 << 6);
      break;
    case 0x6:
      pd = PORTD;
      PORTD = pd & ~(1 << 5);
      break;
    case 0x7:
      pb = PORTB;
      PORTB = pb & ~(1 << 7);
      break;
    case 0x8:
      pb = PORTB;
      PORTB = pb & ~(1 << 6);
      break;
    case 0xE:
      pb = PORTB;
      pd = PORTD;
      PORTB = pb & ~0xCF;
      PORTD = pd & ~0xE0;
      break;
  }
}

void vend_on(uint8_t port)
{
  uint8_t pb, pc;
  switch (port)
  {
    case 0x0:
      pc = PORTC;
      PORTC = pc | (1 << 6);
      break;
    case 0x1:
      pc = PORTC;
      PORTC = pc | (1 << 5);
      break;
    case 0x2:
      pc = PORTC;
      PORTC = pc | (1 << 4);
      break;
    case 0x3:
      pc = PORTC;
      PORTC = pc | (1 << 3);
      break;
    case 0x4:
      pc = PORTC;
      PORTC = pc | (1 << 2);
      break;
    case 0x5:
      pc = PORTC;
      PORTC = pc | (1 << 1);
      break;
    case 0x6:
      pc = PORTC;
      PORTC = pc | (1 << 0);
      break;
    case 0x7:
      pb = PORTB;
      PORTB = pb | (1 << 5);
      break;
    case 0x8:
      pb = PORTB;
      PORTB = pb | (1 << 4);
      break;
    case 0xE:
      pb = PORTB;
      pc = PORTC;
      PORTB = pb | 0x30;
      PORTC = pc | 0x7F;
      break;
  }
}

void lcd_on(uint8_t port)
{
  uint8_t pb, pd;

  switch (port)
  {
    case 0x0:
      pb = PORTB;
      PORTB = pb | (1 << 3);
      break;
    case 0x1:
      pb = PORTB;
      PORTB = pb | (1 << 2);
      break;
    case 0x2:
      pb = PORTB;
      PORTB = pb | (1 << 1);
      break;
    case 0x3:
      pb = PORTB;
      PORTB = pb | (1 << 0);
      break;
    case 0x4:
      pd = PORTD;
      PORTD = pd | (1 << 7);
      break;
    case 0x5:
      pd = PORTD;
      PORTD = pd | (1 << 6);
      break;
    case 0x6:
      pd = PORTD;
      PORTD = pd | (1 << 5);
      break;
    case 0x7:
      pb = PORTB;
      PORTB = pb | (1 << 7);
      break;
    case 0x8:
      pb = PORTB;
      PORTB = pb | (1 << 6);
      break;
    case 0xE:
      pb = PORTB;
      pd = PORTD;
      PORTB = pb | 0xCF;
      PORTD = pd | 0xE0;
      break;
  }
}

int main()
{
  uint8_t i;
  uint8_t in;

  for (i=0; i < 9; i++)
   timer_counter[i] = 0xFF;

  // Setup the I/O ports
  DDRB = 0xFF;
  DDRC = 0x7F;
  DDRD = 0xE0;
  PORTB = 0;
  PORTC = 0;
  PORTD = 0;

  // Timer 1 setup
  OCR1AH = 0x9C;
  OCR1AL = 0x40;
  TIMSK1 = 2;
  TCCR1A = 0;
  TCCR1B = 8;

  // Int setup
  //EICRA = 3;
  //EIMSK = 1;

  sei();
  TCCR1B = 0x0a;

  while(1) 
  {

    in = PIND & 0x1F;

    if (in != previous_input) {
      previous_input = in;

      if (in >= 1 && in <= 9) {
        vend_pulse(in - 1);
      } else if (in >= 10 && in <= 18) {
        lcd_on(in - 10);
      } else if (in == 19) {
        lcd_on(0xE);
      } else if (in >= 20 && in <= 28) {
        lcd_off(in - 20);
      } else if (in >= 29) {
        lcd_off(0xE);
      }
    }
    _delay_us(2);
  }

}
