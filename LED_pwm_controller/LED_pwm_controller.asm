;  LED PWM controller for Caffeine
;
; (c) 2010 Mike Crawford
;
; This code will sit on a ATmega8515, and will control 27 LEDs.
;
; Each LED will have 4 brightness levels, 00 - off to 11 - full.
; LEDs are set by putting 8-bit parallel data on PORTB.
; PORTB[7] - Enable, if this is high the value will be read off PORTB[0:6]
; PORTB[0:4] - LED select
; PORTB[5:6] - LED value
;
; There's a zombie on your lawn.


.include "m8515def.inc"

.org $0000
		rjmp RESET				; Jump to reset vector

.org $0020
RESET:
        ldi r16,high(RAMEND)	; Set stack pointer
        out SPH,r16
        ldi r16,low(RAMEND)
        out SPL,r16
		cli						; Mask interrupts
		ldi r16, $04			; Disable pullup resistors
		out SFIOR, r16
		ldi r16, $FF			; Setup ports a c d and e as out
		out DDRA, r16
		out DDRC, r16
		out DDRD, r16
		out DDRE, r16

		ldi r16, $07			; Load 111 for all LED values
		ldi r17, $1B			; counter for loading values
		ldi r31, $01			; memory pointer, LED values begin
		ldi r30, $00			; at 0x0100 in SRAM
		ldi r29, $01			; Mask counter for PWM


init_loop:						; Save 111 into each of the memory
		st z+, r16				; locations for the LEDs
		dec r17
		brne init_loop

main_loop:
		rcall update_pwm		; Update PWM
		in r18, PINB			; Check PORTB for enable line being high
		ldi r16, $80
		and r16, r18
		breq main_loop			; If it is low, just keep looping
		rcall set_value			; If high call set_value
		rjmp main_loop			; Loop


set_value:						; Set a memory value.  r18 = input port contents
		push r16
		push r17
		push r19
		push r30
		push r31

		ldi r16, $60			; Strip out the value bits into r0
		and r16, r18
		swap r16
		lsr r16					; Shift them back to r0[1:0]

		ldi r30, $1F			; Setup memory address
		ldi r31, $01
		and r30, r18
		ldi r19, $1B
		cp r19, r30
		brmi exit_set_value

		ldi r17, $00			; Make contents for the memory
		cp r17, r16				; val(00) - 000
		breq make_led_pwm_done	; val(01) - 001
make_led_pwm:
		clc
		lsl r17					; val(10) - 011
		inc r17					; val(11) - 111
		dec r16
		brne make_led_pwm

make_led_pwm_done:
		st z, r17				; Save memory
exit_set_value:
		pop r31
		pop r30
		pop r19
		pop r17
		pop r16

		ret						; Return to sender



update_pwm:						; Update the PWM outputs
		push r16
		push r17
		push r19
		push r30
		push r31

		ldi r30, $00			; Setup the memory pointer
		ldi r31, $01

		ldi r17, $00			; Storage for output byte
		ldi r19, $08			; Counter for output
porta_loop:						; Loop for the PORT values
		rcall get_next_mem_value
		dec r19
		brne porta_loop

		out PORTA,r17			; Write out the port values

		ldi r17, $00			; Rinse and repeat
		ldi r19, $08
portc_loop:
		rcall get_next_mem_value
		dec r19
		brne portc_loop

		out PORTC,r17

		ldi r17, $00			; And again!
		ldi r19, $08
portd_loop:
		rcall get_next_mem_value
		dec r19
		brne portd_loop

		out PORTD,r17

		ldi r17, $00			; One more time, THIS TIME WITH FEELING!
		ldi r19, $03
porte_loop:
		rcall get_next_mem_value
		dec r19
		brne porte_loop

		swap r17				; Since we are only using 3 bits for PORTE
		clc						; we need to clean it up, swap nybbles and
		lsr r17					; shift left
		out PORTE,r17			; OUT!

		clc						; Rotate the counter in r29
		lsl r29
		cpi r29, $05
		brmi update_done		; If the counter is above 0x04, go directly
		clr r29					; to 1.  Do not pass go, do not collect $200
		inc r29

update_done:
		pop r31
		pop r30
		pop r19
		pop r17
		pop r16

		ret						; Return to sender




get_next_mem_value:				; Get the next PWM bit from memory
		push r16
		push r19
		ldi r19, $80			; Load bitmask into r19
		clc
		lsr r17					; Shift the register for the output
		ld  r16, z+				; Load memory value
		and r16, r29			; logical and with the counter
		breq bit_unset
		add r17, r19			; If result says bit should be set, add r19 to r17
bit_unset:
		pop r19
		pop r16
		ret						; Return to sender
