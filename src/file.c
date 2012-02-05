/* Locate - file.c
 * (c) Stephen Fryatt, 2012
 *
 * Search file record creation, maipulation and deletion.
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

#include "file.h"

#include "ihelp.h"
#include "results.h"
#include "templates.h"


/* Results window icons. */


struct file_block {
	struct results_window		*results;

	struct file_block		*next;
};

struct file_block		*file_files = NULL;				/**< A linked list of file data.	*/



/**
 * Create a new file with no data associated to it.
 *
 * \return			Pointer to an empty file block, or NULL.
 */

struct file_block *file_create(void)
{
	struct file_block	*new;

	/* Allocate memory. */

	new = heap_alloc(sizeof(struct file_block));
	if (new == NULL) {
		error_msgs_report_error("NoMemFileBlock");
		return NULL;
	}

	/* Link the block into the files list. */

	new->next = file_files;
	file_files = new;

	/* Initialise the block contents. */

	new->results = NULL;

	return new;
}


/**
 * Create a new file block by loading in pre-saved results.
 */

void file_create_results(void)
{
	struct file_block *new;

	new = file_create();
	if (new == NULL)
		return;

	new->results = results_create(new);
	if (new->results == NULL) {
		file_destroy(new);
		return;
	}

	return;
}


/**
 * Destroy a file, freeing its data and closing any windows.
 *
 * \param *block		The handle of the file to destroy.
 */

void file_destroy(struct file_block *block)
{
	struct file_block	*previous;

	debug_printf("\\RDestrying block 0x%x", block);

	/* Find and delink the block from the file list. */

	if (file_files == block) {
		file_files = block->next;
	} else {
		previous = file_files;

		while (previous != NULL && previous->next != block)
			previous = previous->next;

		if (previous == NULL)
			return;

		previous->next = block->next;
	}

	/* Destroy any objects associated with the block. */

	if (block->results != NULL)
		results_destroy(block->results);

	/* Free the block. */

	heap_free(block);
}


/**
 * Destroy all of the open file blocks, freeing data in the process.
 */

void file_destroy_all(void)
{
	while (file_files != NULL)
		file_destroy(file_files);
}

