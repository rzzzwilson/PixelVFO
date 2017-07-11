/***************************************************
 * PixelVFO - a digital VFO driven by touchscreen.
 *
 * VK4FAWR - rzzzwilson@gmail.com
 ****************************************************/

#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

#define MAJOR_VERSION   "1"
#define MINOR_VERSION   "0"

// This is calibration data for the raw touch data to the screen coordinates
// 2.8" calibration
#define TS_MINX     200
#define TS_MINY     340
#define TS_MAXX     3700
#define TS_MAXY     3895

// 2.2" calibration
//#define TS_MINX     170
//#define TS_MINY     180
//#define TS_MAXX     3720
//#define TS_MAXY     3800

// The XPT2046 uses hardware SPI, #4 is CS with #3 for interrupts
#define CS_PIN      4
#define TIRQ_PIN    3
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);

// The display also uses hardware SPI, plus #9 & #10
#define TFT_RST     8
#define TFT_DC      9
#define TFT_CS      10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);  

// touchscreen stuff
int ts_rotation = 0;
int ts_width = tft.width();
int ts_height = tft.height();


void ts_setRotation(uint8_t rot)
{
  ts_rotation = rot % 4;
  switch (ts_rotation)
  {
    case 0:     // 'normal' portrait
    case 2:     // invert portrait
      ts_width = tft.width();
      ts_height = tft.height();
      break;
    case 1:     // 'normal' landscape
    case 3:     // invert landscape
      ts_width = tft.height();
      ts_height = tft.width();
      break;
  }
}

void setup(void)
{
  Serial.begin(115200);
  Serial.printf("PixelVFO %s.%s\n", MAJOR_VERSION, MINOR_VERSION);
  
  ts_setRotation(1);
  
  tft.begin();
  ts.begin();
  
  tft.setRotation(1);

  tft.fillScreen(ILI9341_BLACK);
}

void loop()
{
  // See if there's any  touch data for us
  if (ts.bufferEmpty())
  {
    return;
  }

  // Retrieve a point  
  TS_Point p = ts.getPoint();

  if (p.z == 0)
  {
    return;
  }

  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  Serial.print("New p.x="); Serial.print(p.x);
  Serial.print(", p.y="); Serial.println(p.y);

  tft.fillCircle(p.x, p.y, 5, ILI9341_RED);
}
