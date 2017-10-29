////////////////////////////////////////////////////////////////////////////////
// An event queue system for PixelVFO.
//
// Implemented as a circular buffer.
// Since the code that pushes to the queue is event-driven, we must be
// careful to disable/enable interrupts at the appropriate places.
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "events.h"
#include "PixelVFO.h"

// define the length of the event queue
#define EventQueueLength  20

// the queue itself
VFOEvent event_queue[EventQueueLength];

// queue pointers
int queue_fore = 0;   // points at next event to be popped
int queue_aft = 0;    // points at next free slot for a pushed event

//----------------------------------------
// Convert an event to a display string.
// Used only for debug.
//----------------------------------------

const char *event2display(VFOEvent *event)
{
  static char buffer[64];
  
  switch (event->event)
  {
    case event_None:
      return "{event_None, 0, 0}";
      break;
    case event_Down:
      sprintf(buffer, "{event_Down, %d, %d}", event->x, event->y);
      break;
//    case event_Up:
//      sprintf(buffer, "{event_Up, %d, %d}", event->x, event->y);
//      break;
//    case event_Drag:
//      sprintf(buffer, "{event_Drag, %d, %d}", event->x, event->y);
//      break;
    default:
      sprintf(buffer, "UNKNOWN EVENT");
  }
  
  return buffer;
}

//----------------------------------------
// Push an event onto the event queue.
//     event  number of the event to push
//     x, y   the X and Y coords of event
// If queue is full, abort()!
//
// This routine is called only from interrupt code, so needs no protection.
//----------------------------------------

void event_push(Event event, int x, int y)
{
#if 0
  const char *event_name;
  
  switch (event)
  {
    case event_None:
      event_name =  "event_None";
      break;
    case event_Down:
      event_name =  "event_None";
      break;
    default:
      event_name =  "event_UNKNOWN";
  }
#endif

  // put new event into next empty slot
  event_queue[queue_fore].event = event;
  event_queue[queue_fore].x = x;
  event_queue[queue_fore].y = y;

  // move fore ptr one slot up, wraparound if necessary
  ++queue_fore;
  if (queue_fore >= EventQueueLength)
    queue_fore = 0;

  // if queue full, abort
  if (queue_aft == queue_fore)
  {
      event_dump_queue("ERROR: event queue full!");
      abort("ERROR: event queue full! alpha beta gamma delta epsilon zet eta theta 1 2 3 4 5");
  }
}

static VFOEvent NoneEvent = {event_None, 0, 0};

//----------------------------------------
// Pop next event from the queue.
//
// Returns event_None if queue is empty.
//----------------------------------------

VFOEvent *event_pop(void)
{
  // Must protect from RE code fiddling with queue
  noInterrupts();

  // if queue empty, return None event
  if (queue_fore == queue_aft)
  {
    interrupts();
    return &NoneEvent;
  }

  // get next event
  VFOEvent *result = &event_queue[queue_aft];

  // move aft pointer up one slot, wrap if necessary
  ++queue_aft;
  if (queue_aft >= EventQueueLength)
    queue_aft = 0;

  interrupts();

  return result;
}

//----------------------------------------
// Returns the number of events in the queue.
//----------------------------------------

int event_pending(void)
{
  // Must protect from RE code fiddling with queue
  noInterrupts();

  // get distance between fore and aft pointers
  int result = queue_fore - queue_aft;

  // handle case when events wrap around
  if (result < 0)
    result += EventQueueLength;

  interrupts();

  return result;
}

//----------------------------------------
// Clear out any events in the queue.
//----------------------------------------

void event_flush(void)
{
  // Must protect from RE code fiddling with queue
  noInterrupts();

  queue_fore = 0;
  queue_aft = 0;

  interrupts();
}

//----------------------------------------
// Dump the queue contents to the console.
//     msg  address of message to show
// Debug code.
//----------------------------------------

void event_dump_queue(const char *msg)
{
  // Must protect from RE code fiddling with queue
  noInterrupts();

  Serial.printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  Serial.printf("Queue: %s\n", msg);
  for (int i = 0; i < EventQueueLength; ++i)
  {
    VFOEvent *event = &event_queue[i];

    Serial.printf("  %d -> %s\n", i, event2display(event));
  }
  if (event_pending() == 0)
    Serial.printf("Queue length=0 (or %d)\n", EventQueueLength);
  else
    Serial.printf("Queue length=%d\n", event_pending());
  Serial.printf("queue_aft=%d", queue_aft);
  Serial.printf(", queue_fore=%d\n", queue_fore);
  Serial.printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

  interrupts();
}

