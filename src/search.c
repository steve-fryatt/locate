/* Copyright 2012-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: search.c
 *
 * File search routines.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* OSLib Header files. */

#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/osgbpb.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/string.h"

/* Application header files. */

#include "search.h"

#include "contents.h"
#include "flexutils.h"
#include "ignore.h"
#include "objdb.h"
#include "results.h"


#define SEARCH_ALLOC_STACK 20							/**< The directory depth in which to allocate stack space.		*/

#define SEARCH_MAX_FILENAME 256							/**< The maximum length of a file (object) name.			*/
#define SEARCH_BLOCK_SIZE 4096							/**< The amount of memory to allocate to OS_GBPB.			*/

#define STATUS_LENGTH 128							/**< The maximum size of the status bar text field.			*/
#define ERROR_LENGTH 128							/**< The maximum size of the error message text.			*/
#define NUM_BUF_LENGTH 20							/**< The size of a buffer used to render numbers.			*/

#define SEARCH_NULL 0xffffffff							/**< 'NULL' value for use with the unsigned flex block offsets.		*/

/* A data structure to hold the search stack. */

struct search_stack {
	char			filename[SEARCH_MAX_FILENAME];			/**< The filename of the current directory.				*/
	byte			info[SEARCH_BLOCK_SIZE];			/**< Block for OS_GBPB 10.						*/

	int			read;						/**< The number of files read at the last OS_GBPB call.			*/
	int			context;					/**< The context for the next OS_GBPB call.				*/
	int			next;						/**< The number of the next item to read from the block.		*/
	unsigned		data_offset;					/**< Offset to the data for the next item to be read from the block.	*/

	unsigned		key;						/**< The object database key of the item.				*/
	unsigned		parent;						/**< The object database key of the parent item.			*/

	unsigned		filetype;					/**< The filetype of the current file.					*/

	osbool			file_active;					/**< TRUE if the file is still active; FALSE if fully processed.	*/
	osbool			contents_active;				/**< TRUE if the contents engine is in the middle of a search.		*/
};

/* A data structure defining a search. */

struct search_block {
	struct file_block	*file;						/**< The file to which the search belongs.				*/
	struct objdb_block	*objects;					/**< The object database to store data in.				*/
	struct results_window	*results;					/**< Results module to output results to.				*/

	osbool			active;						/**< TRUE if the search is active; else FALSE.				*/

	osbool			include_imagefs;				/**< TRUE to search inside Image Filing Systems; else FALSE.		*/
	osbool			store_all;					/**< TRUE to save all objects in the database; FALSE for matches.	*/

	int			path_count;					/**< The number of search paths remaining to use.			*/
	char			*paths;						/**< Line containing the full set of search paths.			*/
	char			**path;						/**< Index to each of the search paths.					*/

	struct search_stack	*stack;						/**< The search stack.							*/
	unsigned		stack_level;					/**< The current stack level.						*/
	unsigned		stack_size;					/**< The amount of stack levels currently claimed.			*/

	unsigned		file_count;					/**< The number of files found in the search.				*/
	unsigned		error_count;					/**< The number of errors encountered during the search.		*/

	/* Search Parameters */

	struct ignore_block	*ignore_list;					/**< Handle of the Ignore List, or NULL if there isn't a list defined.	*/

	osbool			include_files;					/**< TRUE to include files in the results; FALSE to exclude.		*/
	osbool			include_directories;				/**< TRUE to include directories in the results; FALSE to exclude.	*/
	osbool			include_applications;				/**< TRUE to include applications in the results; FALSE to exclude.	*/

	osbool			test_filename;					/**< TRUE to test the filename; FALSE to ignore.			*/
	char			*filename;					/**< Pointer to a flex block with the filename to test; NULL if none.	*/
	osbool			filename_any_case;				/**< TRUE if the filename should be tested case insenitively.		*/
	osbool			filename_logic;					/**< The required result of filename comparisons.			*/

	osbool			test_size;					/**< TRUE to test the file size; FALSE to ignore.			*/
	osbool			size_logic;					/**< The required result of the size comparison.			*/
	int			minimum_size;					/**< The minimum allowable file size.					*/
	int			maximum_size;					/**< The maximum allowable file size.					*/

