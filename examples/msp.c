/* msp.c
 * Alex Burka and Andrew Stromme
 * October 2010
 */

#include "msp.h"
#include "ks0108.h"

// singleton "class" instance
volatile ks0108 GLCD;

// set a pin to input or output
// re-implentation of the Arduino's function of the same name
void pinMode(unsigned char port, unsigned char pin, pin_mode mode)
{
#define pmcase(p) case p:\
						if (mode == INPUT)\
						{\
							PxDIR(p) &= ~BITX(pin);\
						}\
						else\
						{\
							PxDIR(p) |= BITX(pin);\
						}\
						break

	switch (port)
	{
		pmcase(1); // case for P1
		pmcase(2); // case for P2
		pmcase(3); // etc
		pmcase(4);
		pmcase(5);
		pmcase(6);
		pmcase(7);
		pmcase(8);
		pmcase(9);
		pmcase(10);
	}

#undef pmcase
}

