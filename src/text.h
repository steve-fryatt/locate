/* Locate - text.h
 * (c) Stephen Fryatt, 2012
 *
 * Text storage in a Flex block.
 */

#ifndef LOCATE_TEXT
#define LOCATE_TEXT

#include "oslib/types.h"
#include "flex.h"

struct text_block;


#define TEXT_NULL 0xffffffff							/**< 'NULL' value for use with the unsigned flex block offsets.		*/


/**
 * Initialise a text storage block.
 *
 * \param allocation	The allocation block size, or 0 for the default.
 * \return		The block handle, or NULL on failure.
 */

struct text_block *text_create(unsigned allocation);


/**
 * Destroy a text dump, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void text_destroy(struct text_block *handle);


/**
 * Return the offset base for a text block. The returned value is only guaranteed
 * to be correct unitl the Flex heap is altered.
 *
 * \param			The block handle.
 * \return			The block base, or NULL on error.
 */

char *text_get_base(struct text_block *text);


/**
 * Store a text string in the text dump, allocating new memory if required,
 * and returning the offset to the stored string.
 *
 * \param *handle		The handle of the text dump to take the string.
 * \param *text			The text to be stored.
 * \return			Offset if successful; TEXT_NULL on failure.
 */

unsigned text_store(struct text_block *handle, char *text);

#endif

