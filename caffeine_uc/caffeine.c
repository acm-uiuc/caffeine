/*
 * caffeine.c v0.2
 *
 * Microcontroller source code for Caffeine 2 controller board.
 *
 * Main controller source code.
 *
 * WARNING, WARNING!  LICENSE JUNK!
 *
 * Licensed under the UIUC/NCSA open source license.
 *
 * Copyright (c) 2010 ACM SIGEmbedded
 * All rights reserved.
 *
 * Developed by:     ACM SIGEmbedded
 *                   University of Illinois ACM student chapter
 *                   http://www.acm.illinois.edu/sigembedded/caffeine
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal with the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 *   - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimers.
 *
 *   - Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following disclaimers
 *   in the documentation and/or other materials provided with the
 *   distribution.
 *
 *   - Neither the names of ACM SIGEmbedded, University of Illinois, UIUC
 *   ACM student chapter, nor the names of its contributors may be used to endorse
 *   or promote products derived from this Software without specific prior
 *   written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 *
 * Now that that's all done...
 *
 * This is the program that goes onto the main microcontroller for the ACM soda
 * machine.  This μC receives and sends RS-232 commands to the computer, 
 * controls vending, sets the LCD strings and their backlight color, monitors
 * button presses and releases, and gets and sends card data.
 *
 * Written for an Atmel ATmega644PA μC.
 *
 * Pin diagram:
 *                      ATmega644PA
 *                       +-------+
 *       Button 0 (PB0) -|1    40|- (PA0) Data Bus 0
 *       Button 1 (PB1) -|2    39|- (PA1) Data Bus 1
 *       Button 2 (PB2) -|3    38|- (PA2) Data Bus 2
 *       Button 3 (PB3) -|4    37|- (PA3) Data Bus 3
 *       Button 4 (PB4) -|5    36|- (PA4) Data Bus 4
 *       Button 5 (PB5) -|6    35|- (PA5) Data Bus 5
 *       Button 6 (PB6) -|7    34|- (PA6) Data Bus 6
 *       Button 7 (PB7) -|8    33|- (PA7) Data Bus 7
 *         Vcc (RESET') -|9    32|- (AREF) Vcc
 *            Vcc (Vcc) -|10   31|- (GND) GND
 *            GND (GND) -|11   30|- (AVcc) Vcc
 *               XTAL 2 -|12   29|- (PC7) LCD R'/W
 *               XTAL 1 -|13   28|- (PC6) LCD RS
 *      RS232 RX (RXD0) -|14   27|- N/C
 *      RS232 TX (TXD0) -|15   26|- (PC4) Vend/select 4
 * Card Present' (INT0) -|16   25|- (PC3) Vend/select 3
 *   Card Clock' (INT1) -|17   24|- (PC2) Vend/select 2
 *     Card Data' (PD4) -|18   23|- (PC1) Vend/select 1
 *                  N/C -|19   22|- (PC0) Vend/select 0
 *       Button 8 (PD6) -|20   21|- (PD7) LED PWM controller select
 *                       +-------+
 *
 * PC[0:4] goes to the 5-18 encoder microcontroller and PD7 and the Data Bus [0:6] goes
 * to the LED PWM controller microcontroller.  See those source files
 * for more on how to hook them up.
 *
 * Version history:
 *
 * v0.4 -
 * 		Updated to use the toggle-based selection microcontroller for the
 * 		vend/LCD selection.
 * 			- mike <mike@tuxnami.org>
 *
 * v0.3 -
 *		Added LCD handling commands
 *			- mike <mike@tuxnami.org>
 *
 * v0.2 -
 *    Added the ability to output strings, capability and version data.
 *      - mike <mike@tuxnami.org>
 *
 * v0.1 -
 * 		Implemented vend selection, button press/release, card input, card data
 * 		checking, serial I/O.
 * 			- mike <mike@tuxnami.org>
 *
 */

// Project specific header files 
#include "defines.h"
#include <inttypes.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "caffeine.h"

#define VERSION_DATA " Caffeine2 v0.3 - (c) 2010 SIGEmbeded, ACM UIUC; licensed under NCSA license."
#define CAPABILITY_DATA " CMD: PCSRVvc  RES: CUDAEFs"

