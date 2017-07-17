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
#include "ArialBold24pt7b.h"
#include "events.h"
#include "hotspot.h"

#define MAJOR_VERSION   "1"
#define MINOR_VERSION   "1"

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

// This is calibration data for the raw touch data to the screen coordinates
#if 0
// 2.8" calibration
#define TS_MINX     200
#define TS_MINY     340
#define TS_MAXX     3700
#define TS_MAXY     3895
#endif

#if 1
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
#define NUM_F_CHAR          8
#define DEPTH_FREQ_DISPLAY  50
#define FREQ_OFFSET_X       50
#define FREQ_OFFSET_Y       40
#define CHAR_WIDTH          25
#define CHAR_HEIGHT         32
#define MHZ_OFFSET_X        (FREQ_OFFSET_X + NUM_F_CHAR*CHAR_WIDTH + 10)

//#define FONT_FREQ           (&ArialBold24pt7b)
#define FONT_BUTTON         (&FreeSansBold12pt7b)
#define FONT_FREQ           (&FreeSansBold24pt7b)

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

// touchscreen stuff
int ts_rotation = 0;
int ts_width = SCREEN_WIDTH;
int ts_height = SCREEN_HEIGHT;

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
//#define ONLINE_FG           ILI9341_BLACK
#define ONLINE_FG           ILI9341_GREEN
#define STANDBY_FG           ILI9341_BLACK

// MENU button definitions
#define MENU_WIDTH          110
#define MENU_HEIGHT         35
#define MENU_X              (ts_width - MENU_WIDTH)
#define MENU_Y              (ts_height - MENU_HEIGHT)
#define MENU_BG             ILI9341_GREEN
//#define MENU_BG2            0x0400
#define MENU_BG2            ILI9341_BLACK
#define MENU_FG             ILI9341_BLACK

// the VFO states
enum VFOState
{
  VFO_Standby,
  VFO_Online
};

// store the VFO frequency here
// the characters in 'freq_display' are stored MSB at left (index 0)
char freq_display[NUM_F_CHAR];          // digits of frequency, as binary values [0-9]
unsigned long frequency;                // frequency as a long integer
uint16_t char_x_offset[NUM_F_CHAR + 1]; // x offset for start/end of each character on display

// index of selected digit in frequency display
int sel_digit = -1;

uint32_t volatile msraw = 0x80000000;
#define MIN_REPEAT_PERIOD   2
bool volatile PenDown = false;

VFOState vfo_state = VFO_Standby;


//-----------------------------------------------
// Abort the application giving as much information about the
// problem as possible.
//-----------------------------------------------

void abort(const char *msg)
{
  while (1);
}

//----------------------------------------
// Interrupt - pen went down or up.
//
// Maintain the correct PenDown state and call touch_read() if DOWN.
// Also push a event_Up event if going UP.
//----------------------------------------

void touch_irq(void)
{
  static int last_x = 0;
  static int last_y = 0;
  uint32_t now = millis();

  // ignore interrupt if too quick
  if ((now - msraw) < MIN_REPEAT_PERIOD) return;
  msraw = now;

  if (digitalRead(TS_IRQ) == 0)
  { // pen DOWN
    if (!PenDown)
    {
      ts_read(&last_x, &last_y);
      event_push(event_Down, last_x, last_y);
    }
    PenDown = true;
  }
  else
  { // pen UP
    if (PenDown)
      event_push(event_Up, last_x, last_y);
    PenDown = false;
  }
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
// Draw the 'thousands' markers
//-----------------------------------------------

void draw_thousands(void)
{
  tft.fillRect(FREQ_OFFSET_X+CHAR_WIDTH*2, 44, 2, 6, FREQ_FG);
  tft.fillRect(FREQ_OFFSET_X+CHAR_WIDTH*5, 44, 2, 6, FREQ_FG);
}

//-----------------------------------------------
// Draw the basic screen
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

void drawMenu(void)
{
  tft.fillRoundRect(MENU_X, MENU_Y, MENU_WIDTH, MENU_HEIGHT, BUTTON_RADIUS, MENU_BG2);
  tft.fillRoundRect(MENU_X+1, MENU_Y+1, MENU_WIDTH-2, MENU_HEIGHT-2, BUTTON_RADIUS, MENU_BG);
  tft.setCursor(MENU_X + 22, SCREEN_HEIGHT - 10);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(MENU_FG);
  tft.print("Menu");
}

void draw_screen(void)
{
  tft.setFont(FONT_FREQ);

  // start drawing things that don't change
  tft.fillRect(0, DEPTH_FREQ_DISPLAY, tft.width(), SCREEN_HEIGHT-DEPTH_FREQ_DISPLAY, SCREEN_BG2);
  tft.setTextWrap(false);
//  tft.fillRect(0, 0, tft.width(), DEPTH_FREQ_DISPLAY, FREQ_BG);
  tft.fillRoundRect(0, 0, tft.width(), DEPTH_FREQ_DISPLAY, 5, FREQ_BG);
  tft.setCursor(MHZ_OFFSET_X, FREQ_OFFSET_Y);
  tft.setTextColor(FREQ_FG);
  tft.print("Hz");

  drawOnline();
  drawMenu();
}

//-----------------------------------------------
//-----------------------------------------------
void ts_setRotation(uint8_t rot)
{
  ts_rotation = rot % 4;
  switch (ts_rotation)
  {
    case 0:     // 'normal' portrait
      ts_width = tft.width();
      ts_height = tft.height();
      break;
    case 1:     // 'normal' landscape
      ts_width = tft.height();
      ts_height = tft.width();
      break;
    case 2:     // invert portrait
      ts_width = tft.width();
      ts_height = tft.height();
      break;
    case 3:     // invert landscape
      ts_width = tft.height();
      ts_height = tft.width();
      break;
  }
  Serial.printf("ts_setRotation: rot=%d, ts_width=%d, ts_height=%d\n", rot, ts_width, ts_height);
}

//-----------------------------------------------
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

  if (p.z == 0)
  {
    return false;
  }

  // Scale from ~0->4000 to tft.width using the calibration #'s
  *x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  *y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  return true;
}

