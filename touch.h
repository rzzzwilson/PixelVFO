#ifndef TOUCH_H
#define TOUCH_H

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "events.h"

//----------------------------------------
// Function prototypes.
//----------------------------------------

void touch_setup(int t_cs, int t_irq);

#endif
