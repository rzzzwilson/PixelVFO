////////////////////////////////////////////////////////////////////////////////
// A menu system for PixelVFO.
//
// The idea is to define a menu definition with an associated hotspot
// definition.  The code draws the menu and items on the screen, and uses the
// hotspot definition to handle screen touches and operation of the menu.
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "PixelVFO.h"
#include "menu.h"
#include "events.h"
#include "hotspot.h"

extern int ts_width;

// constants for the menu system
#define MENU_FG             ILI9341_BLACK
#define MENU_BG             ILI9341_GREEN
#define MENUITEM_HEIGHT     35
#define MAXMENUITEMROWS     5


#define MENUBACK_WIDTH      80
#define MENUBACK_HEIGHT     35
#define MENUBACK_FG         ILI9341_BLACK
#define MENUBACK_BG         ILI9341_BLACK
#define MENUBACK_BG2        ILI9341_GREEN
#define MENUBACK_X          (ts_width - MENUBACK_WIDTH - 1)
#define MENUBACK_Y          ((DEPTH_FREQ_DISPLAY - MENUBACK_HEIGHT)/2)


//----------------------------------------
// Format one HotSpotMenu struct into a display string.
//     mi  address of the Menuitem to dump
//
// Debug function.
//----------------------------------------

//struct MenuItem
//{
//  const char *title;          // menu item display text
//  struct Menu *menu;          // if not NULL, submenu to pass to show_menu()
//  ItemAction action;          // if not NULL, address of action function
//};

static const char *mi_display(struct MenuItem *mi)
{
  static char buffer[128];

  sprintf(buffer, "mi: title=%s, menu=%p, action=%p", mi->title, mi->menu, mi->action);
  
  return buffer;
}

//----------------------------------------
// Dump a Menu struct to the console.
//     msg   message to label dump with
//     menu  address of the Menu struct to dump
//
// Debug function.
//----------------------------------------

void menu_dump(char const *msg, Menu *menu)
{
  Serial.printf(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"));
  Serial.printf(F("Menu: %s\n"), msg);
  Serial.printf(F("  title=%s\n"), menu->title);
  //struct Menuitem *mi_ptr = menu->items;
  struct MenuItem *mi_ptr = *menu->items;

  for (int i = 0; i < menu->num_items; ++i, ++mi_ptr)
  {
    Serial.printf(F("    mi %d: %s\n"), i, mi_display(mi_ptr));
  }
  Serial.printf(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"));
}


//----------------------------------------
// Draw the Menu "Back" button.
//----------------------------------------
  
static void menuBackButton(void)
{ 
  tft.fillRoundRect(MENUBACK_X, MENUBACK_Y, MENUBACK_WIDTH, MENUBACK_HEIGHT, BUTTON_RADIUS, MENUBACK_BG);
  tft.fillRoundRect(MENUBACK_X+1, MENUBACK_Y+1, MENUBACK_WIDTH-2, MENUBACK_HEIGHT-2, BUTTON_RADIUS, MENUBACK_BG2);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(MENUBACK_FG);
  tft.setCursor(MENUBACK_X + 8, MENUBACK_Y + 25);
  tft.print("Back");
}
  
//----------------------------------------
// Draw a menu on the screen.
//     menu  pointer to a Menu structure
//----------------------------------------
  
static void menu_draw(struct Menu *menu)
{
  // clear screen and write menu title on upper row
  tft.fillScreen(SCREEN_BG);
  tft.setTextWrap(false);
    
  // start drawing things that don't change
  tft.fillRect(0, 0, tft.width(), DEPTH_FREQ_DISPLAY, FREQ_BG);
  tft.setCursor(0, FREQ_OFFSET_Y);
  tft.setTextColor(MENU_FG);
  tft.setFont(FONT_MENU);
  tft.print(menu->title);
  menuBackButton();
    
  // draw menuitems 
  tft.setFont(FONT_MENUITEM);
  for (int i = 0; i < menu->num_items; ++ i)
  {
    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;
    int menuitem_y = DEPTH_FREQ_DISPLAY + i*MENUITEM_HEIGHT + MENUITEM_HEIGHT;

    tft.getTextBounds((char *) menu->items[i]->title, 0, 0, &x1, &y1, &w, &h);

    // write indexed item on lower row, right-justified
    tft.fillRect(0, menuitem_y - MENUITEM_HEIGHT, ts_width, MENUITEM_HEIGHT, MENU_BG);
    tft.setCursor(ts_width - w, menuitem_y);
    tft.print(menu->items[i]->title);
  }
}

//----------------------------------------
// Handler if user clicks on "Back" button.
//     hs  address of HotSpot item clicked on (the "Back" button)
//
// Just returns 'true' - a signal that the menu is finished.
//----------------------------------------

bool hs_menuback_handler(HotSpot *hs)
{
  return true;
}

//----------------------------------------
// Handler if user clicks on a MenuItem hotspot.
//     hs  address of HotSpot MenuItem clicked on
//
// Just returns 'false' - the handler is finished, continue with menu.
//----------------------------------------

bool hs_menuitem_handler(HotSpot *hs)
{
  Serial.printf(F("hs_menuitem_handler: entered, hs=\n%s\n"), hs_display(hs));

  return false;
}

// Define the Hotspots the menu uses
static HotSpot hs_menu[] =
{
  {0, 0,                                    ts_width, DEPTH_FREQ_DISPLAY, hs_menuback_handler, 0},
  {0, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*0, ts_width, MENUITEM_HEIGHT,    hs_menuitem_handler, 0},
  {0, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*1, ts_width, MENUITEM_HEIGHT,    hs_menuitem_handler, 1},
  {0, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*2, ts_width, MENUITEM_HEIGHT,    hs_menuitem_handler, 2},
  {0, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*3, ts_width, MENUITEM_HEIGHT,    hs_menuitem_handler, 3},
  {0, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*4, ts_width, MENUITEM_HEIGHT,    hs_menuitem_handler, 4},
};

#define MenuHSLen   (sizeof(hs_menu)/sizeof(hs_menu[0]))

//----------------------------------------
// Handle a menu.
//     menu    pointer to a defining Menu structure
//
// Handle events in the loop here.
//----------------------------------------

bool menu_show(struct Menu *menu)
{  
  Serial.printf(F("menu_show: called\n"));
    
  // draw the menu page
  menu_draw(menu);
          
  // get rid of any stray events to this point
  event_flush();
  menu_dump("menu_show: menu", menu);

  while (true)
  { 
    // get next event and handle it
    VFOEvent *event = event_pop();

    switch (event->event)
    { 
      case event_Down:
        Serial.printf("menu_show: loop: Event %s\n", event2display(event));
        hs_dump("menu_show loop: hotspots", hs_menu, MenuHSLen);
        if (hs_handletouch(event->x, event->y, hs_menu, MenuHSLen))
        {
          Serial.printf(F("menu_show loop: hs_handletouch() returned 'true', refresh display\n"));
          event_flush();
          return true;
        }
        break;
      case event_None:
        break;
      default:
        abort("Unrecognized event!?");
    }
  }
}
