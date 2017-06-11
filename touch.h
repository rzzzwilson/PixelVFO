#ifndef TOUCH_H
#define TOUCH_H

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "events.h"

//----------------------------------------
// Function prototypes for touch.cpp.
//----------------------------------------

void touch_setup(int t_cs, int t_irq, int w, int h);
void touch_setRotation(int rot);

#endif
