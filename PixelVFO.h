#ifndef PIXELVFO_H
#define PIXELVFO_H

////////////////////////////////////////////////////////////////////////////////
// Functions exported from PixelVFO.ino
////////////////////////////////////////////////////////////////////////////////
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// macros to enable tailoring of debug calls
#define DEBUG     debug_ignore
#define DEBUG2    debug

// macro to get number of elements in an array
#define ALEN(a)    (sizeof(a)/sizeof((a)[0]))

// constants for main screen layout
#define NUM_F_CHAR            8     // number digits in frequency display
#define CHAR_WIDTH            27    // width of each frequency digit
#define TOP_BAR_Y             40    // offset from top of frequency digits
#define FREQ_OFFSET_X         40    // offset from left of frequency digits
#define MHZ_OFFSET_X          257   // offset from left of the 'Hz' units

#define TITLE_OFFSET_X        5     // offset from left of menu title string
#define TITLE_OFFSET_Y        37    // offset from top of menu title string

#define SCREEN_BG           ILI9341_BLACK
#define SCREEN_BG3          ILI9341_LIGHTGREY
#define SCREEN_BG2          ILI9341_DARKGREY
#define SCREEN_BG1          ILI9341_BLACK
#define FREQ_FG             ILI9341_BLUE
#define FREQ_BG             ILI9341_WHITE
#define FREQ_SEL_BG         ILI9341_GREEN
#define BOTTOM_BG           ILI9341_WHITE


#define DEPTH_FREQ_DISPLAY    50    // depth of frequency display bar
#define BUTTON_RADIUS         5
#define FONT_BUTTON           (&FreeSansBold12pt7b) // font for button labels
#define FONT_MENU             (&FreeSansBold18pt7b) // font for menuitems
#define FONT_MENUITEM         (&FreeSansBold12pt7b) // font for menuitems

extern Adafruit_ILI9341 tft;

// the abort() function exported from the top-level code
void abort(const char *msg);

// the debug routines - writes to Serial output
void debug(const char *format, ...);
void debug_ignore(const char *format, ...);

#endif