	osbool			test_date;					/**< TRUE to test the datestamp; FALSE to ignore.			*/
	osbool			date_logic;					/**< The required result of the date comparison.			*/
	unsigned		minimum_date_lo;				/**< The low four bytes of the minimum allowable datestamp.		*/
	unsigned		minimum_date_hi;				/**< The high byte of the minimum allowable datestamp.			*/
	unsigned		maximum_date_lo;				/**< The low four bytes of the maximum allowable datestamp.		*/
	unsigned		maximum_date_hi;				/**< The high byte of the maximum allowable datestamp.			*/
	osbool			date_as_age;					/**< TRUE if the date is expressed as age, for the title flags.		*/

	osbool			test_filetype;					/**< TRUE to test the filetype; FALSE to ignore.			*/
	bits			filetypes[4096 / (8 * sizeof(bits))];		/**< Bitmask for the filetype matching.					*/
	osbool			include_untyped;				/**< TRUE if untyped files should match the type test.			*/

	osbool			test_attributes;				/**< TRUE to test the file attributes; FALSE to ignore.			*/
	bits			attributes;					/**< The required attributes.						*/
	bits			attributes_mask;				/**< The bit mask for the required attributes (set to test).		*/

	osbool			test_contents;					/**< TRUE to test the contents; FALSE to ignore.			*/
	struct contents_block	*contents_engine;				/**< Handle for the contents search engine; NULL if no contents search.	*/
	char			*contents;					/**< Pointer to a flex block with the contents to match; NULL if none.	*/
	osbool			contents_any_case;				/**< TRUE if the contents should be tested case insenitively.		*/
	osbool			contents_logic;					/**< The required result of contents comparisons.			*/


	/* Block List */

	struct search_block	*next;						/**< The next active search in the active search list.			*/
};


/* Global variables. */

static struct search_block	*search_active = NULL;				/**< A linked list of all active searches.				*/
static int			search_searches_active = 0;			/**< A count of active searches.					*/

/* Local function prototypes. */

static osbool		search_poll(struct search_block *search, os_t end_time);
static unsigned		search_add_stack(struct search_block *search);
static unsigned		search_drop_stack(struct search_block *search);


/**
 * Create a new search.
 *
 * The list of paths is assumed to have been passed to search_validate_paths()
 * before this call is made; no further tests are done on the constituent
 * parts.
 *
 * \param *file			The file block to which the search belongs.
 * \param *objects		The object database to store information in.
 * \param *results		The results window to which output should be
 *				directed.
 * \param *path			The path(s) to search, comma-separated.
 * \return			The search handle, or NULL on failure.
 */

struct search_block *search_create(struct file_block *file, struct objdb_block *objects, struct results_window *results, char *path)
{
	struct search_block	*new = NULL;
	osbool			mem_ok = TRUE;
	int			i, paths;

	/* Test and count the supplied paths. */

	if (path == NULL)
		return NULL;

	paths = 1;
	for (i = 0; path[i] != '\0'; i++)
		if (path[i] == ',')
			paths++;

	/* Allocate all the memory that we require. */

	new = heap_alloc(sizeof(struct search_block));
	if (new == NULL)
		mem_ok = FALSE;

	if (mem_ok) {
		new->stack = NULL;
		new->paths = NULL;
		new->path = NULL;

		if ((new->paths = heap_strdup(path)) == NULL)
			mem_ok = FALSE;

		if ((new->path = heap_alloc(paths * sizeof(char *))) == NULL)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->stack), SEARCH_ALLOC_STACK * sizeof(struct search_stack)) == 0)
			mem_ok = FALSE;
	}

	/* If any memory allocations failed, free anything that did get
	 * claimed and exit with an error.
	 */

	if (!mem_ok) {
		if (new != NULL && new->stack != NULL)
			flex_free((flex_ptr) &(new->stack));

		if (new != NULL && new->path != NULL)
			heap_free(new->path);

		if (new != NULL && new->paths != NULL)
			heap_free(new->paths);

		if (new != NULL)
			heap_free(new);

		error_msgs_report_error("NoMemSearchCreate");
		return NULL;
	}

	/* Initialise the search contents. */

	new->file = file;
	new->objects = objects;
	new->results = results;

	new->active = FALSE;
	new->next = NULL;

	new->stack_size = SEARCH_ALLOC_STACK;
	new->stack_level = 0;

	new->file_count = 0;
	new->error_count = 0;

	/* The Search criteria. */

	new->ignore_list = NULL;

	new->include_imagefs = FALSE;
	new->store_all = FALSE;

	new->include_files = TRUE;
	new->include_directories = TRUE;
	new->include_applications = TRUE;

	new->test_filename = FALSE;
	new->filename = NULL;
	new->filename_any_case = FALSE;
	new->filename_logic = TRUE;

	new->test_size = FALSE;
	new->size_logic = TRUE;
	new->minimum_size = 0x0;
	new->maximum_size = 0x7fffffff;

	new->test_date = FALSE;
	new->date_logic = TRUE;

	new->test_attributes = FALSE;
	new->attributes = 0x0u;
	new->attributes_mask = 0x0u;

	new->test_filetype = FALSE;
	new->include_untyped = FALSE;

	new->test_contents = FALSE;
	new->contents_engine = NULL;

	new->path_count = paths;

	/* Split the path list into separate paths and link them into .path[]
	 * in reverse order so that .path_count can be decremented during
	 * the search.
	 */

	new->path[--paths] = new->paths;

	for (i = 0; new->paths[i] != '\0'; i++) {
		if (new->paths[i] == ',' && paths > 0) {
			new->path[--paths] = new->paths + i + 1;
			new->paths[i] = '\0';
		}
	}

	return new;
}


