////////////////////////////////////////////////////////////////////////////////
// A Variable Frequency Oscillator (VFO) using the DDS-60 card.
//
// The VFO will generate signals in the range 1.000000MHz to 60.000000MHz
// with a step ranging down to 1Hz.
//
// The interface will be a single 240x320 pixel TFT display with touchscreen.
// The menu system will be as for the DigitalVFO.
////////////////////////////////////////////////////////////////////////////////

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
//#include <Fonts/FreeSerif9pt7b.h>
//#include <Fonts/FreeMono9pt7b.h>
//#include "Arial9pt7b.h"
//#include "Arial12pt7b.h"
#include "Arial22pt7b.h"
//#include "Verdana21pt7b.h"

// data/control and chip-select pins we are using
#define TFT_RST 8
#define TFT_DC  9
#define TFT_CS  10

// Use hardware SPI (#13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define NUM_F_CHAR    8

#define FREQ_OFFSET_X   10
#define FREQ_OFFSET_Y   40
#define CHAR_WIDTH      25
#define CHAR_HEIGHT     30
#define MHZ_OFFSET_X    (FREQ_OFFSET_X + NUM_F_CHAR*CHAR_WIDTH + 10)

#define SCREEN_FG     ILI9341_BLACK
#define FREQ_BG       ILI9341_WHITE
#define FREQ_FG       ILI9341_BLUE
 
void setup()
{
  Serial.begin(115200);
  Serial.println("PixelVFO"); 

  // get the display ready
  tft.begin();
  tft.setFont(&Arial22pt7b);

  // start drawing things that don't change
  tft.fillScreen(SCREEN_FG);
  tft.setRotation(1);
  tft.setTextWrap(false);
  tft.fillRect(0, 0, tft.width(), 50, FREQ_BG);
  tft.setCursor(MHZ_OFFSET_X, FREQ_OFFSET_Y);
  tft.setTextColor(FREQ_FG);
  tft.print("MHz");
  tft.drawLine(0, 50, tft.width(), 50, ILI9341_RED);
}

bool change = true;
unsigned long last_millis = 0;
unsigned long frequency = 12345678;
char display_buff[NUM_F_CHAR];

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

void draw_frequency(unsigned long freq)
{
  int x = FREQ_OFFSET_X;

  freq_to_buff(display_buff, freq);
  for (int i = NUM_F_CHAR-1; i >= 0; --i)
  {
    Serial.printf(".fillRect(%d, %d, %d, %d)", x, FREQ_OFFSET_Y, CHAR_WIDTH, CHAR_HEIGHT);
    tft.fillRect(x-1, FREQ_OFFSET_Y-CHAR_HEIGHT-1, CHAR_WIDTH+2, CHAR_HEIGHT+2, FREQ_BG);
    tft.setCursor(x, FREQ_OFFSET_Y);
    Serial.printf(", .setCursor(%d, %d)\n", x, FREQ_OFFSET_Y);
    tft.drawChar(x, FREQ_OFFSET_Y, display_buff[i], FREQ_FG, FREQ_BG, 1);
    x += CHAR_WIDTH;
  }
}

void loop(void)
{
  if ((millis() - last_millis) > 1000)
  {
      change = true;
      last_millis = millis();
  }
     
  if (change)
  {
    draw_frequency(frequency);
    frequency += 1;

#if 0
    tft.fillRect(0, 0, tft.width(), 50, FREQ_BG);
    tft.setTextSize(1);
    tft.setTextColor(FREQ_FG);
    tft.setCursor(20, 40);
    tft.printf("%08ld MHz", frequency);
#endif
  }
  change = false;
}

