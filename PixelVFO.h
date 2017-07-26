#ifndef PIXELVFO_H
#define PIXELVFO_H

////////////////////////////////////////////////////////////////////////////////
// Functions exported from PixelVFO.ino
////////////////////////////////////////////////////////////////////////////////
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#define DEPTH_FREQ_DISPLAY    50    // depth of frequency display bar
#define BUTTON_RADIUS         5
#define FONT_BUTTON           (&FreeSansBold12pt7b) // font for button labels
#define FONT_MENU             (&FreeSansBold18pt7b) // font for menuitems
#define FONT_MENUITEM         (&FreeSansBold12pt7b) // font for menuitems

extern Adafruit_ILI9341 tft;

// the abort() function exported from the top-level code
void abort(const char *msg);

#endif