/**
 * Destroy a search.
 *
 * \param *handle		The handle of the search to destroy.
 */

void search_destroy(struct search_block *search)
{
	if (search == NULL)
		return;

	/* If the search is active, remove it from the active list. */

	if (search->active)
		search_stop(search);

	/* Free any memory allocated to the search. */

	if (search->stack != NULL)
		flex_free((flex_ptr) &(search->stack));

	if (search->path != NULL)
		heap_free(search->path);

	if (search->paths != NULL)
		heap_free(search->paths);

	if (search->filename != NULL)
		flex_free((flex_ptr) &(search->filename));

	/* Remove the ignore list if present. */

	if (search->ignore_list != NULL)
		ignore_destroy(search->ignore_list);

	/* Remove the contents search if present. */

	if (search->contents_engine != NULL)
		contents_destroy(search->contents_engine);

	heap_free(search);
}


/**
 * Set specific options for a search.
 *
 * \param *search		The search to set the options for.
 * \param search_imagefs	TRUE to search into ImageFSs; FALSE to skip.
 * \param store_all		TRUE to store all file details; FALSE to store matches.
 * \param full_info		TRUE to show full info in the results; FALSE to hide it.
 * \param include_files		TRUE to include files; FALSE to exclude.
 * \param include_directories	TRUE to include directories; FALSE to exclude.
 * \param include_applications	TRUE to include applications; FALSE to exclude.
 */

void search_set_options(struct search_block *search, osbool search_imagefs, osbool store_all, osbool full_info,
		osbool include_files, osbool include_directories, osbool include_applications)
{
	if (search == NULL)
		return;

	search->include_imagefs = search_imagefs;
	search->store_all = store_all;

	search->include_files = include_files;
	search->include_directories = include_directories;
	search->include_applications = include_applications;

	results_set_options(search->results, full_info);
}


/**
 * Set the filename matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param *filename		Pointer to the filename to match.
 * \param any_case		TRUE to match case insensitively; else FALSE.
 * \param invert		TRUE to match files whose names don't match; else FALSE.
 */

void search_set_filename(struct search_block *search, char *filename, osbool any_case, osbool invert)
{
	if (search == NULL)
		return;

	search->test_filename = TRUE;
	search->filename_logic = !invert;
	flexutils_store_string((flex_ptr) &(search->filename), filename);
	search->filename_any_case = any_case;
}


/**
 * Set the filesize matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param in_limits		TRUE to match when in limits; FALSE to negate logic.
 * \param minimum		The minimum size to match in bytes.
 * \param maximum		The maximum size to match in bytes.
 */

void search_set_size(struct search_block *search, osbool in_limits, int minimum, int maximum)
{
	if (search == NULL)
		return;

	search->test_size = TRUE;
	search->size_logic = in_limits;
	search->minimum_size = minimum;
	search->maximum_size = maximum;
}


/**
 * Set the datestamp matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param minimum		The minimum date to match.
 * \param maximum		The maximum date to match.
 * \param in_limits		TRUE to match when in limits; FALSE to negate logic.
 * \param as_age		TRUE to flag the parameters as age, FALSE for date.
 */

