/* Copyright 2016, Stephen Fryatt (info@stevefryatt.org.uk)
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

/* OSLib header files */

#include "oslib/types.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"

/* Application header files */

#include "ignore.h"

/**
 * \file: ignore.c
 *
 * File and folder ignore list.
 */


struct ignore_block {
	int dummy;
};

void ignore_destroy(struct ignore_block *handle)
{
	if (handle == NULL)
		return;

	heap_free(handle);
}

osbool ignore_match_object(struct ignore_block *handle, char *name)
{
	return TRUE;
}

osbool ignore_search_content(struct ignore_block *handle, char *name)
{
	return TRUE;
}

