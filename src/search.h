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
 * Set specific options for a search.
 *
 * \param *search		The search to set the options for.
 * \param search_imagefs	TRUE to search into ImageFSs; FALSE to skip.
 * \param include_files		TRUE to include files; FALSE to exclude.
 * \param include_directories	TRUE to include directories; FALSE to exclude.
 * \param include_applications	TRUE to include applications; FALSE to exclude.
 */

void search_set_options(struct search_block *search, osbool search_imagefs,
		osbool include_files, osbool include_directories, osbool include_applications);


/**
 * Set the filename matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param *filename		Pointer to the filename to match.
 * \param any_case		TRUE to match case insensitively; else FALSE.
 */

void search_set_filename(struct search_block *search, char *filename, osbool any_case);


/**
 * Make a search active so that it will run on subsequent calls to search_poll().
 *
 * \param *search		The handle of the search to make active.
 */

void search_start(struct search_block *search);


/**
 * Stop an active search.
 *
 * \param *Search		The handle of the search to stop.
 */

void search_stop(struct search_block *search);


/**
 * Test to see if a poll is required.
 *
 * \return			TRUE if any searches are active; else FALSE.
 */

osbool search_poll_required(void);


/**
 * Run any active searches in a Null poll.
 */

void search_poll_all(void);

#endif

