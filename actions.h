#ifndef ACTIONS_H
#define ACTIONS_H

////////////////////////////////////////////////////////////////////////////////
// The handlers for menu actions.
//
// These handler will all return 'true' if screen must be redrawn.
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>

#include "PixelVFO.h"
//#include "utils.h"

//-----------------------------------------------
// definitions for all action handlers.
//-----------------------------------------------

bool action_no_reset(void);
bool action_reset(void);
bool action_brightness(void);
bool action_calibrate(void);
bool action_slot_save(void);
bool action_slot_restore(void);
bool action_slot_delete(void);
//bool hs_creditsback_handler(HotSpot *hs, void *ignore);

#endif
