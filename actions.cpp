////////////////////////////////////////////////////////////////////////////////
// The handlers for menu actions.
//
// These handler will all return 'true' if screen must be redrawn.
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>

#include "PixelVFO.h"
#include "hotspot.h"
#include "menu.h"
#include "utils.h"

//-----------------------------------------------
// Reset - no action.
//-----------------------------------------------

bool action_no_reset(void)
{
  DEBUG("action_no_reset: called\n");
  util_alert("Test of alert.");
  return true;    // redraw screen
}

//-----------------------------------------------
// Reset - perform action.
//-----------------------------------------------

bool action_reset(void)
{
  DEBUG("action_reset: called\n");
  bool result = util_confirm("Test of confirm.");
  DEBUG("confirm dialog returnd '%s'\n", (result) ? "true" : "false");
  return false;   // don't redraw screen
}

//-----------------------------------------------
// Settings - adjust brightness.
//-----------------------------------------------

bool action_brightness(void)
{
  DEBUG("action_brightness: called\n");
  return false;   // don't redraw screen
}

//-----------------------------------------------
// Settings - calibrate the DDS-60 chip.
//-----------------------------------------------

bool action_calibrate(void)
{
  DEBUG("action_calibrate: called\n");
  return false;   // don't redraw screen
}

//-----------------------------------------------
// Slots - save frequency to a slot.
//-----------------------------------------------

bool act_menuslot_handler(HotSpot *hs, void *arg)
{
  int offset = (int) arg;
  
  MenuItem *mi_ptr = NULL;
  bool result = false;
  
  DEBUG(">>>>> act_menuslot_handler: entered, hs=\n%s\nmi=\n%s\n",
        hs_display(hs), mi_display(mi_ptr));
  if (mi_ptr->menu)
  {
    menu_show(mi_ptr->menu);
    result = true;
  }
  else
  {
    mi_ptr->action();
    result = false;
  }
  DEBUG("<<<<< act_menuslot_handler: returning '%s'\n",
        (result) ? "true" : "false");
  return result;
}

bool act_scroll_up(HotSpot *hs, void *arg)
{
  return true;
}

bool act_scroll_down(HotSpot *hs, void *arg)
{
  return true;
}

static HotSpot hs_slots[] =
{
  // menuitem hotspots
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*0, ts_width, MENUITEM_HEIGHT, act_menuslot_handler, 0},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*1, ts_width, MENUITEM_HEIGHT, act_menuslot_handler, 1},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*2, ts_width, MENUITEM_HEIGHT, act_menuslot_handler, 2},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*3, ts_width, MENUITEM_HEIGHT, act_menuslot_handler, 3},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*4, ts_width, MENUITEM_HEIGHT, act_menuslot_handler, 4},
  // the 'scroll' hotspots
  {0, DEPTH_FREQ_DISPLAY, 50, 50, act_scroll_up, 1},
  {0, ts_height-50, 50, 50, act_scroll_down, 1},
};

bool action_slot_save(void)
{
  DEBUG("action_slot_save: called\n");

  
  return false;   // don't redraw screen
}

//-----------------------------------------------
// Slots - restore frequency from a slot.
//-----------------------------------------------

bool action_slot_restore(void)
{
  DEBUG("action_slot_restore: called\n");
  return false;   // don't redraw screen
}

//-----------------------------------------------
// Slots - delete contents in a slot.
//-----------------------------------------------

bool action_slot_delete(void)
{
  DEBUG("action_slot_delete: called\n");
  return false;   // don't redraw screen
}

