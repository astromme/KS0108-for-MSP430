/*
  ks0108.cpp - Arduino library support for ks0108 and compatible graphic LCDs
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

  Version:   1.0 - May 8  2008  - first release
  Version:   1.1 - Nov 7  2008  - restructured low level code to adapt to panel speed
                                 - moved chip and panel configuration into seperate header files    
                                 - added fixed width system font
  Version:   2   - May 26 2009   - second release
                                 - added support for Mega and Sanguino, improved panel speed tolerance, added bitmap support

    
  10/2010: modified and gutted by Alex Burka and Andrew Stromme
    added in-RAM buffer to support scrolling
    removed most functionality except SetDot (which updates the buffer)
    (future work: put the other functionality (rectangles, circles, fonts) back in
     and integrate it with the buffer)
     
*/

#include <inttypes.h>

#define ksSOURCE
#include "ks0108.h"

#include "msp.h"

//#define GLCD_DEBUG  // uncomment this if you want to slow down drawing to see how pixels are set

void ks0108_ClearPage(volatile ks0108 *this, uint8_t page, uint8_t color){
    uint8_t x;
    
    for(x=0; x < DISPLAY_WIDTH; x++){   
       ks0108_GotoXY(this, x, page * 8);
       ks0108_WriteData(this, color);
   }    
}

// clear screen, page by page
void ks0108_ClearScreen(volatile ks0108 *this, uint8_t color){
 ks0108_ClearScreenUnsafe(this, color);
 memset(this->buffer, 0, XPAGES*SCREENS*DISPLAY_WIDTH*sizeof(uint8_t)); // clear in-RAM buffer
}

// clear the screen without clearing the buffer
void ks0108_ClearScreenUnsafe(volatile ks0108 *this, uint8_t color){
 uint8_t page;
   for( page = 0; page < 8; page++){
      ks0108_GotoXY(this, 0, page * 8);
      ks0108_ClearPage(this, page, color);
 } 
}

// draw a dot on the screen
void ks0108_SetDot(volatile ks0108 *this, int xx, int yy, uint8_t color) {
    uint8_t data, x, y;
    
    if (xx > 0 && xx < 128 && yy > 0 && yy < 64)        // range check
    {
        x = xx;
        y = yy;
        
        ks0108_GotoXY(this, x, y-y%8);                  // go to the page where x/y lives
        
        data = ks0108_ReadData(this);                   // read current contents of page
        if(color == BLACK) {
            data |= 0x01 << (y%8);                      // set dot
            this->buffer[y/8 + this->startline/XPAGES][x] |= BITX(y%8);
        } else {
            data &= ~(0x01 << (y%8));                   // clear dot
            this->buffer[y/8 + this->startline/XPAGES][x] &= ~BITX(y%8);
        }   
        ks0108_WriteData(this, data);                   // write data back to display
    }
}

// set display to a given X/Y position
// these are external X/Y coords, not chip coords
void ks0108_GotoXY(volatile ks0108 *this, uint8_t x, uint8_t y) {
    uint8_t chip, cmd;
    
    if( (x > DISPLAY_WIDTH-1) || (y > DISPLAY_HEIGHT-1) )   // exit if coordinates are not legal
        return;
    this->Coord.x = x;                                      // save new coordinates
    this->Coord.y = y;

    // Only set the page if the chip is not already in that page
    if(y/8 != this->Coord.page) {
        this->Coord.page = y/8;
        cmd = LCD_SET_PAGE | this->Coord.page;              // set y address on all chips   
        for(chip=0; chip < DISPLAY_WIDTH/CHIP_WIDTH; chip++){
            ks0108_WriteCommand(this, cmd, chip);   
        }
    }
    chip = this->Coord.x/CHIP_WIDTH;                        // pick a chip based on X coord
    x = x % CHIP_WIDTH;                                     // set X coord relative to chip
    cmd = LCD_SET_ADD | x;
    ks0108_WriteCommand(this, cmd, chip);                   // set x address on active chip 
}

