/* Locate - search.c
 * (c) Stephen Fryatt, 2012
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

#include "flexutils.h"
#include "results.h"


#define SEARCH_ALLOC_STACK 20							/**< The directory depth in which to allocate stack space.		*/

#define SEARCH_MAX_FILENAME 256							/**< The maximum length of a file (object) name.			*/
#define SEARCH_BLOCK_SIZE 4096							/**< The amount of memory to allocate to OS_GBPB.			*/

#define SEARCH_NULL 0xffffffff							/**< 'NULL' value for use with the unsigned flex block offsets.		*/

/* A data structure to hold the search stack. */

struct search_stack {
	char			filename[SEARCH_MAX_FILENAME];			/**< The filename of the current directory.				*/
	byte			info[SEARCH_BLOCK_SIZE];			/**< Block for OS_GBPB 10.						*/

	int			read;						/**< The number of files read at the last OS_GBPB call.			*/
	int			context;					/**< The context for the next OS_GBPB call.				*/
	int			next;						/**< The number of the next item to read from the block.		*/
	unsigned		data_offset;					/**< Offset to the data for the next item to be read from the block.	*/

	unsigned		filetype;					/**< The filetype of the current file.					*/
};

/* A data structure defining a search. */

struct search_block {
	struct file_block	*file;						/**< The file to which the search belongs.				*/
	struct results_window	*results;					/**< Results module to output results to.				*/

	osbool			active;						/**< TRUE if the search is active; else FALSE.				*/

	osbool			include_imagefs;				/**< TRUE to search inside Image Filing Systems; else FALSE.		*/

	int			path_count;					/**< The number of search paths remaining to use.			*/
	char			*paths;						/**< Line containing the full set of search paths.			*/
	char			**path;						/**< Index to each of the search paths.					*/

	struct search_stack	*stack;						/**< The search stack.							*/
	unsigned		stack_level;					/**< The current stack level.						*/
	unsigned		stack_size;					/**< The amount of stack levels currently claimed.			*/

	unsigned		file_count;					/**< The number of files found in the search.				*/
	unsigned		error_count;					/**< The number of errors encountered during the search.		*/

	/* Search Parameters */

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

	osbool			test_contents;

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
 * \param *file			The file block to which the search belongs.
 * \param *results		The results window to which output should be
 *				directed.
 * \param *path			The path(s) to search, comma-separated.
 * \return			The search handle, or NULL on failure.
 */

struct search_block *search_create(struct file_block *file, struct results_window *results, char *path)
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
	new->results = results;

	new->active = FALSE;
	new->next = NULL;

	new->stack_size = SEARCH_ALLOC_STACK;
	new->stack_level = 0;

	new->file_count = 0;
	new->error_count = 0;

	/* The Search criteria. */

	new->include_imagefs = FALSE;

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
		flex_free((flex_ptr) & (search->filename));

