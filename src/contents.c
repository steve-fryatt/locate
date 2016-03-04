/* Copyright 2013-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
#define CONTENTS_FILE_BUFFER_SIZE 100						/**< The space in KBytes allocated to load file contents.	*/
#define CONTENTS_FILE_BACKSPACE 8						/**< 1/n of the buffer space retained when block moves forward.	*/


/**
 * The block describing a contents search engine.
 */

struct contents_block {
	struct objdb_block		*objects;				/**< The object database related to the search.			*/
	struct results_window		*results;				/**< The results window related to the search.			*/

	/* File details. */

	unsigned			key;					/**< The ObjectDB key of the file being searched.		*/
	unsigned			parent;					/**< The Results parent of the file being searched.		*/

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


static osbool	contents_test_wildcard(struct contents_block *handle, int pointer, int *end);
static osbool	contents_load_file_chunk(struct contents_block *handle, int position);
static char	contents_get_byte(struct contents_block *handle, int pointer, osbool ignore_case);
static osbool	contents_get_context(struct contents_block *handle, int start, int end, int context, char *buffer, size_t length);


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

#ifdef DEBUG
	debug_printf("Created new content search: 0x%x", new);
#endif

	new->objects = objects;
	new->results = results;

	new->any_case = any_case;
	new->invert = invert;

	new->file_block_size = 1024 * CONTENTS_FILE_BUFFER_SIZE;

	new->key = OBJDB_NULL_KEY;
	new->parent = RESULTS_NULL;

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

#ifdef DEBUG
		debug_printf("String to match: '%s', inverted=%d", new->text, new->invert);
#endif
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

#ifdef DEBUG
	debug_printf("Destroyed content search: 0x%x", handle);
#endif

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
	handle->parent = RESULTS_NULL;

	handle->file_extent = 0;
	handle->file_offset = 0;

	handle->error = FALSE;

	handle->pointer = 0;
	handle->matched = FALSE;

#ifdef DEBUG
	debug_printf("Processing object content: key = %d", key);
#endif

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
		results_add_error(handle->results, error->errmess, handle->key);
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
	char	byte, buffer[1024];
	int	end;

	if (handle == NULL)
		return TRUE;

#ifdef DEBUG
	debug_printf("Starting contents search loop %d at time %u", handle->pointer, os_read_monotonic_time());
#endif

	while (!handle->error && (!handle->invert || !handle->matched) && (handle->pointer < handle->file_extent) &&
			(os_read_monotonic_time() < end_time)) {
		byte = contents_get_byte(handle, handle->pointer, TRUE);

		end = -1;

		if (byte == *(handle->text) && contents_test_wildcard(handle, handle->pointer, &end)) {
#ifdef DEBUG
			debug_printf("Match at offset %d", handle->pointer);
#endif

			if (!handle->invert) {
				if (!handle->matched)
					handle->parent = results_add_file(handle->results, handle->key);

				if (contents_get_context(handle, handle->pointer, end, 30, buffer, 1024))
					results_add_contents(handle->results, handle->key, handle->parent, buffer);
			}

			handle->matched = TRUE;
			handle->pointer = end;
		} else if (end != -1 && end >= handle->file_extent - 1) {
			/* If the wildcard matching reached the end of the file, then give
			 * up because we won't manage to find a match.
			 */

			handle->pointer = end;
		}

		handle->pointer++;
	}

#ifdef DEBUG
	debug_printf("Finishing contents search loop at time %u", os_read_monotonic_time());
#endif

	if (handle->error || (handle->matched && handle->invert) || handle->pointer >= handle->file_extent) {
		if (handle->invert && !handle->matched && !handle->error)
			results_add_file(handle->results, handle->key);

		if (matched != NULL)
			*matched = (handle->invert) ? !handle->matched : handle->matched;

		return TRUE;
	}

	return FALSE;
}


/**
 * Run a wildcard test on the file, starting at the given pointer.
 *
 * \param *handle		The contents search handle.
 * \param pointer		The file pointer at which to start the search.
 * \param *end			Pointer to a variable to take the end pointer for the match.
 * \return			TRUE if the pattern matches; FALSE if not.
 */

static osbool contents_test_wildcard(struct contents_block *handle, int pointer, int *end)
{
	int		i;
	osbool		star = FALSE;
	char		*pattern = handle->text;

	/* NB: Taking a copy of handle->text into pattern assumes that there will
	 *     be NO FLEX ACTIVITY for the duration of the wildcard match. If anything
	 *     does move the flex heap, then this will break messily!
	 */

#ifdef DEBUG
	debug_printf("Entering wildcard routine");
#endif

	if (end != NULL)
		*end = 0;

loopStart:
	for (i = 0; pattern[i] != '\0' && (pointer + i) < handle->file_extent; i++) {
#ifdef DEBUG
		debug_printf("Loop i=%d, for pattern %c", i, pattern[i]);
#endif

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
				if (end != NULL)
					*end = pointer + i - 1;
#ifdef DEBUG
				debug_printf("Returning TRUE");
#endif
				return TRUE;
			}

			goto loopStart;
			break;

		default:
#ifdef DEBUG
			debug_printf("Testing char=%c against pattern=%c", contents_get_byte(handle, pointer + i, TRUE), pattern[i]);
#endif
			if (contents_get_byte(handle, pointer + i, TRUE) != pattern[i])
				goto starCheck;

			break;
		}
	}

	if (end != NULL)
		*end = pointer + i - 1;

	while (pattern[i] == '*')
		++i;

