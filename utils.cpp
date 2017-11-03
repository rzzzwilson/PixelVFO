////////////////////////////////////////////////////////////////////////////////
// Utility routines for PixelVFO.
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

#include "PixelVFO.h"
#include "hotspot.h"
#include "utils.h"


#define BUTTON_RADIUS   5
#define CORNER_RADIUS   7

// size/position of ALERT dialog
#define ALERT_X         30
#define ALERT_Y         20
#define ALERT_W         260
#define ALERT_H         200

// sizes of dialog buttons
#define OK_WIDTH        90
#define OK_HEIGHT       35
#define CANCEL_WIDTH    90
#define CANCEL_HEIGHT   35

#define DLG_BG          ILI9341_RED
#define DLG_BG2         ILI9341_WHITE
#define DLG_FG          ILI9341_BLACK

#define BTN_BG          ILI9341_BLACK
#define BTN_BG2         ILI9341_GREEN
#define BTN_FG          ILI9341_BLACK


//----------------------------------------
// Draw a generic button.
//     title     title text on button
//     x, y      coordinates of top-left button corner
//     w, h      width and height of button
//     bg1, bg2  background colours
//----------------------------------------

void util_button(const char *title, int x, int y, int w, int h,
                 uint16_t bg1, uint16_t bg2, uint16_t fg)
{
  int16_t x1;   // used to get extent of title text
  int16_t y1;
  uint16_t w1;
  uint16_t h1;

  // draw button background
  tft.fillRoundRect(x, y, w, h, BUTTON_RADIUS, bg1);
  tft.fillRoundRect(x+1, y+1, w-2, h-2, BUTTON_RADIUS, bg2);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(fg);
  
  // figure out where to draw centred title
  tft.getTextBounds((char *) title, 1, 1, &x1, &y1, &w1, &h1);
  tft.setCursor(x + (w - w1)/2, y+25);
  tft.print(title);
}

//----------------------------------------
// Handle clicks on the OK and CANCEL dialog buttons
//----------------------------------------

static bool dlg_handler(HotSpot *hs_ptr, void *arg)
{
  int offset = (int) arg;
  
  DEBUG("dlg_handler: called, arg=%d\n", offset);
  return true;    // redraw screen
}

//----------------------------------------
// Define hotspots for the ALERT and CONFIRM dialogs
//----------------------------------------

static HotSpot hs_dlg_alert[] =
{
  {ALERT_X + ALERT_W - OK_WIDTH - 4, ALERT_Y + ALERT_H - OK_HEIGHT - 4,
   OK_WIDTH, OK_HEIGHT, dlg_handler, 0}
};

static HotSpot hs_dlg_confirm[] =
{
  {ALERT_X + ALERT_W - OK_WIDTH - 4, ALERT_Y + ALERT_H - OK_HEIGHT - 4,
   OK_WIDTH, OK_HEIGHT, dlg_handler, 1},
  {ALERT_X + 4, ALERT_Y + ALERT_H - CANCEL_HEIGHT - 4,
   CANCEL_WIDTH, CANCEL_HEIGHT, dlg_handler, 2}
};

//----------------------------------------
// Draw a single button ALERT dialog box.
//     msg  address of message string
// Used by draw_confirm().
//----------------------------------------

static void draw_alert(const char *msg)
{
  // draw dialog body
  tft.fillRoundRect(ALERT_X, ALERT_Y, ALERT_W, ALERT_H, CORNER_RADIUS, DLG_BG);
  tft.fillRoundRect(ALERT_X+2, ALERT_Y+2, ALERT_W-4, ALERT_H-4, CORNER_RADIUS, DLG_BG2);

  // draw text (centred)
  tft.setCursor(ALERT_X + 7, ALERT_Y + 25);
  tft.setFont(FONT_DIALOG);
  tft.setTextColor(DLG_FG);
  tft.print(msg);

  // draw the "OK" button
  util_button("Ok",
              ALERT_X + ALERT_W - OK_WIDTH - 4, ALERT_Y + ALERT_H - OK_HEIGHT - 4,
              OK_WIDTH, OK_HEIGHT, BTN_BG, BTN_BG2, BTN_FG);
}

//----------------------------------------
// Draw a single button ALERT dialog box, wait for click.
//     msg  address of message string
//----------------------------------------

void util_alert(const char *msg)
{
  DEBUG("alert: called, msg='%s'\n", msg);

  draw_alert(msg);  // draw the alert dialog

  while (true)      // wait until the OK button is pressed
  {
    int x;          // pen touch coordinates
    int y;

    if (pen_touch(&x, &y))
    {
      if (hs_handletouch(x, y, hs_dlg_alert, ALEN(hs_dlg_alert)))
      {
        DEBUG("alert: returning, OK selected\n");
        return;
      }
    }
  }
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
  util_button("Cancel",
              ALERT_X + 4, ALERT_Y + ALERT_H - CANCEL_HEIGHT - 4,
              CANCEL_WIDTH, CANCEL_HEIGHT, BTN_BG, BTN_BG2, BTN_FG);
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

  draw_confirm(msg);  // draw the confirm dialog

  while (true)        // wait until one of two buttons pressed
  {
    int x;            // pen touch coordinates
    int y;

    if (pen_touch(&x, &y))
    {
      if (hs_handletouch(x, y, hs_dlg_confirm, ALEN(hs_dlg_confirm)))
      {
        DEBUG("alert: returning, OK selected\n");
        return true;
      }
#if 0
      if (hs_handletouch(x, y, hs_dlg_cancel, ALEN(hs_dlg_cancel)))
      {
        DEBUG("alert: returning, CANCEL selected\n");
        return false;
      }
#endif
    }
  }
}


