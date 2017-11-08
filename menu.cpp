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
#include "hotspot.h"
#include "utils.h"

// constants for the menu system
#define MENU_SCROLL_WIDTH   20
#define MENU_SCROLL_OFFSET  0
#define SCROLL_HEIGHT       20
#define SCROLL_FG           ILI9341_WHITE
#define SCROLL_BG           ILI9341_RED

#define MENUBACK_WIDTH      80
#define MENUBACK_HEIGHT     35
#define MENUBACK_FG         ILI9341_BLACK
#define MENUBACK_BG         ILI9341_BLACK
#define MENUBACK_BG2        ILI9341_GREEN
#define MENUBACK_X          (ts_width - MENUBACK_WIDTH - 1)
#define MENUBACK_Y          ((DEPTH_FREQ_DISPLAY - MENUBACK_HEIGHT)/2)
#define MENU_ITEM_BG        0x0700


// function forward definitions
//const char *mi_display(struct MenuItem *mi);
//void menu_dump(char const *msg, Menu *menu);

//----------------------------------------
// Handler if user clicks on "Back" button.
//     hs  address of HotSpot item clicked on (the "Back" button)
// Just returns 'true' - a signal that we should return from the menu.
//----------------------------------------

bool hs_menuback_handler(HotSpot *hs)
{
  DEBUG("hs_menuback_handler: called\n");
  return true;    // redraw screen
}

//----------------------------------------
// Handler if user clicks on a MenuItem hotspot.
//     hs   address of HotSpot MenuItem clicked on
//----------------------------------------

bool hs_menuitem_handler(HotSpot *hs)
{
  MenuItem *mi_ptr = (MenuItem *) hs->arg;    // address of MenuItem to action
  
  DEBUG(">>>>> hs_menuitem_handler: entered, hs=\n%s\nmi=\n%s\n",
        hs_display(hs), mi_display(mi_ptr));
  if (mi_ptr->menu)
  {
    menu_show(mi_ptr->menu);
    return true;
  }
  else
  {
    mi_ptr->action(mi_ptr->arg);
    return false;
  }
}

//----------------------------------------
// Handler if user clicks UP on a scrollbar widget.
//     hs    address of HotSpot item clicked on
// Returns 'true' (redraw menu) if scroll happened, else 'false'.
//----------------------------------------

bool menu_scroll_up(HotSpot *hs)
{
  Menu *menu = (Menu *) hs->arg;    // address of Menu to action

  // add 1 to menu 'top' value and normalize
  if (menu->top != 0) 
  {
    // we can scroll
    menu->top -= 1;
    if (menu->top < 0)
        menu->top = 0;
    return true;    // redraw screen
  }
  else
  {
    // we can't scroll
    return false;   // don't redraw screen
  }
}

//----------------------------------------
// Handler if user clicks DOWN on a scrollbar widget.
//     hs    address of HotSpot item clicked on
//----------------------------------------

bool menu_scroll_down(HotSpot *hs)
{
  Menu *menu = (Menu *) hs->arg;    // address of Menu to action

  menu_dump("menu_scroll_down: menu", menu); 

  // add 1 to menu 'top' value and normalize
  if (menu->top < (menu->num_items - MAXMENUITEMROWS))
  {
    // we can scroll
    menu->top += 1;
    if (menu->top > menu->num_items - MAXMENUITEMROWS)
        menu->top = menu->num_items - MAXMENUITEMROWS;
    return true;    // redraw screen
  }
  else
  {
    // we can't scroll
    return false;   // don't redraw screen
  }
}

// Define the Hotspots the menu items use
HotSpot hs_menu[] =
{
  // menuitem hotspots
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*0, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 0},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*1, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 1},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*2, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 2},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*3, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 3},
  {100, DEPTH_FREQ_DISPLAY+MENUITEM_HEIGHT*4, ts_width, MENUITEM_HEIGHT, hs_menuitem_handler, 4},
};