#ifdef DEBUG
	debug_printf("Returning %s", (!pattern[i]) ? "TRUE" : "FALSE");
#endif

	return (!pattern[i]);

starCheck:
	if (!star) {
		if (end != NULL)
			*end = pointer + i - 1;
#ifdef DEBUG
		debug_printf("Returning FALSE");
#endif
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

	if (position < 0)
		position = 0;

	/* Open the file. */

	error = xosfind_openinw(osfind_NO_PATH | osfind_ERROR_IF_DIR, handle->filename, NULL, &file);
	if (error != NULL || file == 0) {
		results_add_error(handle->results, (error != NULL) ? error->errmess : "Failed to open file", handle->key);
		return FALSE;
	}

	/* Get the file's extent. */

	error = xosargs_read_extw(file, &extent);
	if (error != NULL) {
		results_add_error(handle->results, error->errmess, handle->key);
		return FALSE;
	}

	/* If the file extent isn't the same as it was at the start, get out. */

	if (extent != handle->file_extent) {
		results_add_error(handle->results, "File changed!", handle->key);
		error = xosfind_close(file);
		if (error != NULL)
			results_add_error(handle->results, error->errmess, handle->key);
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
		results_add_error(handle->results, (error != NULL) ? error->errmess : "Error reading from file", handle->key);
		return FALSE;
	} else {
		handle->file_offset = ptr;
	}

	error = xosfind_close(file);
	if (error != NULL) {
		results_add_error(handle->results, error->errmess, handle->key);
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
 * \param ignore_case		TRUE to adjust characters to be case insenstitive.
 * \return			The required character.
 */

static char contents_get_byte(struct contents_block *handle, int pointer, osbool ignore_case)
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

	if (ignore_case && handle->any_case)
		byte = toupper(byte);

	return byte;
}


/**
 * Extract the context of a match from the current file and place it into the
 * supplied buffer.
 *
 * \param *handle		The contents search handle.
 * \param start			The pointer to the first character of the match.
 * \param end			The pointer to the last character of the match.
 * \param context		The number of characters to include either side.
 * \param *buffer		Pointer to the buffer to take the match.
 * \param length		The size of the supplied buffer.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool contents_get_context(struct contents_block *handle, int start, int end, int context, char *buffer, size_t length)
{
	int prefix, postfix, match_length, skip_from, skip_length, ptr, i;

	if (handle == NULL || buffer == NULL)
		return FALSE;

	/* Get the number of prefix characters to use.  Include up to the required
	 * context, stopping on the first non-printing character.
	 */

	prefix = 0;

	while (((start - (prefix + 1)) >= 0) && isprint(contents_get_byte(handle, start - (prefix + 1), FALSE)) && (prefix < context))
		prefix++;

	/* Get the number of postfix characters to use.  Include up to the required
	 * context, stopping on the first non-printing character.
	 */

	postfix = 0;

	while (((end + (postfix + 1)) < handle->file_extent) && isprint(contents_get_byte(handle, end + (postfix + 1), FALSE)) && (postfix < context))
		postfix++;

	match_length = (end - start) + 1;

	/* If the match plus context is too big to fit the available space, trim
	 * the prefix and postfix, and then if that fails take a chunk from the
	 * centre of the match itself.
	 */

	skip_from = -1;
	skip_length = 0;

	if (prefix + match_length + postfix + 1 > length) {
		while ((postfix > prefix) && ((prefix + match_length + postfix + 1) > length))
			postfix--;

		while ((prefix > postfix) && ((prefix + match_length + postfix + 1) > length))
			prefix--;

		/* By now, the same prefix and postfix are in use. If it still won't
		 * fit, trim both back to the length of the ellipsis.
		 */

		while ((prefix > 3) && ((prefix + match_length + postfix + 1) > length)) {
			prefix--;
			postfix--;
		}

		/* If it's still to big, take a chunk out of the middle of the
		 * match.
		 */

		if ((prefix + match_length + postfix + 1) > length) {
			skip_length = (prefix + match_length + postfix + 4) - length; // 4 = 1 + Ellipsis
			skip_from = ((match_length - skip_length) / 2) + start;
		}
	}

	/* Copy the context from the file into the buffer. */

	for (ptr = start - prefix, i = 0; (ptr <= (end + postfix)) && (i < (length - 1)); ptr++) {
		if (ptr == skip_from) {
			ptr += skip_length;
			buffer[i++] = '.';
			buffer[i++] = '.';
			buffer[i++] = '.';
		} else {
			buffer[i++] = contents_get_byte(handle, ptr, FALSE);
		}
	}

	buffer[i] = '\0';

	/* Add ellipses to the start and end of the string if required. */

	if (((start - prefix) > 0) && isprint(contents_get_byte(handle, start - (prefix + 1), FALSE))) {
		buffer[0] = '.';
		buffer[1] = '.';
		buffer[2] = '.';
	}

	if (((end + postfix) < (handle->file_extent - 1)) && isprint(contents_get_byte(handle, end + (postfix + 1), FALSE))) {
		buffer[i - 1] = '.';
		buffer[i - 2] = '.';
		buffer[i - 3] = '.';
	}

	return TRUE;
}

