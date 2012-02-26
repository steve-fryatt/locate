/* Locate - dialogue.h
 * (c) Stephen Fryatt, 2012
 *
 */

#ifndef LOCATE_DIALOGUE
#define LOCATE_DIALOGUE

#include "file.h"

struct dialogue_block;

/**
 * Initialise the Dialogue module.
 */

void dialogue_initialise(void);


/**
 * Create a new set of dialogue data with the default values.
 *
 * \param *file			The file to which the dialogue belongs.
 * \param *path			The search path to use, or NULL for default.
 * \return			Pointer to the new block, or NULL on failure.
 */

struct dialogue_block *dialogue_create(struct file_block *file, char *path);


/**
 * Destroy a dialogue and its data.
 *
 * \param *dialogue		The dialogue to be destroyed.
 */

void dialogue_destroy(struct dialogue_block *dialogue);


/**
 * Open the Search Dialogue window at the mouse pointer.
 *
 * \param *dialogue		The dialogue details to use to open the window.
 * \param *pointer		The details of the pointer to open the window at.
 */

void dialogue_open_window(struct dialogue_block *dialogue, wimp_pointer *pointer);


/**
 * Identify whether the Search Dialogue window is currently open.
 *
 * \return		TRUE if the window is open; else FALSE.
 */

osbool dialogue_window_is_open(void);

#endif

