/****************************************************
 * PixelVFO - a digital VFO driven by touchscreen.
 *
 * VK4FAWR - rzzzwilson@gmail.com
 ****************************************************/

#include <malloc.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdarg.h>
#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "PixelVFO.h"
#include "events.h"
#include "hotspot.h"
#include "menu.h"

#define MAJOR_VERSION   "0"
#define MINOR_VERSION   "3"
#define CALLSIGN        "vk4fawr"

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

// calibration data for raw touch data to screen coordinates transformation
#if 1
// 2.8" calibration
#define TS_MINX     200
#define TS_MINY     340
#define TS_MAXX     3700
#define TS_MAXY     3895
#endif

#if 0
// 2.2" calibration
#define TS_MINX     170
#define TS_MINY     180
#define TS_MAXX     3720
#define TS_MAXY     3800
#endif

// The XPT2046 uses hardware SPI, #4 is CS with #3 for interrupts
// We don't use the T_IRQ pin as we want to use interrupts in VFO code
// TODO: There is some problem with T_IRQ interrupt.  They just stop
//       working on small code changes and removing/reapplying power
//       sometimes fixes the problem!?  Think about using polling in
//       loop() instead of interrupts.
#define TS_CS       4
#define TS_IRQ      3
XPT2046_Touchscreen ts(TS_CS);

// The display also uses hardware SPI, plus #9 & #10
#define TFT_RST     8
#define TFT_DC      9
#define TFT_CS      10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);  

// display constants - offsets, colours, etc
#define FONT_FREQ           (&FreeSansBold24pt7b) // font for frequency display
#define FONT_CREDIT         (&FreeSansBold24pt7b) // first line of credit
#define FONT_CREDIT2        (&FreeSansBold18pt7b) // second line of credit

// various colours
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */

// MENU button definitions
#define MENUBTN_WIDTH          110
#define MENUBTN_HEIGHT         35
#define MENUBTN_X              (ts_width - MENUBTN_WIDTH)
#define MENUBTN_Y              (ts_height - MENUBTN_HEIGHT)
#define MENUBTN_BG             ILI9341_GREEN
#define MENUBTN_BG2            0x4000
#define MENUBTN_FG             ILI9341_BLUE

#define CREDIT_FG              ILI9341_BLACK
#define CREDIT_BG              ILI9341_GREEN

// ONLINE button definitions
#define ONLINE_WIDTH        110
#define ONLINE_HEIGHT       35
#define ONLINE_X            0
#define ONLINE_Y            (ts_height - ONLINE_HEIGHT)
#define ONLINE_BG           ILI9341_RED
#define ONLINE_BG2          0x4000
#define STANDBY_BG          ILI9341_GREEN
#define STANDBY_BG2         0x4000
#define ONLINE_FG           ILI9341_GREEN
#define STANDBY_FG          ILI9341_BLACK


// the VFO states
enum VFOState
{
  VFO_Standby,
  VFO_Online
};

// touchscreen stuff
//int ts_rotation = 0;
int ts_width = SCREEN_WIDTH;
int ts_height = SCREEN_HEIGHT;

// state variables for frequency - display, etc
// the characters in 'freq_display' are stored MSB at left (index 0)
char freq_display[NUM_F_CHAR];                  // digits of frequency, as char values ['0'-'9']
unsigned long frequency;                        // frequency as a long integer
uint16_t freq_char_x_offset[NUM_F_CHAR + 1];    // x offset for start/end of each character on display
int freq_digit_select = -1;                     // index of selected digit in frequency display

uint32_t msraw = 0x80000000;
#define MIN_REPEAT_PERIOD   250

VFOState vfo_state = VFO_Standby;


//-----------------------------------------------
// Debug routine - Dump some memory usage information.
//-----------------------------------------------

extern "C" char *sbrk(int i);

int32_t unallocated(void)
{
  char tos;
  return &tos - (char*) sbrk(0);
}

void dump_mem(const char *msg)
{
  char *heapend = sbrk(0);
  register char *stack_ptr asm ("sp");
  int32_t freemem = unallocated();
  
  Serial.printf("@@@@@ %s: heapend=%08x, stack_ptr=%08x, free=%d\n",
                msg, heapend, stack_ptr, freemem);
}

