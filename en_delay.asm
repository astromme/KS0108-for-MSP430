; en_delay.asm
; Andrew Stromme and Alex Burka
; October 2011
;
; two delay functions to serve the ks0108/MSP430 library

 .cdecls C,LIST,	"msp430fg4618.h"
 .global EN_DELAY
 .global delay

	.text

; these functions are declared in msp.h

; simple busy loop for the enable pulse that causes the LCD to accept a command
EN_DELAY	mov.w	#6, R12 ; the 6 is EN_DELAY_VALUE (see ks0108_Panel.h)
edloop		dec.w	R12
			and.w	R12, R12
			jne		edloop
			reta
			
; delay for X milliseconds
; ASSUMPTION: processor is running at 8MHz
delay		mov.w	#2666, R13 ; empirical constant that makes a 1 millisecond busy loop
dloop2		dec.w	R13
			jnz		dloop2
			dec.w	R12
			jnz		delay ; loop X times, delaying 1 ms each time
			reta

; word to the wise: turn off the watchdog while testing delay functions
; otherwise they will never delay more than 35.4 ms

 .end
