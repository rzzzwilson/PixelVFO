PixelVFO
========

A rethink of *DigitalVFO* to use a TFT display.

The `DigitalVFO <https://github.com/rzzzwilson/DigitalVFO>`_
repository is a project that generates an RF frequency from a
`DDS-60 daughterboard <http://midnightdesignsolutions.com/dds60/>`_
which is controlled by a
`Teensy microcontroller <https://www.pjrc.com/store/teensy32.html>`_.
The interface for DigitalVFO is a 16x2 character display and an
incremental rotary encoder.

This project uses a 320x240 pixel TFT display with a touch screen to
control the DDS-60 card.  This does away with the rotary encoder.
