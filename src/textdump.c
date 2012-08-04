/* Locate - textdump.c
 * (c) Stephen Fryatt, 2012
 *
 * Text storage in a Flex block.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/heap.h"

/* OSLib Header files. */

#include "oslib/types.h"

/* Application header files. */

#include "textdump.h"


#define TEXTDUMP_ALLOCATION 1024							/**< The default allocation block size.					*/

struct textdump_block {
	char			*text;						/**< The general text string dump.					*/
	unsigned		free;						/**< Offset to the first free character in the text dump.		*/
	unsigned		size;						/**< The current claimed size of the text dump.				*/
	unsigned		allocation;					/**< The allocation block size of the text dump.			*/
};


/**
 * Initialise a text storage block.
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \return			The block handle, or NULL on failure.
 */

struct textdump_block *textdump_create(unsigned allocation)
{
	struct textdump_block	*new;

	new = heap_alloc(sizeof(struct textdump_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? TEXTDUMP_ALLOCATION : allocation;

	new->text = NULL;
	new->free = 0;
	new->size = new->allocation;

	if (flex_alloc((flex_ptr) &(new->text), new->allocation * sizeof(char)) == 0) {
		heap_free(new);
		return NULL;
	}

	return new;
}


/**
 * Destroy a text dump, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void textdump_destroy(struct textdump_block *handle)
{
	if (handle == NULL)
		return;

	if (handle->text != NULL)
		flex_free((flex_ptr) &(handle->text));

	heap_free(handle);
}


/**
 * Return the offset base for a text block. The returned value is only guaranteed
 * to be correct unitl the Flex heap is altered.
 *
 * \param handle		The block handle.
 * \return			The block base, or NULL on error.
 */

char *textdump_get_base(struct textdump_block *handle)
{
	if (handle == NULL)
		return NULL;

	return handle->text;
}


/**
 * Store a text string in the text dump, allocating new memory if required,
 * and returning the offset to the stored string.
 *
 * \param *handle		The handle of the text dump to take the string.
 * \param *text			The text to be stored.
 * \return			Offset if successful; TEXTDUMP_NULL on failure.
 */

unsigned textdump_store(struct textdump_block *handle, char *text)
{
	int		length, blocks;
	unsigned	offset;

	if (handle == NULL || text == NULL)
		return TEXTDUMP_NULL;

	length = strlen(text) + 1;

	if ((handle->free + length) > handle->size) {
		for (blocks = 1; (handle->free + length) > (handle->size + blocks * handle->allocation); blocks++);

		if (flex_extend((flex_ptr) &(handle->text), (handle->size + blocks * handle->allocation) * sizeof(char)) == 0)
			return TEXTDUMP_NULL;

		handle->size += blocks * handle->allocation;
	}

	offset = handle->free;

	strcpy(handle->text + handle->free, text);
	handle->free += length;

	return offset;
}

