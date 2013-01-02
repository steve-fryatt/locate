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

#include "oslib/osargs.h"
#include "oslib/osfind.h"
#include "oslib/osgbpb.h"

/* SF-Lib Header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/general.h"
#include "sflib/heap.h"

/* Application header files. */

#include "discfile.h"


#define DISCFILE_FILE_MAGIC_WORD (0x48435253u)
#define DISCFILE_SECTION_MAGIC_WORD (0x54434553u)
#define DISCFILE_CHUNK_MAGIC_WORD (0x4b4e4843u)

/**
 * Generic file structure handling.
 */

enum discfile_mode {
	DISCFILE_CLOSED = 0,							/**< The file isn't open, or is in an unknown state.		*/
	DISCFILE_READ,								/**< The file is open for reading.				*/
	DISCFILE_WRITE								/**< The file is open for writing.				*/
};

struct discfile_block {
	os_fw				handle;					/**< The file handle of the file.				*/
	enum discfile_mode		mode;					/**< The read-write mode of the file.				*/
	enum discfile_format		format;					/**< The format of the file.					*/
	int				section;				/**< File ptr to the current section, or 0 for none.		*/
	int				chunk;					/**< File ptr to the current chunk, or 0 for none.		*/
};

struct discfile_header {
	unsigned			magic_word;				/**< The file magic word.					*/
	enum discfile_format		format;					/**< The file format identifier.				*/
	unsigned			flags;					/**< The file flags (reserved and always zero).			*/
};

/**
 * Locate 2 Format structure handling.
 */

struct discfile_section {
	unsigned			magic_word;				/**< The section magic word.					*/
	enum discfile_section_type	type;					/**< The section type identifier.				*/
	unsigned			size;					/**< The section size (bytes).					*/
	unsigned			flags;					/**< The overall section flags (reserved and always zero).	*/
};

struct discfile_chunk {
	unsigned			magic_word;				/**< The chunk magic word.					*/
	enum discfile_chunk_type	type;					/**< The chunk type identifier.					*/
	unsigned			id;					/**< The chunk ID word.						*/
	unsigned			size;					/**< The overall chunk size (bytes).				*/
};


static void	discfile_read_header(struct discfile_block *handle);
static void	discfile_write_header(struct discfile_block *handle);
static unsigned	discfile_make_id(char *code);

/**
 * Open a new file for writing and return its handle.
 *
 * \param *filename		Pointer to the filename to open.
 * \return			Pointer to an discfile handle, or NULL.
 */

struct discfile_block *discfile_open_write(char *filename)
{
	struct discfile_block	*new;
	os_error		*error;

	/* Allocate memory. */

	new = heap_alloc(sizeof(struct discfile_block));
	if (new == NULL)
		return NULL;

	error = xosfind_openoutw(osfind_NO_PATH |osfind_ERROR_IF_DIR, filename, NULL, &(new->handle));
	if (error != NULL || new->handle == 0) {
		heap_free(new);
		return NULL;
	}

	new->section = 0;
	new->chunk = 0;
	new->mode = DISCFILE_WRITE;
	new->format = DISCFILE_UNKNOWN_FORMAT;

	discfile_write_header(new);

	return new;
}


/**
 * Write a header to a disc file, and update the data in the block to reflect
 * the new format if successful.
 *
 * \param *handle		The discfile handle to be written to.
 */

static void discfile_write_header(struct discfile_block *handle)
{
	struct discfile_header		header;
	int				unwritten;
	os_error			*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE)
		return;

	/* Set up the header for the file.
	 *
	 * This is where the current file format is set.
	 */

	header.magic_word = DISCFILE_FILE_MAGIC_WORD;
	header.format = DISCFILE_LOCATE2;
	header.flags = 0;

	/* Write the file header. */

	error = xosgbpb_write_atw(handle->handle, (byte *) &header, sizeof(struct discfile_header), 0, &unwritten);
	if (error != NULL || unwritten != 0)
		return;

	handle->format = header.format;
}


/**
 * Open a new section in a disc file, ready for chunks of data to be written to
 * it.
 *
 * \param *handle		The discfile handle to be written to.
 * \param type			The section type for the new section.
 */

void discfile_start_section(struct discfile_block *handle, enum discfile_section_type type)
{
	struct discfile_section		section;
	int				ptr, unwritten;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section != 0 || handle->chunk != 0)
		return;

	section.magic_word = DISCFILE_SECTION_MAGIC_WORD;
	section.type = type;
	section.size = 0;
	section.flags = 0;

	/* Get the curent file position. */

	error = xosargs_read_extw(handle->handle, &ptr);
	if (error != NULL)
		return;

	/* Write the section header. */

	error = xosgbpb_write_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), ptr, &unwritten);
	if (error != NULL || unwritten != 0)
		return;

	handle->section = ptr;
}


/**
 * Close an already open section of a discfile, updating the section header
 * appropriately.
 *
 * \param *handle		The discfile handle to be written to.
 */

void discfile_end_section(struct discfile_block *handle)
{
	struct discfile_section		section;
	int				ptr, unread, unwritten;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section == 0 || handle->chunk != 0)
		return;

	/* Get the curent file position. */

	error = xosargs_read_extw(handle->handle, &ptr);
	if (error != NULL)
		return;

	/* Re-read the section header. */

	error = xosgbpb_read_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), handle->section, &unread);
	if (error != NULL || unread != 0)
		return;

	/* The first word of the section should be a magic word to identify it. */

	if (section.magic_word != DISCFILE_SECTION_MAGIC_WORD)
		return;

	section.size = ptr - handle->section;

	/* Write the modified section header. */

	error = xosgbpb_write_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), handle->section, &unwritten);
	if (error != NULL || unwritten != 0)
		return;

	handle->section = 0;
}


