/* Locate - results.h
 * (c) Stephen Fryatt, 2012
 *
 * Search result and status display.
 */

#ifndef LOCATE_RESULTS
#define LOCATE_RESULTS

#include "file.h"

/**
 * Initialise the Results module.
 */

void results_initialise(void);


/**
 * Create and open a new results window.
 *
 * \param *file			The file block to which the window belongs.
 * \param *title		The title to use for the window.
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
 * Update the window extent to hold all of the defined lines.
 *
 * \param *handle		The handle of the results window to update.
 */

void results_update_extent(struct results_window *handle);

#endif