// Card data globals
char card_data[CARDBUF_L];
unsigned char card_bit_counter, card_byte_counter, card_status = 0;

// Command processing globals
char current_command;

// Serial transmit buffer globals
char transmit_buffer[TX_BUF_L];
volatile unsigned char transmit_length;
unsigned char transmit_index;

// Serial receive buffer
char receive_buffer[RX_BUF_L];
unsigned char receive_index;

// LCD panel globals
volatile unsigned char LCD_flags = 0;
unsigned char lcd_back_buffer[SEL_NUM][LCD_CHARS];
unsigned char lcd_front_buffer[SEL_NUM][LCD_CHARS];
unsigned char lcd_custom_chars[SEL_NUM][8][8];

// Backlight globals
unsigned char LED_backlight_values[SEL_NUM];


// External interrupt pin 0 ISR, triggered on the card detect rising edge
// This is triggered when the card is no longer detected.
ISR(INT0_vect) {
	
	// If the card data check passes, send the card data to the computer
	// otherwise send error.
	if (check_card_data() == 0)
		send_card_data();
	else
		send_error();

	// Reset the card data now that the data has been sent.
	reset_card_data();
}


// Interrupt vector for external interrupt 1, hooked up to the clock of the
// card reader.  This is triggered on the falling edge of the clock, and gets
// a bit of data from the card.
ISR(INT1_vect) {

	// Input data
	unsigned char input_bit = 0;

	// card_bit_counter is a counter/mask for which bit we are reading.  if it
	// is set to 1, the first bit, clear out the byte in the card data.
	if (card_bit_counter == 0x01) {
		card_data[card_byte_counter] = 0;
	}

	// Read the input bit and mask off all but pin PD4, tied to the data line
	// of the card reader.
	input_bit = PIND;
	input_bit = input_bit & 0x10;

	// Since the data is negated on these type of card readers (high is zero and
	// low is one), we will set the bit whenever we see Pd4 to be low.
	if (input_bit == 0) {
		card_data[card_byte_counter] |= card_bit_counter;
		// We want to set card status to 1 when the first one is seen on the card
		// data line.  This is to skip all the leading zeroes on the data stripe
		// in track 2.
		card_status = 1;
	}

	// Rotate the card bit counter left by one if the card status is 1.  Do not
	// advance the counter if there hasn't been a one recived from the card yet.
	if (card_status == 1) 
		card_bit_counter <<= 1;

	// After the fifth bit has been read in, reset the bit counter and advance
	// the byte counter.  Also skip the byte if it was read as all zero.
	if (card_bit_counter > 0x10) {
		card_bit_counter = 0x01;
		if (card_byte_counter < CARDBUF_L && card_data[card_byte_counter] != 0 ) {
			card_byte_counter++;
		}
	}

}