void ks0108_Init(volatile ks0108 *this, boolean invert) {
    uint8_t chip;

    this->startline = 0; // reset scroll position to top
    memset(this->buffer, 0, XPAGES*SCREENS*DISPLAY_WIDTH*sizeof(uint8_t));
                        // clear in-RAM buffer
      
    // set controls pins to output direction
    pinMode(D_I,OUTPUT);
    pinMode(R_W,OUTPUT);    
    pinMode(EN,OUTPUT);     
    pinMode(CSEL1,OUTPUT);
    pinMode(CSEL2,OUTPUT);

    delay(10);

    // should we be resetting the chips here?
    // it seems to work without it
    // the documentation says to pull the reset pin low
    //      for a short time, then high again

    fastWriteLow(D_I);
    fastWriteLow(R_W);
    fastWriteLow(EN);

    // reset current position to top
    this->Coord.x = 0;
    this->Coord.y = 0;
    this->Coord.page = 0;
    
    this->Inverted = invert;
    
    // turn on the chips
    for(chip=0; chip < DISPLAY_WIDTH/CHIP_WIDTH; chip++){
       delay(10);  
       ks0108_WriteCommand(this, LCD_ON, chip);             // power on
       ks0108_WriteCommand(this, LCD_DISP_START, chip);     // display start line = 0
    }
    delay(50);                                              // wait for the chips to come on
    ks0108_ClearScreen(this, invert ? BLACK : WHITE);       // display clear
    ks0108_GotoXY(this, 0,0);
}

// select one chip or the other
inline void ks0108_SelectChip(volatile ks0108 *this, uint8_t chip) {  
//static uint8_t prevchip; 
    if(chipSelect[chip] & 1)
       fastWriteHigh(CSEL1);
    else
       fastWriteLow(CSEL1);

    if(chipSelect[chip] & 2)
       fastWriteHigh(CSEL2);
    else
       fastWriteLow(CSEL2);
}

// wait until LCD busy bit goes to zero
void ks0108_WaitReady(volatile ks0108 *this,  uint8_t chip){

    ks0108_SelectChip(this, chip);
    lcdDataDir(0x00);
    fastWriteLow(D_I);  
    fastWriteHigh(R_W); 
    fastWriteHigh(EN);  
    EN_DELAY();
    while(LCD_DATA_IN_HIGH & LCD_BUSY_FLAG)
        ;
    fastWriteLow(EN);   

    
}

// pulse the enable pin, which causes whichever LCD chip is
// current enabled to accept a command
inline void ks0108_Enable(volatile ks0108 *this) {  
   EN_DELAY();
   fastWriteHigh(EN);       // EN high level width min 450 ns
   EN_DELAY();
   fastWriteLow(EN);
   EN_DELAY();              // some displays may need this delay at the end of the enable pulse
}

// actually read a byte of pixel data from the current position on the display
uint8_t ks0108_DoReadData(volatile ks0108 *this, uint8_t first) {
    uint8_t data, chip;

    chip = this->Coord.x/CHIP_WIDTH;
    ks0108_WaitReady(this, chip);
    if(first){
        if(this->Coord.x % CHIP_WIDTH == 0 && chip > 0){ // if we have to change the X/Y position
          ks0108_GotoXY(this, this->Coord.x, this->Coord.y);
          ks0108_WaitReady(this, chip);
        }
    }   
    fastWriteHigh(D_I);                 // D/I = 1
    fastWriteHigh(R_W);                 // R/W = 1
    
    fastWriteHigh(EN);                  // EN high level width: min. 450ns
    EN_DELAY();

#ifdef LCD_DATA_NIBBLES
     data = (LCD_DATA_IN_LOW & 0x0F) | (LCD_DATA_IN_HIGH & 0xF0);
#else
     data = LCD_DATA_IN_LOW;            // low and high nibbles on same port so read all 8 bits at once
#endif 
    fastWriteLow(EN); 
    if(first == 0) 
      ks0108_GotoXY(this, this->Coord.x, this->Coord.y);    
    if(this->Inverted)
        data = ~data;
    return data;
}

// read a byte of pixel data
inline uint8_t ks0108_ReadData(volatile ks0108 *this) {  
    ks0108_DoReadData(this, 1);                 // dummy read
    return ks0108_DoReadData(this, 0);          // "real" read
    this->Coord.x++;
}

