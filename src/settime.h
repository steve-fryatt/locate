/* Locate - settime.h
 * (c) Stephen Fryatt, 2012
 *
 * Provide a dialogue box for entering dates.
 */

#ifndef LOCATE_SETTIME
#define LOCATE_SETTIME


/**
 * Initialise the Set Time dialogue.
 */

void settime_initialise(void);


/**
 * Open the Set Time dialogue for a text icon, using the date and time given
 * in the field (if recognised) to set the fields up.
 *
 * \param w			The window handle of the parent dialogue.
 * \param i			The icon handle to take the date from.
 * \param *pointer		The current position of the pointer.
 */

void settime_open(wimp_w w, wimp_i i, wimp_pointer *pointer);


/**
 * Notify that a window has been closed.  If it is the Set Time dialogue's
 * current parent, then the dialogue is closed and its contents abandoned.
 *
 * \param w			The handle of the window being closed.
 */

void settime_close(wimp_w w);

#endif

