/* Locate - flexutils.c
 * (c) Stephen Fryatt, 2012
 *
 * Extensions to the Flex implementation.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

/* OSLib Header files. */

#include "oslib/types.h"

/* Application header files. */

#include "flexutils.h"


/**
 * Store a string in an already-created flex block.
 *
 * \param ptr		The flex block to use.
 * \param *text		The text to store.
 * \return		TRUE if successful; else FALSE.
 */

osbool flexutils_store_string(flex_ptr ptr, char *text)
{
	if (flex_extend(ptr, strlen(text) + 1) == 0)
		return FALSE;

	strcpy((char *) *ptr, text);

	return TRUE;
}