unsigned long testLines(uint16_t color)
{
  unsigned long start, t;
  int x1, y1, x2, y2;
  int w = tft.width();
  int h = tft.height();

  tft.fillScreen(ILI9341_BLACK);
  yield();
  
  x1 = y1 = 0;
  y2 = h - 1;
  start = micros();
  
  for (x2=0; x2<w; x2+=6)
    tft.drawLine(x1, y1, x2, y2, color);
  x2 = w - 1;
  for (y2=0; y2<h; y2+=6)
    tft.drawLine(x1, y1, x2, y2, color);
  t = micros() - start; // fillScreen doesn't count against timing

  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();

  x1 = w - 1;
  y1 = 0;
  y2 = h - 1;
  start = micros();
  for (x2=0; x2<w; x2+=6)
    tft.drawLine(x1, y1, x2, y2, color);
  x2 = 0;
  for (y2=0; y2<h; y2+=6)
    tft.drawLine(x1, y1, x2, y2, color);
  t += micros() - start;

  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();

  x1 = 0;
  y1 = h - 1;
  y2 = 0;
  start = micros();
  for (x2=0; x2<w; x2+=6)
    tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2=0; y2<h; y2+=6)
    tft.drawLine(x1, y1, x2, y2, color);
  t += micros() - start;

  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();

  x1 = w - 1;
  y1 = h - 1;
  y2 = 0;
  start = micros();
  for (x2=0; x2<w; x2+=6)
    tft.drawLine(x1, y1, x2, y2, color);
  x2 = 0;
  for (y2=0; y2<h; y2+=6)
    tft.drawLine(x1, y1, x2, y2, color);

  yield();
  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2)
{
  unsigned long start;
  int x, y;
  int w = tft.width();
  int h = tft.height();

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  
  for (y=0; y<h; y+=5)
    tft.drawFastHLine(0, y, w, color1);
  for (x=0; x<w; x+=5)
    tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color)
{
  unsigned long start;
  int n, i, i2;
  int cx = tft.width()  / 2;
  int cy = tft.height() / 2;

  tft.fillScreen(ILI9341_BLACK);
  n = min(tft.width(), tft.height());
  start = micros();
  
  for (i=2; i<n; i+=6)
  {
    i2 = i / 2;
    tft.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2)
{
  unsigned long start, t = 0;
  int n, i, i2;
  int cx = tft.width()  / 2 - 1;
  int cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  n = min(tft.width(), tft.height());
  for (i=n; i>0; i-=6)
  {
    i2 = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, color1);
    t += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, color2);
    yield();
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color)
{
  unsigned long start;
  int x, y;
  int w = tft.width();
  int h = tft.height();
  int r2 = radius * 2;

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for (x=radius; x<w; x+=r2)
  {
    for (y=radius; y<h; y+=r2)
    {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color)
{
  unsigned long start;
  int x, y;
  int r2 = radius * 2;
  int w = tft.width()  + radius;
  int h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for (x=0; x<w; x+=r2)
  {
    for (y=0; y<h; y+=r2)
    {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int n, i;
  int cx = tft.width()  / 2 - 1;
  int cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  n = min(cx, cy);
  start = micros();
  
  for (i=0; i<n; i+=5)
  {
    tft.drawTriangle(cx    , cy - i, // peak
                     cx - i, cy + i, // bottom left
                     cx + i, cy + i, // bottom right
                     tft.color565(i, i, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles()
{
  unsigned long start, t = 0;
  int i;
  int cx = tft.width()  / 2 - 1;
  int cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  for (i=min(cx,cy); i>10; i-=5)
  {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i, tft.color565(0, i*10, i*10));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i, tft.color565(i*10, i*10, 0));
    yield();
  }

  return t;
}

unsigned long testRoundRects()
{
  unsigned long start;
  int w, i, i2;
  int cx = tft.width()  / 2 - 1;
  int cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  w = min(tft.width(), tft.height());
  start = micros();
  
  for (i=0; i<w; i+=6)
  {
    i2 = i / 2;
    tft.drawRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects()
{
  unsigned long start;
  int i, i2;
  int cx = tft.width()  / 2 - 1;
  int cy = tft.height() / 2 - 1;

  tft.fillScreen(ILI9341_BLACK);
  start = micros();
  
  for (i=min(tft.width(), tft.height()); i>20; i-=6)
  {
    i2 = i / 2;
    tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(0, i, 0));
    yield();
  }

  return micros() - start;
}
