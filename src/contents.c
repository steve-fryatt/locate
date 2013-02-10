/* Copyright 2013, Stephen Fryatt
 *
 * Wildcard search based on code by Alessandro Cantatore:
 * http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
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

#include <ctype.h>
#include <string.h>
//#include <stdlib.h>
//#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/osargs.h"
#include "oslib/osfile.h"
#include "oslib/osfind.h"
#include "oslib/osgbpb.h"
#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/string.h"

/* Application header files */

#include "contents.h"

#include "flexutils.h"
#include "objdb.h"
#include "results.h"


#define CONTENTS_FILENAME_SIZE 256						/**< The space in bytes initially allocated to take filenames.	*/
#define CONTENTS_FILE_BUFFER_SIZE 4						/**< The space in KBytes allocated to load file contents.	*/
#define CONTENTS_FILE_BACKSPACE 8						/**< 1/n of the buffer space retained when block moves forward.	*/


/**
 * The block describing a contents search engine.
 */

struct contents_block {
	struct objdb_block		*objects;				/**< The object database related to the search.			*/
	struct results_window		*results;				/**< The results window related to the search.			*/

	/* File details. */

	unsigned			key;					/**< The ObjectDB key of the file being searched.		*/

	osbool				error;					/**< TRUE if an error has occurred; else FALSE.			*/

	char				*filename;				/**< Flex block containing the name of the current file.	*/

	char				*file;					/**< Flex block containing the file data or a subset of it.	*/
	size_t				file_block_size;			/**< The number of bytes allocated to the file block.		*/

	int				file_extent;				/**< The number of bytes of file data on disc.			*/
	int				file_offset;				/**< File ptr for the start of the data in memory.		*/

	/* Search details. */

	char				*text;					/**< Flex block holding the text to match.			*/

	osbool				any_case;				/**< TRUE to match case-insensitively.				*/
	osbool				invert;					/**< TRUE to match files which do not contain the text.		*/

	int				pointer;				/**< Pointer to the current search byte.			*/
	osbool				matched;				/**< TRUE if the file has matched the text at least once.	*/
};


static osbool	contents_test_wildcard(struct contents_block *handle, int pointer);
static osbool	contents_load_file_chunk(struct contents_block *handle, int position);
static char	contents_get_byte(struct contents_block *handle, int pointer);


/**
 * Create a new contents search engine.
 *
 * \param *objects		The object database to which the search will belong.
 * \param *results		The results window to which the search will report.
 * \param *text			Pointer to the string to be matched.
 * \param any_case		TRUE to match case insensitively; else FALSE.
 * \param invert		TRUE to invert the search logic.
 * \return			The new contents search engine handle, or NULL.
 */

struct contents_block *contents_create(struct objdb_block *objects, struct results_window *results, char *text, osbool any_case, osbool invert)
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

	new->any_case = any_case;
	new->invert = invert;

	new->file_block_size = 1024 * CONTENTS_FILE_BUFFER_SIZE;

	new->key = OBJDB_NULL_KEY;

	new->filename = NULL;
	new->file = NULL;
	new->text = NULL;

	new->error = FALSE;

	/* Process the search string to remove all leading wildcards, then store
	 * it in a flex block and finally remove all trailing wildcards.
	 */

	while (*text == '#' || *text == '*')
		text++;

	if (flexutils_store_string((flex_ptr) &(new->text), text)) {
		text = new->text + strlen(new->text) - 1;

		while (text >= new->text && (*text == '#' || *text == '*'))
			*text-- = '\0';

		if (new->any_case)
			string_toupper(new->text);

		debug_printf("String to match: '%s', inverted=%d", new->text, new->invert);
	} else {
		mem_ok = FALSE;
	}

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
	new->file_extent = 0;

	new->pointer = 0;
	new->matched = FALSE;

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

	if (handle->text != NULL)
		flex_free((flex_ptr) &(handle->text));

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

	handle->file_extent = 0;
	handle->file_offset = 0;

	handle->error = FALSE;

	handle->pointer = 0;
	handle->matched = FALSE;

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

	error = xosfile_read_no_path(handle->filename, NULL, NULL, NULL, &handle->file_extent, NULL);
	if (error != NULL) {
		results_add_error(handle->results, error->errmess, handle->filename);
		handle->file_extent = 0;
		return FALSE;
	}

	/* Load the first chunk of data from the file. */

	contents_load_file_chunk(handle, 0);

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
	char	byte;

	if (handle == NULL)
		return TRUE;

	debug_printf("Starting contents search loop %d at time %u", handle->pointer, os_read_monotonic_time());

	while (!handle->error && (!handle->invert || !handle->matched) && (handle->pointer < handle->file_extent) &&
			(os_read_monotonic_time() < end_time)) {
		byte = contents_get_byte(handle, handle->pointer);

		if (byte == *(handle->text) && contents_test_wildcard(handle, handle->pointer)) {
			debug_printf("Match at offset %d", handle->pointer);
			if (!handle->invert && !handle->matched)
				results_add_file(handle->results, handle->key);

			handle->matched = TRUE;
		}

		handle->pointer++;
	}

	debug_printf("Finishing contents search loop at time %u", os_read_monotonic_time());

	if (handle->error || (handle->matched && handle->invert) || handle->pointer >= handle->file_extent) {
		if (handle->invert && !handle->matched && !handle->error)
			results_add_file(handle->results, handle->key);

		if (matched != NULL)
			*matched = (handle->invert) ? !handle->matched : handle->matched;

		return TRUE;
	}

	return FALSE;
}





