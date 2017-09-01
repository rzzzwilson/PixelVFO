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
extern int ts_height;

// constants for the menu system
#define MENU_FG             ILI9341_BLACK
#define MENU_BG             ILI9341_GREEN
#define MENUITEM_HEIGHT     38
#define MAXMENUITEMROWS     5

#define MENU_SCROLL_WIDTH   20
#define SCROLL_FG           ILI9341_WHITE
//#define SCROLL_BG           ILI9341_BLACK
#define SCROLL_BG           ILI9341_RED
#define SCROLL_HEIGHT       20

#define MENUBACK_WIDTH      80
#define MENUBACK_HEIGHT     35
#define MENUBACK_FG         ILI9341_BLACK
#define MENUBACK_BG         ILI9341_BLACK
#define MENUBACK_BG2        ILI9341_GREEN
#define MENUBACK_X          (ts_width - MENUBACK_WIDTH - 1)
#define MENUBACK_Y          ((DEPTH_FREQ_DISPLAY - MENUBACK_HEIGHT)/2)
#define MENU_ITEM_BG        0x0700


//----------------------------------------
// Handler if user clicks on "Back" button.
//     hs  address of HotSpot item clicked on (the "Back" button)
//
// Just returns 'true' - a signal that we should return from the menu.
//----------------------------------------

bool hs_menuback_handler(HotSpot *hs, void *ignore)
{
  return true;
}

//----------------------------------------
// Handler if user clicks on a MenuItem hotspot.
//     hs   address of HotSpot MenuItem clicked on
//     mi   address of MenuItem to action
//----------------------------------------

bool hs_menuitem_handler(HotSpot *hs, void *mi)
{
  MenuItem *mi_ptr = (MenuItem *) mi;
  
  DEBUG3(">>>>> hs_menuitem_handler: entered, hs=\n%s\nmi=\n%s\n",
         hs_display(hs), mi_display(mi_ptr));
  if (mi_ptr->menu)
    menu_show(mi_ptr->menu);
  else
    mi_ptr->action();
  DEBUG3(">>>>> hs_menuitem_handler: returning false\n");
  return false;
}

//----------------------------------------
// Handler if user clicks UP on a scrollbar widget.
//     hs    address of HotSpot item clicked on
//     mptr  address of Menu
//----------------------------------------

bool menu_scroll_up(HotSpot *hs, void *mptr)
{
  Menu *menu = (Menu *) mptr;
  int arg = hs->arg;

  menu_dump("menu_scroll_up: menu", menu); 

  // add 'arg' to menu 'top' value and normalize
  menu->top -= arg;
  if (menu->top < 0)
      menu->top = 0;

  return false;
}

//----------------------------------------
// Handler if user clicks DOWN on a scrollbar widget.
//     hs   address of HotSpot item clicked on
//     mi   address of MenuItem to action
//----------------------------------------

bool menu_scroll_down(HotSpot *hs, void *mptr)
{
  Menu *menu = (Menu *) mptr;
  int arg = hs->arg;

  menu_dump("menu_scroll_down: menu", menu); 

  // add 'arg' to menu 'top' value and normalize
  menu->top += arg;
  if (menu->top > menu->num_items - MAXMENUITEMROWS)
      menu->top = menu->num_items - MAXMENUITEMROWS;

  return false;
}

// Define the Hotspots the menu uses
static HotSpot hs_menu[] =
{
  // menuitem hotspots
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*0, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 0},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*1, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 1},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*2, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 2},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*3, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 3},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*4, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 4},
};

// Define the Hotspots the menu uses
static HotSpot hs_other[] =
{
  // the 'Back' button
  {MENUBACK_X, 0, ts_width-MENUBACK_X, DEPTH_FREQ_DISPLAY, hs_menuback_handler, 0},
  // the 'scroll' hotspots
  {0, DEPTH_FREQ_DISPLAY, 50, 50, menu_scroll_up, 1},
  {0, ts_height-50, 50, 50, menu_scroll_down, 1},
};

//----------------------------------------
// Format one HotSpotMenu struct into a display string.
//     mi  address of the Menuitem to dump
//
// Debug function.
//----------------------------------------

