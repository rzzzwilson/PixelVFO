////////////////////////////////////////////////////////////////////////////////
// A "hotspot" system for PixelVFO.
//
// The idea is to define rectangular extents on the screen with associated
// handler functions and arguments.  Given a touchscreen touch event with
// associated (x, y) coordinates, call the appropriate handler (if any).
////////////////////////////////////////////////////////////////////////////////

#include "hotspot.h"

//----------------------------------------
// Format one HotSpot struct into a display string.
//     hs  address of the HotSpot to dump
// Debug function.
//----------------------------------------

const char *hs_display(HotSpot *hs)
{
  static char buffer[128];

  sprintf(buffer, "hs: x=%3d, y=%3d, w=%3d, h=%3d, handler=%p, arg=%d",
                  hs->x, hs->y, hs->w, hs->h, hs->handler, hs->arg);
  
  return buffer;
}

//----------------------------------------
// Dump the hotspot array to the console.
//     msg       message to label dump with
//     hs_array  address of the HotSpot array to dump
//     len       length of array
// Debug function.
//----------------------------------------

void hs_dump(char const *msg, HotSpot *hs_array, int len)
{
  Serial.printf(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"));
  Serial.printf(F("HotSpot array: %s\n"), msg);
  for (int i = 0; i < len; ++i)
  {
    HotSpot *hs = &hs_array[i];

    Serial.printf(F("  %d: %s\n"), i, hs_display(hs));
  }
  Serial.printf(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"));
}

//----------------------------------------
// Handle a touch on a hotspot.
//     touch_x  X coord of screen touch
//     touch_y  Y coord of screen touch
//     hs       base address of array of HotSpots
//     hs_len   length of 'hs_array'
// Returns 'true' if a touch was handled - screen needs refresh.
//----------------------------------------

bool hs_handletouch(int touch_x, int touch_y,
                    HotSpot *hs, int hs_len)
{
  for (int i = 0; i < hs_len; ++hs, ++i)
  {
    if ((touch_x < hs->x) || (touch_x > hs->x + hs->w) ||
        (touch_y < hs->y) || (touch_y > hs->y + hs->h))
    {
      continue;
    }

    Serial.printf(F("hs_handletouch: calling hs->handler=%p\n"), hs->handler);
    return (hs->handler)(hs);
  }

  return false;
}
