#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "events.h"
#include "touch.h"
#include "PixelVFO.h"

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
static int16_t volatile last_xraw = 32000;
static int16_t volatile last_yraw = 32000;
static bool volatile first_down = true;
static bool volatile PenDown = false;

static int cs_pin = -1;
static int irq_pin = -1;

// Create an IntervalTimer object 
static IntervalTimer myTimer;


//----------------------------------------
// Read X, Y and Z data from the touchscreen.
//
// Pushes the appropriate PixelVFO event to the system queue.
//----------------------------------------

static void touch_read(void)
{
  int16_t xraw;
  int16_t yraw;

  Serial.printf("touch_read: PenDown=%d\n", PenDown);
  
  // if finger not down, do nothing
  if (! PenDown) return;

  // otherwise, read the device
  SPI.beginTransaction(SPI_SETTING);
  digitalWrite(cs_pin, LOW);      // enable comms with slave

  // we seem to read a few times
  SPI.transfer16(0x91 /* X */);
  SPI.transfer16(0xD1 /* Y */);
  SPI.transfer16(0x91 /* X */);
  SPI.transfer16(0xD1 /* Y */);
  SPI.transfer16(0x91 /* X */);
  xraw = SPI.transfer16(0xD0 /* Y */) >> 3;
  yraw = SPI.transfer16(0) >> 3;
  
  digitalWrite(cs_pin, HIGH);     // finished comms with slave
  SPI.endTransaction();
  
  // push event if change is big enough
  if (PenDown && ((abs(xraw - last_xraw) > MOVE_DELTA) ||
                    (abs(yraw - last_yraw) > MOVE_DELTA)))
  {
    last_xraw = xraw;
    last_yraw = yraw;

    if (first_down)
    {
      event_push(event_Down, last_xraw, last_yraw);
      first_down = false;
    }
    else
    {
      event_push(event_Drag, last_xraw, last_yraw);
    }
  }
}

//----------------------------------------
// Interrupt - pen went down or up.
//
// Maintain the correct PenDown state and call touch_read() if DOWN.
// Also push a event_Up event if going UP.
//----------------------------------------

static void touch_irq(void)
{
  Serial.printf("touch_irq");
  
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
    event_push(event_Up, last_xraw, last_yraw);
    first_down = true;
  }
}

//----------------------------------------
// Initialize the touch system.
//----------------------------------------

void touch_setup(int t_cs, int t_irq)
{
  Serial.printf("touch_setup, t_cs=%d, t_irq=%d\n", t_cs, t_irq);
  
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

  // link the irq_pin pint to it's interrupt handler
  attachInterrupt(digitalPinToInterrupt(t_irq), touch_irq, CHANGE);
  
  // start the touchscreen timer - every whenever
  myTimer.begin(touch_read, 100000);
}

