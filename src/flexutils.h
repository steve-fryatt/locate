/* Locate - flexutils.h
 * (c) Stephen Fryatt, 2012
 *
 * Extensions to the Flex implementation.
 */

#ifndef LOCATE_FLEXUTILS
#define LOCATE_FLEXUTILS

#include "oslib/types.h"
#include "flex.h"


/**
 * Store a string in an already-created flex block.
 *
 * \param ptr		The flex block to use.
 * \param *text		The text to store.
 * \return		TRUE if successful; else FALSE.
 */

osbool flexutils_store_string(flex_ptr ptr, char *text);

#endif

