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

/* OSLib header files */

#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"

/* Application header files */

#include "ignore.h"

#include "textdump.h"

/**
 * \file: ignore.c
 *
 * File and folder ignore list.
 */


/* A node in the ignore path set. */

struct ignore_node {
	unsigned name;
	osbool case_sensitive;

	struct ignore_node *next;
};

/**
 * A path ignore instance.
 */

struct ignore_block {
	struct textdump_block *names;
	struct ignore_node *nodes;

};

/* Static function definitions. */

static osbool ignore_compare_path(struct ignore_block *handle, struct ignore_node *node, char *path);

/**
 * Create a new path ignore instance.
 *
 * \return			The new path ignore instance handle, or NULL.
 */

struct ignore_block *ignore_create(void)
{
	struct ignore_block *new = heap_alloc(sizeof(struct  ignore_block));
	if (new == NULL)
		return NULL;

	/* Initialise the node tree. */

	new->nodes = NULL;

	/* Initialise the textdump for the file names. */

	new->names = textdump_create(0, 0, '\0');
	if (new->names == NULL) {
		heap_free(new);
		new = NULL;
	}

	return new;
}

/**
 * Destroy a path ignore instance and free its menory.
 *
 * \param *handle		The handle of the instance to destroy.
 */

void ignore_destroy(struct ignore_block *handle)
{
	if (handle == NULL)
		return;

	textdump_destroy(handle->names);
	heap_free(handle);
}

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

osbool ignore_add_node_for_test(struct ignore_block *handle, char *name, osbool case_sensitive)
{
	if (handle == NULL)
		return FALSE;

	struct ignore_node *new = heap_alloc(sizeof(struct ignore_node));
	if (new == NULL)
		return FALSE;

	new->name = textdump_store(handle->names, name);
	if (new->name == TEXTDUMP_NULL) {
		free(new);
		return FALSE;
	}

	new->case_sensitive = case_sensitive;

	new->next = handle->nodes;
	handle->nodes = new;

	return TRUE;
}

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

osbool ignore_add_path(struct ignore_block *handle, char *name, osbool case_sensitive)
{
	if (handle == NULL || name == NULL)
		return FALSE;

	char *next_name = name;
	struct ignore_node **next_nodes = &(handle->nodes);

	while (*next_name != '\0') {
//		struct ignore_node* node = ignore_match_node(next_nodes, next);


		struct ignore_node *new = heap_alloc(sizeof(struct ignore_node));
		if (new == NULL)
			return FALSE;


		new->case_sensitive = case_sensitive;
	//	new->name =


	}

	return TRUE;
}




void ignore_push_path(struct ignore_block *handle, char *name)
{
	if (handle == NULL)
		return;

	debug_printf("Push path: %s", name);
}

void ignore_push_object(struct ignore_block *handle, char *name)
{
	if (handle == NULL)
		return;

	debug_printf("Push object: %s", name);
}

void ignore_pop_object(struct ignore_block *handle)
{
	if (handle == NULL)
		return;

	debug_printf("Pop object");
}

void ignore_search_complete(struct ignore_block *handle)
{
	if (handle == NULL)
		return;

	debug_printf("The ignore stack should be reset here!");
}

osbool ignore_match_object(struct ignore_block *handle, char *name)
{
	if (handle == NULL)
		return TRUE;

	return TRUE;
}

osbool ignore_search_content(struct ignore_block *handle, char *name)
{
	if (handle == NULL)
		return TRUE;

	return TRUE;
}

/**
 * Given a node, attempt to match one of the child nodes against the first part
 * of the supplied path.
 *
 * \param *handle		The handle of the instance to test against.
 * \param *path			The path to test.
 * \return			A pointer to the matching node, or NULL if no
 *				match could be found.
 */

static struct ignore_node *ignore_match_node(struct ignore_block *handle, char *path)
{
	if (handle == NULL)
		return NULL;

	struct ignore_node *nodes = handle->nodes;

	while (nodes != NULL) {
		if (ignore_compare_path(handle, nodes, path))
			break;

		nodes = nodes->next;
	}

	return nodes;
}

#ifdef UNIT_TESTING

/**
 * Perform a pathname comparison for a unit test.
 *
 * \param *handle		The handle of the instance to test.
 * \param *path			The path to test.
 * \return			TRUE if the path component matches the node; else FALSE.
 */

osbool ignore_compare_string_for_test(struct ignore_block *handle, char *path)
{
	if (handle == NULL)
		return FALSE;

	return ignore_compare_path(handle, handle->nodes, path);
}

#endif

/**
 * Compare the first part of a pathname, up to the next '.' or the end of the
 * string, against the object name in a node.
 *
 * \param *handle		The handle of the instance containing the node.
 * \param *node			The node to test against.
 * \param *path			The path to test.
 * \return			TRUE if the path component matches the node; else FALSE.
 */

static osbool ignore_compare_path(struct ignore_block *handle, struct ignore_node *node, char *path)
{
	if (handle == NULL || node == NULL || path == NULL)
		return FALSE;

	char *text_base = textdump_get_base(handle->names);
	if (text_base == NULL)
		return FALSE;

	char *node_name = text_base + node->name;

	while ((*node_name != '\0') && (*path != '\0') && (*path != '.') &&
			(((*node_name == *path) && node->case_sensitive) || ((*node_name == tolower(*path)) && !node->case_sensitive))) {
		node_name++;
		path++;
	}

	return ((*node_name == '\0') && (*path == '\0' || *path == '.')) ? TRUE : FALSE;
}
