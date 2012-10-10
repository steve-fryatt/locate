/* Copyright 2012, Stephen Fryatt
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
 * \file: results.h
 *
 * Search result and status window implementation.
 */

#ifndef LOCATE_RESULTS
#define LOCATE_RESULTS

#include "file.h"
#include "objdb.h"

struct results_window;


/**
 * Initialise the Results module.
 *
 * \param *sprites		Pointer to the sprite area to be used.
 */

void results_initialise(osspriteop_area *sprites);


/**
 * Create and open a new results window.
 *
 * \param *file			The file block to which the window belongs.
 * \param *objects		The object database from which file data should be taken.
 * \param *title		The title to use for the window, or NULL to allocate
 *				an empty buffer of TITLE_LENGTH.
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_create(struct file_block *file, struct objdb_block *objects, char *title);


/**
 * Close and destroy a results window.
 *
 * \param *handle		The handle of the results window to destroy.
 */

void results_destroy(struct results_window *handle);


/**
 * Update the status bar text for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *status		The text to be copied into the status bar.
 */

void results_set_status(struct results_window *handle, char *status);


/**
 * Update the status bar text for a results window using a template and truncating
 * the inset text to fit if necessary.
 *
 * \param *handle		The handle of the results window to update.
 * \param *token		The messagetrans token of the template.
 * \param *text 		The text to be inserted into placeholder %0.
 */

void results_set_status_template(struct results_window *handle, char *token, char *text);


/**
 * Update the title text for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *title		The text to be copied into the title.
 */

void results_set_title(struct results_window *handle, char *title);


/**
 * Add an error message to the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *message		The error message text.
 * \param *path			The path of the folder where the error occurred,
 *				or NULL if not applicable.
 */

void results_add_error(struct results_window *handle, char *message, char *path);


/**
 * Add a file to the end of the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param key			The database key for the file.
 */

void results_add_file(struct results_window *handle, unsigned key);


/**
 * Reformat lines in the results window to take into account the current
 * display width.
 *
 * \param *handle		The handle of the results window to update.
 * \param all			TRUE to format all lines; FALSE to format
 *				only those added since the last update.
 */

void results_reformat(struct results_window *handle, osbool all);

#endif

