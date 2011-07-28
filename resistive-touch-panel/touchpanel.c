#include "touchscreen.h"

volatile unsigned long xx, yy;
volatile char lopass = 1;
volatile long firstx, firsty;
volatile int wasvalid = 0;

// for vertical reading, set bot=0, top=5, read right
void setup_vert()
{   
    P6DIR = 0x45;
    TPORT = 0x4;
    P6SEL = 0x8;
    //SETBIT(P6DIR, TTOP); SETBIT(P6DIR, TBOTTOM);
    //SETBIT(TPORT, TTOP); CLRBIT(TPORT, TBOTTOM);
    //SETBIT(P6SEL, TRIGHT);
}

// for horizontal reading, set left=0, right=5, read top
void setup_horiz()
{
    P6DIR = 0x4A;
    TPORT = 0x8;
    P6SEL = 0x4;
    //SETBIT(P6DIR, TLEFT); SETBIT(P6DIR, TRIGHT);
    //SETBIT(TPORT, TRIGHT); CLRBIT(TPORT, TLEFT);
    //SETBIT(P6SEL, TTOP);
}

#pragma vector=TIMERB0_VECTOR
__interrupt void TB0_ISR(void)
{
    // Start another conversion
    ADC12CTL0 |= ADC12SC;
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
    int i;
    
    // disable interrupts so that this function runs in full
    _bic_SR_register(GIE);

    
    // grab the x or y coordinate, then switch the ADC to read the other one
    // during the next interrupt
    if ((ADC12CTL1 & 0xF000) == 0) // we have a value for TTOP
    {
        if (ADC12MEM0 > 500 && ADC12MEM0 < 2900 && ADC12MEM1 > 700 && ADC12MEM1 < 3500) // range checking
        {
            if (wasvalid < IGNORE) // if we are still in the startup transient
            {
                ++wasvalid;
            }
            else // we are out of the startup transient
            {
                xx = lopass ? (xx*3 + ADC12MEM0)/4 : ADC12MEM0; // simple averaging low-pass filter (if enabled)
                if (wasvalid == IGNORE || wasvalid == IGNORE+1)
                { // if we are just coming out of the startup transient, we want to remember
                    // the first valid set of coordinates.
                    // either the first X coord will be remembered first (in which case wasvalid==IGNORE
                    // and then it is incremented here to IGNORE+1) and then the Y coord will be remembered
                    // next time the ISR is called, or vice versa. therefore, when wasvalid >IGNORE+1, firstx
                    // and firsty are trustworthy.
                    firstx = xx;
                    ++wasvalid; // so that the coords don't get remembered again
                }
            }
        }
        else // if out of range, reset the startup transient counter
        {
            wasvalid = 0;
        }
        
        // prepare to read the vertical axis
        setup_vert();
        ADC12CTL0 &= ~ENC;
        ADC12CTL1 &= 0x0fff;
        ADC12CTL1 |= CSTARTADD_1;
        ADC12CTL0 |= ENC;
    }
    else // we have a value for TRIGHT
    {
        if (ADC12MEM1 > 700 && ADC12MEM1 < 3500 && ADC12MEM0 > 500 && ADC12MEM0 < 2900) // range checking
        {
            if (wasvalid < IGNORE) // if we are still in the startup transient
            {
                ++wasvalid;
            }
            else // we are out of the startup transient
            {
                yy = lopass ? (yy*3 + ADC12MEM1)/4 : ADC12MEM1; // simple averaging low-pass filter (if enabled)
                if (wasvalid == IGNORE || wasvalid == IGNORE+1) // see explanation of this at xx above
                {
                    firsty = yy;
                    ++wasvalid; // so that the coords don't get remembered again
                }
            }
        }
        else // if out of range, reset the startup transient counter
        {
            wasvalid = 0;
        }
        
        // prepare to read the vertical axis
        setup_horiz();
        ADC12CTL0 &= ~ENC;
        ADC12CTL1 &= 0x0fff;
        ADC12CTL1 |= CSTARTADD_0;
        ADC12CTL0 |= ENC;
    }
    
    // turn back on interrupts
    __bis_SR_register(GIE);
    
    __bic_SR_register_on_exit(LPM0_bits); // wake up so we can start the next conversion
}