//-----------------------------------------------
// Debug routine - IGNORE THE CALL
//     format  the printf-style format string
//     ...     the args to 'format'
//
// This routine does nothing.
//-----------------------------------------------

void debug_ignore(const char *format, ...)
{
}

//-----------------------------------------------
// Debug routine - print debug sring to Serial.printf.
//     format  the printf-style format string
//     ...     the args to 'format'
//
// There is a limit of 512 bytes to the resutant string.
//-----------------------------------------------

void debug(const char *format, ...)
{
  static char buff[256];
  va_list aptr;

  va_start(aptr, format);
  vsprintf(buff, format, aptr);
  va_end(aptr);

  Serial.printf(buff);
}

#ifdef DEBUG_ON
//-----------------------------------------------
// Helper for the dumphex() function.
//     base  (void *) pointer to a byte in memory
//     num   number of bytes to dump
//-----------------------------------------------

void dumphex_helper(char *base, int num)
{
  char ascii[17];

  memset(ascii, 0, sizeof(ascii));

  Serial.printf("%08x  ", base);
  
  for (int i = 0; i < 16; ++i)
  {
    char ch = *(base + i);
    
    Serial.printf(F("%02x "), ch);
    if (isprint(ch))
      *(ascii + i) = ch;
    else
      *(ascii + i) = '.';
  }
  Serial.printf(F("  %s\n"), ascii);
}

//-----------------------------------------------
// Print a hex dump of a portion of memory.
//     msg   informative message label for dump
//     base  (void *) pointer to a byte in memory
//     size  number of bytes to dump
//-----------------------------------------------