// receive complete ISR on USART0.  Whenever a byte is received by USART0 this
// ISR is triggered.  
ISR(USART0_RX_vect) {

	unsigned char input_char, i, j;

	// Read in the input character
	input_char = UDR0;

	// RESET_CMD is the command processor reset command, defined in defines.h.
	// Resetting the command processor will abandon any current active command
	// being processed and puts the chip back in a receptive state for the next
	// command.
	if (input_char == RESET_CMD) {

		// Dump the current command
		reset_command_processing();
		
	} else {

		// If the RESET_CMD was not sent, process the command.
		//
		// NOTE:  when adding new commands, the command input must be added and
		// the processing is added separately to the next section.  This is to
		// prevent junk from getting into the current_command variable.
		//
		// If there is no current command being processed, we want to start
		// processing the command based on what is sent.
		if (current_command == 0) {

			// Switch table to select the command.
			switch (input_char) {
				case 'V':  // Vend command
					current_command = input_char;
					break;
				case 'v':  // Version
					send_string(VERSION_DATA);
					break;
				case 'c': // capability
					send_string(CAPABILITY_DATA);
					break;
				case 'C': // Set character on LCD
					current_command = 'C';
					receive_index = 0;
					break;
				case 'S': // Set string on LCD
					current_command = 'S';
					receive_index = 0;
					break;
				case 'B': // Set backlight
					current_command = 'B';
					receive_index = 0;
					break;
				case 'P': // Ping
					send_ack();
					break;
				case 'R': // Refresh
					refresh_lcd();
				default:  // Anything else, set the command to null and break.
					current_command = 0;
					break;
			}

		} else {

			// If a command is being processed, do the appropraite action.
			switch (current_command) {
				case 'V':  // We are in the vend command
					input_char -= 0x30;  // Strip off ascii encoding for numbers
					// If the input character is between 0 and the number of selections
					// we will vend.
					if (input_char >= 0 || input_char < SEL_NUM) {  
						vend(input_char);
					}
					// Done processing the command.
					current_command = 0;
					break;
				case 'C':
					receive_buffer[receive_index] = input_char;
					if (receive_index == 0 && (input_char > (0x30 + SEL_NUM) || input_char < 0x30) ) {
						reset_command_processing();
						send_error();
						break;
					}
					if (receive_index == 1 && (input_char > (0x30 + LCD_LINES) || input_char < 0x30) ) {
						reset_command_processing();
						send_error();
						break;
					}
					if (receive_index == 2 && (input_char > (0x30 + LCD_CHARS/LCD_LINES) || input_char < 0x30) ) {
						reset_command_processing();
						send_error();
						break;
					}
					receive_index++;
					if (receive_index == 4) {
						receive_buffer[0] -= 0x30;
						i = receive_buffer[2] - 0x30;
						i = i + (receive_buffer[1] - 0x30) * LCD_CHARS/LCD_LINES;
						lcd_front_buffer[receive_buffer[0]][i] = receive_buffer[3];
						send_ack();
						current_command = 0;
					}
					break;
				case 'B':
					receive_buffer[receive_index] = input_char;
					if (receive_index == 0 && (input_char > (0x30 + SEL_NUM) || input_char < 0x30) ) {
						reset_command_processing();
						send_error();
						break;
					}
					if (receive_index == 1 && (input_char > (0x32) || input_char < 0x30)) {
						reset_command_processing();
						send_error();
						break;
					}
					if (receive_index == 2 && (input_char > 0x33 || input_char < 0x30) ) {
						reset_command_processing();
						send_error();
						break;
					}
					receive_index++;
					if (receive_index == 3) {
						receive_buffer[0] -= 0x30;
						receive_buffer[1] -= 0x30;
						receive_buffer[2] -= 0x30;
						receive_buffer[2] >>= 5;
						i = receive_buffer[0] * 3 + receive_buffer[1];
						i = i | recieve_buffer[2];
						cli();
						j = PINB;
						PORTB = i;
						PORTD = PORTD | 0x80;
						_delay_us(10);
						PORTD = PORTD & 0x7f;
						PORTB = j;
						sei();
					}
					break;
				case 'S':
					receive_buffer[receive_index] = input_char;
					if (receive_index == 0 && (input_char > (0x30 + SEL_NUM) || input_char < 0x30) ) {
						reset_command_processing();
						send_error();
						break;
					}
					receive_index++;
					if (receive_index == 33) {
						receive_buffer[0] -= 0x30;
						for(i = 0; i < LCD_CHARS; i++)
							lcd_front_buffer[receive_buffer[0]][i] = receive_buffer[i + 1];
						send_ack();
						current_command = 0;
					}
					break;
				default:  // If the current command is invalid, reset.
					current_command = 0;
					break;

			}
		}
	}
}

// Transmit complete ISR vector for USART0.  Here we will send any additional
// characters in the transmit buffer.
ISR(USART0_TX_vect) {

	// If the index of the next character to be transmitted is less than the
	// length of the message and the buffer transmit and advance the index.
	if (transmit_index < TX_BUF_L && transmit_index < transmit_length) {
		UDR0 = transmit_buffer[transmit_index];
		transmit_index++;

	// otherwise reset the transmit index and length, squashing any transmit.
	} else {
		transmit_index = 0;
		transmit_length = 0;
	}
}

