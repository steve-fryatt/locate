/* Copyright 2012-2026, Stephen Fryatt (info@stevefryatt.org.uk)
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

/**
 * \file: flexutils.c
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

#include "sflib/string.h"

/* OSLib Header files. */

#include "oslib/types.h"

/* Application header files. */

#include "flexutils.h"


/**
 * Store a string in an already-created flex block.
 *
 * \param ptr		The flex block to use.
 * \param *text		The text to store.
 * \param *wrapper	Additional text in which to wrap the entry, or NULL
 *			to simply store the entry itself.
 * \return		TRUE if successful; else FALSE.
 */

osbool flexutils_store_string(flex_ptr ptr, char *text, char *wrapper)
{
	if (ptr == NULL || text == NULL)
		return FALSE;

	/* Work out the lengths of the strings and required buffer. */

	size_t text_length = strlen(text);
	size_t wrapper_length = (wrapper == NULL) ? 0 : strlen(wrapper);

	size_t length = text_length + (2 * wrapper_length) + 1;

	/* Resize the flex block. */

	if (*ptr == NULL) {
		if (flex_alloc(ptr, length) == 0)
			return FALSE;
	} else {
		if (flex_extend(ptr, length) == 0)
			return FALSE;
	}

	/* Copy the strings over. */

	char *p = (char *) *ptr;

	if (length > 0 && wrapper != NULL) {
		string_copy(p, wrapper, length);
		length -= wrapper_length;
		p += wrapper_length;
	}

	if (length > 0) {
		string_copy(p, text, length);
		length -= text_length;
		p += text_length;
	}

	if (length > 0 && wrapper != NULL) {
		string_copy(p, wrapper, length);
		length -= wrapper_length;
	}

	if (length <= 0) {
		*((char * ) ptr) = '\0';
		return FALSE;
	}

	return TRUE;
}
