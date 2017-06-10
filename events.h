#ifndef EVENTS_H
#define EVENTS_H

////////////////////////////////////////////////////////////////////////////////
// An event queue system for PixelVFO.
////////////////////////////////////////////////////////////////////////////////

// define the VFOevents
enum Event
{
  event_None,
  event_Down,
  event_Up,
  event_Drag
};

// an event
typedef struct
{
  Event event;    // type of event
  int x;          // X coord for the event
  int y;          // Y coord for the event
} VFOEvent;

// event functions
void event_push(Event event, int x, int y);
VFOEvent *event_pop(void);
int event_pending(void);
void event_flush(void);
void event_dump_queue(const char *msg);
const char *event2display(VFOEvent *event);

#endif
