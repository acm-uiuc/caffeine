/*
 * Decoder to enable LCDs/Vends.
 *
 * It uses 4-bit commnands from the main micro on port D[0:4]
 * Pin PD2 is a data ready input, the decoder will read the port
 * on the rising edge of this input.
 *
 * Most operations are broken up into two commands, one for the
 * actual command and the second is the parameter for the command.
 *
 * Commands:
 * Vend set
 * 0x0 - Vend pulse ( 0.75s )
 * 0x1 - Toggle
 * 0x2 - On
 * 0x3 - Off
 *
 * 0x4 - LCD pulse ( 2 clock cycles )
 * 0x5 - Toggle
 * 0x6 - On
 * 0x7 - Off
 *
 * All commands take a parameter
 * 0-8 is the vend port/LCD
 * 0xE will perform the operation on all of that bank
 *
 * 0xF - Soft Reset
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#define NOP()  __asm__ __volatile__("nop")

// Globals
uint8_t timer_counter[9];
uint8_t command_counter;
uint8_t previous_input = 255;

// Function prototypes
void vend_pulse(uint8_t);
void vend_toggle(uint8_t);
void vend_off(uint8_t);
void vend_on(uint8_t);
void lcd_pulse(uint8_t);
void lcd_toggle(uint8_t);
void lcd_off(uint8_t);
void lcd_on(uint8_t);
void lock();
void unlock();

// Interrupt for the data set ready pin.
ISR(INT0_vect) {
		uint8_t in_h, in_l;

		lock();
		in_l = PIND;
		in_h = in_l & 0x18;
		in_l &= 0x03;
		in_l = in_l | ( in_h >> 1 );

		if (in_l == 0xf)
		{
				previous_input = 255;
				vend_off(0xE);
				lcd_off(0xE);
		}
		else
		{
				if (previous_input == 255)
						previous_input = in_l;
				else
				{
						switch (previous_input)
						{
								case 0x0:
										vend_pulse(in_l);
										break;
								case 0x1:
										vend_toggle(in_l);
										break;
								case 0x2:
										vend_on(in_l);
										break;
								case 0x3:
										vend_off(in_l);
										break;
								case 0x4:
										lcd_pulse(in_l);
										break;
								case 0x5:
										lcd_toggle(in_l);
										break;
								case 0x6:
										lcd_on(in_l);
										break;
								case 0x7:
										lcd_off(in_l);
										break;
						}
						previous_input = 255;
				}
		}
		unlock();
}

ISR(TIMER1_COMPA_vect) {
		uint8_t i;

		lock();
/*		if (previous_input != 255) {
				if (command_counter < 5) {
						command_counter++;
				} else {
						previous_input = 255;
						command_counter = 0;
				}
		}*/		
		for (i = 0; i < 9; i++) {
				if (timer_counter[i] == 150) {
						vend_off(i);
				} else if (timer_counter[i] < 150) {
						timer_counter[i]++;
				}
		}
		unlock();
}
void lock()
{
		TCCR1B = 0x08;
		cli();
}

void unlock()
{
		sei();
		TCCR1B = 0x0a;
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

void lcd_pulse(uint8_t port)
{
		lcd_on(port);
		NOP();
		NOP();
		lcd_off(port);
		NOP();
		NOP();
}

void vend_toggle(uint8_t port)
{
		uint8_t pb, pc;

		switch (port)
		{
				case 0x0:
						pc = PORTC;
						PORTC = pc ^ (1 << 6);
						break;
				case 0x1:
						pc = PORTC;
						PORTC = pc ^ (1 << 5);
						break;
				case 0x2:
						pc = PORTC;
						PORTC = pc ^ (1 << 4);
						break;
				case 0x3:
						pc = PORTC;
						PORTC = pc ^ (1 << 3);
						break;
				case 0x4:
						pc = PORTC;
						PORTC = pc ^ (1 << 2);
						break;
				case 0x5:
						pc = PORTC;
						PORTC = pc ^ (1 << 1);
						break;
				case 0x6:
						pc = PORTC;
						PORTC = pc ^ (1 << 0);
						break;
				case 0x7:
						pb = PORTB;
						PORTB = pb ^ (1 << 5);
						break;
				case 0x8:
						pb = PORTB;
						PORTB = pb ^ (1 << 4);
						break;
				case 0xE:
						pb = PORTB;
						pc = PORTC;
						PORTB = pb ^ 0x30;
						PORTC = pc ^ 0x7F;
						break;
		}
}

void lcd_toggle(uint8_t port)
{
		uint8_t pb, pd;

		switch (port)
		{
				case 0x0:
						pb = PORTB;
						PORTB = pb ^ (1 << 3);
						break;
				case 0x1:
						pb = PORTB;
						PORTB = pb ^ (1 << 2);
						break;
				case 0x2:
						pb = PORTB;
						PORTB = pb ^ (1 << 1);
						break;
				case 0x3:
						pb = PORTB;
						PORTB = pb ^ (1 << 0);
						break;
				case 0x4:
						pd = PORTD;
						PORTD = pd ^ (1 << 7);
						break;
				case 0x5:
						pd = PORTD;
						PORTD = pd ^ (1 << 6);
						break;
				case 0x6:
						pd = PORTD;
						PORTD = pd ^ (1 << 5);
						break;
				case 0x7:
						pb = PORTB;
						PORTB = pb ^ (1 << 7);
						break;
				case 0x8:
						pb = PORTB;
						PORTB = pb ^ (1 << 6);
						break;
				case 0xE:
						pb = PORTB;
						pd = PORTD;
						PORTB = pb ^ 0xCF;
						PORTD = pd ^ 0xE0;
						break;
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
		EICRA = 3;
		EIMSK = 1;

		unlock();

		while(1) {}

}
