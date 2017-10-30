PixelVFO Design
===============

The main hardware components for the VFO are:

+-----------------+-------------------------------------------+
| Component       | Usage                                     |
+=================+===========================================+
| Microcontroller | Controls the VFO.                         |
+-----------------+-------------------------------------------+
| LCD display     | Display the VFO frequency as well as show |
|                 | the menus, etc.                           |
+-----------------+-------------------------------------------+
| DDS-60 VFO      | Generates the desired VFO frequency.      |
+-----------------+-------------------------------------------+

Operation
---------

The VFO has two modes: 'ONLINE' and 'standby'.  The status will be shown
in a button on the screen.  Clicking the button will toggle the status.

The displayed frequency will be changed by clicking on a frequency
digit, which will present a keypad (digits 1->9, 0, #).  Clicking on
a keypad digit will change the highlighted frequency digit and move to
the next digit.  Changing the last digit in the frequency buffer will
result in the highlight remaining on the last digit.  Pressing the '#'
keypad digit will dismiss the keypad.  Selecting any digit in the
frequency buffer will move the highlight to that digit.

The menu will be shown after pressing the 'Menu' button.

Touchscreen interrupts
----------------------

NOTE: Due to problems of reliability with the touchscreen interupt, the
code will **not** use this interrupt.  We will poll for screen touches.
So the interrupt and event code is not used.

The VFO touchscreen is driven by an XPT2046 driver chip.  This chip is
controlled via SPI and has these accessible lines:

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
| T_IRQ   | pen, LOW means pen down       |
+---------+-------------------------------+

The code handling the touchscreen interface will use the standard SPI pins
to read the touchscreen status.  The T_IRQ pin will be treated as an
interrupt line and will place "pen down" events in the system event
queue.

The touchscreen code uses the SPI interface and the T_IRQ interrupt to
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
    };

Every event has associated X and Y coordinate values.

+---------------+-------------------------------------------+
| Event Name    | Description                               |
+===============+===========================================+
| vfo_None      | No event, X and Y are 0                   |
+---------------+-------------------------------------------+
| vfo_Down      | The pen went down                         |
+---------------+-------------------------------------------+

Event Queue
-----------

The touchscreen code is partly interrupt driven and a decision was made to
**not** directly drive the VFO logic from interrupt code.  We do this by having
the touchscreen code generate the events above and place them into an event
queue.

There will be two functions to push/pop events onto and off the queue::

    push_event(event);
    VFOEvent *event = pop_event();

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

Menu Implementation
-------------------

There will be data structures called *Menu* and *MenuItem*.  A Menu
will define one menu and will contain a title string and one or more
MenuItem references.  The MenuItem structure will contain a display string
and either a sub-menu reference or a reference to an action function,
depending on whether clicking on the item draws a sub-menu or performs
some action, respectively.

    struct Menu
    {
        const char *title;          // title displayed on menu page
        int top;                    // index of top displayed item
        int num_items;              // number of items in the array below
        struct MenuItem **items;    // array of pointers to MenuItem data
    };
    
    struct MenuItem
    {
        const char *title;          // menu item display text
        struct Menu *menu;          // if not NULL, submenu to pass to show_menu()
        ItemAction action;          // if not NULL, address of action function
    };

A menu will be drawn by calling *bool menu_show(struct Menu *menu)*.
This function will draw the menu and menuitems and wait for a click on one of:

* a displayed menuitem
* an up/down widget
* the BACK button

Clicking on a displayed menuitem will call either menu_show() passing the
sub-menu reference *or* will call the action handler.  The recursive call of
menu_show() will always return **false** and the action handler will return
either **true** or **false**.  The return value from menu_show() determines
whether the current menu returns or is redrawn.

Clicking on an up/down widget will scroll the menu up or down.  This is
accomplished by adjusting the **top** value for the menu and redrawing it.

Clicking on the back button calls the handler that returns **true**, thereby
exiting the current (sub-)menu.

