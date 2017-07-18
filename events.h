#ifndef EVENTS_H
#define EVENTS_H

////////////////////////////////////////////////////////////////////////////////
// An event queue system for PixelVFO.
////////////////////////////////////////////////////////////////////////////////

// define the types VFOevents
enum Event
{
  event_None,
  event_Down,
};

// an event
typedef struct
{
  Event event;    // type of event
  uint16_t x;     // X coord for the event
  uint16_t y;     // Y coord for the event
} VFOEvent;

// event functions
void event_push(Event event, int x, int y);
VFOEvent *event_pop(void);
int event_pending(void);
void event_flush(void);
void event_dump_queue(const char *msg);
const char *event2display(VFOEvent *event);

#endif