void search_set_date(struct search_block *search, osbool in_limits, os_date_and_time minimum, os_date_and_time maximum, osbool as_age)
{
	if (search == NULL)
		return;

	search->test_date = TRUE;
	search->date_logic = in_limits;
	search->minimum_date_lo = minimum[0] | (minimum[1] << 8) | (minimum[2] << 16) | (minimum[3] << 24);
	search->minimum_date_hi = minimum[4];
	search->maximum_date_lo = maximum[0] | (maximum[1] << 8) | (maximum[2] << 16) | (maximum[3] << 24);
	search->maximum_date_hi = maximum[4];

	search->date_as_age = as_age;
}


/**
 * Set the filetype matching options for a search.
 *
 * Type matching is done via a bitmask, with one bit in an array of 32-bit words
 * for each filetype. Bit 0 of the first word is for 0x000; Bit 31 of the 128th
 * word is for 0xfff.  Bits are *always* set to allow a type, so the invert
 * option starts by setting all the bits and then clearing those that are excluded.
 *
 * \param *search		The search to set the options for.
 * \param type_list[]		An 0xffffffffu terminated list of filetypes.
 *				0x1000u is used to signify "untyped".
 * \param invert		TRUE to exclude listed types; FALSE to include.
 */

void search_set_types(struct search_block *search, unsigned type_list[], osbool invert)
{
	int	i;


	if (search == NULL || type_list == NULL)
		return;

	search->test_filetype = TRUE;

	/* Set the bitmask to the default state. */

	for (i = 0; i < 4096 / (8 * sizeof(bits)); i++)
		search->filetypes[i] = (invert) ? 0xffffffffu : 0x0u;

	search->include_untyped = invert;

	/* Set or clear individual bits to suit the type list passed in. */

	i = 0;

	while (type_list[i] != 0xffffffffu) {
		if (type_list[i] == 0x1000u) {
			search->include_untyped = !invert;
		} else if ((type_list[i] >= 0x000u) && (type_list[i] <= 0xfffu)) {
			if (invert)
				search->filetypes[type_list[i] / (8 * sizeof(bits))] &= ~(1 << (type_list[i] % (8 * sizeof(bits))));
			else
				search->filetypes[type_list[i] / (8 * sizeof(bits))] |= (1 << (type_list[i] % (8 * sizeof(bits))));
		}

		i++;
	}
}


/**
 * Set the attribute matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param mask			A mask setting bits to add to the test.
 * \param required		The required states for the masked bits.
 */

void search_set_attributes(struct search_block *search, fileswitch_attr mask, fileswitch_attr required)
{
	if (search == NULL && mask != 0x0u)
		return;

	search->test_attributes = TRUE;
	search->attributes_mask |= mask;
	search->attributes |= required;
}


/**
 * Set the contents matching options for a search.
 *
 * \param *search		The search to set the options for.
 * \param *contents		Pointer to the content string to match.
 * \param any_case		TRUE to match case insensitively; else FALSE.
 * \param invert		TRUE to match files whose names don't match; else FALSE.
 */

void search_set_contents(struct search_block *search, char *contents, osbool any_case, osbool invert)
{
	if (search == NULL)
		return;

	search->test_contents = TRUE;
	search->contents_engine = contents_create(search->objects, search->results, contents, any_case, invert);
}


/**
 * Make a search active so that it will run on subsequent calls to search_poll().
 *
 * \param *search		The handle of the search to make active.
 */

void search_start(struct search_block *search)
{
	unsigned	stack, object_key;
	char		title[256], flag[10], flags[10];


	if (search == NULL || search->path_count == 0)
		return;

	/* Set the window title up. */

	*flags = '\0';

	if (search->test_size == TRUE) {
		msgs_lookup("SizeFlag", flag, sizeof(flag));
		strcat(flags, flag);
	}

	if (search->test_date == TRUE) {
		if (search->date_as_age)
			msgs_lookup("AgeFlag", flag, sizeof(flag));
		else
			msgs_lookup("DateFlag", flag, sizeof(flag));
		strcat(flags, flag);
	}

	if (search->include_files == FALSE || search->include_directories == FALSE || search->include_applications == FALSE ||
			search->test_filetype == TRUE) {
		msgs_lookup("TypeFlag", flag, sizeof(flag));
		strcat(flags, flag);
	}

	if (search->test_attributes == TRUE) {
		msgs_lookup("AttrFlag", flag, sizeof(flag));
		strcat(flags, flag);
	}

	if (search->test_contents == TRUE) {
		msgs_lookup("ContFlag", flag, sizeof(flag));
		strcat(flags, flag);
	}

	msgs_param_lookup("ResWindTitle", title, sizeof(title), (search->filename != NULL) ? search->filename: "", flags, NULL, NULL);

	results_set_title(search->results, title);

	/* Allocate a search stack and set up the first search folder. */

	if ((stack = search_add_stack(search)) == SEARCH_NULL)
		return;

	string_copy(search->stack[stack].filename, search->path[--search->path_count], SEARCH_MAX_FILENAME);
	object_key = objdb_add_root(search->objects, search->stack[stack].filename);
	search->stack[stack].parent = object_key;

	/* Flag the search as active. */

	search->active = TRUE;

	search_searches_active++;

	/* Link the search into the active search list. */

	search->next = search_active;
	search_active = search;
}


