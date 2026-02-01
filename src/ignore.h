/* Copyright 2016-2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Locate:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: ignore.h
 *
 * File and folder ignore list.
 */

#ifndef LOCATE_IGNORE
#define LOCATE_IGNORE

/* OSLib header files */

#include "oslib/types.h"

/**
 * A path ignore instance pointer.
 */

struct ignore_block;

/**
 * Create a new path ignore instance.
 *
 * \return			The new path ignore instance handle, or NULL.
 */

struct ignore_block *ignore_create(void);

/**
 * Destroy a path ignore instance and free its menory.
 *
 * \param *handle		The handle of the instance to destroy.
 */

void ignore_destroy(struct ignore_block *handle);

#ifdef UNIT_TESTING

/**
 * Add a node for unit test purposes.
 *
 * \param *handle		The handle of the instance to update.
 * \param *name			Pointer to the file name for the node.
 * \param case_sensitive	TRUE if the node should match case sensitively,
 *				else FALSE.
 * \return			TRUE if the node was added successfully,
 *				else FALSE.
 */

osbool ignore_add_node_for_test(struct ignore_block *handle, char *name, osbool case_sensitive);

#endif

/**
 * Add a path to be ignored to a path ignore instance.
 *
 * If it is required, a copy of the path will be taken so that the caller
 * may free their copy.
 *
 * \param *handle		The handle of the instance to use.
 * \param *name			Pointer to the path to be added.
 * \param case_sensitive	TRUE if the path should be matched case
 *				sensitively; else FALSE.
 * \return			TRUE if successful; FALSE on failure.
 */

osbool ignore_add_path(struct ignore_block *handle, char *name, osbool case_sensitive);

void ignore_push_path(struct ignore_block *handle, char *name);

void ignore_push_object(struct ignore_block *handle, char *name);

void ignore_pop_object(struct ignore_block *handle);

void ignore_search_complete(struct ignore_block *handle);

osbool ignore_match_object(struct ignore_block *handle, char *name);

osbool ignore_search_content(struct ignore_block *handle, char *name);

#ifdef UNIT_TESTING

/**
 * Perform a pathname comparison for a unit test.
 *
 * \param *handle		The handle of the instance to test.
 * \param *path			The path to test.
 * \return			TRUE if the path component matches the node; else FALSE.
 */

osbool ignore_compare_string_for_test(struct ignore_block *handle, char *name);

#endif
#endif
