////////////////////////////////////////////////////////////////////////////////
// Interface to the code to handle EEPROM data.
////////////////////////////////////////////////////////////////////////////////

#ifndef EEPROM_H
#define EEPROM_H

#include <EEPROM.h>
#include "PixelVFO.h"

// Define the address in EEPROM of various things.
// The "NEXT_FREE" value is the address of the next free slot address.
// Ignore "redefine errors" - not very helpful in this case!
// The idea is that we are free to rearrange objects below with minimum fuss.

// start storing at address 0
#define NEXT_FREE   (0)

#if 0
// address for Frequency 'frequency'
const int AddressFreq = NEXT_FREE;
#define NEXT_FREE   (AddressFreq + sizeof(Frequency))

// address for int 'selected digit'
const int AddressSelDigit = NEXT_FREE;
#define NEXT_FREE   (AddressSelDigit + sizeof(SelOffset))

// address for 'VfoClockOffset' calibration
const int AddressVfoClockOffset = NEXT_FREE;
#define NEXT_FREE   (AddressVfoClockOffset + sizeof(VfoClockOffset))

// address for byte 'contrast'
const int AddressContrast = NEXT_FREE;
#define NEXT_FREE   (AddressContrast + sizeof(LcdContrast))

// address for byte 'brightness'
const int AddressBrightness = NEXT_FREE;
#define NEXT_FREE   (AddressBrightness + sizeof(LcdBrightness))
#endif

// number of frequency save slots in EEPROM
const int NumSaveSlots = 10;

const int SaveFreqBase = NEXT_FREE;
#define NEXT_FREE   (SaveFreqBase + NumSaveSlots * sizeof(Frequency))

//also save the offset for each frequency
const int SaveOffsetBase = NEXT_FREE;
#define NEXT_FREE   (SaveOffsetBase + NumSaveSlots * sizeof(SelOffset);

// additional EEPROM saved items go here


// Given slot number, return freq/offset.
void slot_get(int slot_num, Frequency &freq, SelOffset &offset);
void slot_put(int slot_num, Frequency freq, SelOffset offset);
void eeprom_init(void);

#endif