/**
 * Stop an active search.
 *
 * \TODO -- This does not delete the search itself. It could be useful to
 *          look at deleting the search: this would need to be done via the
 *          file module as there's a pointer to the search there.
 *
 * \param *Search		The handle of the search to stop.
 */

void search_stop(struct search_block *search)
{
	struct search_block	*active;
	char			status[STATUS_LENGTH], errors[ERROR_LENGTH], number[NUM_BUF_LENGTH];


	if (search == NULL || search->active == FALSE)
		return;

	/* Flag the search inactive. */

	search->active = FALSE;

	search_searches_active--;

	/* Free any stack that's allocated.
	 *
	 * NB: This will need to be reallocated if the search is restarted, so
	 * it's terminal.  Any search history will be lost.
	 */

	if (search->stack != NULL) {
		search->stack_size = 0;
		search->stack_level = 0;

		flex_free((flex_ptr) &(search->stack));
		search->stack = NULL;
	}

	/* Sort out the status bar text. */

	if (search->error_count == 0) {
		*errors = '\0';
	} else {
		string_printf(number, NUM_BUF_LENGTH, "%d", search->error_count);
		msgs_param_lookup("Errors", errors, ERROR_LENGTH, number, NULL, NULL, NULL);
	}

	string_printf(number, NUM_BUF_LENGTH, "%d", search->file_count);
	msgs_param_lookup("Found", status, STATUS_LENGTH, number, errors, NULL, NULL);

	results_set_status(search->results, status);

	/* If the search is at the head of the list, remove it... */

	if (search_active == search) {
		search_active = search->next;
		return;
	}

	/* ...otherwise find it in the list and remove it. */

	active = search_active;

	while (active != NULL && active->next != search)
		active = active->next;

	if (active != NULL)
		active->next = search->next;
}


/**
 * Test to see if a poll is required.
 *
 * \return			TRUE if any searches are active; else FALSE.
 */

osbool search_poll_required(void)
{
	return (search_active == NULL) ? FALSE : TRUE;
}


/**
 * Test to see if a given search is active.
 *
 * \param *search		The search to test.
 * \return			TRUE if active; else FALSE.
 */

osbool search_is_active(struct search_block *search)
{
	return (search == NULL || !search->active) ? FALSE : TRUE;
}


/**
 * Run any active searches in a Null poll.
 */

void search_poll_all(void)
{
	struct search_block	*search = search_active;
	os_t			time_slice;

	time_slice = (os_t) (config_int_read("MultitaskTimeslot") / search_searches_active);

	while (search != NULL) {
		search_poll(search, os_read_monotonic_time() + time_slice);
		search = search->next;
	}
}


/**
 * Poll an active search for a given timeslice.
 *
 * \param *search		The search to be polled.
 * \param end_time		The time by which control must return.
 * \return			TRUE or FALSE. \TODO!
 */

