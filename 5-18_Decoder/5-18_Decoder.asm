; 1 of 18 decoder implemented on an Atmel AVR ATmega48PA
;
; WARNING, WARNING!  LICENSE JUNK!
;
; Licensed under the UIUC/NCSA open source license.
;
; Copyright (c) 2010 ACM SIGEmbedded
; All rights reserved.
;
; Developed by:     ACM SIGEmbedded
;                   University of Illinois ACM student chapter
;                   http://www.acm.illinois.edu/sigembedded/caffeine
;
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the
; "Software"), to deal with the Software without restriction, including
; without limitation the rights to use, copy, modify, merge, publish,
; distribute, sublicense, and/or sell copies of the Software, and to
; permit persons to whom the Software is furnished to do so, subject to
; the following conditions:
;
;   - Redistributions of source code must retain the above copyright
;   notice, this list of conditions and the following disclaimers.
;
;   - Redistributions in binary form must reproduce the above
;   copyright notice, this list of conditions and the following disclaimers
;   in the documentation and/or other materials provided with the
;   distribution.
;
;   - Neither the names of ACM SIGEmbedded, University of Illinois, UIUC
;   ACM student chapter, nor the names of its contributors may be used to endorse
;   or promote products derived from this Software without specific prior
;   written permission.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
; IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
; ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
; TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
; SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
;
;
; Now that that's all done...
;
; Input pins are PD[0:4]
; If all inputs are 0 or above 18 the outputs are previous value.  Otherwise
; the selected output is toggled and the remaining values are their previous
; value.  29 clears all, 30 clears LCD pins, 31 sets LCD pins.
; Outputs are assigned from PC[6] rotating counterclockwise
; to PB[6].
;
; Look at my horse, my horse is amazing.

; Revision history:
;
; v2.0
; - Changed behavior to toggle instead of only being on when the
;   input is at the value, bugfix for LCD enable lines.
;   - mike <mike@tuxnami.org>
; v1.1
;	- Added a condition that if the inputs are 11111b the LCD enable
;	  lines will be active.
;		- mike <mike@tuxnami.org>
; v1.0
;	- Initial code implementation
;		- mike <mike@tuxnami.org>

.include "m48def.inc"


.org $0000					; Vector table
		rjmp RESET			; Reset vector

.org $0020
RESET:
		LDI r16, $CF
		MOV r0, r16
		LDI r16, $D0
		MOV r1, r16
		LDI r16, $00		; Load some constants for init
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
		LDI r22, $00		; r22 - Toggle mask
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

		LDI r22, $00		; clear the toggle mask
		IJMP				; Perform the dreaded IJMP

.org $0040
		RJMP poll_loop		; If input is zero all outputs should be off

.org $0042
		LDI r22, $40		; 1
		RJMP toggle_c
		LDI r22, $20		; 2
		RJMP toggle_c
		LDI r22, $10		; 3
		RJMP toggle_c
		LDI r22, $08		; 4
		RJMP toggle_c
		LDI r22, $04		; 5
		RJMP toggle_c
		LDI r22, $02		; 6
		RJMP toggle_c
		LDI r22, $01		; 7
		RJMP toggle_c
		LDI r22, $20		; 8
		RJMP toggle_b
		LDI r22, $10		; 9
		RJMP toggle_b
		LDI r22, $08		; 10
		RJMP toggle_b
		LDI r22, $04		; 11
		RJMP toggle_b
		LDI r22, $02		; 12
		RJMP toggle_b
		LDI r22, $01		; 13
		RJMP toggle_b
		LDI r22, $80		; 14
		RJMP toggle_d
		LDI r22, $40		; 15
		RJMP toggle_d
		LDI r22, $20		; 16
		RJMP toggle_d
		LDI r22, $40		; 17
		RJMP toggle_b
		LDI r22, $80		; 18
		RJMP toggle_b
		NOP							; 19
		RJMP poll_loop
		NOP							; 20
		RJMP poll_loop
		NOP							; 21
		RJMP poll_loop
		NOP							; 22
		RJMP poll_loop
		NOP							; 23
		RJMP poll_loop
		NOP							; 24
		RJMP poll_loop
		NOP							; 25
		RJMP poll_loop
		NOP							; 26
		RJMP poll_loop
		NOP							; 27
		RJMP poll_loop
		NOP							; 28
		RJMP poll_loop
		NOP							; 29
		RJMP clear_all
		NOP							; 30
		RJMP clear_lcd
		NOP							; 31
		RJMP set_lcd

toggle_b:
		IN r23, PINB
		EOR r23, r22
		OUT PORTB, r23
		RJMP poll_loop

toggle_c:
		IN r23, PINC
		EOR r23, r22
		OUT PORTC, r23
		RJMP poll_loop

toggle_d:
		IN r23, PIND
		EOR r23, r22
		OUT PORTD, r23
		RJMP poll_loop

clear_all:
		OUT PORTB, r16
		OUT PORTC, r16
		OUT PORTD, r16
		RJMP poll_loop

clear_lcd:
		LDI r22, $30
		IN r23, PINB
		AND r23, r22
		OUT PORTB, r23

		LDI r22, $1F
		IN r23, PIND
		AND r23, r22
		OUT PORTD, r23
		RJMP poll_loop

set_lcd:
		LDI r22, $CF
		IN r23, PINB
		OR r23, r22
		OUT PORTB, r23

		LDI r22, $EO
		IN r23, PINB
		OR r23, r22
		OUT PORTB, r23
		RJMP poll_loop
