////////////////////////////////////////////////////////////////////////////////
// Utility routines for PixelVFO.
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

#include "PixelVFO.h"
#include "utils.h"


#define UTIL_BUTTON_RADIUS         5

//----------------------------------------
// Draw a single button ALERT dialog box.
//     msg  address of message string
// Used by draw_confirm().
//----------------------------------------

static void draw_alert(const char *msg)
{
}

//----------------------------------------
// Draw a single button ALERT dialog box, wait for click.
//     msg  address of message string
//----------------------------------------

void util_alert(const char *msg)
{
  DEBUG("alert: called, msg='%s'\n", msg);

#if 0
  draw_alert(msg);  // draw the alert dialog

  while (true)      // wait until the button is pressed
  {
    int x;          // pen touch coordinates
    int y;

    if (pen_touch(&x, &y))
    {
      if (hs_handletouch(x, y, hs_alert_ok, ALEN(hs_alert_ok)))
      {
        DEBUG("alert: returning, OK selected\n");
        return;
      }
    }
  }
#endif
}

//----------------------------------------
// Draw a two button CONFIRM dialog box.
//     msg  address of message string
//----------------------------------------

static void draw_confirm(const char *msg)
{
  // draw MOST of the dialog
  draw_alert(msg);

  // add the CANCEL button

}

//----------------------------------------
// Draw a two button CONFIRM dialog box, wait for click.
//     msg  address of message string
// Returns 'true' if 'OK' button selected, else 'false'.
//----------------------------------------

bool util_confirm(const char *msg)
{
  DEBUG("confirm: called, msg='%s'\n", msg);
  DEBUG("confirm: returning 'true', OK selected\n");
  return true

#if 0
  draw_confirm();   // draw the confirm dialog

  while (true)      // wait until one of two buttons pressed
  {
    int x;          // pen touch coordinates
    int y;

    if (pen_touch(&x, &y))
    switch (event->event)
    {
      if (hs_handletouch(x, y, hs_confirm_ok, ALEN(confirm_ok)))
      {
        DEBUG("confirm: returning 'true', OK selected\n");
        return true;
      }

      if (hs_handletouch(x, y, hs_confirm_cancel, ALEN(confirm_cancel)))
      {
        DEBUG("confirm: returning 'false', CANCEL selected\n");
        return false;
      }
    }
  }
#endif
}

//----------------------------------------
// Draw a generic button.
//     title     title text on button
//     x, y      coordinates of top-left button corner
//     w, h      width and height of button
//     bg1, bg2  background colours
//----------------------------------------

void util_button(const char *title, int x, int y, int w, int h,
                 uint16_t bg1, uint16_t bg2)
{
  tft.fillRoundRect(x, y, w, h, UTIL_BUTTON_RADIUS, bg1);
  tft.fillRoundRect(x+1, y+1, w-2, h-2, UTIL_BUTTON_RADIUS, bg2);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(bg1);
  tft.setCursor(x + 8, y + 25);
  tft.print(title);
}

//----------------------------------------
// Draw a button with default colours.
//     title  title text on button
//     x, y   coordinates of top-left button corner
//     w, h   width and height of button
//----------------------------------------

void util_button_std(const char *title, int x, int y, int w, int h)
{
  tft.fillRoundRect(x, y, w, h, UTIL_BUTTON_RADIUS, MENUBACK_BG);
  tft.fillRoundRect(x+1, y+1, w-2, h-2, UTIL_BUTTON_RADIUS, MENUBACK_BG2);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(MENUBACK_FG);
  tft.setCursor(x + 8, y + 25);
  tft.print(title);
}

