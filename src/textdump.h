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
 * \file: textdump.h
 *
 * Text storage in a flex block.
 *
 * A text dump maintains a block on the flex heap, which is used to store strings
 * of text.  A string is added using textdump_store(), which returns an offset
 * from the base of the block.  If
 *
 *  offset = textdump_store(dump, "String");
 *
 * then the address of "String" can always be found via
 *
 *  textdump_get_base(dump) + offset;
 *
 * The block's base must always be refound whenever there is a chance that
 * blocks on the flex heap might have moved.
 *
 * If the block is initialised with hash = 0, then strings will be added
 * byte-aligned to the block with '\0' byte terminators between them.
 * Identical strings will be added mutliple times.
 *
 * If the block is initialised with hash > 0, then a hash of that size will be
 * created and all new strings will be looked up via it.  If a exact duplicate
 * of an existing string is added, then the offset of the previous copy is
 * returned instead.  In this mode, all strings are stored word-aligned and an
 * overhead of up to 7 bytes is incurred for each new string stored (on top of
 * the string plus its '\0' terminator).
 *
 * Alternatively, if the terminator is set to other than \0, strings added to
 * the dump will have the non-standard terminator used instead of \0. If hashing
 * is used, terminators must be left as the standard \0 (otherwise the text
 * dump will fail to initialise and return a handle of NULL).
 */

#ifndef LOCATE_TEXTDUMP
#define LOCATE_TEXTDUMP

#include <stdlib.h>
#include "oslib/types.h"
#include "flex.h"

#include "discfile.h"

struct textdump_block;


#define TEXTDUMP_NULL 0xffffffff						/**< 'NULL' value for use with the unsigned flex block offsets.		*/


/**
 * Initialise a text storage block.
 *
 * \param allocation		The allocation block size, or 0 for the default.
 * \param hash			The size of the duplicate hash table, or 0 for none.
 * \param terminator		The character to terminate dumped strings with. This
 *				must be \0 if hashing is to be used.
 * \return			The block handle, or NULL on failure.
 */

struct textdump_block *textdump_create(unsigned allocation, unsigned hash, char terminator);


/**
 * Destroy a text dump, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void textdump_destroy(struct textdump_block *handle);


/**
 * Clear the contents of a text dump, so that it will behave as if just created.
 *
 * \param *handle		The block to be destroyed.
 */

void textdump_clear(struct textdump_block *handle);


/**
 * Return the offset base for a text block. The returned value is only guaranteed
 * to be correct unitl the Flex heap is altered.
 *
 * \param			The block handle.
 * \return			The block base, or NULL on error.
 */

char *textdump_get_base(struct textdump_block *text);


/**
 * Return the size of the contents of text block. The returned value covers used
 * space, and does not include any remaining allocated but unused memory.
 *
 * \param handle		The block handle.
 * \return			The used memory size, or 0 on error.
 */

size_t textdump_get_size(struct textdump_block *handle);


/**
 * Store a text string in the text dump, allocating new memory if required,
 * and returning the offset to the stored string.
 *
 * \param *handle		The handle of the text dump to take the string.
 * \param *text			The text to be stored.
 * \return			Offset if successful; TEXTDUMP_NULL on failure.
 */

unsigned textdump_store(struct textdump_block *handle, char *text);


/**
 * Save the text from a text dump into a file chunk.
 *
 * \param *handle		The handle of the text dump to be saved.
 * \param *file			The file to be saved, which should have an
 *				open section.
 */

void textdump_save_file(struct textdump_block *handle, struct discfile_block *file);

#endif