//-----------------------------------------------
// Draw the frequency byte buffer to the screen.
//     select  index of digit to highlight
// Updates all digits on the screen.  Skips leading zeros.
//-----------------------------------------------

void display_frequency(int select=-1)
{
  bool leading_space = true;

  tft.setFont(FONT_FREQ);
  
  for (int i = 0; i < NUM_F_CHAR; ++i)
  {
    if (i == select)
      tft.fillRect(char_x_offset[i], 2, CHAR_WIDTH+2, DEPTH_FREQ_DISPLAY-4, FREQ_SEL_BG);
    else
      tft.fillRect(char_x_offset[i], 2, CHAR_WIDTH+2, DEPTH_FREQ_DISPLAY-4, FREQ_BG);
                   
    if ((freq_display[i] != '0') || !leading_space)
    {
      tft.drawChar(char_x_offset[i], FREQ_OFFSET_Y, freq_display[i], FREQ_FG, FREQ_BG, 1);
      leading_space = false;
    }
  }
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
// Setup the whole shebang.
//-----------------------------------------------
void setup(void)
{
  Serial.begin(115200);
  Serial.printf("PixelVFO %s.%s\n", MAJOR_VERSION, MINOR_VERSION);
  
  // link the TS_IRQ pin to its interrupt handler
  attachInterrupt(digitalPinToInterrupt(TS_IRQ), touch_irq, CHANGE);
  
  // set up the VFO frequency data structures
  frequency = 1000000L;
  freq_to_buff(freq_display, 1000000L);

  // initialize 'char_x_offset' array
  int x_offset = FREQ_OFFSET_X;
  for (int i = 0; i <= NUM_F_CHAR; ++i)
  {
    char_x_offset[i] = x_offset;
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
    tft.drawFastVLine(char_x_offset[i], 44, 6, ILI9341_RED);
#endif

  // show the frequency
  display_frequency();

  hs_dump("hs_mainscreen", hs_mainscreen, MainScreenHSLen);
}

//-----------------------------------------------
// Show the frequency adjust keypad startinbg at the given char.
//     offset  offset of the selected char
// We use an event loop here to handle the keypad events.
//-----------------------------------------------

#define KEYPAD_W      (KEYPAD_MARGIN*4 + KEYPAD_BUTTON_W*3)
#define KEYPAD_H      (KEYPAD_MARGIN*5 + KEYPAD_BUTTON_H*4)
#define KEYPAD_X      ((SCREEN_WIDTH - KEYPAD_W) / 2)
#define KEYPAD_Y      (DEPTH_FREQ_DISPLAY + 1)
#define KEYPAD_EDGE_COLOR   ILI9341_BLUE
#define KEYPAD_FILL_COLOR   ILI9341_WHITE
#define KEYPAD_BUTTON_W   44
#define KEYPAD_BUTTON_H   44
#define KEYPAD_MARGIN     2

char keypad_chars[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};

bool keypad_handler(HotSpot *hs)
{
  Serial.printf("keypad_handler: called, arg=%d\n", hs->arg);
  return false;
}

bool keypad_close_handler(HotSpot *hs)
{
  Serial.printf("keypad_close_handler: called\n");
  return true;
}

bool keypad_freq_handler(HotSpot *hs)
{
  Serial.printf("keypad_freq_handler: called, arg=%d\n", hs->arg);
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
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_handler, 0},
  {0, 0, KEYPAD_BUTTON_W, KEYPAD_BUTTON_H, keypad_close_handler, 0},
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


void show_freq_keypad(int offset)
{
  Serial.printf("show_freq_keypad: offset=%d\n", offset);
  display_frequency(offset);
  tft.fillRoundRect(KEYPAD_X, KEYPAD_Y, KEYPAD_W, KEYPAD_H, BUTTON_RADIUS, ILI9341_BLACK);
  tft.fillRoundRect(KEYPAD_X+1, KEYPAD_Y+1, KEYPAD_W-2, KEYPAD_H-2, BUTTON_RADIUS, KEYPAD_EDGE_COLOR);

  for (int y = 0; y < 3; ++y)
  {
    for (int x = 0; x < 3; ++x)
    {
      char ch;
      
      Serial.printf("x=%d, y=%d, i=%d, char='%c'\n", x, y, y*3 + x, keypad_chars[y*3 + x]);
      tft.drawRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x,
                        KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y,
                        KEYPAD_BUTTON_H, KEYPAD_BUTTON_H,
                        BUTTON_RADIUS, ILI9341_BLACK);
      tft.fillRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1,
                        KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y+1,
                        KEYPAD_BUTTON_H-2, KEYPAD_BUTTON_H-2,
                        BUTTON_RADIUS, KEYPAD_FILL_COLOR);
      tft.setCursor(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1 + 14,
                    KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y + 30);
      tft.setFont(FONT_BUTTON);
      tft.setTextColor(STANDBY_FG);
      ch = '0' + y*3 + x + 1;
      tft.print(ch);

      hs_keypad[y*3 + x].x = KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x;
      hs_keypad[y*3 + x].y = KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y;
    }
  }
    
  int x = 1;
  int y = 3;
  tft.drawRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x,
                    KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y,
                    KEYPAD_BUTTON_H, KEYPAD_BUTTON_H,
                    BUTTON_RADIUS, ILI9341_BLACK);
  tft.fillRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1,
                    KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y+1,
                    KEYPAD_BUTTON_H-2, KEYPAD_BUTTON_H-2,
                    BUTTON_RADIUS, KEYPAD_FILL_COLOR);
  tft.setCursor(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1 + 14,
                KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y + 30);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(STANDBY_FG);
  tft.print("0");
  hs_keypad[y*3 + x - 1].x = KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x;
  hs_keypad[y*3 + x - 1].y = KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y;

  x = 2;
  y = 3;
  tft.drawRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x,
                    KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y,
                    KEYPAD_BUTTON_H, KEYPAD_BUTTON_H,
                    BUTTON_RADIUS, ILI9341_BLACK);
  tft.fillRoundRect(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1,
                    KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y+1,
                    KEYPAD_BUTTON_H-2, KEYPAD_BUTTON_H-2,
                    BUTTON_RADIUS, KEYPAD_FILL_COLOR);
  tft.setCursor(KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x+1 + 14,
                KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y + 30);
  tft.setFont(FONT_BUTTON);
  tft.setTextColor(ILI9341_RED);
  tft.print("#");
  hs_keypad[y*3 + x - 1].x = KEYPAD_X+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_W)*x;
  hs_keypad[y*3 + x - 1].y = KEYPAD_Y+KEYPAD_MARGIN+(KEYPAD_MARGIN+KEYPAD_BUTTON_H)*y;

  // event loop
  // flush any pending events & handle new ones
  event_flush();
  while (true)
  {
    // get next event and handle it
    VFOEvent *event = event_pop();
    if (event->event == event_None)
      continue;

    uint16_t x = event->x;
    uint16_t y = event->y;

    switch (event->event)
    {
      case event_Down:
        Serial.printf("show_freq_keypad: event %s\n", event2display(event));
        if (hs_handletouch(x, y, hs_keypad, KeypadHSLen))
        {
          Serial.printf("hs_handletouch() returned 'true', end of keypad\n");
//          draw_screen();
//          display_frequency();
          return;
        }
        break;
      default:
        break;
    }
  }
}

