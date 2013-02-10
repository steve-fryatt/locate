/* Copyright 2013, Stephen Fryatt
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
 * \file: contents.h
 *
 * File contents search.
 */

#ifndef LOCATE_CONTENTS
#define LOCATE_CONTENTS

#include "objdb.h"
#include "results.h"

//#include <stdlib.h>
//#include "oslib/types.h"

struct contents_block;


/**
 * Create a new contents search engine.
 *
 * \param *objects		The object database to which the search will belong.
 * \param *results		The results window to which the search will report.
 * \param *text			Pointer to the string to be matched.
 * \param any_case		TRUE to match case insensitively; else FALSE.
 * \param invert		TRUE to invert the search logic.
 * \return			The new contents search engine handle, or NULL.
 */

struct contents_block *contents_create(struct objdb_block *objects, struct results_window *results, char *text, osbool any_case, osbool invert);


/**
 * Destroy a contents search engine and free its memory.
 *
 * \param *handle		The handle of the engine to destroy.
 */

void contents_destroy(struct contents_block *handle);


/**
 * Add a file to the search engine, to be processed on subsequent search polls.
 *
 * \param *handle		The handle of the engine to take the file.
 * \param key			The ObjectDB key for the file to be searched.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool contents_add_file(struct contents_block *handle, unsigned key);


/**
 * Poll a search to allow it to process the current file.
 *
 * \param *handle		The handle of the engine to poll.
 * \param end_time		The latest time at which control must return.
 * \param *matched		Pointer to a variable to return the matched state,
 *				if search completes.
 * \return			TRUE if the search has completed; else FALSE.
 */

osbool contents_poll(struct contents_block *handle, os_t end_time, osbool *matched);

#endif