/**
 * Open a new chunk in a disc file, ready for data to be written to it.
 *
 * \param *handle		The discfile handle to be written to.
 * \param type			The section type for the new section.
 */

void discfile_start_chunk(struct discfile_block *handle, enum discfile_chunk_type type, char *id)
{
	struct discfile_chunk		chunk;
	int				ptr, unwritten;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section == 0 || handle->chunk != 0)
		return;

	chunk.magic_word = DISCFILE_CHUNK_MAGIC_WORD;
	chunk.type = type;
	chunk.id = discfile_make_id(id);
	chunk.size = 0;

	/* Get the curent file position. */

	error = xosargs_read_extw(handle->handle, &ptr);
	if (error != NULL)
		return;

	/* Write the section header. */

	error = xosgbpb_write_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), ptr, &unwritten);
	if (error != NULL || unwritten != 0)
		return;

	handle->chunk = ptr;
}


/**
 * Close an already open chunk of a discfile, updating the chunk header
 * appropriately.
 *
 * \param *handle		The discfile handle to be written to.
 */

void discfile_end_chunk(struct discfile_block *handle)
{
	struct discfile_chunk		chunk;
	int				ptr, unread, unwritten;
	unsigned			size, zero = 0;
	os_error			*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section == 0 || handle->chunk == 0)
		return;

	/* Get the curent file position. */

	error = xosargs_read_extw(handle->handle, &ptr);
	if (error != NULL)
		return;

	/* Re-read the chunk header. */

	error = xosgbpb_read_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), handle->chunk, &unread);
	if (error != NULL || unread != 0)
		return;

	/* The first word of the chunk should be a magic word to identify it. */

	if (chunk.magic_word != DISCFILE_CHUNK_MAGIC_WORD)
		return;

	/* If the data wasn't a multiple of four bytes, pad the chunk out with zeros. */

	size = ptr - handle->chunk;
	chunk.size = WORDALIGN(size);

	if (size != chunk.size) {
		error = xosgbpb_writew(handle->handle, (byte *) &zero, chunk.size - size, &unwritten);
		if (error != NULL || unwritten != 0)
			return;
	}

	/* Write the modified chunk header. */

	error = xosgbpb_write_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), handle->chunk, &unwritten);
	if (error != NULL || unwritten != 0)
		return;

	handle->chunk = 0;
}
















/**
 * Write a string chunk to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *id			The ID (1-4 characters) for the chunk.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_string(struct discfile_block *handle, char *text)
{
	if (text == NULL)
		return;

	discfile_write_chunk(handle, (byte *) text, strlen(text) + 1);
}


/**
 * Write generic chunk data to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *id			The ID (1-4 characters) for the chunk.
 * \param *data			Pointer to the first byte of data to be written.
 * \param size			The number of bytes to be written.
 */

void discfile_write_chunk(struct discfile_block *handle, byte *data, unsigned size)
{
	int				unwritten;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section == 0 || handle->chunk == 0)
		return;

	/* Write the contents. */

	error = xosgbpb_writew(handle->handle, (byte *) data, size, &unwritten);
	if (error != NULL || unwritten != 0)
		return;
}


/**
 * Open an existing file for reading and return its handle.
 *
 * \param *filename		Pointer to the filename to open.
 * \return			Pointer to an discfile handle, or NULL.
 */

struct discfile_block *discfile_open_read(char *filename)
{
	struct discfile_block	*new;
	os_error		*error;

	/* Allocate memory. */

	new = heap_alloc(sizeof(struct discfile_block));
	if (new == NULL)
		return NULL;

	error = xosfind_openinw(osfind_NO_PATH |osfind_ERROR_IF_DIR, filename, NULL, &(new->handle));
	if (error != NULL || new->handle == 0) {
		heap_free(new);
		return NULL;
	}

	new->section = 0;
	new->chunk = 0;
	new->mode = DISCFILE_READ;
	new->format = DISCFILE_UNKNOWN_FORMAT;

	discfile_read_header(new);

	return new;
}


/**
 * Read the header from a disc-based file, and update the data in the block
 * to reflect the file contents.
 */

static void discfile_read_header(struct discfile_block *handle)
{
	struct discfile_header		header;
	int				unread;
	os_error			*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ)
		return;

	/* Read the file header.  If this fails, give up with an unknown format. */

	error = xosgbpb_read_atw(handle->handle, (byte *) &header, sizeof(struct discfile_header), 0, &unread);
	if (error != NULL || unread != 0)
		return;

	/* The first word of the header should be a magic word to identify it. */

	if (header.magic_word != DISCFILE_FILE_MAGIC_WORD)
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
		xosfind_closew(handle->handle);

	heap_free(handle);
}


/**
 * Return a four-byte word containing an ID code, made up from the first four
 * characters in a string.
 *
 * \param *code			The string to use for the code.
 * \return			The numeric ID code.
 */

static unsigned discfile_make_id(char *code)
{
	unsigned	id = 0;
	int		i;

	for (i = 0; i < 4 && code[i] != '\0'; i++)
		id += (code[i] << (8 * i));

	return id;
}