// Vend a selection.
void vend(unsigned char selection) {

	// Check for validity of the selection
	if (selection < SEL_NUM) {

		toggle_select(selection + 1);
		// Wait a second and a half and then take out the selection.
		_delay_ms(1500);
		toggle_select(selection + 1);

		// Give the computer an ack that the soda has been vended.  One day
		// we may add a jam ('lo jams) detection in.
		send_ack();

	}
}

void toggle_select(unsigned char selection) {

	unsigned char i;

	if (selection < 32) {

		while (PINC & 0x1f) 
			_delay_us(2);

		cli();
		i = PINC;
		PORTC = i + (selection + 1);
		_delay_us(2);
		PORTC = i;

		if (selection > 18)
			_delay_us(25);
		else
			_delay_us(4);
		sei();

	}
}

// Send an ack to the computer.
void send_ack() {

	// Become a turning machine while we wait for the send buffer to clear
	while (transmit_length != 0)
		_delay_us(100);

	// Send the letter A, clear the transmit buffer flags.
	UDR0 = 'A';
	transmit_length = 0;
	transmit_index = 0;

}

void send_char(char c) {
	// Become a turning machine while we wait for the send buffer to clear
	while (transmit_length != 0)
		_delay_us(100);

	// Send the letter A, clear the transmit buffer flags.
	UDR0 = c;
	transmit_length = 0;
	transmit_index = 0;
}


// Oops, bad card data.  ERR-OR!
void send_error() {

	// Leekspin while the send buffer is tossing bits at the computer.
	while (transmit_length != 0)
		_delay_us(100);

	// Send the letter E, clear the transmit buffer flags.
	UDR0 = 'E';
	transmit_length = 0;
	transmit_index = 0;

}

// Someone just pushed my buttons!  Send the button press.
void send_button_down(unsigned char button) {

	// ROUND AND ROUND WE GO!  WHEN THE BUFFER IS CLEAR, THAT'S WHEN WE GO!
	while (transmit_length != 0)
		_delay_us(100);

	// Send the letter D, set the length to 1, index 0.
	UDR0 = 'D';
	transmit_length = 1;
	transmit_index = 0;
	// Put the button pushed into the buffer along with a friendly ASCII
	// encoding!
	transmit_buffer[0] = button + 0x30;

}

// Aw, they stopped pushing our buttons. :(  Guess we need to let
// the computer know.
void send_button_up(unsigned char button) {

	// Spin spin spin while we wait for the talking stick.
	while (transmit_length != 0)
		_delay_us(100);

	// Send our U and put our button into the transmit buffer along with a nice
	// ASCII sauce.
	UDR0 = 'U';
	transmit_length = 1;
	transmit_index = 0;
	transmit_buffer[0] = button + 0x30;

}

// Did someone screw up a swipe?  (how hard is it to sipe, really?)
char check_card_data() {

	unsigned char i, parity_count, parity_mask;

	// Loop for the number of bytes in the card buffer.  In the loop
	// check the parity (odd) on the datas.
	for (i = 0; i < CARDBUF_L; i++) {
		parity_count = 0;

		// Check each bit in the word, if it's one, increment the count
		for (parity_mask = 1; parity_mask < 0x10; parity_mask <<= 1) {
			if ((parity_mask & card_data[i]) != 0)
				parity_count++;
		}
		
		// Well, we only need the LSB, so let's ignore the rest.
		parity_count &= 1;

		// If the LSB is equal to the parity bit and the byte isn't zero
		// we have an ERR-OR, so it's time to skedaddle and tell our caller
		// as such.
		if ((parity_count == (card_data[i] >> 4)) && (card_data[i] != 0))
			return 1;

		// Let's play ASCII!
		card_data[i] |= 0x30;
	}

	// Oh, goodie, we got through the check without any ERR-ORs.  Let's
	// inform who asked that we are ERR-OR free and let them go about their day.
	return 0;

}

// We got junk in the trunk of the card data.  Let's clean that junk out of
// the trunk.
void reset_card_data() {

	unsigned char i;

	// Throw some bleach on the buffer and wipe it down.
	for (i = 0; i < CARDBUF_L; i++) {
		card_data[i] = 0;
	}

	// Set the bit counter to 1, byte counter ot 0, and card status to 0,
	// aka NO CARD DATA READ YET!
	card_bit_counter = 0x01;
	card_byte_counter = 0;
	card_status = 0;

}

