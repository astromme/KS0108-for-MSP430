#ifndef _E91_MSP_H_
#define _E91_MSP_H_

/* msp.h
 * Alex Burka, Andrew Stromme
 * October 2011
 *
 * This function defines various functions and macros specific to our combination of the ks0108 LCD and the MSP430
 */

// our processor
#include "msp430fg4618.h"

// UTILITY MACROS (these are useful, i.e. PxDIR(3) expands to P3DIR at compile time)
#define PXGLUE(a,b,c)	a ## b ## c
#define PxDIR(x)		PXGLUE(P,x,DIR)
#define PxOUT(x)		PXGLUE(P,x,OUT)
#define PxIN(x) 		PXGLUE(P,x,IN)
#define PxSEL(x)		PXGLUE(P,x,SEL)

#define BITX(b) (1<<(b))
#define SETBIT(p,b) p |= (1<<(b))
#define CLRBIT(p,b) p &= ~(1<<(b))
#define TGLBIT(p,b) p ^= (1<<(b))

// COMPATIBILITY HACKS
extern void EN_DELAY(void); // implemented in en_delay.asm
extern void delay(unsigned int); // delay for X milliseconds

// in ks0108_msp430.h this is used for pin definitions, i.e. P4.2 is denoted as pp(4,2)
// the macro adds flexibility -- it could, for instance, pack them into a struct or
//      an unsigned long
// in this case, all the functions that use the definitions take two parameters, in the
//      correct order, so this works fine
#define pp(a,b) a,b

// this is implemented in msp.c
void pinMode(unsigned char port, unsigned char pin, pin_mode mode);

#endif