// Define the Hotspots the menu scroll widgets use
static HotSpot hs_scroll[] =
{
  // the 'scroll' hotspots
  {0, DEPTH_FREQ_DISPLAY, 50, 50, menu_scroll_up, 1},
  {0, ts_height-50, 50, 50, menu_scroll_down, 1},
};

// Define the Hotspot for the BACK button
static HotSpot hs_back[] =
{
  // the 'Back' button
  {MENUBACK_X, 0, ts_width-MENUBACK_X, DEPTH_FREQ_DISPLAY, hs_menuback_handler, 0},
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

  sprintf(buffer, "mi: %p, title='%s', menu=%p, action=%p, arg=%08X\n",
          mi, mi->title, mi->menu, mi->action, (unsigned int) mi->arg);
  
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
  Serial.printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  Serial.printf("Menu: %s\n", msg);
  Serial.printf("  title=%s, top=%d, num items=%d, indexed=%s\n",
                menu->title, menu->top, menu->num_items, (menu->indexed) ? "true" : "false");

  for (int i = 0; i < menu->num_items; ++i)
  {
    struct MenuItem *mi_ptr = menu->items[i];
    Serial.printf("    mi %d: %s", i, mi_display(mi_ptr));
  }
  Serial.printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}


//----------------------------------------
// Draw the Menu "Back" button.
//----------------------------------------
  
void menuBackButton(void)
{
  util_button("Back", MENUBACK_X, MENUBACK_Y, MENUBACK_WIDTH, MENUBACK_HEIGHT,
              MENUBACK_BG, MENUBACK_BG2, MENUBACK_FG);
}

//----------------------------------------
// Draw a menu on the screen.
//     menu  pointer to a Menu structure
//----------------------------------------
  
