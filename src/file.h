/* Copyright 2012-2013, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: file.h
 *
 * Search file record creation, maipulation and deletion.
 */

#ifndef LOCATE_FILE
#define LOCATE_FILE

struct file_block;

#include "oslib/wimp.h"
#include "dialogue.h"

/**
 * Create a new file with no data associated to it.
 *
 * \return			Pointer to an empty file block, or NULL.
 */

struct file_block *file_create(void);


/**
 * Create a new file block by opening a search window using the value of the
 * -open parameter from the command line. The parameter is in the form
 * "XxY".
 *
 * \param *coords		Pointer to the parameter value to parse.
 */

void file_create_dialogue_at(char *coords);


/**
 * Create a new file block by opening a search window.
 *
 * \param *pointer		The pointer position to open the dialogue at.
 * \param *path			A path to use, or NULL for the default.
 * \param *template		A template to use, or NULL for the default.
 */

void file_create_dialogue(wimp_pointer *pointer, char *path, struct dialogue_block *template);


/**
 * Create a new search and results window for the file.
 *
 * \param *file			The file to create the objects for.
 * \param *paths		The path(s) to search.
 * \return			The new search block, or NULL on failure.
 */

struct search_block *file_create_search(struct file_block *file, char *paths);


/**
 * Create a new file block by loading in pre-saved data.
 *
 * \param *filename		The file to be loaded.
 */

void file_create_from_saved(char *filename);


/**
 * Perform a full file save on a file block, storing object database,
 * results window and search dialogue settings.
 *
 * \param *block		The handle of the file to save.
 * \param *filename		Pointer to the filename to save to.
 * \return			TRUE on success; FALSE on failure.
 */

osbool file_full_save(struct file_block *block, char *filename);


/**
 * Perform a dialogue save on a file block, storing only the search
 * dialogue settings.
 *
 * \param *block		The handle of the file to save.
 * \param *filename		Pointer to the filename to save to.
 * \return			TRUE on success; FALSE on failure.
 */

osbool file_dialogue_save(struct file_block *block, char *filename);


/**
 * Destroy a file, freeing its data and closing any windows.
 *
 * \param *block		The handle of the file to destroy.
 */

void file_destroy(struct file_block *block);


/**
 * Destroy all of the open file blocks, freeing data in the process.
 */

void file_destroy_all(void);


/**
 * Identify whether a file has a search active.
 *
 * \param *file			The file to be tested.
 * \return			TRUE if it has an active search; else FALSE.
 */

osbool file_search_active(struct file_block *file);


/**
 * Return the dialogue data associated with a file, or NULL if there is not
 * any.
 *
 * \param *file			The file to be tested.
 * \return			The file's dialogue handle, or NULL if none.
 */

struct dialogue_block *file_get_dialogue(struct file_block *file);


/**
 * Stop any active search associated with a file.
 *
 * \param *file			The file to be stopped.
 */

void file_stop_search(struct file_block *file);

#endif

