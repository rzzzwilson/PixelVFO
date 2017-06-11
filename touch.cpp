////////////////////////////////////////////////////////////////////////////////
// Code to handle touch events for PixelVFO.
//
// Screen is assumed to be 240x320.
//
// The raw data is presented to the system event queue as one of:
//     event_Down
//     event_Up
//     event_Drag
// All events above have associated X and Y coordinates.
////////////////////////////////////////////////////////////////////////////////

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "events.h"
#include "touch.h"
#include "PixelVFO.h"

// calibration data
#define TS_MINX 220
#define TS_MINY 340
#define TS_MAXX 3780
#define TS_MAXY 3850

#define MAX_X   320
#define MAX_Y   240

#define SPI_SETTING         SPISettings(2000000, MSBFIRST, SPI_MODE0)

// set some values
// minimum touch repeat period in milliseconds
#define MIN_REPEAT_PERIOD   2
// amount pen position must drag before generating new event
#define MOVE_DELTA          20


//-----
// The touch system variables
//-----

static uint32_t volatile msraw = 0x80000000;
static uint16_t volatile last_xcalib = 64000;
static uint16_t volatile last_ycalib = 64000;
static bool volatile first_down = true;
static bool volatile PenDown = false;
static uint16_t volatile x_calib = 0;
static uint16_t volatile y_calib = 0;

static int cs_pin = -1;
static int irq_pin = -1;

static int width = 0;
static int height = 0;

// Create an IntervalTimer object 
static IntervalTimer myTimer;


//----------------------------------------
// Read X, Y and Z data from the touchscreen.
//
// Pushes the appropriate PixelVFO event to the system queue.
//----------------------------------------

static void touch_read(void)
{
  uint16_t xraw;
  uint16_t yraw;

  // if finger not down, do nothing
  if (! PenDown) return;

  // otherwise, read the device
  SPI.beginTransaction(SPI_SETTING);
  digitalWrite(cs_pin, LOW);      // enable comms with slave

  SPI.transfer16(0x91 /* X */);   // we seem to read a few times
  SPI.transfer16(0xD1 /* Y */);
  SPI.transfer16(0x91 /* X */);
  SPI.transfer16(0xD1 /* Y */);
  SPI.transfer16(0x91 /* X */);
  xraw = SPI.transfer16(0xD0 /* Y */) >> 3;
  yraw = SPI.transfer16(0) >> 3;

  digitalWrite(cs_pin, HIGH);     // finished comms with slave
  SPI.endTransaction();
  
  // convert to calibrated X and Y
  x_calib = ((xraw - TS_MINX) * MAX_X) / (TS_MAXX - TS_MINX);
  y_calib = ((yraw - TS_MINY) * MAX_Y) / (TS_MAXY - TS_MINY);
  if (x_calib < 0) x_calib = 0;
  if (x_calib > MAX_X) x_calib = MAX_X;
  if (y_calib < 0) y_calib = 0;
  if (y_calib > MAX_Y) y_calib = MAX_Y;
  
  // push event if change is big enough
  if (PenDown && ((abs(x_calib - last_xcalib) > MOVE_DELTA) ||
                    (abs(y_calib - last_ycalib) > MOVE_DELTA)))
  {
    last_xcalib = x_calib;
    last_ycalib = y_calib;

    if (first_down)
    {
      event_push(event_Down, last_xcalib, last_ycalib);
      first_down = false;
    }
    else
    {
      event_push(event_Drag, last_xcalib, last_ycalib);
    }
  }
}

//----------------------------------------
// Set the touchscreen rotation.
//
//     rot  0 - default, portrait
//          1 - rotated 90 degrees, landscape
//          2 - rotated 180 degrees, portrait
//          3 - rotated 270 degrees, landscape
//
// Matches the orientation set for the TFT display.
//----------------------------------------

void touch_setRotation(int rot)
{
//  rotation = rot % 4;
//  Serial.printf("Touchscreen rotation set to %d\n", rotation);
}

//----------------------------------------
// Interrupt - pen went down or up.
//
// Maintain the correct PenDown state and call touch_read() if DOWN.
// Also push a event_Up event if going UP.
//----------------------------------------

static void touch_irq(void)
{
  uint32_t now = millis();

  // ignore interrupt if too quick
  if ((now - msraw) < MIN_REPEAT_PERIOD) return;
  msraw = now;

  if (digitalRead(irq_pin) == 0)
  { // pen DOWN
    PenDown = true;
    touch_read();
  }
  else
  { // pen UP
    PenDown = false;
    event_push(event_Up, last_xcalib, last_ycalib);
    first_down = true;
  }
}

//----------------------------------------
// Initialize the touch system.
//----------------------------------------

void touch_setup(int t_cs, int t_irq, int w, int h)
{
  // save the pins we are going to use
  cs_pin = t_cs;
  irq_pin = t_irq;

  // initialize the pins, mode and level
  pinMode(t_cs, OUTPUT);
  digitalWrite(t_cs, HIGH);
  pinMode(t_irq, INPUT_PULLUP);

  // initiaize some state
  PenDown = false;
  first_down = true;
  width = w;
  height = h;

  // link the irq_pin pint to it's interrupt handler
  attachInterrupt(digitalPinToInterrupt(t_irq), touch_irq, CHANGE);
  
  // start the touchscreen timer - every whenever
  myTimer.begin(touch_read, 100000);
}