void dumphex(const char *msg, void *base, int num)
{
  char *off = (char *) base;

  Serial.printf(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"), msg);
  Serial.printf(F("HexDump: %s\n"), msg);
  while (num > 0)
  {
    dumphex_helper(off, 16);
    num -= 16;
    off += 16;
  }
  Serial.printf(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"), msg);
}
#endif

//-----------------------------------------------
// Abort the application giving as much information about the
// problem as possible.  Try to use the TFT screen.
//-----------------------------------------------

void abort(const char *msg)
{
  char *mptr = (char *) msg;
  int  offset_y = 30;
  
  // first, send error message to console
  Serial.printf(F("*********************************************************\n"));
  Serial.printf(F("* %s\n"), msg);
  Serial.printf(F("*********************************************************\n"));

  // write message to the TFT screen
  tft.setFont(FONT_ABORT);
  tft.fillRect(0, 0, tft.width(), tft.height(), ABORT_BG);
  tft.setTextColor(ABORT_FG);
  tft.setCursor(5, offset_y);

  while (msg)
  {
    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;

    mptr = (char *) msg;
    while (*mptr)
    {
      tft.getTextBounds(mptr, 0, 0, &x1, &y1, &w, &h);
      if (w > tft.width())
      {
        char save;
        save = *mptr;
        *mptr = '\0';
        tft.print(msg);
        offset_y += 30;
        tft.setCursor(5, offset_y);
        *mptr = save;
        msg = mptr;
      }
      ++mptr;
    }
    
    tft.print(msg);
  }

  // wait here, forever
  while (1);
}

//----------------------------------------
// Interrupt - pen went down or up.
//
// Ignore quickly repeated interrupts and create an event_Down event.
// Keeps reading while returned results not close to same values.
//----------------------------------------

void touch_irq(void)
{
  uint32_t now = millis();
  static int last_x;
  static int last_y;

  // ignore interrupt if too quick
  if ((now - msraw) < MIN_REPEAT_PERIOD)
  {
    msraw = now;
    return;
  }
  msraw = now;

  // read touch position, wait until reads return similar positions
  noInterrupts();   // ignore interrupts from ts_read()
  for (;;)
  {
    int old_x = last_x;
    int old_y = last_y;
    
    ts_read(&last_x, &last_y);
    
    if (abs(last_x - old_x) < 10 && abs(last_y - old_y) < 10)
      break;
      
    old_x = last_x;
    old_y = last_y;
  }
  interrupts();
  
  // add DOWN event to event queue
  event_push(event_Down, last_x, last_y);
}

//////////////////////////////////////////////////////////////////////////////
// Code to handle the 'frequency' widget at screen top
//////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------
// Draw the frequency byte buffer to the screen.
//     select  index of digit to highlight
//
// Updates all digits on the screen.  Skips leading zeros.
//-----------------------------------------------

void freq_show(int select=-1)
{
  bool leading_space = true;

  tft.setFont(FONT_FREQ);
  
  for (int i = 0; i < NUM_F_CHAR; ++i)
  {
    if (i == select)
      tft.fillRect(freq_char_x_offset[i], 2, CHAR_WIDTH+2, DEPTH_FREQ_DISPLAY-4, FREQ_SEL_BG);
    else
      tft.fillRect(freq_char_x_offset[i], 2, CHAR_WIDTH+2, DEPTH_FREQ_DISPLAY-4, FREQ_BG);
                   
    if ((freq_display[i] != '0') || !leading_space)
    {
      tft.drawChar(freq_char_x_offset[i], TOP_BAR_Y, freq_display[i], FREQ_FG, FREQ_BG, 1);
      leading_space = false;
    }
  }
}

//-----------------------------------------------
// Unselect a digit in the frequency display.
//     select  index of digit to unselect
//-----------------------------------------------

void freq_unselect(int select)
{
  tft.fillRect(freq_char_x_offset[select]-1, 2,
               CHAR_WIDTH+1, DEPTH_FREQ_DISPLAY-4, FREQ_BG);
  tft.setFont(FONT_FREQ);
  tft.drawChar(freq_char_x_offset[select], TOP_BAR_Y,
               freq_display[select], FREQ_FG, FREQ_BG, 1);
}

//-----------------------------------------------
// Fills the frequency char buffer from a given frequency.
//
//     buff  address of char buffer to fill
//     freq  the frequency to put into the buffer
//
// Fills the buffer from the left with MSB first.
//-----------------------------------------------

void freq_to_buff(char *buff, unsigned long freq)
{
  int rem;

  for (int i = NUM_F_CHAR - 1; i >= 0; --i)
  {
    rem = freq % 10;
    freq = freq / 10;
    buff[i] = '0' + rem;
  }
}

//-----------------------------------------------
// Gets an integer frequency from the frequency char buffer.
//     buff  address of char buffer to fill
//
// Returns the buffer value as an unsigned long value.
//-----------------------------------------------

unsigned long freq_to_int(char *buff)
{
  unsigned long result = 0;

  for (int i = 0; i < NUM_F_CHAR; ++i)
  {
    result = result * 10 + freq_display[i] - '0';
  }

  
  return result;
}

//-----------------------------------------------
// Draw the frequency 'thousands' markers
//-----------------------------------------------

void draw_thousands(void)
{
  tft.fillRect(FREQ_OFFSET_X+CHAR_WIDTH*2, 44, 2, 6, FREQ_FG);
  tft.fillRect(FREQ_OFFSET_X+CHAR_WIDTH*5, 44, 2, 6, FREQ_FG);
}

//----------------------------------------
// Show that *something* happened.
// Flash the screen in a possibly eye-catching way.
//----------------------------------------

void display_flash(void)
{
  Serial.printf(F("display_flash: called\n"));
}

//-----------------------------------------------
// Menuitem action handlers
//-----------------------------------------------

void reset_no_action(void)
{
  DEBUG("reset_no_action: called\n");
}

void reset_action(void)
{
  DEBUG("reset_action: called\n");
}

void brightness_action(void)
{
  DEBUG("brightness_action: called\n");
}

void calibrate_action(void)
{
  DEBUG("calibrate_action: called\n");
}

void saveslot_action(void)
{
  DEBUG("saveslot_action: called\n");
}

void restoreslot_action(void)
{
  DEBUG("restoreslot_action: called\n");
}

void deleteslot_action(void)
{
  DEBUG("deleteslot_action: called\n");
}

bool hs_creditsback_handler(HotSpot *hs, void *ignore)
{
  return true;
}

static HotSpot hs_credits[] =
{
  {0, 0,                                    ts_width, DEPTH_FREQ_DISPLAY, hs_creditsback_handler, 0},
};

#define CreditsHSLen   ALEN(hs_credits)

void credits_action(void)
{
  // draw the credits screen
  tft.fillRect(0, 0, tft.width(), tft.height(), CREDIT_BG);
  tft.fillRoundRect(0, 0, tft.width(), DEPTH_FREQ_DISPLAY, 5, FREQ_BG);
  tft.setTextColor(CREDIT_FG);
  tft.setFont(FONT_MENU);
  tft.setCursor(5, TOP_BAR_Y);
  tft.print("Credits");
  menuBackButton();
  tft.setFont(FONT_CREDIT);
  tft.setTextColor(CREDIT_FG);
  tft.setCursor(15, 150);
  tft.printf("PixelVFO %s.%s", MAJOR_VERSION, MINOR_VERSION);
  tft.setFont(FONT_CREDIT2);
  tft.setCursor(180, 230);
  tft.printf("%s", CALLSIGN);
  
  // handle all events in the queue
  event_flush();
  while (true)
  {
    // get next event and handle it
    VFOEvent *event = event_pop();

    switch (event->event)
    {
      case event_Down:
        if (hs_handletouch(event->x, event->y, hs_credits, CreditsHSLen))
        {
          DEBUG("credits_action: hs_handletouch() returned 'true', exiting\n");
          return;
        }
      default:
        break;
    }
  }
}

//-----------------------------------------------
// Define the PixelVFO menu system
//-----------------------------------------------

struct MenuItem mi_reset_no = {"No", NULL, &reset_no_action};
struct MenuItem mi_reset_yes = {"Yes", NULL, &reset_action};
struct MenuItem *mia_reset[] = {&mi_reset_no, &mi_reset_yes};
struct Menu reset_menu = {"Reset all", 0, ALEN(mia_reset), mia_reset};          

struct MenuItem mi_brightness = {"Brightness", NULL, &brightness_action};
struct MenuItem mi_calibrate = {"Calibrate", NULL, &calibrate_action};
struct MenuItem *mia_settings[] = {&mi_brightness, &mi_calibrate};
struct Menu settings_menu = {"Settings", 0, ALEN(mia_settings), mia_settings};

struct MenuItem mi_saveslot = {"Save slot", NULL, &saveslot_action};
struct MenuItem mi_restoreslot = {"Restore slot", NULL, &restoreslot_action};
struct MenuItem mi_deleteslot = {"Delete slot", NULL, &deleteslot_action};
struct MenuItem *mia_slots[] = {&mi_saveslot, &mi_restoreslot, &mi_deleteslot};
struct Menu slots_menu = {"Slots", 0, ALEN(mia_slots), mia_slots};

struct MenuItem mi_slots = {"Slots", &slots_menu, NULL};
struct MenuItem mi_settings = {"Settings", &settings_menu, NULL};
struct MenuItem mi_reset = {"Reset all", &reset_menu, NULL};
struct MenuItem mi_credits = {"Credits", NULL, &credits_action};
#if 0
struct MenuItem mi_credits2 = {"Credits2", NULL, &credits_action};
struct MenuItem mi_credits3 = {"Credits3", NULL, &credits_action};
struct MenuItem mi_credits4 = {"Credits4", NULL, &credits_action};
struct MenuItem *mia_main[] = {&mi_slots, &mi_settings, &mi_reset, &mi_credits, &mi_credits2, &mi_credits3, &mi_credits4};
#endif
struct MenuItem *mia_main[] = {&mi_slots, &mi_settings, &mi_reset, &mi_credits};
struct Menu menu_main = {"Menu", 0, ALEN(mia_main), mia_main};

//////////////////////////////////////////////////////////////////////////////
// Code to handle the 'online' and 'menu' buttons.
//////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------
// Draw the ONLINE/standby button.
// Uses global 'vfo_state' to determine button text.
//-----------------------------------------------

void drawOnline(void)
{
  if (vfo_state == VFO_Standby)
  {
    tft.fillRoundRect(ONLINE_X, ONLINE_Y, ONLINE_WIDTH, ONLINE_HEIGHT, BUTTON_RADIUS, STANDBY_BG2);
    tft.fillRoundRect(ONLINE_X+1, ONLINE_Y+1, ONLINE_WIDTH-2, ONLINE_HEIGHT-2, BUTTON_RADIUS, STANDBY_BG);
    tft.setCursor(ONLINE_X + 7, SCREEN_HEIGHT - 10);
    tft.setFont(FONT_BUTTON);
    tft.setTextColor(STANDBY_FG);
    tft.print("Standby");
  }
  else
  {
    tft.fillRoundRect(ONLINE_X, ONLINE_Y, ONLINE_WIDTH, ONLINE_HEIGHT, BUTTON_RADIUS, ONLINE_BG2);
    tft.fillRoundRect(ONLINE_X+1, ONLINE_Y+1, ONLINE_WIDTH-2, ONLINE_HEIGHT-2, BUTTON_RADIUS, ONLINE_BG);
    tft.setCursor(ONLINE_X + 7, SCREEN_HEIGHT - 10);
    tft.setFont(FONT_BUTTON);
    tft.setTextColor(ONLINE_FG);
    tft.print("ONLINE");
  }
}

void undrawOnline(void)
{
  tft.fillRect(ONLINE_X, ONLINE_Y, ONLINE_WIDTH, ONLINE_HEIGHT, SCREEN_BG2);
}

//-----------------------------------------------
// Draw and undraw the Menu button.
//-----------------------------------------------

void drawMenuButton(void)
{
  tft.fillRoundRect(MENUBTN_X, MENUBTN_Y, MENUBTN_WIDTH, MENUBTN_HEIGHT, BUTTON_RADIUS, MENUBTN_BG2);
  tft.fillRoundRect(MENUBTN_X+1, MENUBTN_Y+1, MENUBTN_WIDTH-2, MENUBTN_HEIGHT-2, BUTTON_RADIUS, MENUBTN_BG);
  tft.setCursor(MENUBTN_X + 22, SCREEN_HEIGHT - 10);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(MENUBTN_FG);
  tft.print("Menu");
}

void undrawMenuButton(void)
{
  tft.fillRect(MENUBTN_X, MENUBTN_Y, MENUBTN_WIDTH, MENUBTN_HEIGHT, SCREEN_BG2);
}

//-----------------------------------------------
// Draw the entire screen (the bits that don't change).
//-----------------------------------------------

void draw_screen(void)
{
  tft.setFont(FONT_FREQ);

  // start drawing things that don't change
  tft.fillRect(0, DEPTH_FREQ_DISPLAY, tft.width(), SCREEN_HEIGHT-DEPTH_FREQ_DISPLAY, SCREEN_BG2);
  tft.setTextWrap(false);
  tft.fillRoundRect(0, 0, tft.width(), DEPTH_FREQ_DISPLAY, 5, FREQ_BG);
  tft.fillRect(0, 0, tft.width(), DEPTH_FREQ_DISPLAY, FREQ_BG);
  tft.setCursor(MHZ_OFFSET_X, TOP_BAR_Y);
  tft.setTextColor(FREQ_FG);
  tft.print("Hz");

  drawOnline();
  drawMenuButton();
}

//-----------------------------------------------
// Get touch information, if anyu.
//     x, y  pointers to cells to receive X and Y position
// Returns 'true' if touch found - X and Y cells updated.
// Returns 'false' if no touch - X and Y cells NOT updated.
//-----------------------------------------------
bool ts_read(int *x, int *y)
{
  // See if there's any touch data for us
  if (ts.bufferEmpty())
  {
    return false;
  }

  // Retrieve a point  
  TS_Point p = ts.getPoint();

  if (p.z < 100)
  {
    return false;
  }

  // Scale from ~0->4000 to tft.width using the calibration #'s
  *x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  *y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  return true;
}

// main screen HotSpot definitions
HotSpot hs_mainscreen[] =
{
  // digits in the frequency display
  {FREQ_OFFSET_X + 0*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 0},
  {FREQ_OFFSET_X + 1*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 1},
  {FREQ_OFFSET_X + 2*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 2},
  {FREQ_OFFSET_X + 3*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 3},
  {FREQ_OFFSET_X + 4*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 4},
  {FREQ_OFFSET_X + 5*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 5},
  {FREQ_OFFSET_X + 6*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 6},
  {FREQ_OFFSET_X + 7*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 7},
  // the "ONLINE/Standby" button
  {ONLINE_X, ONLINE_Y, ONLINE_WIDTH, ONLINE_HEIGHT, online_hs_handler, 0},
  // the "Menu" button
  {MENUBTN_X, MENUBTN_Y, MENUBTN_WIDTH, MENUBTN_HEIGHT, menu_hs_handler, 0},
};

//-----------------------------------------------
// Screen hotspot handlers.
//-----------------------------------------------

bool freq_hs_handler(HotSpot *hs_ptr, void *ignore)
{
  freq_digit_select = hs_ptr->arg;
  keypad_show(hs_ptr->arg);
  return true;
}

//-----------------------------------------------
// Handle pressing the 'ONLINE/Standby' button.
//     hs_ptr  a pointer to the actioned HotSpot item
//     ignore  ignored
// Returns 'false' as no screen redraw required.
//-----------------------------------------------

bool online_hs_handler(HotSpot *hs_ptr, void *ignore)
{
  // toggle state and redraw the button
  if (vfo_state == VFO_Standby)
  {
    vfo_state = VFO_Online;
    // TODO: set up DDS
  }
  else
  {
    vfo_state = VFO_Standby;
    // TODO: turn off DDS
  }
    
  drawOnline();   // redraws button with appropriate text

  return false;
}

//-----------------------------------------------
// Action routine called when the "Menu" button is pressed.
//     hs_ptr  a pointer to the HotSpot item actioned
//     ignore  not used
// Returns 'true' as we will need to redraw the screen.
//-----------------------------------------------

bool menu_hs_handler(HotSpot *hs_ptr, void *ignore)
{
  menu_show(&menu_main);
  return true;
}

//-----------------------------------------------
// Show the frequency adjust keypad starting at the given char.
//     offset  offset of the selected char
// We use an event loop here to handle the keypad events.
//-----------------------------------------------

#define KEYPAD_W            (KEYPAD_MARGIN*4 + KEYPAD_BUTTON_W*3)
#define KEYPAD_H            (KEYPAD_MARGIN*5 + KEYPAD_BUTTON_H*4)
#define KEYPAD_X            ((SCREEN_WIDTH - KEYPAD_W) / 2)
#define KEYPAD_Y            (DEPTH_FREQ_DISPLAY + 1)
#define KEYPAD_BG           ILI9341_GREEN
#define KEYPAD_FILL_COLOR   ILI9341_WHITE
#define KEYPAD_BUTTON_W     44
#define KEYPAD_BUTTON_H     44
#define KEYPAD_MARGIN       2

char keypad_chars[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};

//-----------------------------------------------
// User pressed keypad button.
//     hs  address of hotspot data object
// Update the frequency display.
//-----------------------------------------------

bool keypad_handler(HotSpot *hs, void *ignore)
{
  int arg = hs->arg;

  freq_display[freq_digit_select] = '0' + arg;
  freq_digit_select += 1;
  if (freq_digit_select >= NUM_F_CHAR)
    freq_digit_select = NUM_F_CHAR - 1;
  freq_show(freq_digit_select);

  return false;
}

bool keypad_not_used(HotSpot *hs, void *ignore)
{
  DEBUG("keypad_not_used: called, arg=%d\n", hs->arg);
  abort("keypad_not_used() called, SHOULD NOT BE!?\n");
  return false;
}

bool keypad_close_handler(HotSpot *hs, void *ignore)
{
  return true;
}

bool keypad_freq_handler(HotSpot *hs, void *ignore)
{
  freq_digit_select = hs->arg;
  freq_show(freq_digit_select);
  
  return false;
}

// main screen HotSpot definitions
HotSpot hs_keypad[] =
{
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 1},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 2},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 3},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 4},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 5},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 6},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 7},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 8},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 9},
  {-1, -1, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_not_used, -1},     // empty slot
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 0},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_close_handler, 0},  // the '#' button
  {FREQ_OFFSET_X + 0*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 0},
  {FREQ_OFFSET_X + 1*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 1},
  {FREQ_OFFSET_X + 2*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 2},
  {FREQ_OFFSET_X + 3*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 3},
  {FREQ_OFFSET_X + 4*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 4},
  {FREQ_OFFSET_X + 5*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 5},
  {FREQ_OFFSET_X + 6*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 6},
  {FREQ_OFFSET_X + 7*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, keypad_freq_handler, 7},
};  

