PixelVFO Design
===============

The main hardware components for the VFO are:

+-----------------+-------------------------------------------+
| !Microcontroller | Controls the VFO.                         |
+-----------------+-------------------------------------------+
| LCD display     | Display the VFO frequency as well as show |
|                 | the menus, etc.                           |
+-----------------+-------------------------------------------+
| DDS-60 VFO      | Generates the desired VFO frequency.      |
+-----------------+-------------------------------------------+

Operation
---------

The VFO has two modes: 'ONLINE' and 'standby'.

The displayed frequency will be changed by ...

The menu will be shown after ...

Touchscreen interrupts
----------------------

The VFO touchscreen is drivem by a XPT2046 controller.  This chip is controlled
via SPI and has these pins:

+---------+-------------------------------+
| Pin     | Usage                         |
+=========+===============================+
| T_CLK   | SPI data clock                |
+---------+-------------------------------+
| T_CS    | SPI chip select               |
+---------+-------------------------------+
| T_DIN   | SPI data in (MOSI)            |
+---------+-------------------------------+
| T_DO    | SPI data out (MISO)           |
+---------+-------------------------------+
| T_IRQ   | interrupt, LOW means pen down |
+---------+-------------------------------+

The code handling the touchscreen interface will use the standard pins
to read the touchscreen status, but the interrupt pin will be used to
handle the poen goinf DOWN and UP.

The touchscreen code uses the SPI interface and the interrup pin to
create logical events.  These events are held in a *VFOEvent* structure::
    
    typedef struct
    {
      Event event;    // type of event
      int x;          // X coord for the event
      int y;          // Y coord for the event
    } VFOEvent;

where the *Event* type is defined::

    enum Event
    {
      vfo_None,
      vfo_Down,
      vfo_Up,
      vfo_Drag
    };

Every event has associated X and Y coordinate values.

+---------------+-------------------------------------------+
| Event Name    | Description                               |
+===============+===========================================+
| vfo_None      | No event, X and Y are 0                   |
+---------------+-------------------------------------------+
| vfo_Down      | The pen went down                         |
+---------------+-------------------------------------------+
| vfo_Up        | The pen went up                           |
+---------------+-------------------------------------------+
| vfo_Drag      | Movement while the pen is down            |
+---------------+-------------------------------------------+

Event Queue
-----------

The touchscreen code is partly interrupt driven and a decision was made **not**
to directly drive the VFO logic from interrupt code.  We do this by having the
touchscreen code generate the events above and place them into an event queue.

There will be two functions to push/pop events onto and off the queue::

    push_event(event);
    event = pop_event();

Note that the touchscreen code will never **push** a *vfo_None* event onto the
queue.  The *pop_event()* function will return a *vfo_None* event if the queue
is empty.

The queue will be implemented as a circular buffer with length of about
20 events.  Note that the *pop_event()* function *must* be thread safe.
The *push_event()* function runs in the interrupt handler so is safe.

Main Event loop
---------------

There will be a main event loop within the Arduino *loop()* function to handle
most top-level interaction.  There will be some smaller event loops within some
menu action handler routines.

Menu System
===========

There will be a menu system that will allow the user to:

* Save/Restore/Delete slots.  Slots hold frequency and other information.
* Reset certain parameters that render the UI unusable, such as brightness, contrast, etc.
* Reset all slots and set configurable parameters to the defaults.
* Configure certain values for brightness, contrast, etc.
* Calibrate the VFO oscillator.
* Etc.

The menu system will have this structure::

    Menu      Slots     Save slot
                        Restore slot
                        Delete Slot
              Settings  Brightness
                        Contrast
                        Hold click
                        Double click
                        Calibrate
              Reset all No
                        Yes
              Credits
