/****************************************************
 * PixelVFO - a digital VFO driven by touchscreen.
 *
 * VK4FAWR - rzzzwilson@gmail.com
 ****************************************************/

#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "PixelVFO.h"
#include "events.h"
#include "hotspot.h"

#define MAJOR_VERSION   "0"
#define MINOR_VERSION   "1"

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

// This is calibration data for the raw touch data to the screen coordinates
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

// constants for main screen layout
#define NUM_F_CHAR            8     // number digits in frequency display
#define CHAR_WIDTH            27    // width of each frequency digit
#define DEPTH_FREQ_DISPLAY    50    // depth of frequency display bar
#define FREQ_OFFSET_Y         40    // offset from top of frequency digits
#define FREQ_OFFSET_X         40    // offset from left of frequency digits
#define MHZ_OFFSET_X          257   // offset from left of the 'Hz' units

// display constants - offsets, colours, etc
#define FONT_FREQ           (&FreeSansBold24pt7b) // font for frequency display
#define FONT_BUTTON         (&FreeSansBold12pt7b) // font for button labels

// various colours
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */

#define SCREEN_BG           ILI9341_BLACK
#define SCREEN_BG3          ILI9341_LIGHTGREY
#define SCREEN_BG2          ILI9341_DARKGREY
#define SCREEN_BG1          ILI9341_BLACK
#define FREQ_FG             ILI9341_BLUE
#define FREQ_BG             ILI9341_WHITE
#define FREQ_SEL_BG         ILI9341_GREEN
#define BOTTOM_BG           ILI9341_WHITE

// ONLINE button definitions
#define BUTTON_RADIUS       5

#define ONLINE_WIDTH        110
#define ONLINE_HEIGHT       35
#define ONLINE_X            0
#define ONLINE_Y            (ts_height - ONLINE_HEIGHT)
#define ONLINE_BG           ILI9341_RED
#define ONLINE_BG2          0x4000
#define STANDBY_BG          ILI9341_GREEN
#define STANDBY_BG2         0x4000
#define ONLINE_FG           ILI9341_GREEN
#define STANDBY_FG           ILI9341_BLACK

// MENU button definitions
#define MENU_WIDTH          110
#define MENU_HEIGHT         35
#define MENU_X              (ts_width - MENU_WIDTH)
#define MENU_Y              (ts_height - MENU_HEIGHT)
#define MENU_BG             ILI9341_GREEN
#define MENU_BG2            ILI9341_BLACK
#define MENU_FG             ILI9341_BLACK

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
#define MIN_REPEAT_PERIOD   100

VFOState vfo_state = VFO_Standby;


//-----------------------------------------------
// Abort the application giving as much information about the
// problem as possible.
//-----------------------------------------------

void abort(const char *msg)
{
  // TODO: write message on the screen, with wrap-around
  while (1);
}

//----------------------------------------
// Interrupt - pen went down or up.
//
// Ignore quickly repeated interrupts and create an event_Down event.
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

  // read touch position, add DOWN event to event queue
  ts_read(&last_x, &last_y);
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
      tft.drawChar(freq_char_x_offset[i], FREQ_OFFSET_Y, freq_display[i], FREQ_FG, FREQ_BG, 1);
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
  Serial.printf("freq_unselect: select=%d\n", select);
  tft.fillRect(freq_char_x_offset[select]-1, 2,
               CHAR_WIDTH+1, DEPTH_FREQ_DISPLAY-4, FREQ_BG);
  tft.setFont(FONT_FREQ);
  tft.drawChar(freq_char_x_offset[select], FREQ_OFFSET_Y,
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

void drawMenu(void)
{
  tft.fillRoundRect(MENU_X, MENU_Y, MENU_WIDTH, MENU_HEIGHT, BUTTON_RADIUS, MENU_BG2);
  tft.fillRoundRect(MENU_X+1, MENU_Y+1, MENU_WIDTH-2, MENU_HEIGHT-2, BUTTON_RADIUS, MENU_BG);
  tft.setCursor(MENU_X + 22, SCREEN_HEIGHT - 10);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(MENU_FG);
  tft.print("Menu");
}

void undrawMenu(void)
{
  tft.fillRect(MENU_X, MENU_Y, MENU_WIDTH, MENU_HEIGHT, SCREEN_BG2);
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
  tft.setCursor(MHZ_OFFSET_X, FREQ_OFFSET_Y);
  tft.setTextColor(FREQ_FG);
  tft.print("Hz");

  drawOnline();
  drawMenu();
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
  {FREQ_OFFSET_X + 0*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 0},
  {FREQ_OFFSET_X + 1*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 1},
  {FREQ_OFFSET_X + 2*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 2},
  {FREQ_OFFSET_X + 3*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 3},
  {FREQ_OFFSET_X + 4*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 4},
  {FREQ_OFFSET_X + 5*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 5},
  {FREQ_OFFSET_X + 6*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 6},
  {FREQ_OFFSET_X + 7*CHAR_WIDTH, 0, CHAR_WIDTH, DEPTH_FREQ_DISPLAY-4, freq_hs_handler, 7},
  {ONLINE_X, ONLINE_Y, ONLINE_WIDTH, ONLINE_HEIGHT, online_hs_handler, 0},
  {MENU_X, MENU_Y, MENU_WIDTH, MENU_HEIGHT, menu_hs_handler, 0},
};

#define MainScreenHSLen   (sizeof(hs_mainscreen)/sizeof(hs_mainscreen[0]))

//-----------------------------------------------
// Screen hotspot handlers.
//-----------------------------------------------

bool freq_hs_handler(HotSpot *hs_ptr)
{
  freq_digit_select = hs_ptr->arg;
  keypad_show(hs_ptr->arg);
  return true;
}

bool online_hs_handler(HotSpot *hs_ptr)
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
    
  drawOnline();

  return false;
}

bool menu_hs_handler(HotSpot *hs_ptr)
{
  Serial.printf("menu_hs_handler: would display menu\n");
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

bool keypad_handler(HotSpot *hs)
{
  int arg = hs->arg;

  freq_display[freq_digit_select] = '0' + arg;
  freq_digit_select += 1;
  if (freq_digit_select >= NUM_F_CHAR)
    freq_digit_select = NUM_F_CHAR - 1;
  freq_show(freq_digit_select);

  return false;
}

bool keypad_not_used(HotSpot *hs)
{
  Serial.printf("keypad_not_used: called, arg=%d\n", hs->arg);
  abort("keypad_not_used() called, SHOULD NOT BE!?\n");
  return false;
}

bool keypad_close_handler(HotSpot *hs)
{
  return true;
}

bool keypad_freq_handler(HotSpot *hs)
{
  Serial.printf("keypad_freq_handler: called, arg=%d\n", hs->arg);

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

#define KeypadHSLen   (sizeof(hs_keypad)/sizeof(hs_keypad[0]))

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
  undrawMenu();
  
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
          Serial.printf("hs_handletouch() returned 'true', end of keypad\n");
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
        Serial.printf("Event %s\n", event2display(event));
        if (hs_handletouch(event->x, event->y, hs_mainscreen, MainScreenHSLen))
        {
          Serial.printf("hs_handletouch() returned 'true', refreshing display\n");
          draw_screen();
          freq_show();
          //event_flush();
        }
        break;
      case event_None:
        return;
      default:
        abort("Unrecognized event!?");
    }
  }
}
