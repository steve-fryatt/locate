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
 * \file: textdump.c
 *
 * Text storage in a flex block.
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

#include "discfile.h"


#define TEXTDUMP_ALLOCATION 1024						/**< The default allocation block size.					*/

struct textdump_block {
	byte			*text;						/**< The general text string dump.					*/
	unsigned		*hash;						/**< The hash table, or NULL if none.					*/
	unsigned		free;						/**< Offset to the first free character in the text dump.		*/
	unsigned		size;						/**< The current claimed size of the text dump.				*/
	unsigned		allocation;					/**< The allocation block size of the text dump.			*/
	unsigned		hashes;						/**< The size of the hash table, or 0 if none.				*/
	char			terminator;					/**< The terminating character for strings added to the text dump.	*/
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
 * \param terminator		The character to terminate dumped strings with. This
 *				must be \0 if hashing is to be used.
 * \return			The block handle, or NULL on failure.
 */

struct textdump_block *textdump_create(unsigned allocation, unsigned hash, char terminator)
{
	struct textdump_block	*new;
	int			i;

	/* Terminators must be \0 if hashing is to be used! */

	if (hash > 0 && terminator != '\0')
		return NULL;

	new = heap_alloc(sizeof(struct textdump_block));
	if (new == NULL)
		return NULL;

	new->allocation = (allocation == 0) ? TEXTDUMP_ALLOCATION : allocation;

	new->text = NULL;
	new->free = 0;
	new->size = new->allocation;

	new->hashes = hash;
	new->hash = NULL;

	new->terminator = terminator;

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
 * Clear the contents of a text dump, so that it will behave as if just created.
 *
 * \param *handle		The block to be destroyed.
 */

void textdump_clear(struct textdump_block *handle)
{
	int	i;

	if (handle == NULL)
		return;

	handle->free = 0;

	if (handle->hash != NULL)
		for (i = 0; i < handle->hashes; i++)
			handle->hash[i] = TEXTDUMP_NULL;

	if (handle->text == NULL)
		return;

	if (flex_extend((flex_ptr) &(handle->text), handle->allocation * sizeof(byte)) == 1)
		handle->size = handle->allocation;
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
 * Return the size of the contents of text block. The returned value covers used
 * space, and does not include any remaining allocated but unused memory.
 *
 * \param handle		The block handle.
 * \return			The used memory size, or 0 on error.
 */

size_t textdump_get_size(struct textdump_block *handle)
{
	if (handle == NULL)
		return 0;

	return (size_t) handle->free;
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

	if (handle->hash != NULL) {
		hash = textdump_make_hash(handle, text);

		offset = handle->hash[hash];

		while (offset != TEXTDUMP_NULL &&
				strcmp(((struct textdump_header *) (handle->text + offset))->text, text) != 0)
			offset = ((struct textdump_header *) (handle->text + offset))->next;

		if (offset != TEXTDUMP_NULL) {
			return offset + sizeof(unsigned);
		}

		length = (strlen(text) + sizeof(struct textdump_header)) & 0xfffffffc;
	} else {
		length = strlen(text) + 1;
	}

	if ((handle->free + length) > handle->size) {
		for (blocks = 1; (handle->free + length) > (handle->size + blocks * handle->allocation); blocks++);

		if (flex_extend((flex_ptr) &(handle->text), (handle->size + blocks * handle->allocation) * sizeof(char)) == 0)
			return TEXTDUMP_NULL;

		handle->size += blocks * handle->allocation;
	}

	offset = handle->free;

	if (handle->hash != NULL && hash != -1) {
		((struct textdump_header *) (handle->text + offset))->next = handle->hash[hash];
		handle->hash[hash] = offset;
		offset += sizeof(unsigned);
	}

	strcpy((char *) (handle->text + offset), text);

	if (handle->terminator != '\0')
		*(((char *) handle->text) + offset + length - 1) = handle->terminator;

	handle->free += length;

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

void textdump_save_file(struct textdump_block *handle, struct discfile_block *file)
{
	if (handle == NULL || file == NULL)
		return;

	discfile_start_chunk(file, DISCFILE_BLOB_CHUNK, "TEXT");

	//discfile_write_blob(file, "DMPT", handle->text, handle->free);
	//if (handle->hash != NULL)
	//	discfile_write_blob(file, "DMPH", handle->hash, handle->hashes * sizeof(unsigned));
	discfile_end_chunk(file);
}

