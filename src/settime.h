/* Locate - settime.h
 * (c) Stephen Fryatt, 2012
 *
 * Provide a dialogue box for entering dates.
 */

#ifndef LOCATE_SETTIME
#define LOCATE_SETTIME


/**
 * Initialise the date dialogue.
 */

void settime_initialise(void);

void settime_open(wimp_w w, wimp_i i, wimp_pointer *pointer);

void settime_close(wimp_w w);

#endif