static osbool search_poll(struct search_block *search, os_t end_time)
{
	os_error		*error;
	int			i;
	unsigned		stack, object_key;
	byte			*original, copy[SEARCH_BLOCK_SIZE];
	osgbpb_info		*file_data = (osgbpb_info *) copy;
	char			filename[4996], leafname[SEARCH_MAX_FILENAME];
	osbool			contents_match;

	// \TODO -- The allocation of copy[] is ugly.


	if (search == NULL || !search->active)
		return TRUE;

	/* If there's no stack set up, the search must have ended. */

	if (search->stack_level == 0) {
		search_stop(search);
		return FALSE;
	}

	/* Get the current stack level, and enter the search loop. */

	stack = search->stack_level - 1;

	while (stack != SEARCH_NULL && (os_read_monotonic_time() < end_time)) {
		/* **** Bracket here to skip search if directory is on ignore??? **** */
		
		if (search->stack[stack].contents_active == FALSE) {
			/* If there are no outstanding entries in the current buffer, call
			 * OS_GBPB 10 to get another set of file details.
			 */

			error = NULL;

			if (search->stack[stack].next >= search->stack[stack].read) {
				*filename = '\0';

				for (i = 0; i <= stack; i++) {
					if (i > 0)
						strcat(filename, ".");
					strcat(filename, search->stack[i].filename);
				}

				error = xosgbpb_dir_entries_info(filename, (osgbpb_info_list *) search->stack[stack].info, 1000, search->stack[stack].context,
						SEARCH_BLOCK_SIZE, "*", &(search->stack[stack].read), &(search->stack[stack].context));

				search->stack[stack].next = 0;
				search->stack[stack].data_offset = 0;
			}

			/* Handle any errors thrown by the OS_GBPB call by dropping back out of
			 * the current directory. Because we're adding an error with a database
			 * key, it's important that we now continue out of the current loop and
			 * don't call objdb_delete_last_key() in the usual way -- otherwise, the
			 * key that's linked to the error will probably get removed from the
			 * object database.
			 */

			if (error != NULL) {
				search->error_count++;
				results_add_error(search->results, error->errmess, search->stack[stack].parent);

				stack = search_drop_stack(search);

				continue;
			}
		}

		/* Process the buffered details. */

		while ((os_read_monotonic_time() < end_time) &&
				((search->stack[stack].contents_active == TRUE) || (search->stack[stack].next < search->stack[stack].read))) {
			if (search->stack[stack].contents_active == FALSE) {
				/* Take a copy of the current file data into static memory, so that any pointers that we
				 * use on it remain valid even if the flex heap moved around.
				 *
				 * At the head of the function, file_data = copy, so we can now access the block via a
				 * sensible data type.
				 */

				original = search->stack[stack].info + search->stack[stack].data_offset;

				for (i = 0; i < 20 || original[i] != '\0'; i++)
					copy[i] = original[i];

				copy[i] = '\0';

				/* Update the data offsets for the next file. */

				search->stack[stack].data_offset += ((i + 4) & 0xfffffffc);
				search->stack[stack].next++;

				/* Add the file to the database.
				 *
				 * The object key is saved to a local variable and then put into the stack,
				 * as the stack could move mid function call (due to the Object DB shuffling
				 * the flex heap) and this might result in the return value getting written
				 * back to the wrong place if it went straight to a flex block pointer.
				 */

				object_key = objdb_add_file(search->objects, search->stack[stack].parent, file_data);
				search->stack[stack].key = object_key;
				search->stack[stack].file_active = TRUE;

				/* Work out a filetype using the convention 0x000-0xfff, 0x1000, 0x2000, 0x3000. */

				if (file_data->obj_type == fileswitch_IS_DIR && file_data->name[0] == '!')
					search->stack[stack].filetype = osfile_TYPE_APPLICATION;
				else if (file_data->obj_type == fileswitch_IS_DIR)
					search->stack[stack].filetype = osfile_TYPE_DIR;
				else if ((file_data->load_addr & 0xfff00000u) != 0xfff00000)
					search->stack[stack].filetype = osfile_TYPE_UNTYPED;
				else
					search->stack[stack].filetype = (file_data->load_addr & osfile_FILE_TYPE) >> osfile_FILE_TYPE_SHIFT;

				/* Test the object that we have found, starting by making sure that it's not included in the ignore list. */

				if (((search->ignore_list == NULL) || (ignore_match_object(search->ignore_list, file_data->name))) &&

						/* Is the object type one that we want? */

						((((search->stack[stack].filetype >= 0x000 && search->stack[stack].filetype <= 0xfff) ||
						(search->stack[stack].filetype == osfile_TYPE_UNTYPED)) && search->include_files) ||
						((search->stack[stack].filetype == osfile_TYPE_DIR) && search->include_directories) ||
						((search->stack[stack].filetype == osfile_TYPE_APPLICATION) && search->include_applications)) &&

						/* If we're testing filename, does the name match? */

						(!search->test_filename || (string_wildcard_compare(search->filename, file_data->name, search->filename_any_case) == search->filename_logic)) &&

						/* If we're testing filesize, does it fall into range? */

						(!search->test_size || (search->stack[stack].filetype == osfile_TYPE_DIR) || (search->stack[stack].filetype == osfile_TYPE_APPLICATION) ||
								(((file_data->size >= search->minimum_size) && (file_data->size <= search->maximum_size)) == search->size_logic)) &&

						/* If we're testing date, does it fall into range? */

						(!search->test_date || (search->stack[stack].filetype == osfile_TYPE_UNTYPED) ||
								(((((file_data->load_addr & 0xffu) > search->minimum_date_hi) ||
										(((file_data->load_addr & 0xffu) == search->minimum_date_hi) &&
										(file_data->exec_addr >= search->minimum_date_lo))) &&
								(((file_data->load_addr & 0xffu) < search->maximum_date_hi) ||
										(((file_data->load_addr & 0xffu) == search->maximum_date_hi) &&
										(file_data->exec_addr <= search->maximum_date_lo)))) == search->date_logic)) &&

						/* If we're testing filetype and the type falls between 0x000 and 0xfff, is it set in the bitmask? */

						(!search->test_filetype || ((search->stack[stack].filetype >= 0x000) && (search->stack[stack].filetype <= 0xfff) &&
								((search->filetypes[search->stack[stack].filetype /
										(8 * sizeof(bits))] & (1 << (search->stack[stack].filetype % (8 * sizeof(bits))))) != 0)) ||
								((search->stack[stack].filetype == osfile_TYPE_UNTYPED) && search->include_untyped) ||
								(search->stack[stack].filetype == osfile_TYPE_APPLICATION) || (search->stack[stack].filetype == osfile_TYPE_DIR)) &&

						/* If we're testing attributes, do the bits cancel out? */

						(!search->test_attributes || (((file_data->attr ^ search->attributes) & search->attributes_mask) == 0x0u))

						) {
					*filename = '\0';

					for (i = 0; i <= stack; i++) {
						if (i > 0)
							strcat(filename, ".");
						strcat(filename, search->stack[i].filename);
					}

					strcat(filename, ".");
					strcat(filename, file_data->name);

					/* Files (and image files if not being treated as folders) get passed to the contents
					 * search if one is configured; otherwise the get added to the results window
					 * immediately.
					 */

					if (search->contents_engine != NULL && (file_data->obj_type == fileswitch_IS_FILE ||
							(!search->include_imagefs && file_data->obj_type == fileswitch_IS_IMAGE))) {
						if (contents_add_file(search->contents_engine, search->stack[stack].key))
							search->stack[stack].contents_active = TRUE;
					} else {
						search->file_count++;
						results_add_file(search->results, search->stack[stack].key);
						search->stack[stack].file_active = FALSE;
					}
				}
			}

			if (search->stack[stack].contents_active == TRUE && contents_poll(search->contents_engine, end_time, &contents_match)) {
				search->stack[stack].contents_active = FALSE;
				if (contents_match) {
					search->file_count++;
					search->stack[stack].file_active = FALSE;
				}
			}

			if (search->stack[stack].contents_active == FALSE) {
				/* If the object is a folder, recurse down into it. */

				if (file_data->obj_type == fileswitch_IS_DIR || (search->include_imagefs && file_data->obj_type == fileswitch_IS_IMAGE)) {
					/* Take a copy of the name before we shift the flex heap. */

					string_copy(leafname, file_data->name, SEARCH_MAX_FILENAME);

					stack = search_add_stack(search);

					string_copy(search->stack[stack].filename, leafname, SEARCH_MAX_FILENAME);
					search->stack[stack].parent = search->stack[stack - 1].key;

					continue;
				} else if (search->stack[stack].file_active && !search->store_all) {
					objdb_delete_last_key(search->objects, search->stack[stack].key);
					search->stack[stack].file_active = FALSE;
				}
			}
		}

		/***** Bracket down to here??? *****/

		/* If that was all the files in the current folder, return to the
		 * parent.
		 */

		if ((search->stack[stack].contents_active == FALSE) && (search->stack[stack].next >= search->stack[stack].read) &&
				(search->stack[stack].context == -1)) {
			stack = search_drop_stack(search);

			if (stack != SEARCH_NULL && search->stack[stack].file_active && !search->store_all) {
				objdb_delete_last_key(search->objects, search->stack[stack].key);
				search->stack[stack].file_active = FALSE;
			}

			continue;
		}
	}

	/* If the stack is empty, the current path has been completed.  Either prepare
	 * the next path for the next poll, or terminate the search if there are
	 * no more paths to search.
	 */

	if (stack == SEARCH_NULL) {
		if ((search->path_count > 0) && ((stack = search_add_stack(search)) != SEARCH_NULL)) {
			/* Re-allocate a search stack and set up the first search folder. */

			string_copy(search->stack[stack].filename, search->path[--search->path_count], SEARCH_MAX_FILENAME);

			object_key = objdb_add_root(search->objects, search->stack[stack].filename);
			search->stack[stack].parent = object_key;
		} else {
			search_stop(search);
		}
	}

	/* If the search continues, update the status bar. */

	if (stack != SEARCH_NULL) {
		*filename = '\0';

		for (i = 0; i <= stack; i++) {
			if (i > 0)
				strcat(filename, ".");
			strcat(filename, search->stack[i].filename);
		}

		results_set_status_template(search->results, "Searching", filename);
	}

	results_accept_lines(search->results);

	return TRUE;
}