//-----------------------------------------------
// Screen hotspot handlers.
//-----------------------------------------------

bool freq_hs_handler(HotSpot *hs_ptr)
{
  Serial.printf("freq_hs_handler: hs_ptr->%s\n", hs_display(hs_ptr));
  show_freq_keypad(hs_ptr->arg);
  return true;
}

bool online_hs_handler(HotSpot *hs_ptr)
{
  if (vfo_state == VFO_Standby)
    vfo_state = VFO_Online;
  else
    vfo_state = VFO_Standby;

  drawOnline();

  return false;
}

bool menu_hs_handler(HotSpot *hs_ptr)
{
  Serial.printf("menu_hs_handler: would display menu\n");
  return true;
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

    if (event->event == event_None)
      break;

    uint16_t x = event->x;
    uint16_t y = event->y;

    Serial.printf("Event %s\n", event2display(event));
    
    switch (event->event)
    {
      case event_Down:
        if (hs_handletouch(x, y, hs_mainscreen, MainScreenHSLen))
        {
          Serial.printf("hs_handletouch() returned 'true', refreshing display\n");
          draw_screen();
          display_frequency();
          event_flush();
        }
        break;
      case event_Up:
      case event_Drag:
        break;
      default:
        abort("Unrecognized event!?");
    }
  }
}