// Time to send the datas!
void send_card_data() {

	// We don't want to step on someone else's transmit.  Let's SPIN while we 
	// wait for the datas to be tossed at the computar.
	while (transmit_length != 0)
		_delay_us(100);

	// Put the card data buffer into the transmit buffer.  Now, if you muck around
	// with the defines, DO NOT make the transmit buffer smaller than the card buffer.
	// Doing so would be stupid.  About as stupid as jamming a rusty spoon into one's
	// eye.
	unsigned char i;
	for(i = 0; i < CARDBUF_L; i++)
		transmit_buffer[i] = card_data[i];

	// Send the card data token C, set the transmit length and begin transmitting.
	UDR0 = 'C';
	transmit_length = CARDBUF_L;
	transmit_index = 0;

}

// Send a string over RS-232
void send_string(char *string) {

	int length = 0;

	// The usual transmit delay spin.
	while (transmit_length != 0)
		_delay_us(100);

	// Make sure we're not doing anything dumb with the string.
	length = strlen(string);
	if (length > TX_BUF_L)
		length = TX_BUF_L;
	
	// copy the string into the buffer, start the transmit
	memcpy(transmit_buffer, string, length);
	UDR0 = 's';

	transmit_length = length;
	transmit_index = 0;

}

// Reset the command processor, just set the vars to 0
inline void reset_command_processing() {

	receive_index = 0;
	current_command = 0;

}

// Send a command to an LCD panel.  LCD 0-8 or 31 for all LCDs.
inline void lcd_command(char command, char lcd) {

	lcd = lcd & 0x1f;
	if (lcd != 0x1f)
		lcd += 10;

	if ( (lcd >= 10 && lcd <= 18) && lcd == 31) { 
		PORTA = command;
		if (lcd == 31) {
			toggle_select(31);
			_delay_us(1);
			toggle_select(30);
			_delay_us(1);
		} else {
			toggle_select(lcd);
			_delay_us(1);
			toggle_select(lcd);
			_delay_us(1);
		}
	}
}

// Write a character to an LCD panel.  LCD 0-8 or 31 for all LCDs.
inline void lcd_write(char c, char lcd) {

	lcd = lcd & 0x1f;
	if (lcd != 0x1f)
		lcd += 10;

	if ( (lcd >= 10 && lcd <= 18) && lcd == 31) {
		PORTA = c;
		if (lcd == 31) {
			toggle_select(31);
			_delay_us(1);
			toggle_select(30);
			_delay_us(1);
		} else {
			toggle_select(lcd);
			_delay_us(1);
			toggle_select(lcd);
			_delay_us(1);
		}
	}
}

inline void lcd_home(char lcd) {

	lcd = lcd & 0x1f;
	if (lcd != 0x1f)
		lcd += 10;

	if ( (lcd >= 10 && lcd <= 18) && lcd == 31) {
		PORTA = 0x02;
		if (lcd == 31) {
			toggle_select(31);
			_delay_us(1);
			toggle_select(30);
			_delay_us(1);
		} else {
			toggle_select(lcd);
			_delay_us(1);
			toggle_select(lcd);
			_delay_us(1);
		}
	}
	_delay_us(39);
}

inline void lcd_clear(char lcd) {

	lcd = lcd & 0x1f;
	if (lcd != 0x1f)
		lcd += 10;

	if ( (lcd >= 10 && lcd <= 18) && lcd == 31) {
		PORTA = 0x01;	
		if (lcd == 31) {
			toggle_select(31);
			_delay_us(1);
			toggle_select(30);
			_dealy_us(1);
		} else {
			toggle_select(lcd);
			_delay_us(1);
			toggle_select(lcd);
			_dalay_us(1);
		}
	}
	_delay_us(39);
}


