#include "defines.h"
#include "caffeine.h"

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

char card_data[CARDBUF_L];
uint8_t card_bit_counter, card_byte_counter, card_status = 0;

char current_command;

char transmit_buffer[SER_BUF_L];
uint8_t transmit_length, transmit_index;

uint8_t LCD_flags = 0;

uint8_t button_debounce[9];

ISR(INT0_vect) {
	
	if (check_card_data() == 0)
		send_card_data();
	else
		send_error();

	reset_card_data();
}


ISR(INT1_vect) {

	uint8_t input_bit = 0;

	if (card_bit_counter == 0) {
		card_data[card_byte_counter] = 0;
	}


	input_bit = PORTD & 16;
	card_bit_counter++;

	card_data[card_byte_counter] <<= 1;

	if (input_bit == 0)
		card_data[card_byte_counter]++;

	if (card_bit_counter == 5) {
		card_bit_counter = 0;
		
		if (card_byte_counter < CARDBUF_L - 1) {
			card_byte_counter++;
		}
	}

}

ISR(USART0_RX_vect) {

	unsigned char input_char;

	input_char = UDR0;

	if (input_char == RESET_CMD) {

		current_command = 0;

		// revert_LCD_buffer();
		
	} else {

		if (current_command == 0) {

			switch (input_char) {
				case 'V':
					current_command = input_char;
					break;
				default:
					current_command = 0;
					break;
			}

		} else {

			switch (current_command) {
				case 'V':
					input_char -= 48;
					if (input_char >= 0 || input_char <= SEL_NUM) {
						vend(input_char);
					}
					current_command = 0;
					break;
				default:
					current_command = 0;
					break;

			}
		}

	}

}

ISR(USART0_TX_vect) {

	if (transmit_index < SER_BUF_L && transmit_index < transmit_length) {
		UDR0 = transmit_buffer[transmit_index];
		transmit_index++;
	} else {
		transmit_index = 0;
		transmit_length = 0;
	}

}

void vend(char selection) {

	if (selection <= SEL_NUM) {

		// Spinlock while we are waiting for the LCD to finish writing.
		uint8_t i;
		for (i = 0; i < 255; i++) {
			if (LCD_flags == 0)
				break;
			_delay_us(50);
		}

		i = PORTC;
		i &= 224;
		i += (selection + 1);
		PORTC = i;

		_delay_ms(500);
		i -= (selection + 1);
		PORTC = i;

		send_ack();

	}
}

void send_ack() {

	while (transmit_length != 0)
		_delay_us(100);

	UDR0 = 'A';
	transmit_length = 0;
	transmit_index = 0;

}

void send_error() {
	while (transmit_length != 0)
		_delay_us(100);

	UDR0 = 'A';
	transmit_length = 0;
	transmit_index = 0;

}

void send_button_down(uint8_t button) {

	while (transmit_length != 0)
		_delay_us(100);

	UDR0 = 'D';
	transmit_length = 1;
	transmit_index = 0;
	transmit_buffer[0] = button + 48;

}

void send_button_up(uint8_t button) {

	while (transmit_length != 0)
		_delay_us(100);

	UDR0 = 'U';
	transmit_length = 1;
	transmit_index = 0;
	transmit_buffer[0] = button + 48;

}

char check_card_data() {

	uint8_t i;

	for (i = 0; i < CARDBUF_L; i++) {
		card_data[i] &= 0x0f;
		card_data[i] += 0x30;
	}

	return 0;

}

void reset_card_data() {

	uint8_t i;

	for (i = 0; i < CARDBUF_L; i++) {
		card_data[i] = 0;
	}

	card_bit_counter = 0;
	card_byte_counter = 0;

}

void send_card_data() {

	while (transmit_length != 0)
		_delay_us(100);

	uint8_t i;
	for(i = 0; i < CARDBUF_L; i++)
		transmit_buffer[i] = card_data[i];

	UDR0 = 'C';
	transmit_length = CARDBUF_L;
	transmit_index = 0;

}


int main() {
	uint8_t input_l, input_h, i;
	uint16_t input;
	uint16_t button_status = 0;
	uint16_t  mask;
	

	// RS-232 setup.  115200kbps, interrups for RX and TX complete, enable TX and RX.
	UCSR0B = _BV(RXCIE0) | _BV(TXCIE0) | _BV(RXEN0) | _BV(TXEN0);
	UBRR0H = 0;
	UBRR0L = 12;

	// Enable the interrupt on the clock line of the card reader.
	EICRA = _BV(ISC11) | _BV(ISC00) | _BV(ISC01);
	EIMSK = _BV(INT1) | _BV(INT0);

	// IO setup.  Disable pull-up resistors, set port A, C, and D7 to out; rest to in.
	DDRD = 0x80;
	DDRC = 0xff;
	DDRB = 0x00;
	DDRA = 0xff;

	sei();

	
	// Main loop
	while (1) {

		input_l = PINB;
		input_h = PIND;
		input_h &= 64;
		input = input_l;
		if (input_h != 0)
			input += 256;
		
		mask = 1;
		for (i = 0; i < 9; i++) {
			if ( (input & mask) != 0 && ((mask & button_status) == 0)) {
				button_debounce[i]++;
				if (button_debounce[i] == 2) {
					button_status |= mask;
					send_button_down(i);
					button_debounce[i] = 0;
				}
			} else {
				button_debounce[i] = 0;
				if ( (input & mask) == 0 && ((mask & button_status) != 0)) {
					button_status = button_status & (~mask);
					send_button_up(i);
				}
			}
			mask <<= 1;
		}

		_delay_ms(1);

	}

}
