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
 * \file: discfile.c
 *
 * Locate disc file format implementation.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* OSLib Header files. */

#include "oslib/osfind.h"
#include "oslib/osgbpb.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/string.h"

/* Application header files. */

#include "file.h"

#include "dialogue.h"
#include "ihelp.h"
#include "objdb.h"
#include "results.h"
#include "search.h"
#include "templates.h"



#define DISCFILE_MAGIC_WORD 0x53524348

struct discfile_block {
	os_fw				handle;					/**< The file handle of the file.			*/
	enum discfile_format		format;					/**< The format of the file.				*/
};

struct disfile_header {
	unsigned			magic_word;				/**< The magic word.					*/
	enum discfile_format		format;					/**< The file format identifier.			*/
	unsigned			flags;					/**< The file flags (reserved and always zero).		*/
};


static void	discfile_process_header(struct discfile_block *handle);

/**
 * Open a new file for writing and return its handle.
 *
 * \param *filename		Pointer to the filename to open.
 * \return			Pointer to an discfile handle, or NULL.
 */

struct discfile_block *discfile_open_write(void)
{
	struct discfile_block	*new;
	os_error		*error;

	/* Allocate memory. */

	new = heap_alloc(sizeof(struct discfile_block));
	if (new == NULL)
		return NULL;

	error = osfindopenoutw(osfind_NO_PATH |osfind_ERROR_IF_DIR, filename, NULL, &(new->handle));
	if (error != NULL || new->handle == 0) {
		heap_free(new);
		return NULL;
	}

	new->format = DISCFILE_UNKNOWN_FORMAT;
	discfile_process_header(new);

	return new;
}


/**
 * Open an existing file for reading and return its handle.
 *
 * \param *filename		Pointer to the filename to open.
 * \return			Pointer to an discfile handle, or NULL.
 */

struct discfile_block *discfile_open_read(void)
{
	struct discfile_block	*new;
	os_error		*error;

	/* Allocate memory. */

	new = heap_alloc(sizeof(struct discfile_block));
	if (new == NULL)
		return NULL;

	error = osfindopeninw(osfind_NO_PATH |osfind_ERROR_IF_DIR, filename, NULL, &(new->handle));
	if (error != NULL || new->handle == 0) {
		heap_free(new);
		return NULL;
	}

	new->format = DISCFILE_UNKNOWN_FORMAT;
	discfile_process_header(new);

	return new;
}


static void discfile_process_header(struct discfile_block *handle)
{
	struct discfile_header		header;
	int				unread;
	os_error			*error;

	if (handle == NULL || handle->handle == 0)
		return;

	/* Read the file header.  If this fails, give up with an unknown format. */

	error = xosgbpb_read_atw(handle->handle, &header, sizeof(struct discfile_header), 0, &unread);
	if (error != NULL || unread != 0)
		return;

	/* The first word of the header should be a magic word to identify it. */

	if (header.magic_word != DISCFILE_MAGIC_WORD)
		return;

	/* There are only two valid formats. */

	if (header.format != DISCFILE_LOCATE1 && header.format != DISCFILE_LOCATE2)
		return;

	/* The flags are reserved, and should always be zero. */

	if (header.flags != 0)
		return;

	handle->format = header.format;
}


/**
 * Close a discfile and free any memory associated with it.
 *
 * \param *handle		The discfile handle to be closed.
 */

void discfile_close(struct discfile_block *handle)
{
	if (handle == NULL)
		return;

	if (handle->handle != 0)
		xosfile_closew(handle->handle);

	heap_free(handle);
}

