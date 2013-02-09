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

#include "flex.h"

/* OSLib header files */

#include "oslib/osargs.h"
#include "oslib/osfile.h"
#include "oslib/osfind.h"
#include "oslib/osgbpb.h"
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


#define CONTENTS_FILENAME_SIZE 256						/**< The space in bytes initially allocated to take filenames.	*/
#define CONTENTS_FILE_BUFFER_SIZE 50						/**< The space in KBytes allocated to load file contents.	*/


/**
 * The block describing a contents search engine.
 */

struct contents_block {
	struct objdb_block		*objects;				/**< The object database related to the search.			*/
	struct results_window		*results;				/**< The results window related to the search.			*/

	unsigned			key;					/**< The ObjectDB key of the file being searched.		*/

	char				*filename;				/**< Flex block containing the name of the current file.	*/

	char				*file;					/**< Flex block containing the file data or a subset of it.	*/
	size_t				file_block_size;			/**< The number of bytes allocated to the file block.		*/

	int				file_length;				/**< The number of bytes of file data on disc.			*/
	int				file_offset;				/**< File ptr for the start of the data in memory.		*/

	unsigned			count;
};



static osbool	contents_load_file_chunk(struct contents_block *handle, int position);
static char	contents_get_byte(struct contents_block *handle, unsigned byte);


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
	osbool			mem_ok = TRUE;

	if (objects == NULL || results == NULL)
		return NULL;

	new = heap_alloc(sizeof(struct contents_block));
	if (new == NULL)
		return NULL;

	debug_printf("Created new content search: 0x%x", new);

	new->objects = objects;
	new->results = results;

	new->file_block_size = 1024 * CONTENTS_FILE_BUFFER_SIZE;

	new->key = OBJDB_NULL_KEY;

	new->filename = NULL;
	new->file = NULL;

	if (flex_alloc((flex_ptr) &(new->filename), CONTENTS_FILENAME_SIZE) == 0)
		mem_ok = FALSE;

	if (flex_alloc((flex_ptr) &(new->file), new->file_block_size) == 0)
		mem_ok = FALSE;

	if (!mem_ok) {
		if (new->filename != NULL)
			flex_free((flex_ptr) &(new->filename));

		if (new->filename != NULL)
			flex_free((flex_ptr) &(new->file));

		heap_free(new);

		return NULL;
	}

	new->file_offset = 0;
	new->file_length = 0;

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

	if (handle->filename != NULL)
		flex_free((flex_ptr) &(handle->filename));

	if (handle->file != NULL)
		flex_free((flex_ptr) &(handle->file));

	heap_free(handle);
}


/**
 * Add a file to the search engine, to be processed on subsequent search polls.
 *
 * \param *handle		The handle of the engine to take the file.
 * \param key			The ObjectDB key for the file to be searched.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool contents_add_file(struct contents_block *handle, unsigned key)
{
	size_t		filename_length;
	os_error	*error;

	if (handle == NULL || key == OBJDB_NULL_KEY)
		return FALSE;

	handle->key = key;

	handle->file_length = 0;
	handle->file_offset = 0;

	debug_printf("Processing object content: key = %d", key);

	/* Find the filename of the file to be searched. */

	filename_length = objdb_get_name_length(handle->objects, key);
	if (filename_length > flex_size((flex_ptr) &handle->filename) &&
			flex_extend((flex_ptr) &handle->filename, filename_length) == 0)
		return FALSE;

	if (!objdb_get_name(handle->objects, key, handle->filename, filename_length))
		return FALSE;

	/* Get the size of the file to be searched and decide if it will fit into the
	 * available space.
	 */

	error = xosfile_read_no_path(handle->filename, NULL, NULL, NULL, &handle->file_length, NULL);
	if (error != NULL) {
		results_add_error(handle->results, error->errmess, handle->filename);
		handle->file_length = 0;
		return FALSE;
	}

	/* Load the first chunk of data from the file. */

	contents_load_file_chunk(handle, 0);




	//block_length = (size > handle->file_block_size) ? handle->file_block_size : size;

	//debug_printf("Filename: %s, size: %d, block length: %d", handle->filename, size, block_length);

	/* Load the first block of the file into memory. */


	results_add_file(handle->results, key);

	handle->count = 1;

	return TRUE;
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



static osbool contents_load_file_chunk(struct contents_block *handle, int position)
{
	int		extent, ptr, bytes, unread;
	os_fw		file;
	os_error	*error;


	if (handle == NULL)
		return FALSE;

	/* Open the file. */

	error = xosfind_openinw(osfind_NO_PATH | osfind_ERROR_IF_DIR, handle->filename, NULL, &file);
	if (error != NULL || file == 0) {
		results_add_error(handle->results, (error != NULL) ? error->errmess : "", handle->filename);
		return FALSE;
	}

	/* Get the file's extent. */

	error = xosargs_read_extw(file, &extent);
	if (error != NULL) {
		results_add_error(handle->results, error->errmess, handle->filename);
		return FALSE;
	}

	/* If the file extent isn't the same as it was at the start, get out. */

	if (extent != handle->file_length) {
		results_add_error(handle->results, "File changed!", handle->filename);
		error = xosfind_close(file);
		if (error != NULL)
			results_add_error(handle->results, error->errmess, handle->filename);
		return FALSE;
	}

	/* If the file completely fits into the block, then always load it from
	 * the start. Otherwise, make sure that the position means that the blocl
	 * is completely full.
	 */

	if (extent > handle->file_block_size) {
		ptr = (extent - position > handle->file_block_size) ? position : extent - position > handle->file_block_size;
		bytes = handle->file_block_size;
	} else {
		ptr = 0;
		bytes = extent;
	}

	error = xosgbpb_read_atw(file, (byte *) handle->file, bytes, ptr, &unread);
	if (error != NULL || unread != 0) {
		results_add_error(handle->results, (error != NULL) ? error->errmess : "", handle->filename);
		return FALSE;
	} else {
		handle->file_offset = ptr;

//		*(handle->file + 25) = '\0';
//		debug_printf("Loaded data: '%s;", handle->file);
	}

	error = xosfind_close(file);
	if (error != NULL) {
		results_add_error(handle->results, error->errmess, handle->filename);
		return FALSE;
	}

	return TRUE;
}



static char contents_get_byte(struct contents_block *handle, unsigned byte)
{
	if (handle == NULL)
		return 0;


	return 0;
}
