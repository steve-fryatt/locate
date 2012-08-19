/* Locate - results.h
 * (c) Stephen Fryatt, 2012
 *
 * Search result and status display.
 */

#ifndef LOCATE_RESULTS
#define LOCATE_RESULTS

#include "file.h"

struct results_window;


/**
 * Initialise the Results module.
 *
 * \param *sprites		Pointer to the sprite area to be used.
 */

void results_initialise(osspriteop_area *sprites);


/**
 * Create and open a new results window.
 *
 * \param *file			The file block to which the window belongs.
 * \param *title		The title to use for the window, or NULL to allocate
 *				an empty buffer of TITLE_LENGTH.
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_create(struct file_block *file, char *title);


/**
 * Close and destroy a results window.
 *
 * \param *handle		The handle of the results window to destroy.
 */

void results_destroy(struct results_window *handle);


/**
 * Update the title text for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *title		The text to be copied into the title.
 */

void results_set_title(struct results_window *handle, char *title);


/**
 * Add a line of unspecific text to the end of a results window.
 *
 * \param *handle		The handle to the results window to update.
 * \param *text			The text to add.
 * \param *sprite		The name of the sprite to use.
 * \param small			TRUE to plot the sprite half-size; else FALSE.
 * \param colour		The colour to use for the text.
 */

void results_add_text(struct results_window *handle, char *text, char *sprite, osbool small, wimp_colour colour);


/**
 * Reformat lines in the results window to take into account the current
 * display width.
 *
 * \param *handle		The handle of the results window to update.
 * \param all			TRUE to format all lines; FALSE to format
 *				only those added since the last update.
 */

void results_reformat(struct results_window *handle, osbool all);

#endif