// write a command to the screen
// (normal version with D_I and R_W low, for most commands)
void ks0108_WriteCommand(volatile ks0108 *this, uint8_t cmd, uint8_t chip) {
    ks0108_DoWriteCommand(this, cmd, chip, 0, 0);
}

// extra configurability for sending commands that don't have
// D_I and R_W both low (for example when we want to directly
// write pixel data without going through WriteData)
void ks0108_DoWriteCommand(volatile ks0108 *this, uint8_t cmd, uint8_t chip, boolean d_i, boolean r_w) {
     if(this->Coord.x % CHIP_WIDTH == 0 && chip > 0){
        EN_DELAY();
    }
    ks0108_WaitReady(this, chip);
    d_i ? fastWriteHigh(D_I) : fastWriteLow(D_I);   // D/I = d_i
    r_w ? fastWriteHigh(R_W) : fastWriteLow(R_W);   // R/W = r_w
    lcdDataDir(0xFF);

    EN_DELAY();
    lcdDataOut(cmd);
    ks0108_Enable(this);                            // enable pulse min width 450 ns
    EN_DELAY();
    EN_DELAY();
    lcdDataOut(0x00);
}

// write pixel data to the screen
void ks0108_WriteData(volatile ks0108 *this, uint8_t data) {
    uint8_t displayData, yOffset, chip;
    volatile uint16_t i;

#ifdef LCD_CMD_PORT 
    uint8_t cmdPort;    
#endif

#ifdef GLCD_DEBUG
    for(i=0; i<5000; i++);
#endif

    if(this->Coord.x >= DISPLAY_WIDTH)
        return;
    chip = this->Coord.x/CHIP_WIDTH; 
    ks0108_WaitReady(this, chip);


    if(this->Coord.x % CHIP_WIDTH == 0 && chip > 0){ // if we have to change the X/Y position
        ks0108_GotoXY(this, this->Coord.x, this->Coord.y);
    }

    fastWriteHigh(D_I);                 // D/I = 1
    fastWriteLow(R_W);                  // R/W = 0
    lcdDataDir(0xFF);                   // data port is output
    
    yOffset = this->Coord.y%8; // calculate intra-page offset

    if(yOffset != 0) { // we have to split the write across two pages
        // first page
#ifdef LCD_CMD_PORT 
        cmdPort = LCD_CMD_PORT;                     // save command port
#endif
        displayData = ks0108_ReadData(this);
#ifdef LCD_CMD_PORT         
        LCD_CMD_PORT = cmdPort;                     // restore command port
#else
        fastWriteHigh(D_I);                         // D/I = 1
        fastWriteLow(R_W);                          // R/W = 0
        ks0108_SelectChip(this, chip);
#endif
        lcdDataDir(0xFF);                           // data port is output
        
        displayData |= data << yOffset;
        if(this->Inverted)
            displayData = ~displayData;
        lcdDataOut( displayData);                   // write data
        ks0108_Enable(this);                        // enable
        
        // second page
        ks0108_GotoXY(this, this->Coord.x, this->Coord.y+8);
        
        displayData = ks0108_ReadData(this);

#ifdef LCD_CMD_PORT         
        LCD_CMD_PORT = cmdPort;                     // restore command port
#else       
        fastWriteHigh(D_I);                         // D/I = 1
        fastWriteLow(R_W);                          // R/W = 0  
        ks0108_SelectChip(this, chip);
#endif
        lcdDataDir(0xFF);                           // data port is output
        
        displayData |= data >> (8-yOffset);
        if(this->Inverted)
            displayData = ~displayData;
        lcdDataOut(displayData);                    // write data
        ks0108_Enable(this);                        // enable
        
        ks0108_GotoXY(this, this->Coord.x+1, this->Coord.y-8);
    }
    else // the whole write is on one page
    {
        if(this->Inverted)
            data = ~data;     
        EN_DELAY();
        lcdDataOut(data);                           // write data
        ks0108_Enable(this);                        // enable
        this->Coord.x++;
    }
}

