/* Locate - dialogue.h
 * (c) Stephen Fryatt, 2012
 *
 */

#ifndef LOCATE_DIALOGUE
#define LOCATE_DIALOGUE


/**
 * Initialise the Dialogue module.
 */

void dialogue_initialise(void);


/**
 * Open the Search Dialogue window at the mouse pointer.
 *
 * \param *pointer		The details of the pointer to open the window at.
 */

void dialogue_open_window(wimp_pointer *pointer);


/**
 * Identify whether the Search Dialogue window is currently open.
 *
 * \return		TRUE if the window is open; else FALSE.
 */

osbool dialogue_window_is_open(void);

#endif

