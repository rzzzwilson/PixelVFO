#ifndef UTILS_H
#define UTILS_H

////////////////////////////////////////////////////////////////////////////////
// Utility routines for PixelVFO.
////////////////////////////////////////////////////////////////////////////////


// simple informative message box, just one button "OK"
void util_alert(const char *msg);

// a YES/NO dialog box, returns 'true' on YES
bool util_confirm(const char *msg);

// standard buttons
void util_button(const char *title, int x, int y, int w, int h,
                                 uint16_t bg1, uint16_t bg2);
void util_button_std(const char *title, int x, int y, int w, int h);


#endif
