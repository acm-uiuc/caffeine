; 1 of 18 decoder implemented on an Atmel AVR ATmega48PA
;
; (c) 2010 Mike Crawford
;
; Input pins are PD[0:5]
; If all inputs are 0 or above 18 the outputs are 0.  Otherwise
; the selected output is 1 and the remaining are zero.
; Outputs are assigned from PC[6] rotating counterclockwise
; to PB[6].
;
; Look at my horse, my horse is amazing.

.include "m48def.inc"


.org $0000					; Vector table
		rjmp RESET			; Reset vector

.org $0020
RESET:	LDI r16, $00		; Load some constants for init
		LDI r17, $FF
		LDI r18, $7F
		LDI r19, $E0

		OUT MCUCR, r16		; Clear MCUCR
		OUT DDRB, r17		; Set port b, c and d[5:7] to output
		OUT PORTB, r16		; and d[0:4] to input
		OUT DDRC, r18
		OUT PORTC, r16
		OUT DDRD, r19
		OUT PORTD, r16

		CLI					; Globally mask all interrupts

		LDI r20, $1F		; r20 - Input mask
		LDI r21, $40		; r21 - offset for jumping
		LDI r28, $00		; r28 - store previous value of input lines
		LDI r31, $00		; r31 - Top half of Z register, must be zero for IJMP

poll_loop:					; main program loop
		IN r29, PIND		; Input the select values from d[0:4] into r29
		AND r29, r20		; Strip out d[5:7]
		CP r29, r28			; compare vs last value
		BREQ poll_loop		; repeat loop if equal
		MOV r28, r29		; copy into r28
		MOV r30, r29		; copy into r30
		LSL r30				; multiply by 2
		ADD r30, r21		; Add offset to r30

		OUT PORTB, r16		; Clear the output ports
		OUT PORTC, r16
		OUT PORTD, r16
		IJMP				; Perform the dreaded IJMP

.org $0040
		RJMP poll_loop		; If input is zero all outputs should be off

.org $0042
		SBI PORTC, 6		; Raise outputs based on where the IJMP jumps
		RJMP poll_loop
		SBI PORTC, 5
		RJMP poll_loop
		SBI PORTC, 4
		RJMP poll_loop
		SBI PORTC, 3
		RJMP poll_loop
		SBI PORTC, 2
		RJMP poll_loop
		SBI PORTC, 1
		RJMP poll_loop
		SBI PORTC, 0
		RJMP poll_loop
		SBI PORTB, 5
		RJMP poll_loop
		SBI PORTB, 4
		RJMP poll_loop
		SBI PORTB, 3
		RJMP poll_loop
		SBI PORTB, 2
		RJMP poll_loop
		SBI PORTB, 1
		RJMP poll_loop
		SBI PORTB, 0
		RJMP poll_loop
		SBI PORTD, 7
		RJMP poll_loop
		SBI PORTD, 6
		RJMP poll_loop
		SBI PORTD, 5
		RJMP poll_loop
		SBI PORTB, 6
		RJMP poll_loop
		SBI PORTB, 7
		RJMP poll_loop
		NOP					; NOP!!!!!!
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		OUT PORTB, r16		; Oops, someone done something dum, like putting a
		OUT PORTC, r16		; value > 18 on the input lines.  Clear outputs, 
		OUT PORTD, r16		; return to sender.
		RJMP poll_loop

