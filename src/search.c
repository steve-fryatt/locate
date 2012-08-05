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

	int			read;
	int			next;
};

/* A data structure defining a search. */

struct search_block {
	struct file_block	*file;						/**< The file to which the search belongs.				*/
	struct results_window	*results;					/**< Results module to output results to.				*/

	osbool			active;						/**< TRUE if the search is active; else FALSE.				*/

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

/* Local function prototypes. */

static osbool	search_poll(struct search_block *search, os_t end_time);
static unsigned	search_add_stack(struct search_block *search);


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

	if (search == NULL)
		return;

	/* Allocate a search stack. */

	if ((stack = search_add_stack(search)) == SEARCH_NULL)
		return;

	/* Flag the search as active. */

	search->active = TRUE;

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

	while (search != NULL) {
		search_poll(search, (os_t) 0);
		search = search->next;
	}
}


/**
 * Poll
 */

static osbool search_poll(struct search_block *search, os_t end_time)
{
	os_error		*error;
	osbool			done = FALSE;
	int			read, next;
	struct search_stack	*stack;

	if (search == NULL || !search->active)
		return TRUE;


	// \TODO -- this is for debugging only!

	results_add_text(search->results, "This is some text", "small_unf", FALSE, wimp_COLOUR_RED);


/*
	stack = &(search->stack[search->stack_level]);

	if (stack->remaining == 0) {
		error = xosgbpb_dir_entries_info("path", 1000,
				stack->next, SEARCH_BLOCK_SIZE, NULL,
				&(stack->remaining), &(stack->next));


	}
*/


/*
	while (!done && os_read_monotonic_time() < end_time) {
		error = xosgbpb_dir_entries_info(path, 1000, search->stack[search->stack_level].next,
				SEARCH_BLOCK_SIZE, NULL, &read, &next);

	}
	*/

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
	search->stack[offset].next = 0;

	return offset;
}

