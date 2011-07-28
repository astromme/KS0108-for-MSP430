#ifndef KS0108_CONFIG_H
#define KS0108_CONFIG_H

#include "msp.h"

/*********************************************************/
/*  Configuration for assigning LCD bits to MSP430 Pins */
/*********************************************************/

// command pins
#define LCD_CMD_PORT        P3OUT       // port on which the command pins reside
#define CSEL1               pp(LCD_CMD_PORT,3)      // chip select 1
#define CSEL2               pp(LCD_CMD_PORT,4)      // chip select 2
#define R_W                 pp(LCD_CMD_PORT,1)      // read/write
#define D_I                 pp(LCD_CMD_PORT,0)      // D/I (also R_S in the docs)
#define EN                  pp(LCD_CMD_PORT,2)      // enable bit
#define RESET               pp(LCD_CMD_PORT,6)     // reset bit

// these macros  map pins to ports using the defines above  
#undef LCD_DATA_NIBBLES // we are not using nibble mode (in nibble mode, the data pins
                        // could be split across two ports)
#define LCD_DATA_LOW_NBL   7   // port for low nibble
#define LCD_DATA_HIGH_NBL  LCD_DATA_LOW_NBL   // port for high nibble

// convenience functions for pulling pins high/low
inline void fastWriteHigh(port,pin) { SETBIT(LCD_CMD_PORT,pin); }
inline void fastWriteLow(port,pin)  { CLRBIT(LCD_CMD_PORT,pin); }

#endif
