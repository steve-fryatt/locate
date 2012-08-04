/* Locate - text.c
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

#include "text.h"


#define TEXT_ALLOCATION 1024							/**< The default allocation block size.					*/

struct text_block {
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

struct text_block *text_create(unsigned allocation)
{
	struct text_block	*new;

	new = heap_alloc(sizeof(struct text_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? TEXT_ALLOCATION : allocation;

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

void text_destroy(struct text_block *handle)
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

char *text_get_base(struct text_block *handle)
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
 * \return			Offset if successful; TEXT_NULL on failure.
 */

unsigned text_store(struct text_block *handle, char *text)
{
	int		length, blocks;
	unsigned	offset;

	if (handle == NULL || text == NULL)
		return TEXT_NULL;

	length = strlen(text) + 1;

	if ((handle->free + length) > handle->size) {
		for (blocks = 1; (handle->free + length) > (handle->size + blocks * handle->allocation); blocks++);

		if (flex_extend((flex_ptr) &(handle->text), (handle->size + blocks * handle->allocation) * sizeof(char)) == 0)
			return TEXT_NULL;

		handle->size += blocks * handle->allocation;
	}

	offset = handle->free;

	strcpy(handle->text + handle->free, text);
	handle->free += length;

	return offset;
}

