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
#include "eeprom.h"
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
  DEBUG("confirm dialog returned '%s'\n", (result) ? "true" : "false");
  return true;   // don't redraw screen
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

//***********************************************
// Slots
//***********************************************

bool act_save_slot(int slot_num)
{
  DEBUG("act_save_slot: called, slot_num=%d, returning 'true'\n", slot_num);
  slot_put(slot_num, frequency, freq_digit_select);
  return true;
}

bool act_save_slot0(void)
{
  return act_save_slot(0);
}

bool act_save_slot1(void)
{
  return act_save_slot(1);
}

bool act_save_slot2(void)
{
  return act_save_slot(2);
}

bool act_save_slot3(void)
{
  return act_save_slot(3);
}

bool act_save_slot4(void)
{
  return act_save_slot(4);
}

bool act_save_slot5(void)
{
  return act_save_slot(5);
}

bool act_save_slot6(void)
{
  return act_save_slot(6);
}

bool act_save_slot7(void)
{
  return act_save_slot(7);
}

bool act_save_slot8(void)
{
  return act_save_slot(8);
}

bool act_save_slot9(void)
{
  return act_save_slot(9);
}

// slots menu - menu and menuitem titles and menuitem action value filled in dynamically
struct MenuItem act_slot0 = {NULL, NULL, act_save_slot0};
struct MenuItem act_slot1 = {NULL, NULL, act_save_slot1};
struct MenuItem act_slot2 = {NULL, NULL, act_save_slot2};
struct MenuItem act_slot3 = {NULL, NULL, act_save_slot3};
struct MenuItem act_slot4 = {NULL, NULL, act_save_slot4};
struct MenuItem act_slot5 = {NULL, NULL, act_save_slot5};
struct MenuItem act_slot6 = {NULL, NULL, act_save_slot6};
struct MenuItem act_slot7 = {NULL, NULL, act_save_slot7};
struct MenuItem act_slot8 = {NULL, NULL, act_save_slot8};
struct MenuItem act_slot9 = {NULL, NULL, act_save_slot9};

struct MenuItem *mia_f_slots[] = {
                                  &act_slot0, &act_slot1, &act_slot2, &act_slot3, &act_slot4,
                                  &act_slot5, &act_slot6, &act_slot7, &act_slot8, &act_slot9,
                                 };

struct Menu menu_slots = {"Slots", 0, ALEN(mia_f_slots), mia_f_slots};

//-----------------------------------------------
// Populate the slots menu above with data about the saved slots
//-----------------------------------------------

#define SlotBufferLength    (13 + 1)    // +1 for NULL byte end-of-string

void slots_populate(void)
{
  DEBUG("slots_populate: called\n");
  
  Frequency frequency;
  SelOffset offset;
  int address = SaveFreqBase;

  for (unsigned int i = 0; i < ALEN(mia_f_slots); ++i)
  {
    MenuItem *mi_ptr = mia_f_slots[i];
    slot_get(address, frequency, offset);

    // if no slot display buffer, create it
    if (mi_ptr->title == NULL)
    {
      mi_ptr->title = (const char *) malloc(SlotBufferLength);
    }

    // create slot menuitem title text
    if (frequency > 0)
    {
      sprintf((char *) mi_ptr->title, "%d: %8ldHz", i, frequency);
    }
    else
    {
      sprintf((char *) mi_ptr->title, "%d:           ", i);
    }

    // move to next slot address
    ++address;
  }
}

//-----------------------------------------------
// Slots - save frequency to a slot.
//-----------------------------------------------

bool action_slot_save(void)
{
  DEBUG("action_slot_save: called\n");

  // populate the slot menuitems with current saved slot data
  slots_populate();
  menu_dump("Populated menu", &menu_slots);

  // show the menu
  menu_show(&menu_slots);
  
  DEBUG("action_slot_save: returning 'false'\n");
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