/**
 * Claim a new line from the search stack, allocating more memory if required,
 * set it up and return its offset.
 *
 * \param *handle		The handle of the search.
 * \return			The offset of the new entry, or SEARCH_NULL.
 */

static unsigned search_add_stack(struct search_block *search)
{
	unsigned offset;

	if (search == NULL)
		return SEARCH_NULL;

	/* Make sure that there is enough space in the block to take the new
	 * level, allocating more if required.
	 */

	if (search->stack_level >= search->stack_size) {
		if (flex_extend((flex_ptr) &(search->stack), (search->stack_size + SEARCH_ALLOC_STACK) * sizeof(struct search_stack)) == 0)
			return SEARCH_NULL;

		search->stack_size += SEARCH_ALLOC_STACK;
	}

	/* Get the new level and initialise it. */

	offset = search->stack_level++;

	search->stack[offset].read = 0;
	search->stack[offset].context = 0;
	search->stack[offset].next = 0;
	search->stack[offset].data_offset = 0;
	search->stack[offset].file_active = FALSE;
	search->stack[offset].contents_active = FALSE;

	return offset;
}


/**
 * Drop back down a line on the search stack and return the new offset.
 *
 * \param *handle		The handle of the search.
 * \return			The offset of the new entry, or SEARCH_NULL.
 */

