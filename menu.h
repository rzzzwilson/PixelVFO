#ifndef MENU_H
#define MENU_H

////////////////////////////////////////////////////////////////////////////////
// A menu system for PixelVFO.
//
// The idea is to define a menu definition with an associated hotspot
// definition.  The code draws the menu and items on the screen and uses the
// hotspot definition to handle screen touches and operation of the menu.
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "hotspot.h"


// handler for selection of an item
typedef bool (*ItemAction)(void);

// structure defining a menu item
struct MenuItem
{
  const char *title;          // menu item display text
  struct Menu *menu;          // if not NULL, submenu to pass to show_menu()
  ItemAction action;          // if not NULL, address of action function
};

// A structure defining a menu
struct Menu
{
  const char *title;          // title displayed on menu page
  int top;                    // number of items scrolled off the top
  int num_items;              // number of items in the array below
  struct MenuItem **items;    // array of pointers to MenuItem data
};


// menu functions
void menu_dump(const char *msg, struct Menu *menu);
void menu_show(struct Menu *menu);
const char *mi_display(struct MenuItem *mi);
void menuBackButton(void);

#endif
