//******************************************************************************
//  MSP430FG461X LCD/Touchscreen finger painting: MSPaint
//
//  Description; Touchscreen X/Y data paints dots on the LCD screen.
//        S1 toggles pencil/eraser. S2 toggles draw/scroll mode.
//  ACLK = n/a, MCLK = 8Mhz
//
//                MSP430FG461X
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |            E91 I/O board (note pins 7 of J7 and J8 are
//            |                 |                           tied together so we can ground
//            |                 |                           that pin without using
//            |                 |           -------------   two headers)
//            |                 |          |             |
//            |               P6|-/-H8-J8->|             |
//            |                 |          ---------------
//            |                 |
//            |                 |        ks0108b LCD
//       S1-->|P1.0           P3|-/-H7->|cmd        |
//       S2-->|P1.1           P7|-/-H6->|data       |
//            |                 |        -----------
//            |                 |
//             -----------------
//  A. Burka / A. Stromme
//  Texas Instruments, Inc
//  September 2010
//  Built with CCE Version: 4.1.3
//******************************************************************************


#include  <msp430xg46x.h>
#include "touchscreen.h"
#include "msp.h"
#include "ks0108.h"

volatile enum { DRAW, SCROLL } mode = DRAW;
volatile enum { PENCIL, ERASER } drawmode = PENCIL;

void main(void) {
      int x = 0, y = 0, newx, newy, i;
      
      // chain watchdog to a tree
      WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
      
      // MCLK = 8MHz
      FLL_CTL0 |= DCOPLUS + XCAP18PF;           // DCOPLUS set with a frequency of external * D * (N+1)
      SCFI0 |= FN_4;                            // Multiply DCO freq by 2
      SCFQCTL = 121;                            // (121 + 1) * 32768 * 2 = ~8MHz
    
      // setup buttons
      P1DIR = 0;    // ports set to pinput
      P1IE = 0x03;  // interrupts enabled on P1.0 and P1.1
      P1IES = 0x03; // interrupt on falling edge
      P1IFG = 0;    // clear the interrupt flags
      
      // Set up ADC conversions and interrupts
      ADC12CTL0 = ADC12ON + SHT0_0;          // turn on A/D, set sampling time
      ADC12CTL1 = SHP + CONSEQ_1 + ADC12SSEL_0 + ADC12DIV_0;
                                             // use sampling timer, repeat sequence
      ADC12MCTL0 = SREF_0 + INCH_2 + EOS;    // top: Vr+ = Vcc, Vr- = Vss, INCH = TSTOP
      ADC12MCTL1 = SREF_0 + INCH_3 + EOS;    // right: Vr+ = Vcc, Vr- = Vss, INCH = TSRIGHT
      SETBIT(ADC12IE, 0);                    // turn on interrupts for ADC12 (0 and 1)
      SETBIT(ADC12IE, 1);
      
      // pull P6.6 low
      P6DIR = 0x40;     // set that pin to output
      CLRBIT(P6OUT, 6); // pull the pin low
      
      setup_horiz();            // prepare for horizontal measurement
      ADC12CTL1 |= CSTARTADD_0; // the horizontal measurement configuration is on ADC12.0
      
      // Setup Timer B to trigger interrupts which will trigger conversion
      TBCCTL0 = CCIE;
      TBCTL = TBSSEL_2 + MC_1;  // SMCLK, up-mode
      TBCCR0 = 24000;           // slow down to remove inconsistencies
      
      ADC12CTL0 |= ENC;            // enable conversion
      
      ks0108_Init(&GLCD, 0);    // initialize screens
      ks0108_DumpBuffer(&GLCD); // clear the screens and put up status bar
      
      while (1)
      {
          __bis_SR_register(LPM0_bits + GIE);     // go to sleep until ADC12_ISR
          
          __bic_SR_register(GIE);                 // turn off interrupts while we draw stuff
          
        // turn on the chips in case they've turned themselves off
        ks0108_WriteCommand(&GLCD, 0x3F, 0);
        ks0108_WriteCommand(&GLCD, 0x3F, 1);
                  
          if (xx > 500 && xx < 2900 && yy > 700 && yy < 3500 && wasvalid > IGNORE+1) // range check
                    // see touchpanel.h for explanation of the startup transient problem
          {
              newx = (xx - 500)/19;                                    // scale touch panel coordinates to LCD coordinates
              newy = 63 - (yy - 700)/44;

              if (newx != x || newy != y)                              // only draw if the coordinates have changed
              {
                  x = newx;                                            // remember the new coordinates
                  y = newy;
                  if (mode == DRAW && (x > 63 || y > 8))               // status bar is read only
                  {
                      if (drawmode == PENCIL)                          // the pencil is drawing a pixel
                      {
                          ks0108_SetDot(&GLCD, x, y, BLACK);
                      }
                      else                                             // ERASER
                      {
                          for (newx = x - 6; newx < x + 6; ++newx)     // the eraser is a 12x12 square
                          {
                              for (newy = y - 6; newy < y + 6; ++newy) // clear these pixels
                              {
                                  ks0108_SetDot(&GLCD, newx, newy, WHITE);
                              }
                          }
                      }
                  }
                  else // SCROLL
                  {
                    // the way scrolling works is we remember the first valid coordinates
                    // (firstx, firsty) and when the y coordinate changes by more than 8
                    // (in units of LCD pixels) we scroll the display by 8 pixels in the
                    // appropriate direction, then reset firsty to the current position.
                    // This allows scrolling by dragging on the touch panel, much like two-finger
                    // scrolling on a Macbook or the hand tool in Adobe PDF Reader.
                    if (firsty - y > 8)
                    {
                        if (GLCD.startline < 192)               // don't go below the bottom
                        {
                            GLCD.startline += 8;                // scroll
                            ks0108_DumpBuffer(&GLCD);           // refresh display
                        }
                        firsty = y;
                    }
                    else if (y - firsty > 8)
                    {
                        if (GLCD.startline > 0)                 // don't go above the top
                        {
                            GLCD.startline -= 8;                // scroll
                            ks0108_DumpBuffer(&GLCD);           // refresh display
                        }
                        firsty = y;
                    }
                  }
              }
          }
        
        _bis_SR_register(GIE); // turn interrupts back on
        
      }
}

// dump the in-RAM screen buffer out to the display
//    (i.e., the part of it currently displayed, which
//    is determined by GLCD.startline)
// this isn't as slow as you would think
// even though this is declared in ks0108.h, it has to be implemented here
//      because it needs to know about mode and drawmode for the status bar
//      (in the future they really should be decoupled)
void ks0108_DumpBuffer(volatile ks0108 *this)
{
    uint8_t chip, x, y, i;
    for (chip = 0; chip < 2; ++chip) // two LCD chips
    {
        if (chip == 0) // the top page of the left chip is used for a status bar
        {
            ks0108_WriteCommand(this, 0xB8, chip); // chipX = 0
            ks0108_WriteCommand(this, 0x40, chip); // chipY = 0
            
            // pixel 0: on if draw mode
            ks0108_DoWriteCommand(this, (mode == DRAW) ? 0xFF : 0, chip, 1, 0);
            
            // pixel 1: top half if pencil, bottom half if eraser
            ks0108_DoWriteCommand(this, (drawmode == PENCIL) ? 0xF0 : 0x0F, chip, 1, 0);
            
            // pixel 2: on if erase mode
            ks0108_DoWriteCommand(this, (mode == SCROLL) ? 0xFF : 0, chip, 1, 0);
            
            // pixel 3: off
            ks0108_DoWriteCommand(this, 0, chip, 1, 0);
            
            // pixels 4-11: scrollbar
            //      see the code description document
            //      for an explanation of the scrollbar shape
            for (y = 0; y < 8; ++y)
            {
                i = 0;
                if (this->startline/XPAGES > y) i |= 0xC0;
                if (this->startline/XPAGES > 8+y) i |= 0x30;
                if (this->startline/XPAGES > 16+y) i |= 0xC;
                if (this->startline/XPAGES > 24+y) i |= 0x3;
                ks0108_DoWriteCommand(this, i, chip, 1, 0);
            }
        }
        for (x = 1-chip; x < 8; ++x) // start at the second page on the left chip, but the first page on the right chip
        {
            ks0108_WriteCommand(this, x | 0xB8, chip);  // set chipX
            ks0108_WriteCommand(this, 0x40, chip);      // set chipY
            for (y = 0; y < 64; ++y)                    // write out the page
            {
                ks0108_DoWriteCommand(this, this->buffer[x + this->startline/XPAGES][y + CHIP_WIDTH*chip], chip, 1, 0);
                // like the calls for chip==0 above, this call to DoWriteCommand is actually writing
                // a byte of pixel data to the screen (you can see that if you correlate the values
                // of D_I(RS) and R_W with the ks0108 datasheet
            }
        }
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR()                                // handle button events
{
    int chip, y, i, j;
    __bic_SR_register(GIE);                                 // turn off interrupts so we can process this one in peace
    
    if (P1IFG & 0x01)
    {
        drawmode = (drawmode == PENCIL) ? ERASER : PENCIL;  // change drawing tool
        ks0108_DumpBuffer(&GLCD);                           // so that the status bar gets redrawn
        
        CLRBIT(P1IFG, 0);                                   // clear interrupt flag
    }
    else if (P1IFG & 0x02)
    {
        mode = (mode == DRAW) ? SCROLL : DRAW;              // change mode
        lopass = 1 - lopass;                                // no low-pass filter in scroll mode (see touchscreen.{c,h})
        ks0108_DumpBuffer(&GLCD);                           // so that the status bar gets redrawn
        
        delay(20);                                          // try to prevent the chips from turning off
        CLRBIT(P1IFG, 1);                                   // clear interrupt flag
    }
    
    __bis_SR_register(GIE);                                 // turn interrupts back on
}