static unsigned search_drop_stack(struct search_block *search)
{
	if (search == NULL)
		return SEARCH_NULL;

	if (search->stack_level > 0)
		search->stack_level--;

	return (search->stack_level == 0) ? SEARCH_NULL : search->stack_level - 1;
}


/**
 * Validate a list of pathnames, checking that each is not null and that it
 * exists as a directory or an image file. Testing stops on an error, and
 * detals are reported to the user.
 *
 * \param *paths		Pointer to the list of paths to test.
 * \param report		TRUE to flag errors up to the user; FALSE to
 *				return quietly.
 * \return			TRUE if all paths are valid; FALSE if an error
 *				was found.
 */

osbool search_validate_paths(char *paths, osbool report)
{
	char			*copy, *path, *path_end, errbuf[256];
	osbool			done = FALSE, success = TRUE;
	fileswitch_object_type	type;
	os_error		*error;

	copy = strdup(paths);
	if (copy == NULL)
		return FALSE;

	path = copy;

	while (!done && success) {
		for (path_end = path; *path_end != '\0' && *path_end != ','; path_end++);

		if (*path_end == '\0')
			done = TRUE;
		else
			*path_end = '\0';

		if (*path != '\0') {
			error = xosfile_read_no_path(path, &type, NULL, NULL, NULL, NULL);
			if (error != NULL) {
				if (report)
					error_report_error(error->errmess);
				success = FALSE;
			} else if (type == fileswitch_NOT_FOUND || type == fileswitch_IS_FILE) {
				if (report) {
					if (strlen(path) > 200)
						strcpy(path + 197, "..."); /* There's no bounds check, but we know that the buffer is longer than 200. */
					msgs_param_lookup("BadPath", errbuf, sizeof(errbuf), path, NULL, NULL, NULL);
					error_report_info(errbuf);
				}
				success = FALSE;
			}
		} else {
			if (report)
				error_msgs_report_info("EmptyPath");
			success = FALSE;
		}

		path = path_end + 1;
	}

	free(copy);

	return success;
}

