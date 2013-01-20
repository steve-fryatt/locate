/* Copyright 2012-2013, Stephen Fryatt
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
 * \file: file.c
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

#include "oslib/hourglass.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
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
 * \param *template		A template to use, or NULL for the default.
 */

void file_create_dialogue(wimp_pointer *pointer, char *path, struct file_block *template)
{
	struct file_block *new;

	new = file_create();
	if (new == NULL)
		return;

	new->dialogue = dialogue_create(new, path, (template != NULL) ? template->dialogue : NULL);
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

	file->objects = objdb_create(file);
	if (file->objects == NULL) {
		file_destroy(file);
		return NULL;
	}

	file->results = results_create(file, file->objects, NULL);
	if (file->results == NULL) {
		file_destroy(file);
		return NULL;
	}

	file->search = search_create(file, file->objects, file->results, paths);
	if (file->search == NULL) {
		file_destroy(file);
		return NULL;
	}

	return file->search;
}


/**
 * Create a new file block by loading in pre-saved data.
 *
 * \param *filename		The file to be loaded.
 */

void file_create_from_saved(char *filename)
{
	struct file_block	*new;
	struct discfile_block	*load;
	wimp_pointer		pointer;


	new = file_create();
	if (new == NULL)
		return;

	hourglass_on();

	load = discfile_open_read(filename);

	/* Load an object database if there is one. */

	new->objects = objdb_load_file(new, load);


	/* Load the results window if one is present. */

	if (new->objects != NULL)
		new->results = results_load_file(new, new->objects, load);

	/* Load the search settings, if present. */

	new->dialogue = dialogue_load_file(new, load);

	hourglass_off();

	/* If an error is raised when the file closes, or none of the items
	 * loaded, give up.
	 */

	if (discfile_close(load) ||
			(new->dialogue == NULL && new->objects == NULL && new->results == NULL)) {
		file_destroy(new);
		return;
	}

	/* If there were no results to display, then open a search dialogue. */

	if (new->results == NULL) {
		wimp_get_pointer_info(&pointer);

		if (new->dialogue == NULL)
			new->dialogue = dialogue_create(new, NULL, NULL);

		if (new->dialogue != NULL)
			dialogue_open_window(new->dialogue, &pointer);
		else
			file_destroy(new);
	}
}


/**
 * Perform a full file save on a file block, storing object database,
 * results window and search dialogue settings.
 *
 * \param *block		The handle of the file to save.
 * \param *filename		Pointer to the filename to save to.
 * \return			TRUE on success; FALSE on failure.
 */

osbool file_full_save(struct file_block *block, char *filename)
{
	struct discfile_block		*out;

	out = discfile_open_write(filename);
	if (out == NULL)
		return FALSE;

	hourglass_on();

	objdb_save_file(block->objects, out);
	results_save_file(block->results, out);
	dialogue_save_file(block->dialogue, out);

	hourglass_off();

	discfile_close(out);

	osfile_set_type(filename, DISCFILE_LOCATE_FILETYPE);

	return TRUE;
}


/**
 * Perform a dialogue save on a file block, storing only the search
 * dialogue settings.
 *
 * \param *block		The handle of the file to save.
 * \param *filename		Pointer to the filename to save to.
 * \return			TRUE on success; FALSE on failure.
 */

osbool file_dialogue_save(struct file_block *block, char *filename)
{
	struct discfile_block		*out;

	out = discfile_open_write(filename);
	if (out == NULL)
		return FALSE;

	hourglass_on();

	dialogue_save_file(block->dialogue, out);

	hourglass_off();

	discfile_close(out);

	osfile_set_type(filename, DISCFILE_LOCATE_FILETYPE);

	return TRUE;
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


/**
 * Identify whether a file has a search active.
 *
 * \param *file			The file to be tested.
 * \return			TRUE if it has an active search; else FALSE.
 */

osbool file_search_active(struct file_block *file)
{
	if (file == NULL)
		return FALSE;

	return search_is_active(file->search);
}


/**
 * Identify whether a file has a set of dialogue data associated with it.
 *
 * \param *file			The file to be tested.
 * \return			TRUE if there is dialogue data; FALSE if not.
 */

osbool file_has_dialogue(struct file_block *file)
{
	if (file == NULL)
		return FALSE;

	return (file->dialogue == NULL) ? FALSE : TRUE;
}


/**
 * Stop any active search associated with a file.
 *
 * \param *file			The file to be stopped.
 */

void file_stop_search(struct file_block *file)
{
	if (file != NULL && file->search != NULL)
		search_stop(file->search);
}

