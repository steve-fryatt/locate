/* Locate - results.h
 * (c) Stephen Fryatt, 2012
 *
 */

#ifndef LOCATE_RESULTS
#define LOCATE_RESULTS


/**
 * Initialise the Results module.
 */

void results_initialise(void);


/**
 * Create and open a new results window.
 *
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_create(void);


/**
 * Close and destroy a results window.
 *
 * \param *handle		The handle of the results window to destroy.
 */

void results_destroy(struct results_window *handle);

#endif

