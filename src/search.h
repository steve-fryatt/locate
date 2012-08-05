/* Locate - search.h
 * (c) Stephen Fryatt, 2012
 *
 * File search routines.
 */

#ifndef LOCATE_SEARCH
#define LOCATE_SEARCH

#include "results.h"

struct search_block;


/**
 * Create a new search.
 *
 * \param *file			The file block to which the search belongs.
 * \param *results		The results window to which output should be
 *				directed.
 * \param *path			The path(s) to search, comma-separated.
 * \return			The search handle, or NULL on failure.
 */

struct search_block *search_create(struct file_block *file, struct results_window *results, char *path);


/**
 * Destroy a search.
 *
 * \param *handle		The handle of the search to destroy.
 */

void search_destroy(struct search_block *search);


/**
 * Make a search active so that it will run on subsequent calls to search_poll().
 *
 * \param *search		The handle of the search to make active.
 */

void search_start(struct search_block *search);

#endif

