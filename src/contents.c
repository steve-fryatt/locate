/* Copyright 2013, Stephen Fryatt
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
 * \file: contents.c
 *
 * File contents search.
 */

/* ANSI C header files */

//#include <stdlib.h>
//#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

//#include "oslib/osfile.h"
//#include "oslib/types.h"
//#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
//#include "sflib/errors.h"
//#include "sflib/event.h"

/* Application header files */

#include "contents.h"

#include "objdb.h"
#include "results.h"


/**
 * The block describing a contents search engine.
 */

struct contents_block {
	struct objdb_block		*objects;				/**< The object database related to the file.		*/
	struct results_window		*results;				/**< The results window related to the file.		*/

	unsigned			count;
};


/**
 * Create a new contents search engine.
 *
 * \param *objects		The object database to which the search will belong.
 * \param *results		The results window to which the search will report.
 * \return			The new contents search engine handle, or NULL.
 */

struct contents_block *contents_create(struct objdb_block *objects, struct results_window *results)
{
	struct contents_block	*new;

	if (objects == NULL || results == NULL)
		return NULL;

	new = heap_alloc(sizeof(struct contents_block));
	if (new == NULL)
		return NULL;

	debug_printf("Created new content search: 0x%x", new);

	new->objects = objects;
	new->results = results;

	new->count = 0;

	return new;
}


/**
 * Destroy a contents search engine and free its memory.
 *
 * \param *handle		The handle of the engine to destroy.
 */

void contents_destroy(struct contents_block *handle)
{
	if (handle == NULL)
		return;

	debug_printf("Destroyed content search: 0x%x", handle);

	heap_free(handle);
}


/**
 * Add a file to the search engine, to be processed on subsequent search polls.
 *
 * \param *handle		The handle of the engine to take the file.
 * \param key			The ObjectDB key for the file to be searched.
 */

void contents_add_file(struct contents_block *handle, unsigned key)
{
	if (handle == NULL)
		return;

	debug_printf("Processing object content: key = %d", key);

	results_add_file(handle->results, key);

	handle->count = 5;
}


/**
 * Poll a search to allow it to process the current file.
 *
 * \param *handle		The handle of the engine to poll.
 * \param end_time		The latest time at which control must return.
 * \param *matched		Pointer to a variable to return the matched state,
 *				if search completes.
 * \return			TRUE if the search has completed; else FALSE.
 */

osbool contents_poll(struct contents_block *handle, os_t end_time, osbool *matched)
{
	if (handle == NULL)
		return TRUE;

	debug_printf("Starting contents search loop %d at time %u", handle->count--, os_read_monotonic_time());

	while (handle->count > 0 && os_read_monotonic_time() < end_time);

	debug_printf("Finishing contents search loop at time %u", os_read_monotonic_time());

	return (handle->count == 0) ? TRUE : FALSE;
}

