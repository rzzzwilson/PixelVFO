#ifndef HOTSPOT_H
#define HOTSPOT_H

////////////////////////////////////////////////////////////////////////////////
// A "hotspot" system for PixelVFO.
//
// The idea is to define rectangular extents on the screen with associated
// handler functions and arguments.  Given a touchscreen touch event with
// associated (x, y) coordinates, call the appropriate handler (if any).
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

// forward definition of HotSpot struct
struct HotSpot;

// a hotspot handler function typedef
// a handler returns 'true' if the screen should be redrawn
typedef bool (*HS_Handler)(HotSpot *);

// a hotspot definition
struct HotSpot
{
  int x;                // X coord of the hotspot top-left corner
  int y;                // Y coord of the hotspot top-left corner
  int w;                // hotspot width in pixels
  int h;                // hotspot height in pixels
  HS_Handler handler;   // address of handler function
  int arg;              // first arg to handler
};

// hotspot functions
bool hs_handletouch(int touch_x, int touch_y, HotSpot *hs, int hs_len);
HotSpot * hs_touched(int touch_x, int touch_y, HotSpot *hs, int hs_len);
const char *hs_display(HotSpot *hs);
void hs_dump(char const *msg, HotSpot *hs, int len);

#endif
