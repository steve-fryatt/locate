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




struct contents_block {
	struct objdb_block		*objects;				/**< The object database related to the file.		*/
	struct results_window		*results;				/**< The results window related to the file.		*/
};



/**
 * Initialise the contents search system.
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

	return new;
}


void contents_destroy(struct contents_block *handle)
{
	if (handle == NULL)
		return;

	debug_printf("Destroyed content search: 0x%x", handle);

	heap_free(handle);
}


void contents_add_file(struct contents_block *handle, unsigned key)
{
	if (handle == NULL)
		return;

	debug_printf("Processing object content: key = %d", key);

	results_add_file(handle->results, key);
}