#define KeypadHSLen   ALEN(hs_keypad)

//-----------------------------------------------
// Draw a keypad button.
//     ch    the character to show on button
//     x, y  the X and Y index into the keypad matrix
//-----------------------------------------------

void keypad_button_draw(char ch, int x, int y)
{
  // draw the keypad button
  tft.drawRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x,
                    KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y,
                    KEYPAD_BUTTON_H, KEYPAD_BUTTON_H,
                    BUTTON_RADIUS, ILI9341_BLACK);
  tft.fillRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1,
                    KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y+1,
                    KEYPAD_BUTTON_H-2, KEYPAD_BUTTON_H-2,
                    BUTTON_RADIUS, KEYPAD_FILL_COLOR);
  tft.setCursor(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1 + 8,
                KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y + 37);
  //tft.setFont(FONT_BUTTON);
    tft.setFont(FONT_FREQ);
  tft.setTextColor(STANDBY_FG);
  tft.print(ch);

  // fill in the X & Y position in the hotspot data
  hs_keypad[y*3 + x].x = KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x;
  hs_keypad[y*3 + x].y = KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y;
}

//-----------------------------------------------
// Draw the keypad, handle interactions with it.
//     offset  the index into the frequency buffer of digit to change.
//
// We highlight the digit we are going to change.
// We have a small event loop here to handle the keypad.
//-----------------------------------------------

