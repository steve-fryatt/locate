/* Copyright 2012, Stephen Fryatt
 *
 * This file is part of Locate:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: search.h
 *
 * File search routines.
 */

#ifndef LOCATE_SEARCH
#define LOCATE_SEARCH

#include "oslib/fileswitch.h"

#include "objdb.h"
#include "results.h"

struct search_block;


/**
 * Create a new search.
 *
 * \param *file			The file block to which the search belongs.
 * \param *objects		The object database to store information in.
 * \param *results		The results window to which output should be
 *				directed.
 * \param *path			The path(s) to search, comma-separated.
 * \return			The search handle, or NULL on failure.
 */

struct search_block *search_create(struct file_block *file, struct objdb_block *objects, struct results_window *results, char *path);


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
 * \param invert		TRUE to match files whose names don't match; else FALSE.
 */

void search_set_filename(struct search_block *search, char *filename, osbool any_case, osbool invert);


/**
 * Set the filesize matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param in_limits		TRUE to match when in limits; FALSE to negate logic.
 * \param minimum		The minimum size to match in bytes.
 * \param maximum		The maximum size to match in bytes.
 */

void search_set_size(struct search_block *search, osbool in_limits, int minimum, int maximum);


/**
 * Set the datestamp matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param in_limits		TRUE to match when in limits; FALSE to negate logic.
 * \param minimum		The minimum date to match.
 * \param maximum		The maximum date to match.
 * \param as_age		TRUE to flag the parameters as age, FALSE for date.
 */

void search_set_date(struct search_block *search, osbool in_limits, os_date_and_time minimum, os_date_and_time maximum, osbool as_age);


/**
 * Set the filetype matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param type_list[]		An 0xffffffffu terminated list of filetypes.
 * \param invert		TRUE to exclude listed types; FALSE to include.
 */

void search_set_types(struct search_block *search, unsigned type_list[], osbool invert);


/**
 * Set the attribute matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param mask			A mask setting bits to add to the test.
 * \param required		The required states for the masked bits.
 */

void search_set_attributes(struct search_block *search, fileswitch_attr mask, fileswitch_attr required);


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
 * Test to see if a given search is active.
 *
 * \param *search		The search to test.
 * \return			TRUE if active; else FALSE.
 */

osbool search_is_active(struct search_block *search);


/**
 * Run any active searches in a Null poll.
 */

void search_poll_all(void);

#endif