	heap_free(search);
}


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
		osbool include_files, osbool include_directories, osbool include_applications)
{
	if (search == NULL)
		return;

	search->include_imagefs = search_imagefs;

	search->include_files = include_files;
	search->include_directories = include_directories;
	search->include_applications = include_applications;
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

	debug_printf("Setting size options: %d to %d on logic %u", minimum, maximum, in_limits);
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
 * Make a search active so that it will run on subsequent calls to search_poll().
 *
 * \param *search		The handle of the search to make active.
 */

void search_start(struct search_block *search)
{
	unsigned	stack;
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

	strncpy(search->stack[stack].filename, search->path[--search->path_count], SEARCH_MAX_FILENAME);

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
 * \param *Search		The handle of the search to stop.
 */

void search_stop(struct search_block *search)
{
	struct search_block	*active;
	char			status[256], errors[256], number[20];


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
		snprintf(number, sizeof(number), "%d", search->error_count);
		msgs_param_lookup("Errors", errors, sizeof(errors), number, NULL, NULL, NULL);
	}

	snprintf(number, sizeof(number), "%d", search->file_count);
	msgs_param_lookup("Found", status, sizeof(status), number, errors, NULL, NULL);

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
	unsigned		stack;
	osgbpb_info		*file_data;
	char			filename[4996], leafname[SEARCH_MAX_FILENAME];


	if (search == NULL || !search->active)
		return TRUE;

	debug_printf("Entering search poll 0x%x", search);

	/* If there's no stack set up, the search must have ended. */

	if (search->stack_level == 0) {
		search_stop(search);
		return FALSE;
	}

	/* Get the current stack level, and enter the search loop. */

	stack = search->stack_level - 1;

	while (stack != SEARCH_NULL && (os_read_monotonic_time() < end_time)) {
		debug_printf("In Stack: %u, Filename: '%s', Read: %d, context: %d, next: %d", stack, search->stack[stack].filename, search->stack[stack].read, search->stack[stack].context, search->stack[stack].next);

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

			debug_printf("Search path: '%s'", filename);

			error = xosgbpb_dir_entries_info(filename, (osgbpb_info_list *) search->stack[stack].info, 1000, search->stack[stack].context,
					SEARCH_BLOCK_SIZE, "*", &(search->stack[stack].read), &(search->stack[stack].context));

			search->stack[stack].next = 0;
			search->stack[stack].data_offset = 0;

			debug_printf("Out Read: %d, context: %d, next: %d", search->stack[stack].read, search->stack[stack].context, search->stack[stack].next);
		}

		/* Handle any errors thrown by the OS_GBPB call by dropping back out of
		 * the current directory.
		 */

		if (error != NULL) {
			search->error_count++;

			stack = search_drop_stack(search);
			debug_printf("Up out of folder: stack %u", stack);

			results_add_error(search->results, error->errmess, filename);

			continue;
		}

		/* Process the buffered details. */

		while ((os_read_monotonic_time() < end_time) && (search->stack[stack].next < search->stack[stack].read)) {
			file_data = (osgbpb_info *) ((unsigned) search->stack[stack].info + search->stack[stack].data_offset);

			/* Work out a filetype using the convention 0x000-0xfff, 0x1000, 0x2000, 0x3000. */

			if (file_data->obj_type == fileswitch_IS_DIR && file_data->name[0] == '!')
				search->stack[stack].filetype = osfile_TYPE_APPLICATION;
			else if (file_data->obj_type == fileswitch_IS_DIR)
				search->stack[stack].filetype = osfile_TYPE_DIR;
			else if ((file_data->load_addr & 0xfff00000u) != 0xfff00000)
				search->stack[stack].filetype = osfile_TYPE_UNTYPED;
			else
				search->stack[stack].filetype = (file_data->load_addr & osfile_FILE_TYPE) >> osfile_FILE_TYPE_SHIFT;

			debug_printf("Looping %d of %d with offset %u to address 0x%x for file '%s' of type 0x%x", search->stack[stack].next, search->stack[stack].read, search->stack[stack].data_offset, file_data, file_data->name, search->stack[stack].filetype);

			/* Test the object that we have found, starting by making sure that the object type is one that we want. */

			if (((((search->stack[stack].filetype >= 0x000 && search->stack[stack].filetype <= 0xfff) || (search->stack[stack].filetype == osfile_TYPE_UNTYPED)) && search->include_files) ||
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
									(((file_data->load_addr & 0xffu) == search->minimum_date_hi) && (file_data->exec_addr >= search->minimum_date_lo))) &&
							(((file_data->load_addr & 0xffu) < search->maximum_date_hi) ||
									(((file_data->load_addr & 0xffu) == search->maximum_date_hi) && (file_data->exec_addr <= search->maximum_date_lo)))) == search->date_logic)) &&

					/* If we're testing filetype and the type falls between 0x000 and 0xfff, is it set in the bitmask? */

					(!search->test_filetype || ((search->stack[stack].filetype < 0x000) && (search->stack[stack].filetype > 0xfff)) ||
							((search->filetypes[search->stack[stack].filetype / (8 * sizeof(bits))] & (1 << (search->stack[stack].filetype % (8 * sizeof(bits))))) != 0)) &&

					/* If we're testing attributes, do the bits cancel out? */

					(!search->test_attributes || (((file_data->attr ^ search->attributes) & search->attributes_mask) == 0x0u))

					) {
				search->file_count++;

				*filename = '\0';

				for (i = 0; i <= stack; i++) {
					if (i > 0)
						strcat(filename, ".");
					strcat(filename, search->stack[i].filename);
				}

				strcat(filename, ".");
				strcat(filename, file_data->name);

				results_add_file(search->results, filename, search->stack[stack].filetype);
			}

			/* Refind the file data, as it could have moved if the flex heap shuffled.
			 *
			 * Update the data offsets for the next file.
			 */

			file_data = (osgbpb_info *) ((unsigned) search->stack[stack].info + search->stack[stack].data_offset);

			search->stack[stack].data_offset += ((24 + strlen(file_data->name)) & 0xfffffffc);
			search->stack[stack].next++;

			/* If the object is a folder, recurse down into it. */

			if (file_data->obj_type == fileswitch_IS_DIR || (search->include_imagefs && file_data->obj_type == fileswitch_IS_IMAGE)) {
				/* Take a copy of the name before we shift the flex heap. */

				strncpy(leafname, file_data->name, SEARCH_MAX_FILENAME);

				stack = search_add_stack(search);

				strncpy(search->stack[stack].filename, leafname, SEARCH_MAX_FILENAME);
				debug_printf("Down into folder: stack %u", stack);
				continue;
			}
		}

		/* If that was all the files in the current folder, return to the
		 * parent.
		 */

		if ((search->stack[stack].next >= search->stack[stack].read) && (search->stack[stack].context == -1)) {
			stack = search_drop_stack(search);
			debug_printf("Up out of folder: stack %u", stack);

			continue;
		}
	}

	/* If the stack is empty, the current path has been completed.  Either prepare
	 * the next path for the next poll, or terminate the search if there are
	 * no more paths to search.
	 */

	if (stack == SEARCH_NULL) {
		if (search->path_count > 0) {
			/* Re-allocate a search stack and set up the first search folder. */

			if ((stack = search_add_stack(search)) != SEARCH_NULL)
				strncpy(search->stack[stack].filename, search->path[--search->path_count], SEARCH_MAX_FILENAME);
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

	results_reformat(search->results, FALSE);

	debug_printf("Exiting search poll 0x%x", search);

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

