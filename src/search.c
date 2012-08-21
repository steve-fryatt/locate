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
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/string.h"

/* Application header files. */

#include "search.h"

#include "results.h"


#define SEARCH_ALLOC_STACK 20							/**< The directory depth in which to allocate stack space.		*/

#define SEARCH_MAX_FILENAME 256							/**< The maximum length of a file (object) name.			*/
#define SEARCH_BLOCK_SIZE 4096							/**< The amount of memory to allocate to OS_GBPB.			*/

#define SEARCH_NULL 0xffffffff							/**< 'NULL' value for use with the unsigned flex block offsets.		*/

/* A data structure to hold the search stack. */

struct search_stack {
	char			filename[SEARCH_MAX_FILENAME];			/**< The filename of the current directory.				*/
	bits			info[SEARCH_BLOCK_SIZE];			/**< Block for OS_GBPB 10.						*/

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

	new->include_imagefs = FALSE;

	new->stack_size = SEARCH_ALLOC_STACK;
	new->stack_level = 0;

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

	flex_free((flex_ptr) &(search->stack));
	heap_free(search->path);
	heap_free(search->paths);

	heap_free(search);
}


/**
 * Make a search active so that it will run on subsequent calls to search_poll().
 *
 * \param *search		The handle of the search to make active.
 */

void search_start(struct search_block *search)
{
	unsigned stack;

	if (search == NULL || search->path_count == 0)
		return;

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
	}

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

	while (stack != SEARCH_NULL && (search->stack[stack].context != -1) && (os_read_monotonic_time() < end_time)) {
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
			if (TRUE) {
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

		if (search->stack[stack].context == -1) {
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

