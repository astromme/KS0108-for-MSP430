/*
  ks0108.h - Arduino library support for ks0108 and compatable graphic LCDs
  Copyright (c)2008 Michael Margolis All right reserved
  mailto:memargolis@hotmail.com?subject=KS0108_Library 

  The high level functions of this library are based on version 1.1 of ks0108 graphics routines
  written and copyright by Fabian Maximilian Thiele. His sitelink is dead but
  you can obtain a copy of his original work here:
  http://www.scienceprog.com/wp-content/uploads/2007/07/glcd_ks0108.zip

  Code changes include conversion to an Arduino C++ library, rewriting the low level routines 
  to read busy status flag and support a wider range of displays, adding more flexibility
  in port addressing and improvements in I/O speed. The interface has been made more Arduino friendly
  and some convenience functions added. 

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

  Version:   1.0  - May  8 2008  - initial release
  Version:   1.0a - Sep  1 2008  - simplified command pin defines  
  Version:   1.0b - Sep 18 2008  - replaced <wiring.h> with boolean typedef for rel 0012  
  Version:   1.1  - Nov  7 2008  - restructured low level code to adapt to panel speed
                                 - moved chip and panel configuration into seperate header files    
                                 - added fixed width system font
  Version:   2   - May 26 2009   - second release
                                 - added support for Mega and Sanguino, improved panel speed tolerance, added bitmap support
     
*/

#include <inttypes.h>
typedef uint8_t boolean;
typedef uint8_t byte;

#ifndef KS0108_H
#define KS0108_H

#define GLCD_VERSION 2 // software version of this library

// Chip specific includes
#include "ks0108_msp430.h"

#include "ks0108_Panel.h"      // this contains LCD panel specific configuration


// paste together the port definitions if using nibbles
#define LCD_DATA_IN_LOW     PxIN(LCD_DATA_LOW_NBL)  // Data I/O Register, low nibble
#define LCD_DATA_OUT_LOW    PxOUT(LCD_DATA_LOW_NBL)  // Data Output Register - low nibble
#define LCD_DATA_DIR_LOW    PxDIR(LCD_DATA_LOW_NBL) // Data Direction Register for Data Port, low nibble

#define LCD_DATA_IN_HIGH    PxIN(LCD_DATA_HIGH_NBL) // Data Input Register  high nibble
#define LCD_DATA_OUT_HIGH   PxOUT(LCD_DATA_HIGH_NBL)    // Data Output Register - high nibble
#define LCD_DATA_DIR_HIGH   PxDIR(LCD_DATA_HIGH_NBL)    // Data Direction Register for Data Port, high nibble

#define lcdDataOut(_val_) LCD_DATA_OUT(_val_) 
#define lcdDataDir(_val_) LCD_DATA_DIR(_val_) 

// macros to handle data output
#ifdef LCD_DATA_NIBBLES  // data is split over two ports 
#define LCD_DATA_OUT(_val_) \
    LCD_DATA_OUT_LOW =  (LCD_DATA_OUT_LOW & 0xF0)| (_val_ & 0x0F); LCD_DATA_OUT_HIGH = (LCD_DATA_OUT_HIGH & 0x0F)| (_val_ & 0xF0); 

#define LCD_DATA_DIR(_val_)\
    LCD_DATA_DIR_LOW =  (LCD_DATA_DIR_LOW & 0xF0)| (_val_ & 0x0F); LCD_DATA_DIR_HIGH = (LCD_DATA_DIR_HIGH & 0x0F)| (_val_ & 0xF0);
#else  // all data on same port (low equals high)
#define LCD_DATA_OUT(_val_) LCD_DATA_OUT_LOW = (_val_);     
#define LCD_DATA_DIR(_val_) LCD_DATA_DIR_LOW = (_val_);
#endif


// Commands
#ifdef HD44102 
#define LCD_ON              0x39
#define LCD_OFF             0x38
#define LCD_DISP_START      0x3E   // Display start page 0
#else
#define LCD_ON              0x3F
#define LCD_OFF             0x3E
#define LCD_DISP_START      0xC0
#endif

#define LCD_SET_ADD         0x40
#define LCD_SET_PAGE        0xB8

#define LCD_BUSY_FLAG       0x80 

// Colors
#define BLACK               0xFF
#define WHITE               0x00

// useful user contants
#define NON_INVERTED false
#define INVERTED     true


// Uncomment for slow drawing
// #define DEBUG

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t page; // the chips split up the X axis (which is the Y axis externally) into 8-pixel pages
} lcdCoord;

// macros to convert common operations into primitives
#define ks0108_ClearScreenX(this) ks0108_FillRect(this, 0, 0, (DISPLAY_WIDTH-1), (DISPLAY_HEIGHT-1), WHITE)

// number of pages on a screen
#define XPAGES  8
// number of screens we will buffer in memory
#define SCREENS 4

// BEGIN ks0108 class ported from C++ to C

typedef struct   // shell struct for ks0108 glcd code
{
    lcdCoord            Coord; // current screen coordinate
    boolean             Inverted; // is the screen inverted (this is handled in software)
    uint8_t             buffer[XPAGES*SCREENS][DISPLAY_WIDTH]; // in-RAM screen buffer (same width, 4x height of physical screen)
    int                 startline; // current Y position in the buffer
} ks0108;

// inter-chip communication functions
inline uint8_t ks0108_ReadData(volatile ks0108 *this);
    // read pixel data
uint8_t ks0108_DoReadData(volatile ks0108 *this, uint8_t first);
    // helper function for ReadData
void ks0108_WriteCommand(volatile ks0108 *this, uint8_t cmd, uint8_t chip);
    // write a command (see ks0108 docs for instruction set)
void ks0108_DoWriteCommand(volatile ks0108 *this, uint8_t cmd, uint8_t chip, boolean d_i, boolean r_w);
    // write a command (more flexible than WriteCommand because the D_I(RS) and R_W lines are configurable)
void ks0108_WriteData(volatile ks0108 *this, uint8_t ks0108_data);
    // write pixel data
inline void ks0108_Enable(volatile ks0108 *this);
    // create the enable pulse that causes the ks0108 to accept a command
inline void ks0108_SelectChip(volatile ks0108 *this, uint8_t chip);
    // select one of the LCD chips
void ks0108_WaitReady(volatile ks0108 *this,  uint8_t chip);
    // wait for the LCD chip to be ready for input

// Control functions
void ks0108_Init(volatile ks0108 *this, boolean invert);
    // call this function first or nothing will work
void ks0108_GotoXY(volatile ks0108 *this, uint8_t x, uint8_t y);
    // move the "cursor" to a specific X/Y position
    // this is external X/Y, where X=[0,127] and Y=[0,63]
    //
// Graphic Functions
void ks0108_ClearPage(volatile ks0108 *this, uint8_t page, uint8_t color);
void ks0108_ClearScreen(volatile ks0108 *this, uint8_t color);
void ks0108_SetDot(volatile ks0108 *this, int xx, int yy, uint8_t color);

// New Functions (by Burka/Stromme)
void ks0108_ClearScreenUnsafe(volatile ks0108 *this, uint8_t color);
    // does not clear the buffer
void ks0108_DumpBuffer(volatile ks0108 *this);
    // implemented in paint.c

// END ks0108 class ported from C++ to C

extern volatile ks0108 GLCD; // singleton "class" instance
#endif
