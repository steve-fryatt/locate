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
 * Update the status bar text for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *status		The text to be copied into the status bar.
 */

void results_set_status(struct results_window *handle, char *status);


/**
 * Update the title text for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *title		The text to be copied into the title.
 */

void results_set_title(struct results_window *handle, char *title);


/**
 * Add an error message to the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *message		The error message text.
 * \param *path			The path of the folder where the error occurred,
 *				or NULL if not applicable.
 */

void results_add_error(struct results_window *handle, char *message, char *path);


/**
 * Add a file to the end of the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *name			The complete filename.
 * \param type			The type of the file.
 */

void results_add_file(struct results_window *handle, char *name, unsigned type);


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

