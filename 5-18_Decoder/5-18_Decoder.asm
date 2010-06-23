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
; Input pins are PD[0:5]
; If all inputs are 0 or above 18 the outputs are 0.  Otherwise
; the selected output is 1 and the remaining are zero.
; Outputs are assigned from PC[6] rotating counterclockwise
; to PB[6].
;
; Look at my horse, my horse is amazing.

; Revision history:
;
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
		SBRC r16, 0
		RJMP lcd_enable_set
		OUT PORTB, r16		; Oops, someone done something dum, like putting a
		OUT PORTC, r16		; value > 18 on the input lines.  Clear outputs, 
		OUT PORTD, r16		; return to sender.
		RJMP poll_loop
lcd_enable_set:
		OUT PORTB, r0
		OUT PORTC, r16
		OUT PORTD, r1
		RJMP poll_loop