void keypad_show(int offset)
{
  // highlight the frequency digit we are changing
  freq_show(offset);

  // remove the online/menu buttons
  undrawOnline();
  undrawMenuButton();
  
  // draw keypad basic outline
  tft.fillRoundRect(KEYPAD_X, KEYPAD_Y, KEYPAD_W, KEYPAD_H, BUTTON_RADIUS, ILI9341_BLACK);
  tft.fillRoundRect(KEYPAD_X+1, KEYPAD_Y+1, KEYPAD_W-2, KEYPAD_H-2, BUTTON_RADIUS, KEYPAD_BG);

  // draw buttons on the keypad
  for (int y = 0; y < 3; ++y)
  {
    for (int x = 0; x < 3; ++x)
    {
      keypad_button_draw('0' + y*3 + x + 1, x, y);
    }
  }
  keypad_button_draw('0', 1, 3);    // '0' is in non-linear place
  keypad_button_draw('#', 2, 3);    // '#' is in non-linear place

  // event loop
  while (true)
  {
    // get next event and handle it
    VFOEvent *event = event_pop();

    switch (event->event)
    {
      case event_Down:
        if (hs_handletouch(event->x, event->y, hs_keypad, KeypadHSLen))
        {
          //freq_show();
          return;
        }
        break;
      default:
        break;
    }
  }
}

