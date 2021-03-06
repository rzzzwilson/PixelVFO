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
typedef bool (*ItemAction)(void *arg);

// structure defining a menuitem
struct MenuItem
{
  const char *title;          // menu item display text
  struct Menu *menu;          // if not NULL, submenu to pass to show_menu()
  ItemAction action;          // if not NULL, address of action function
  void *arg;                  // arg for the action function
};

// structure defining a menu
struct Menu
{
  const char *title;          // title displayed on menu screen
  int top;                    // index of top displayed item
  int num_items;              // number of items in the array below
  struct MenuItem **items;    // array of pointers to MenuItem data
  bool indexed;               // 'true' if menu is indexed
};

#define MENU_FG             ILI9341_BLACK
#define MENU_BG             ILI9341_GREEN
#define MENUITEM_HEIGHT     38
#define MAXMENUITEMROWS     5
#define INDEX_COLUMN        30




// menu functions
void menu_dump(const char *msg, struct Menu *menu);
const char *mi_display(struct MenuItem *mi);
void menuBackButton(void);

//**************************************
// Draw a menu on the screen.
//     menu  address of the Menu structure to draw
// Returns 'true' if the menu is finished or
// 'false' if the menu is to be redrawn.
//**************************************

void menu_show(struct Menu *menu);

#endif
