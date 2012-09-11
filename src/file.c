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

#include "dialogue.h"
#include "ihelp.h"
#include "objdb.h"
#include "results.h"
#include "search.h"
#include "templates.h"


/* Results window icons. */


struct file_block {
	struct dialogue_block		*dialogue;				/**< The dialogue settings related to the file.		*/
	struct search_block		*search;				/**< The search operation related to the file.		*/
	struct objdb_block		*objects;				/**< The object database related to the file.		*/
	struct results_window		*results;				/**< The results window related to the file.		*/

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

	new->dialogue = NULL;
	new->search = NULL;
	new->objects = NULL;
	new->results = NULL;

	return new;
}


/**
 * Create a new file block by opening a search window.
 *
 * \param *pointer		The pointer position to open the dialogue at.
 * \param *path			A path to use, or NULL for the default.
 */

void file_create_dialogue(wimp_pointer *pointer, char *path)
{
	struct file_block *new;

	new = file_create();
	if (new == NULL)
		return;

	new->dialogue = dialogue_create(new, path);
	if (new->dialogue == NULL) {
		file_destroy(new);
		return;
	}

	dialogue_open_window(new->dialogue, pointer);
}


/**
 * Create a new search, object database and results window for the file.
 *
 * \param *file			The file to create the objects for.
 * \param *paths		The path(s) to search.
 * \return			The new search block, or NULL on failure.
 */

struct search_block *file_create_search(struct file_block *file, char *paths)
{

	if (file == NULL)
		return NULL;

	debug_printf("Starting to create file");

	file->objects = objdb_create(file);
	if (file->objects == NULL) {
		file_destroy(file);
		return NULL;
	}

	debug_printf("Objects: 0x%x", file->objects);

	file->results = results_create(file, file->objects, NULL);
	if (file->results == NULL) {
		file_destroy(file);
		return NULL;
	}

	debug_printf("Results: 0x%x", file->results);

	file->search = search_create(file, file->objects, file->results, paths);
	if (file->search == NULL) {
		file_destroy(file);
		return NULL;
	}

	debug_printf("Search: 0x%x", file->search);

	return file->search;
}


/**
 * Create a new file block by loading in pre-saved results.
 */

void file_create_results(void)
{
	struct file_block *new;
	char title[128]; // \TODO -- Remove
	int i;	// \TODO -- Remove

	new = file_create();
	if (new == NULL)
		return;

	sprintf(title, "Results block 0x%x", (int) new); // \TODO -- Remove

	new->results = results_create(new, NULL, title);
	if (new->results == NULL) {
		file_destroy(new);
		return;
	}

	new->search = search_create(new, NULL, new->results, "ADFS::4.$,ADFS::4.$.Transient,ADFS::4.$.Test");
	if (new->search == NULL) {
		file_destroy(new);
		return;
	}

	/*
	results_add_text(new->results, "This is some text", "small_unf", FALSE, wimp_COLOUR_RED);
	results_add_text(new->results, "This is some more text", "small_1ca", FALSE, wimp_COLOUR_BLACK);
	results_add_text(new->results, "A third text line", "file_1ca", TRUE, wimp_COLOUR_BLACK);
	results_add_text(new->results, "And a foruth to finish off with", "small_xxx", FALSE, wimp_COLOUR_BLACK);
	for (i = 0; i < 1000; i++) {
		sprintf(title, "Dummy line %d", i);
		results_add_text(new->results, title, "small_ffb", FALSE, wimp_COLOUR_DARK_BLUE);
	}

	results_reformat(new->results, FALSE);
	*/

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

	if (block->search != NULL)
		search_destroy(block->search);

	if (block->objects != NULL)
		objdb_destroy(block->objects);

	if (block->dialogue != NULL)
		dialogue_destroy(block->dialogue);

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

