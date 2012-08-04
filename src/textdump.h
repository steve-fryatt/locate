/* Locate - textdump.h
 * (c) Stephen Fryatt, 2012
 *
 * Text storage in a Flex block.
 */

#ifndef LOCATE_TEXTDUMP
#define LOCATE_TEXTDUMP

#include "oslib/types.h"
#include "flex.h"

struct textdump_block;


#define TEXTDUMP_NULL 0xffffffff							/**< 'NULL' value for use with the unsigned flex block offsets.		*/


/**
 * Initialise a text storage block.
 *
 * \param allocation	The allocation block size, or 0 for the default.
 * \return		The block handle, or NULL on failure.
 */

struct textdump_block *textdump_create(unsigned allocation);


/**
 * Destroy a text dump, freeing the memory associated with it.
 *
 * \param *handle		The block to be destroyed.
 */

void textdump_destroy(struct textdump_block *handle);


/**
 * Return the offset base for a text block. The returned value is only guaranteed
 * to be correct unitl the Flex heap is altered.
 *
 * \param			The block handle.
 * \return			The block base, or NULL on error.
 */

char *textdump_get_base(struct textdump_block *text);


/**
 * Store a text string in the text dump, allocating new memory if required,
 * and returning the offset to the stored string.
 *
 * \param *handle		The handle of the text dump to take the string.
 * \param *text			The text to be stored.
 * \return			Offset if successful; TEXTDUMP_NULL on failure.
 */

unsigned textdump_store(struct textdump_block *handle, char *text);

#endif

