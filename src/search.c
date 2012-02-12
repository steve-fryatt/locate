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

#include "oslib/osbyte.h"
#include "oslib/wimp.h"

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


#define SEARCH_ALLOC_STACK 20							/**< The directory depth in which to allocate stack space.	*/

#define SEARCH_MAX_FILENAME 256							/**< The maximum length of a file (object) name.		*/


struct search_stack {
	char			filename[SEARCH_MAX_FILENAME];			/**< The filename of the current directory.			*/
};

struct search_block {
	struct file_block	*file;						/**< The file to which the search belongs.			*/
	struct results_window	*results;					/**< Results module to output results to.			*/

	struct search_stack	*stack;						/**< The search stack.						*/
};



/**
 * Create a new search.
 *
 * \param *file			The file block to which the search belongs.
 * \param *results		The results window to which output should be
 *				directed.
 * \return			The search handle, or NULL on failure.
 */

struct search_block *search_create(struct file_block *file, struct results_window *results)
{
	struct search_block	*new = NULL;
	osbool			mem_ok = TRUE;

		/* Allocate all the memory that we require. */

	new = heap_alloc(sizeof(struct search_block));
	if (new == NULL)
		mem_ok = FALSE;

	if (mem_ok) {
		new->stack = NULL;

		if (flex_alloc((flex_ptr) &(new->stack), SEARCH_ALLOC_STACK * sizeof(struct search_stack)) == 0)
			mem_ok = FALSE;
	}

	/* If any memory allocations failed, free anything that did get
	 * claimed and exit with an error.
	 */

	if (!mem_ok) {
		if (new != NULL && new->stack != NULL)
			flex_free((flex_ptr) &(new->stack));

		if (new != NULL)
			heap_free(new);

		error_msgs_report_error("NoMemSearchCreate");
		return NULL;
	}

	new->file = file;
	new->results = results;

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

	flex_free((flex_ptr) &(search->stack));

	heap_free(search);
}
