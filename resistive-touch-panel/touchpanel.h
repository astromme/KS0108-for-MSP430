#ifndef TOUCHPANEL_H_
#define TOUCHPANEL_H_

#include "msp.h"

extern volatile unsigned long xx, yy; // the current coordinates being read

// when the panel is not being touched, we read a value with an invalid Y
// coordinate, so it is easy to ignore that. However, as the finger/stylus
// comes down, there are some invalid points in between this invalid coordinate
// and the real data. In draw mode with the pencil, you get a line from the edge
// of the screen to the touch position. In scroll mode, it might scroll the
// wrong way. To fix this, when the data is invalid wasvalid is set to zero,
// and when the data is valid wasvalid is increased BUT the data is ignored until
// wasvalid reaches IGNORE/IGNORE+1.
extern volatile int wasvalid;
extern volatile long firstx, firsty; // these remember the coordinates from when
                    // wasvalid hit IGNORE/IGNORE+1 (this is needed for scrolling)
#define IGNORE 30 // number of data points to ignore

extern volatile char lopass; // whether the low-pass filter is enabled

// touch panel pins
#define TPORT       6 // has to be port 6 because that's where the ADC is
#define TRIGHT      3
#define TLEFT       1
#define TBOTTOM     0
#define TTOP        2

void setup_vert(); // prepare to read vertical position
void setup_horiz(); // prepare to read horizontal position

#endif /*TOUCHPANEL_H_*/