const char *mi_display(struct MenuItem *mi)
{
  static char buffer[128];

  sprintf(buffer, "mi: %p, title=%s, menu=%p, action=%p\n", mi, mi->title, mi->menu, mi->action);
  
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
  DEBUG3("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  DEBUG3("Menu: %s\n", msg);
  DEBUG3("  title=%s, top=%d, num items=%d\n", menu->title, menu->top, menu->num_items);

  for (int i = 0; i < menu->num_items; ++i)
  {
    struct MenuItem *mi_ptr = menu->items[i];
    DEBUG3("    mi %d: %s", i, mi_display(mi_ptr));
  }
  DEBUG3("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}


//----------------------------------------
// Draw the Menu "Back" button.
//----------------------------------------
  
void menuBackButton(void)
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
  DEBUG3(">>>>>>>>>>>>>>>>>>>> menu_draw: entered\n");

  // clear screen and write menu title on upper row
  tft.fillScreen(SCREEN_BG);
  tft.setTextWrap(false);
    
  // start drawing things that don't change
  tft.fillRect(0, 0, ts_width, DEPTH_FREQ_DISPLAY, FREQ_BG);
  tft.setCursor(TITLE_OFFSET_X, TITLE_OFFSET_Y);
  tft.setTextColor(MENU_FG);
  tft.setFont(FONT_MENU);
  tft.print(menu->title);
  menuBackButton();
  DEBUG3("After menuBackButton\n");

  // draw menuitems (at least, those that fit on screen
  tft.setFont(FONT_MENUITEM);
  int mi_y = DEPTH_FREQ_DISPLAY + MENUITEM_HEIGHT;
  for (int i = menu->top; i < menu->top + MAXMENUITEMROWS; ++i)
  {
    if (i > menu->num_items)
    {
      break;
    }
      
    DEBUG3("Top of loop, i=%d, top=%d\n", i, menu->top);

    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;
     
    tft.getTextBounds((char *) menu->items[i]->title, 1, 1, &x1, &y1, &w, &h);

    // write indexed item on lower row, right-justified
    DEBUG3("fillRect(%d, %d, %d, %d, MENU_BG)\n",
           0, mi_y - MENUITEM_HEIGHT, ts_width, MENUITEM_HEIGHT);
    tft.fillRect(0, mi_y - MENUITEM_HEIGHT, ts_width, MENUITEM_HEIGHT, MENU_BG);
    tft.setCursor(ts_width - w - 5, mi_y - 10);
    tft.print(menu->items[i]->title);
    mi_y += MENUITEM_HEIGHT;
    DEBUG3("End of loop, i=%d, top=%d\n", i, menu->top);
  }
  DEBUG3("After draw MenuItems\n");

  // highlight the active menuitems
  for (int i = 0; i < menu->num_items; ++i) // skip the "Back" button
  {
    if (i >= MAXMENUITEMROWS)   // break out if max showable is reached
      break;
      
    Serial.printf(F("Drawing highlight %d of %d\n"), i, menu->num_items);

    HotSpot *hs = &hs_menu[i];
    
    //tft.drawFastHLine(0, hs->y+MENUITEM_HEIGHT, ts_width, MENU_ITEM_BG);
    tft.drawRect(hs->x, hs->y, hs->w, hs->h, MENU_ITEM_BG);
  }
  DEBUG3("After draw highlights\n");

  // draw the scroll widget if required
  if (menu->num_items > MAXMENUITEMROWS)
  {
    tft.fillRect(0, DEPTH_FREQ_DISPLAY,
                 MENU_SCROLL_WIDTH, ts_height - DEPTH_FREQ_DISPLAY, SCROLL_BG);
    tft.fillTriangle(0, DEPTH_FREQ_DISPLAY+SCROLL_HEIGHT,
                     MENU_SCROLL_WIDTH-1, DEPTH_FREQ_DISPLAY+SCROLL_HEIGHT,
                     MENU_SCROLL_WIDTH/2, DEPTH_FREQ_DISPLAY,
                     SCROLL_FG);
    tft.fillTriangle(0, ts_height-1-SCROLL_HEIGHT,
                     MENU_SCROLL_WIDTH-1, ts_height-1-SCROLL_HEIGHT,
                     MENU_SCROLL_WIDTH/2, ts_height-1,
                     SCROLL_FG);
    DEBUG3("After draw scroll\n");
  }

  DEBUG3("<<<<<<<<<<<<<<<<<<<< menu_draw: exit\n");
}

//----------------------------------------
// Handle a touch on a menu hotspot.
//     x        X coord of screen touch
//     y        Y coord of screen touch
//     hs       base address of array of HotSpots
//     hslen    length of 'hs_array'
//     is_menu  'true' if this a MenuItem touch
//     menu     address of menu structure (NULL if no menu)
//
// If spot selected then call menu action routine if 'menu' not NULL,
// else call HotSpot action routine.  Return value of either routine.
//
// Returns 'true' if menu is finished.
//----------------------------------------

bool menu_handletouch(int x, int y, HotSpot *hs, int hslen, bool is_menu, struct Menu *menu)
{
  DEBUG3(">>>>>>>>>>>>>>>>>>>> menu_handletouch: entered, is_menu=%s\n", is_menu ? "true" : "false");

  for (int i = 0; i < hslen; ++hs, ++i)
  {
    if ((x >= hs->x) && (x < hs->x + hs->w) &&
        (y >= hs->y) && (y < hs->y + hs->h))
    {
      if (is_menu)
      { // we have a menu
        struct MenuItem *mi = menu->items[i];

        if (mi->menu)
        {
          struct MenuItem *mi = menu->items[i];
          event_flush();
          menu_show(mi->menu);
//          menu_draw(menu);    // redraw current menu
          DEBUG3("<<<<<<<<<<<<<<<<<<<< menu_handletouch: action, returning false\n");
          return false;
        }
        
        // else call mi->action()
        mi->action();
        event_flush();
        menu_draw(menu);    // redraw current menu
        DEBUG3("<<<<<<<<<<<<<<<<<<<< menu_handletouch: action, returning false\n");
        return false;
      }
      else
      { // just call action HotSpot routine
        event_flush();
        DEBUG3("menu_handletouch: calling HotSpot handler: %p\n", hs->handler);
        event_dump_queue("menu_handletouch: event queue before HotSpot handler:");
        return hs->handler(hs, (void *) menu);
      }
    }
  }
  
  DEBUG3("<<<<<<<<<<<<<<<<<<<< menu_handletouch: end of items, returning 'false'\n");
  return false;
}

//----------------------------------------
// Handle a menu.
//     menu  pointer to a Menu structure
//----------------------------------------

void menu_show(struct Menu *menu)
{ 
  dump_mem("menu_show");
 
  // draw the menu page
  menu_draw(menu);
          
  // get rid of any stray events to this point
  event_flush();

  while (true)
  { 
    // get next event and handle it
    VFOEvent *event = event_pop();

    switch (event->event)
    { 
      case event_Down:       
        if (menu_handletouch(event->x, event->y, hs_menu, ALEN(hs_menu), true, menu))
        {
          return;
        }
        
        if (menu_handletouch(event->x, event->y, hs_other, ALEN(hs_other), false, menu))
        {
          return;
        }
        
        menu_draw(menu);
        break;
      case event_None:
        break;
      default:
        abort("Unrecognized event!?");
    }
  }
}
