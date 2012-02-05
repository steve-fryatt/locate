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

#endif