void menu_draw(struct Menu *menu)
{
  DEBUG(">>>>>>>>>> menu_draw: entered, menu title=%s\n", menu->title);

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

  // draw menuitems (at least, those that fit on screen
  tft.setFont(FONT_MENUITEM);
  int mi_y = DEPTH_FREQ_DISPLAY + MENUITEM_HEIGHT;
  for (int i = menu->top; i < menu->top + MAXMENUITEMROWS; ++i)
  {
    if (i >= menu->num_items)
    {
      break;
    }

    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;
     
    tft.getTextBounds((char *) menu->items[i]->title, 1, 1, &x1, &y1, &w, &h);

    // write indexed item on lower row, right-justified
    tft.fillRect(0, mi_y - MENUITEM_HEIGHT, ts_width-1, MENUITEM_HEIGHT - 1, MENU_BG);
    tft.setCursor(ts_width - w - 5, mi_y - 10);
    tft.print(menu->items[i]->title);

    // if we are indexing, write index text in correct column
    if (menu->indexed)
    {
      char buff[16];

      sprintf(buff, "%d:", i);
      //tft.getTextBounds(buff, 1, 1, &x1, &y1, &w, &h);
      tft.setCursor(INDEX_COLUMN, mi_y - 10);
      tft.print(buff);
    }
    
    mi_y += MENUITEM_HEIGHT;
  }
  
  // draw the scroll widget if required
  if (menu->num_items > MAXMENUITEMROWS)
  {
    tft.fillRect(MENU_SCROLL_OFFSET, DEPTH_FREQ_DISPLAY,
                 MENU_SCROLL_WIDTH, ts_height - DEPTH_FREQ_DISPLAY, SCROLL_BG);
    tft.fillTriangle(MENU_SCROLL_OFFSET, DEPTH_FREQ_DISPLAY+SCROLL_HEIGHT,
                     MENU_SCROLL_OFFSET + MENU_SCROLL_WIDTH-1, DEPTH_FREQ_DISPLAY+SCROLL_HEIGHT,
                     MENU_SCROLL_OFFSET + MENU_SCROLL_WIDTH/2, DEPTH_FREQ_DISPLAY,
                     SCROLL_FG);
    tft.fillTriangle(MENU_SCROLL_OFFSET, ts_height-1-SCROLL_HEIGHT,
                     MENU_SCROLL_OFFSET + MENU_SCROLL_WIDTH-1, ts_height-1-SCROLL_HEIGHT,
                     MENU_SCROLL_OFFSET + MENU_SCROLL_WIDTH/2, ts_height-1,
                     SCROLL_FG);
  }

  DEBUG("<<<<<<<<<< menu_draw: exit, menu title=%s\n", menu->title);
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
  DEBUG(">>>>>>>>>>>>>>> menu_handletouch: entered, x=%d, y=%d, is_menu=%s, hslen=%d, menu->top=%d\n",
        x, y, is_menu ? "true" : "false", hslen, menu->top);
  menu_dump("Menu:", menu);
  hs_dump("Hotspots:", hs, hslen);

  int max_scan = (is_menu) ? MAXMENUITEMROWS : hslen;

  for (int i = 0; i < max_scan; ++hs, ++i)
  {
    if ((x >= hs->x) && (x < hs->x + hs->w) &&
        (y >= hs->y) && (y < hs->y + hs->h))
    {
      int ndx = i + menu->top;   // index of actual menuitem/action
            
      if (is_menu)
      { // we have a menu
        struct MenuItem *mi = menu->items[ndx];

        if (mi->menu)
        {
          DEBUG("menu_handletouch: calling menu_show('%s')\n", mi->menu->title);
          menu_show(mi->menu);
          DEBUG("<<<<<<<<<<<<<<< menu_handletouch: menu, returning 'true'\n");
          return true;
        }
        else
        {
          DEBUG("menu_handletouch: calling mi->action('%08x')\n", (unsigned long) mi->arg);
          bool result = mi->action(mi->arg);
          //menu_draw(menu);
          DEBUG("<<<<<<<<<<<<<<< menu_handletouch: action, returning '%s'\n",
                (result) ? "true" : "false");
          return result;
        }
      }
      else
      { // just call action HotSpot routine
        DEBUG("menu_handletouch: calling HotSpot handler: %p\n", hs->handler);
        hs->arg = (int) menu;    // dynamically plug menu to handle into HotSpot
        bool result = hs->handler(hs);
        DEBUG("menu_handletouch: returning '%s'\n", (result) ? "true" : "false");
        return result;
      }
    }
  }
  
  DEBUG("menu_handletouch: end of items, returning 'false'\n");
  return false;
}

//**************************************
// Draw a menu on the screen.
//     menu  address of the Menu structure to draw
//**************************************

void menu_show(struct Menu *menu)
{ 
  DEBUG(">>>>> menu_show: entered, menu->title=%s\n", menu->title);

  // first draw of menu, set scroll to top of menuitems
  menu->top = 0;

  // draw the menu screen
  menu_draw(menu);
          
  // event loop for handling menu
  while (true)
  { 
    int x;    // pen touch coordinates
    int y;
  
    if (pen_touch(&x, &y))
    {
      DEBUG("menu_show: Checking menuitem touch\n");
      if (menu_handletouch(x, y, hs_menu, ALEN(hs_menu), true, menu))
      {
        DEBUG("<<<<< menu_show: menuitem touch handled, menu->title=%s\n", menu->title);
        menu_draw(menu);
        continue;
      }

      DEBUG("menu_show: Checking scroll touch\n");
      if (menu_handletouch(x, y, hs_scroll, ALEN(hs_scroll), false, menu))
      {
        DEBUG("<<<<< menu_show: 'scroll' touch handled, menu->title=%s\n", menu->title);
        menu_draw(menu);
        continue;
      }
      
      DEBUG("menu_show: Checking BACK touch\n");
      if (menu_handletouch(x, y, hs_back, ALEN(hs_back), false, menu))
      {
        DEBUG("<<<<< menu_show: 'BACK' touch handled, menu->title=%s\n", menu->title);
        return;
      }
    }
  }
}

