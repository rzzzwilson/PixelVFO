////////////////////////////////////////////////////////////////////////////////
// Code to handle EEPROM data.
////////////////////////////////////////////////////////////////////////////////

#include "eeprom.h"


//##############################################################################
// Code to save/restore in EEPROM.
//##############################################################################

// Define the address in EEPROM of various things.
// The "NEXT_FREE" value is the address of the next free slot address.
// Ignore "redefine errors" - not very helpful in this case!
// The idea is that we are free to rearrange objects below with minimum fuss.

// start storing at address 0
#define NEXT_FREE   (0)

// address for Frequency 'frequency'
const int AddressFreq = NEXT_FREE;
#define NEXT_FREE   (AddressFreq + sizeof(Frequency))

// address for int 'selected digit'
const int AddressSelDigit = NEXT_FREE;
#define NEXT_FREE   (AddressSelDigit + sizeof(SelOffset))

#if 0
// address for 'VfoClockOffset' calibration
const int AddressVfoClockOffset = NEXT_FREE;
#define NEXT_FREE   (AddressVfoClockOffset + sizeof(VfoClockOffset))

// address for byte 'contrast'
const int AddressContrast = NEXT_FREE;
#define NEXT_FREE   (AddressContrast + sizeof(LcdContrast))

// address for byte 'brightness'
const int AddressBrightness = NEXT_FREE;
#define NEXT_FREE   (AddressBrightness + sizeof(LcdBrightness))

const int SaveFreqBase = NEXT_FREE;
#define NEXT_FREE   (SaveFreqBase + NumSaveSlots * sizeof(Frequency))

//also save the offset for each frequency
const int SaveOffsetBase = NEXT_FREE;
#define NEXT_FREE   (SaveOffsetBase + NumSaveSlots * sizeof(SelOffset);
#endif 

// additional EEPROM saved items go here

#if 0
//----------------------------------------
// Save VFO state to EEPROM.
// Everything except slot data.
//----------------------------------------

void save_to_eeprom(void)
{
  EEPROM.put(AddressFreq, VfoFrequency);
  EEPROM.put(AddressSelDigit, VfoSelectDigit);
  EEPROM.put(AddressVfoClockOffset, VfoClockOffset);
  EEPROM.put(AddressBrightness, LcdBrightness);
  EEPROM.put(AddressContrast, LcdContrast);
  EEPROM.put(AddressHoldClickTime, ReHoldClickTime);
  EEPROM.put(AddressDClickTime, ReDClickTime);
}

//----------------------------------------
// Restore VFO state from EEPROM.
// Everything except slot data.
//----------------------------------------

void restore_from_eeprom(void)
{
  EEPROM.get(AddressFreq, VfoFrequency);
  EEPROM.get(AddressSelDigit, VfoSelectDigit);
  EEPROM.get(AddressVfoClockOffset, VfoClockOffset);
  EEPROM.get(AddressBrightness, LcdBrightness);
  EEPROM.get(AddressContrast, LcdContrast);
  EEPROM.get(AddressHoldClickTime, ReHoldClickTime);
  EEPROM.get(AddressDClickTime, ReDClickTime);
}
#endif

//----------------------------------------
// Given slot number, return freq/offset.
//     freq    pointer to Frequency item to be updated
//     offset  pointer to selection offset item to be updated
//----------------------------------------

void slot_get(int slot_num, Frequency &freq, SelOffset &offset)
{
  int freq_address = SaveFreqBase + slot_num * sizeof(Frequency);
  int offset_address = SaveOffsetBase + slot_num * sizeof(SelOffset);

  EEPROM.get(freq_address, freq);
  EEPROM.get(offset_address, offset);
}

//----------------------------------------
// Put frequency+offset into given slot number.
//     freq    pointer to Frequency item to be saved in slot
//     offset  pointer to selection offset item to be saved in slot
//----------------------------------------

void slot_put(int slot_num, Frequency freq, SelOffset offset)
{
  int freq_address = SaveFreqBase + slot_num * sizeof(Frequency);
  int offset_address = SaveOffsetBase + slot_num * sizeof(SelOffset);

  EEPROM.put(freq_address, freq);
  EEPROM.put(offset_address, offset);
}

#if 0
//----------------------------------------
// Print all EEPROM saved data to console.
//----------------------------------------

void dump_eeprom(void)
{
  Frequency freq;
  SelOffset offset;
  int clkoffset;
  int brightness;
  int contrast;
  UINT hold;
  UINT dclick;

  EEPROM.get(AddressFreq, freq);
  EEPROM.get(AddressSelDigit, offset);
  EEPROM.get(AddressVfoClockOffset, clkoffset);
  EEPROM.get(AddressBrightness, brightness);
  EEPROM.get(AddressContrast, contrast);
  EEPROM.get(AddressHoldClickTime, hold);
  EEPROM.get(AddressDClickTime, dclick);
  
  Serial.printf(F("=================================================\n"));
  Serial.printf(F("dump_eeprom: VfoFrequency=%ld\n"), freq);
  Serial.printf(F("             AddressSelDigit=%d\n"), offset);
  Serial.printf(F("             VfoClockOffset=%d\n"), clkoffset);
  Serial.printf(F("             LcdBrightness=%d\n"), brightness);
  Serial.printf(F("             LcdContrast=%d\n"), contrast);
  Serial.printf(F("             ReHoldClickTime=%dmsec\n"), hold);
  Serial.printf(F("             ReDClickTime=%dmsec\n"), dclick);

  for (int i = 0; i < NumSaveSlots; ++i)
  {
    get_slot(i, freq, offset);
    Serial.printf(F("Slot %d: freq=%ld, seldig=%d\n"), i, freq, offset);
  }

  Serial.printf(F("=================================================\n"));
}

#endif
