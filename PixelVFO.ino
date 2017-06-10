////////////////////////////////////////////////////////////////////////////////
// A Variable Frequency Oscillator (VFO) using the DDS-60 card.
//
// The VFO will generate signals in the range 1.000000MHz to 60.000000MHz
// with a step ranging down to 1Hz.
//
// The interface will be a single 240x320 pixel TFT display with touchscreen.
// The menu system will be as for the DigitalVFO.
////////////////////////////////////////////////////////////////////////////////

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "PixelVFO.h"
#include "ArialBold24pt7b.h"
#include "events.h"
#include "touch.h"

// PixelVFO program name & version
const char *ProgramName = "PixelVFO";
const char *Version = "1.0";
const char *MinorVersion = "1";
const char *Callsign = "vk4fawr";

// TFT display chip-select and data/control pins
#define TFT_RST 8
#define TFT_DC  9
#define TFT_CS  10

// pins for touchscreen and IRQ line
#define T_CS    4
#define T_IRQ   3

#define NUM_F_CHAR      8

#define FREQ_OFFSET_X   50
#define FREQ_OFFSET_Y   40
#define CHAR_WIDTH      25
#define CHAR_HEIGHT     32
#define MHZ_OFFSET_X    (FREQ_OFFSET_X + NUM_F_CHAR*CHAR_WIDTH + 10)

#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */

#define SCREEN_BG       ILI9341_BLACK
#define SCREEN_BG1      ILI9341_DARKGREY
#define SCREEN_BG2      ILI9341_LIGHTGREY
#define FREQ_FG         ILI9341_BLUE
#define FREQ_BG         ILI9341_WHITE
#define BOTTOM_BG       ILI9341_WHITE

// static const uint8_t  PROGMEM myBitmap[] = {0xff...
// tft.drawXBitmap(220, 160, myBitmap, 82, 77, BLACK); //drawXBitmap

// Declare the display and touchscreen interfaces
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

bool change = true;
unsigned long last_millis = 0;

// store the VFO frequency here
// the characters in 'freq_display' are in reverse order, MSB at right
char freq_display[NUM_F_CHAR];   // digits of frequency, as binary values [0-9]
unsigned long frequency;         // frequency as a long integer
uint16_t char_x_offset[NUM_F_CHAR]; // x offset for start of each character on display


//-----------------------------------------------
// Abort the application giving as much information about the
// problem as possible.
//-----------------------------------------------

void abort(const char *msg)
{
  while (1);
}

//-----------------------------------------------
// Initialize PixelVFO.
//-----------------------------------------------

void setup(void)
{
  // initialize the serial connection, announce we've started
  Serial.begin(115200);
  Serial.printf("%s %s.%s\n", ProgramName, Version, MinorVersion); 

  // set up the VFO frequency data structures
  frequency = 1000000L;
  freq_to_buff(freq_display, 1000000L);

  // initialize 'char_x_offset' array
  int x = FREQ_OFFSET_X;
  for (int i = NUM_F_CHAR-1; i >= 0; --i)
  {
    char_x_offset[i] = x;
    x += CHAR_WIDTH;
  }

  // kick off the SPI system
  SPI.begin();

  // get the display ready
  tft.begin();
  tft.setFont(&ArialBold24pt7b);

  // initialize the touch stuff
  touch_setup(T_CS, T_IRQ);

  // start drawing things that don't change
  tft.fillScreen(SCREEN_BG);
  tft.setRotation(1);
  tft.setTextWrap(false);
  tft.fillRect(0, 0, tft.width(), 50, FREQ_BG);
  tft.drawRect(0, 0, tft.width(), 50, SCREEN_BG1);
  tft.drawRect(1, 1, tft.width()-2, 48, SCREEN_BG2);
  tft.setCursor(MHZ_OFFSET_X, FREQ_OFFSET_Y);
  tft.setTextColor(FREQ_FG);
  tft.print("Hz");

  // draw the 'thousands' markers
  tft.fillRect(FREQ_OFFSET_X+CHAR_WIDTH*2, 44, 2, 6, FREQ_FG);
  tft.fillRect(FREQ_OFFSET_X+CHAR_WIDTH*5, 44, 2, 6, FREQ_FG);

  // fill in the remainder of the display
  tft.fillRect(0, 50, tft.width(), tft.height() - 50, SCREEN_BG2);
  // draw dividing line
//  tft.drawFastHLine(0, 50, tft.width(), ILI9341_RED);
  
  display_frequency();
}

//-----------------------------------------------
// Fills the frequency char buffer from a given frequency.
//
//     buff  address of char buffer to fill
//     freq  the frequency to put into the buffer
//
// Fills the buffer from the left with LSB first.
//-----------------------------------------------

void freq_to_buff(char *buff, unsigned long freq)
{
  int rem;

  for (int i = 0; i < NUM_F_CHAR; ++i)
  {
    rem = freq % 10;
    freq = freq / 10;
    buff[i] = '0' + rem;
  }
}

//-----------------------------------------------
// Draw the frequency byte buffer to the screen.
// Updates all digits on the screen.  Skips leading zeros.
//-----------------------------------------------

void display_frequency(void)
{
  bool leading_space = true;

  for (int i = NUM_F_CHAR-1; i >= 0; --i)
  {
    tft.fillRect(freq_display[i]-1, FREQ_OFFSET_Y-CHAR_HEIGHT-1,
                 CHAR_WIDTH+2, CHAR_HEIGHT+2, FREQ_BG);
//    tft.setCursor(char_x_offset[i], FREQ_OFFSET_Y);
    if ((freq_display[i] != '0') || !leading_space)
    {
      tft.drawChar(char_x_offset[i], FREQ_OFFSET_Y, freq_display[i], FREQ_FG, FREQ_BG, 1);
      leading_space = false;
    }
  }
}

//-----------------------------------------------
// Main event loop is here.
//-----------------------------------------------

void loop(void)
{
  // handle all events in the queue
  while (true)
  {
    // get next event and handle it
    VFOEvent *event = event_pop();

    if (event->event == event_None)
      break;

    Serial.printf("Event: %s\n", event2display(event));

    switch (event->event)
    {
      case event_Down:
        break;
      case event_Up:
        break;
      case event_Drag:
        break;
      default:
        abort("Unrecognized event!?");
    }
  }
}