void init_lcd_panels() {

	unsigned char i, j;

	for(i=0; i < SEL_NUM; i++) {
		for(j=0; j < LCD_CHARS; j++) {
			lcd_front_buffer[i][j] = ' ';
			lcd_back_buffer[i][j] = ' ';
		}
	}

	_delay_ms(15);

	lcd_command(0x38, 31);
	_delay_us(38);
	lcd_command(0x0c, 31);
	_delay_us(38);
	lcd_command(0x04, 31);
	_delay_us(38);

	lcd_clear(31);
	lcd_home(31);

	refresh_lcd();
	lcd_home(31);

}

void refresh_lcd() {

	unsigned char i, j, k, offset;

	lcd_home(31);

	cli();
	memcpy(lcd_back_buffer, lcd_front_buffer, SEL_NUM * LCD_CHARS);
	sei();

	send_ack();

	for (j = 0; j < LCD_LINES; j++) {
		offset = j * ( LCD_CHARS/LCD_LINES );
		for (k = 0; k < (LCD_CHARS/LCD_LINES); k++) {
			for (i = 0; i < SEL_NUM; i++) {
				lcd_write(lcd_back_buffer[i][k + offset], i);
				_delay_us(1);
			}
			_delay_us(20);
		}
		lcd_command(0xc0, 31);
		_delay_us(38);
	}

	send_char('F');

}

int main() {
	// Variables for our inputs, iterators,  button status and a bitmask.
	unsigned char input_l, input_h, i;
	uint16_t input;
	uint16_t button_status = 0;
	uint16_t  mask;
	
	// Array for debouncing.
	unsigned char button_debounce[SEL_NUM];

	// RS-232 setup.  115200kbps, interrups for RX and TX complete, enable TX and RX.
	UCSR0B = _BV(RXCIE0) | _BV(TXCIE0) | _BV(RXEN0) | _BV(TXEN0);
	// FIXME:  future improvement, add speed, parity and stop bit config, maybe even have the computer be able to change them
	UBRR0H = 0;
	UBRR0L = 9;

	// Enable the interrupt on the clock line of the card reader.
	EICRA = 0x0b;
	EIMSK = _BV(INT1) | _BV(INT0);

	// IO setup.  Disable pull-up resistors, set port A, C, and D7 to out; rest to in.
	DDRD = 0x80;
	DDRC = 0xff;
	DDRB = 0x00;
	DDRA = 0xff;

	// Initalize data, LCDs
	reset_card_data();
	reset_command_processing();
	init_lcd_panels();

	// We'd be useless without interrupts.  It may be a good idea to turn them on.
	sei();

	
	// Main loop
	while (1) {

		// Let's get the input pins.
		input_l = PINB;
		input_h = PIND;
		// Mask off the junk for the high bits.  These bits are thathigh.
		input_h &= 64;

		// create our 16-bit word for the input (we use 9 of the 16 bits.)
		input = input_l;
		if (input_h != 0)
			input += 256;

		// Invert the input bits and strip off the extra ones!!!11111one
		input = ~input;
		input &= 0x1ff;
		
		// Set the mask and begin looping around checking each input
		mask = 1;
		for (i = 0; i < SEL_NUM; i++) {

			// if the current bit is high and the status is low we will want to
			// increment the debounce and check the debounce.
			if ( (input & mask) != 0 && ((mask & button_status) == 0)) {
				button_debounce[i]++;

				// if the debounce has gone through 64 cycles of debounce
				// set the button status bit, send our button down, reset the button
				// debounce to zero.  BOOOOINNNNGGG!
				if (button_debounce[i] == 64) {
					button_status |= mask;
					send_button_down(i);
					button_debounce[i] = 0;
				}

			// Ok, we got a bounce or a button release.
			} else {
				// Clear the debounce buffer.  FLOP!
				button_debounce[i] = 0;
				// If the button was previously down, well, we're up now.  So let's clear
				// the status bit, tell the computer that we're up, and go about our merry way.
				if ( (input & mask) == 0 && ((mask & button_status) != 0)) {
					button_status = button_status & (~mask);
					send_button_up(i);
				}
			}
			// CHANGE PLACES!
			mask <<= 1;
		}

		// Wait a tick.
		_delay_ms(1);

	}  // LET'S GO BACK AND DO IT ALL AGAIN!

} // A-badey, a-badey, a-badey, that's all folks!

