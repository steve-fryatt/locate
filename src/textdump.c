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

#include "sflib/debug.h"
#include "sflib/heap.h"

/* OSLib Header files. */

#include "oslib/types.h"

/* Application header files. */

#include "textdump.h"


#define TEXTDUMP_ALLOCATION 1024						/**< The default allocation block size.					*/

struct textdump_block {
	byte			*text;						/**< The general text string dump.					*/
	unsigned		*hash;						/**< The hash table, or NULL if none.					*/
	unsigned		free;						/**< Offset to the first free character in the text dump.		*/
	unsigned		size;						/**< The current claimed size of the text dump.				*/
	unsigned		allocation;					/**< The allocation block size of the text dump.			*/
	unsigned		hashes;						/**< The size of the hash table, or 0 if none.				*/
};

struct textdump_header {
	unsigned		next;						/**< Offset to the next entry in the hash chain.			*/
	char			text[UNKNOWN];					/**< The stored text string.						*/
};


static int	textdump_make_hash(struct textdump_block *handle, char *text);

/**
 * Initialise a text storage block.
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \param hash			The size of the duplicate hash table, or 0 for none.
 * \return			The block handle, or NULL on failure.
 */

struct textdump_block *textdump_create(unsigned allocation, unsigned hash)
{
	struct textdump_block	*new;
	int			i;

	new = heap_alloc(sizeof(struct textdump_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? TEXTDUMP_ALLOCATION : allocation;

	new->text = NULL;
	new->free = 0;
	new->size = new->allocation;

	new->hashes = hash;
	new->hash = NULL;

	/* If a hash table has been requested, claim and initialise the storage. */

	if (hash > 0) {
		new->hash = heap_alloc(hash * sizeof(unsigned));

		if (new->hash == NULL) {
			heap_free(new);
			return NULL;
		}

		for (i = 0; i < hash; i++)
			new->hash[i] = TEXTDUMP_NULL;
	}

	/* Claim the memory for the dump itself. */

	if (flex_alloc((flex_ptr) &(new->text), new->allocation * sizeof(byte)) == 0) {
		if (new->hash != NULL)
			heap_free(new->hash);
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

	if (handle->hash != NULL)
		heap_free(handle->hash);

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

	return (char *) handle->text;
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
	int		hash = -1;

	if (handle == NULL || text == NULL)
		return TEXTDUMP_NULL;

	// \TODO -- Remove debug code!
	debug_printf("\\BText dump '%s' into 0x%x", text, handle);

	if (handle->hash != NULL) {
		hash = textdump_make_hash(handle, text);

		debug_printf("Searching in hash %d", hash);

		offset = handle->hash[hash];

		while (offset != TEXTDUMP_NULL &&
				strcmp(((struct textdump_header *) (handle->text + offset))->text, text) != 0)
			offset = ((struct textdump_header *) (handle->text + offset))->next;

		if (offset != TEXTDUMP_NULL) {
			debug_printf("Found in hash %d", hash);
			return offset + sizeof(unsigned);
		}

		length = (strlen(text) + sizeof(struct textdump_header)) & 0xfffffffc;
	} else {
		length = strlen(text) + 1;
	}

	// \TODO -- Remove debug code!
	debug_printf("Length %d (including terminator)", length);

	if ((handle->free + length) > handle->size) {
		for (blocks = 1; (handle->free + length) > (handle->size + blocks * handle->allocation); blocks++);

		// \TODO -- Remove debug code!
		debug_printf("Need to extend block by %u", blocks * handle->allocation * sizeof(char));

		if (flex_extend((flex_ptr) &(handle->text), (handle->size + blocks * handle->allocation) * sizeof(char)) == 0)
			return TEXTDUMP_NULL;

		handle->size += blocks * handle->allocation;

		// \TODO -- Remove debug code!
		debug_printf("...done (%u bytes)", handle->size);
	}

	offset = handle->free;

	if (handle->hash != NULL && hash != -1) {
		((struct textdump_header *) (handle->text + offset))->next = handle->hash[hash];
		handle->hash[hash] = offset;
		offset += sizeof(unsigned);
	}

	strcpy((char *) (handle->text + offset), text);

	handle->free += length;

	// \TODO -- Remove debug code!
	debug_printf("Stored at offset %u, free from %u", offset, handle->free);

	return offset;
}


/**
 * Create a hash for a given text string in a given text dump.
 *
 * \param *handle		The handle of the relevant text dump.
 * \param *text			Pointer to the string to hash.
 * \return			The calculated hash, or -1.
 */

static int textdump_make_hash(struct textdump_block *handle, char *text)
{
	int hash = 0, length, i;

	if (handle == NULL || text == NULL)
		return -1;

	length = strlen(text);
	for (i = 0; i < length; i++)
		hash += text[i];

	return hash % handle->hashes;
}


/**
 * Dump details of a textdump's hash to Reporter to aid debugging.
 *
 * \TODO -- Remove once unused.
 *
 * \param *handle		The handle of the relevant text dump.
 */

void textdump_output_hash_data(struct textdump_block *handle)
{
	int		i, items;
	unsigned	next;

	if (handle == NULL || handle->hashes == 0)
		return;

	debug_printf("\\GHash contents for textdump 0x%x", handle);

	for (i = 0; i < handle->hashes; i++) {
		items = 0;
		next = handle->hash[i];

		while (next != TEXTDUMP_NULL) {
			items++;
			next = ((struct textdump_header *) (handle->text + next))->next;
		}

		debug_printf("Hash %02d contains %d items", i, items);
	}
}