//-----------------------------------------------
// Setup the whole shebang.
//-----------------------------------------------

void setup(void)
{
  Serial.begin(115200);
  Serial.printf("PixelVFO %s.%s\n", MAJOR_VERSION, MINOR_VERSION);

  dump_mem("setup");
  
  // link the TS_IRQ pin to its interrupt handler
  attachInterrupt(digitalPinToInterrupt(TS_IRQ), touch_irq, FALLING);
  
  // set up the VFO frequency data structures
  frequency = 1000000L;
  freq_to_buff(freq_display, 1000000L);

  // initialize 'freq_char_x_offset' array
  int x_offset = FREQ_OFFSET_X;
  for (int i = 0; i <= NUM_F_CHAR; ++i)
  {
    freq_char_x_offset[i] = x_offset;
    x_offset += CHAR_WIDTH;
  }

  // start handling devices
  SPI.begin();
  
  tft.begin();
  tft.setRotation(1);
    
  ts.begin();
  
  // draw the basic screen
  draw_screen();
  
  // draw the 'thousands' markers
  draw_thousands();

#if 0
  // draw the 'edge of digits' markers
  for (int i = 0; i <= NUM_F_CHAR; ++i)
    tft.drawFastVLine(freq_char_x_offset[i], 44, 6, ILI9341_RED);
#endif

  // show the frequency
  freq_show();
}

//-----------------------------------------------
// Arduino main loop function.
//-----------------------------------------------
void loop()
{
  // handle all events in the queue
  while (true)
  {
    // get next event and handle it
    VFOEvent *event = event_pop();

    switch (event->event)
    {
      case event_Down:
//        DEBUG("loop: Event %s\n", event2display(event));
//        hs_dump("main loop:", hs_mainscreen, ALEN(hs_mainscreen));
        if (hs_handletouch(event->x, event->y, hs_mainscreen, ALEN(hs_mainscreen)))
        {
//          DEBUG("loop: hs_handletouch() returned 'true', refreshing display\n");
          draw_screen();
          freq_show();
//          DEBUG("loop: finished refreshing display\n");
        }
//        DEBUG("loop: After event_Down in loop()\n");
        break;
      case event_None:
        return;
      default:
        abort("Unrecognized event!?");
    }
  }
}