static osbool contents_test_wildcard(struct contents_block *handle, int pointer)
{
	int		i;
	osbool		star = FALSE;
	char		*pattern = handle->text;

	/* NB: Taking a copy of handle->text into pattern assumes that there will
	 *     be NO FLEX ACTIVITY for the duration of the wildcard match. If anything
	 *     does move the flex heap, then this will break messily!
	 */

	debug_printf("Entering wildcard routine");

loopStart:
	for (i = 0; pattern[i] != '\0' && (pointer + i) < handle->file_extent; i++) {
		debug_printf("Loop i=%d, for pattern %c", i, pattern[i]);

		switch (pattern[i]) {
		case '?':
			break;

		case '*':
			star = TRUE;

			pointer += i;
			pattern += i;

			do {
				++pattern;
			} while (*pattern == '*');

			if (!*pattern) {
				debug_printf("Returning TRUE");
				return TRUE;
			}

			goto loopStart;

		default:
			debug_printf("Testing char=%c against pattern=%c", contents_get_byte(handle, pointer + i), pattern[i]);
			if (contents_get_byte(handle, pointer + i) != pattern[i])
				goto starCheck;

			break;
		}
	}

	while (pattern[i] == '*')
		++i;

	debug_printf("Returning %s", (!pattern[i]) ? "TRUE" : "FALSE");

	return (!pattern[i]);

starCheck:
	if (!star) {
		debug_printf("Returning FALSE");
		return FALSE;
	}

	pointer++;

	goto loopStart;
}


/**
 * Load a chunk of the file into the memory buffer, starting at the given file
 * position.
 *
 * \param *handle		The contents search handle.
 * \param position		The file pointer for the start of the data.
 * \return			TRUE if successful; FALSE if an error occurs.
 */

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

	if (extent != handle->file_extent) {
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
		ptr = ((extent - position) > handle->file_block_size) ? position : extent - handle->file_block_size;
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
	}

	error = xosfind_close(file);
	if (error != NULL) {
		results_add_error(handle->results, error->errmess, handle->filename);
		return FALSE;
	}

	return TRUE;
}


/**
 * Return the character from a given location within the file, loading the
 * necessary data into memory if required. If the search is case-insensitive,
 * returned characters are adjusted accordingly.
 *
 * \param *handle		The contents search handle.
 * \param pointer		The file pointer of the required byte.
 * \return			The required character.
 */

static char contents_get_byte(struct contents_block *handle, int pointer)
{
	char	byte;

	if (handle == NULL || handle->error)
		return 0;

	if ((pointer < handle->file_offset || pointer >= handle->file_offset + handle->file_block_size) &&
			!contents_load_file_chunk(handle, pointer - (handle->file_block_size / CONTENTS_FILE_BACKSPACE))) {
		handle->error = TRUE;
		return 0;
	}

	if (pointer < handle->file_offset || pointer >= handle->file_offset + handle->file_block_size)
		return 0;

	byte = handle->file[pointer - handle->file_offset];

	if (handle->any_case)
		byte = toupper(byte);

	return byte;
}

