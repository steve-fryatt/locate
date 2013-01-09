/* Copyright 2012-2013, Stephen Fryatt
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

#include "oslib/fileswitch.h"
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
	DISCFILE_WRITE,								/**< The file is open for writing.				*/
	DISCFILE_ERROR								/**< The file is probably open, but there has been an error.	*/
};

struct discfile_block {
	os_fw				handle;					/**< The file handle of the file.				*/
	enum discfile_mode		mode;					/**< The read-write mode of the file.				*/
	enum discfile_format		format;					/**< The format of the file.					*/
	int				section;				/**< File ptr to the current section, or 0 for none.		*/
	int				chunk;					/**< File ptr to the current chunk, or 0 for none.		*/
	int				chunk_size;				/**< The size of the current chunk, or 0 for none.		*/
	char				*error_token;				/**< Pointer to a MessageTrans token for an error, or NULL.	*/
};

/**
 * Locate 1 and Locate 2 file header.
 */

struct discfile_header {
	unsigned			magic_word;				/**< The file magic word.					*/
	enum discfile_format		format;					/**< The file format identifier.				*/
	unsigned			flags;					/**< The file flags (reserved and always zero).			*/
};

/**
 * Locate 2 Format structure handling.
 */

/**
 * The file consists of a sequence of sections, each a multiple of four bytes
 * long and starting immediately after the file header.
 *
 * Section sizes are stored in the file as exact multiples of four, as sections
 * should always be multiples of four.
 */

struct discfile_section {
	unsigned			magic_word;				/**< The section magic word.					*/
	enum discfile_section_type	type;					/**< The section type identifier.				*/
	unsigned			size;					/**< The section size (bytes), always a multiple of four.	*/
	unsigned			flags;					/**< The overall section flags (reserved and always zero).	*/
};

/**
 * Each section contains a sequence of chunks. Each is rounded up to a multiple
 * of four bytes by adding up to three zero bytes if required.
 *
 * Chunk sizes are stored in the file unrounded, (and so consist of the header
 * size plus the data size). Thus they might be up to three bytes less than the
 * rounded chunk size.
 */

struct discfile_chunk {
	unsigned			magic_word;				/**< The chunk magic word.					*/
	enum discfile_chunk_type	type;					/**< The chunk type identifier.					*/
	unsigned			size;					/**< The unrounded chunk size (bytes), before rounding up.	*/
	unsigned			flags;					/**< The overall chunk flags (reserved and always zero).	*/
};


static void	discfile_write_header(struct discfile_block *handle);
static void	discfile_read_header(struct discfile_block *handle);
static void discfile_validate_structure(struct discfile_block *handle);

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
	new->chunk_size = 0;
	new->mode = DISCFILE_WRITE;
	new->format = DISCFILE_UNKNOWN_FORMAT;
	new->error_token = NULL;

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

void discfile_start_chunk(struct discfile_block *handle, enum discfile_chunk_type type)
{
	struct discfile_chunk		chunk;
	int				ptr, unwritten;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section == 0 || handle->chunk != 0)
		return;

	chunk.magic_word = DISCFILE_CHUNK_MAGIC_WORD;
	chunk.type = type;
	chunk.size = 0;
	chunk.flags = 0;

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
	unsigned			zero = 0;
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

	/* Calculate the current chunk size, and then round up to a word multiple. */

	chunk.size = ptr - handle->chunk;

	debug_printf("Chunk size = %d, rounded = %d", chunk.size, WORDALIGN(chunk.size));

	/* Write the modified chunk header. */

	error = xosgbpb_write_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), handle->chunk, &unwritten);
	if (error != NULL || unwritten != 0)
		return;

	/* If the data wasn't a multiple of four bytes, pad the chunk out with zeros. */

	if (WORDALIGN(chunk.size) != chunk.size) {
		error = xosgbpb_write_atw(handle->handle, (byte *) &zero, WORDALIGN(chunk.size) - chunk.size, ptr, &unwritten);
		if (error != NULL || unwritten != 0)
			return;
	}

	handle->chunk = 0;
}


/**
 * Write a string chunk to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *text			Pointer to the text to be written.
 * \return			Pointer to the byte after the string terminator.
 */

char *discfile_write_string(struct discfile_block *handle, char *text)
{
	unsigned	length;

	if (text == NULL)
		return NULL;

	length = strlen(text) + 1;

	discfile_write_chunk(handle, (byte *) text, length);

	return text + length;
}


/**
 * Write generic chunk data to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
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
	new->chunk_size = 0;
	new->mode = DISCFILE_READ;
	new->format = DISCFILE_UNKNOWN_FORMAT;
	new->error_token = NULL;

	discfile_read_header(new);

	switch (new->format) {
	case DISCFILE_LOCATE1:
		new->error_token = "BadFile";
		new->mode = DISCFILE_ERROR;
		break;
	case DISCFILE_LOCATE2:
		discfile_validate_structure(new);
		break;
	default:
		if (new->error_token == NULL)
			new->error_token = "BadFile";
		new->mode = DISCFILE_ERROR;
		break;
	}

	return new;
}


/**
 * Read the header from a disc-based file, and update the data in the block
 * to reflect the file contents.
 *
 * On return, handle->format will be set appropriately, while handle->error_token
 * will be pointing to an error token if an error occurred.
 *
 * \param *handle		The handle of the file block to process.
 */

static void discfile_read_header(struct discfile_block *handle)
{
	struct discfile_header		header;
	int				unread;
	os_error			*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return;
	}

	/* Read the file header.  If this fails, give up with an unknown format. */

	error = xosgbpb_read_atw(handle->handle, (byte *) &header, sizeof(struct discfile_header), 0, &unread);
	if (error != NULL || unread != 0) {
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	/* The first word of the header should be a magic word to identify it. */

	if (header.magic_word != DISCFILE_FILE_MAGIC_WORD) {
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	/* There are only two valid formats. */

	if (header.format != DISCFILE_LOCATE1 && header.format != DISCFILE_LOCATE2) {
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	/* The flags are reserved, and should always be zero. */

	if (header.flags != 0) {
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	handle->format = header.format;
}


/**
 * Walk through a Locate 2 file, checking that all of the sections and chunks
 * add up and are consistant in size.
 *
 * On return, handle->format will be set appropriately, while handle->error_token
 * will be pointing to an error token if an error occurred.
 *
 * \param *handle		The handle of the file block to process.
 */

static void discfile_validate_structure(struct discfile_block *handle)
{
	struct discfile_section		section;
	struct discfile_chunk		chunk;
	int				section_ptr, chunk_ptr, file_extent, unread;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->format != DISCFILE_LOCATE2) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return;
	}

	handle->format = DISCFILE_UNKNOWN_FORMAT;

	/* Get the file extent. */

	error = xosargs_read_extw(handle->handle, &file_extent);
	if (error != NULL) {
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	/* Walk through the sections of the file, checking that all of the sizes
	 * make sense.
	 */

	section_ptr = sizeof(struct discfile_header);

	while (section_ptr < file_extent) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), section_ptr, &unread);
		if (error != NULL || unread != 0) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
			return;
		}

		if (section.magic_word != DISCFILE_SECTION_MAGIC_WORD) {
			handle->error_token = "FileUnrec";
			handle->mode = DISCFILE_ERROR;
			return;
		}

		if (section.flags != 0) {
			handle->error_token = "FileUnrec";
			handle->mode = DISCFILE_ERROR;
			return;
		}

		/* Walk through the chunks in the section, checking that all of
		 * the sizes make sense.
		 */

		chunk_ptr = section_ptr + sizeof(struct discfile_section);

		while (chunk_ptr < section_ptr + section.size) {
			error = xosgbpb_read_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), chunk_ptr, &unread);
			if (error != NULL || unread != 0) {
				handle->error_token = "FileError";
				handle->mode = DISCFILE_ERROR;
				return;
			}

			if (chunk.magic_word != DISCFILE_CHUNK_MAGIC_WORD) {
				handle->error_token = "FileUnrec";
				handle->mode = DISCFILE_ERROR;
				return;
			}

			if (chunk.flags != 0) {
				handle->error_token = "FileUnrec";
				handle->mode = DISCFILE_ERROR;
				return;
			}

			chunk_ptr += WORDALIGN(chunk.size);
		}

		/* The end of the last chunk should align exactly with the end
		 * of the current section.
		 */

		if (chunk_ptr != (section_ptr + section.size)) {
			handle->error_token = "FileUnrec";
			handle->mode = DISCFILE_ERROR;
			return;
		}

		section_ptr += section.size;
	}

	/* The end of the last section should align exactly with the end of the
	 * file.
	 */

	if (section_ptr != file_extent) {
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	handle->format = DISCFILE_LOCATE2;
}


/**
 * Open a section from a disc file, ready for chunks of data to be read from
 * it.
 *
 * \param *handle		The discfile handle to be read from.
 * \param type			The type of the section to be opened.
 * \return			TRUE if the section was opened; else FALSE.
 */

osbool discfile_open_section(struct discfile_block *handle, enum discfile_section_type type)
{
	struct discfile_section		section;
	int				ptr, extent, unread;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->section != 0 || handle->chunk != 0) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return FALSE;
	}

	/* The first section starts immediately following the file header. */

	ptr = sizeof(struct discfile_header);

	/* Get the file extent. */

	error = xosargs_read_extw(handle->handle, &extent);
	if (error != NULL) {
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return FALSE;
	}

	while (ptr < extent && handle->section == 0) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), ptr, &unread);
		if (error != NULL || unread != 0) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
			return FALSE;
		}

		if (section.magic_word != DISCFILE_SECTION_MAGIC_WORD || section.flags != 0) {
			handle->error_token = "FileUnrec";
			handle->mode = DISCFILE_ERROR;
			return FALSE;
		}

		if (section.type == type)
			handle->section = ptr;
		else
			ptr += section.size;
	}

	return (handle->section == 0) ? FALSE : TRUE;
}


/**
 * Close a section from a disc file after reading from it.
 *
 * \param *handle		The discfile handle to be read from.
 */

void discfile_close_section(struct discfile_block *handle)
{
	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->section == 0 || handle->chunk != 0) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return;
	}

	handle->section = 0;
}


/**
 * Open a chunk from a disc file, ready for data to be read from it.
 *
 * \param *handle		The discfile handle to be read from.
 * \param type			The type of the chunk to be opened.
 * \return			TRUE if the section was opened; else FALSE.
 */

osbool discfile_open_chunk(struct discfile_block *handle, enum discfile_chunk_type type)
{
	struct discfile_section		section;
	struct discfile_chunk		chunk;
	int				ptr, extent, unread;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->section == 0 || handle->chunk != 0) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return FALSE;
	}

	/* The first chunk starts immediately following the section header. */

	ptr = handle->section + sizeof(struct discfile_section);

	/* Get the section extent. */

	error = xosgbpb_read_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), handle->section, &unread);
	if (error != NULL || unread != 0) {
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return FALSE;
	}

	if (section.magic_word != DISCFILE_SECTION_MAGIC_WORD || section.flags != 0) {
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return FALSE;
	}

	extent = handle->section + section.size;

	while (ptr < extent && handle->chunk == 0) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), ptr, &unread);
		if (error != NULL || unread != 0) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
			return FALSE;
		}

		if (chunk.magic_word != DISCFILE_CHUNK_MAGIC_WORD || chunk.flags != 0) {
			handle->error_token = "FileUnrec";
			handle->mode = DISCFILE_ERROR;
			return FALSE;
		}

		if (chunk.type == type) {
			handle->chunk = ptr;
			handle->chunk_size = chunk.size;
		} else {
			ptr += WORDALIGN(chunk.size);
		}
	}

	return (handle->chunk == 0) ? FALSE : TRUE;
}


/**
 * Close a chunk from a disc file after reading from it.
 *
 * \param *handle		The discfile handle to be read from.
 */

void discfile_close_chunk(struct discfile_block *handle)
{
	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->section == 0 || handle->chunk == 0) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return;
	}

	handle->chunk = 0;
	handle->chunk_size = 0;
}


/**
 * Return the size of the currently open chunk from a disc file.
 *
 * \param *handle		The discfile handle to be read from.
 * \return			The number of bytes of data in the chunk.
 */

int discfile_chunk_size(struct discfile_block *handle)
{
	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->section == 0 || handle->chunk == 0) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return 0;
	}

	return handle->chunk_size - sizeof(struct discfile_chunk);
}


/**
 * Read a string chunk from disc, out of an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *text			Pointer to the text to be written.
 * \return			Pointer to the next free byte in the buffer.
 */

char *discfile_read_string(struct discfile_block *handle, char *text, size_t size)
{
	int		read, max_bytes, ptr;
	bits		flags;
	os_error	*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section == 0 || handle->chunk == 0 || text == NULL) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return text;
	}

	/* Get the curent file position. */

	error = xosargs_read_ptrw(handle->handle, &ptr);
	if (error != NULL) {
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return text;
	}

	/* Work out how many bytes can be read, based on the size of the
	 * supplied buffer and the number of bytes left in the chunk.
	 */

	max_bytes = handle->chunk + handle->chunk_size - ptr;

	if (max_bytes > size)
		max_bytes = size;

	/* Read the bytes in from disc. */

	read = 0;

	while (flags != 2 && read < max_bytes && error == NULL && (read == 0 || text[read - 1] != '\0'))
		error = xos_bgetw(handle->handle, text + (read++), &flags);

	if (error != NULL || read == 0) {
		text[0] = '\0';
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return text + 1;
	}

	if (text[read - 1] != '\0') {
		text[read - 1] = '\0';
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return text + read;
	}

	return text;
}


/**
 * Read a generic chunk data from disc, out of an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *data			Pointer to the buffer into which to read the data.
 * \param size			The number of bytes to be read.
 */

void discfile_read_chunk(struct discfile_block *handle, byte *data, unsigned size)
{
	int		unread, ptr;
	os_error	*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_WRITE ||
			handle->section == 0 || handle->chunk == 0 || data == NULL) {
		if (handle != NULL) {
			handle->error_token = "FileError";
			handle->mode = DISCFILE_ERROR;
		}
		return;
	}

	/* Get the curent file position. */

	error = xosargs_read_ptrw(handle->handle, &ptr);
	if (error != NULL) {
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	/* Check that there are enough bytes left in the chunk. */

	if (size > (handle->chunk + handle->chunk_size - ptr)) {
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	/* Read the contents. */

	error = xosgbpb_readw(handle->handle, (byte *) data, size, &unread);
	if (error != NULL) {
		handle->error_token = "FileError";
		handle->mode = DISCFILE_ERROR;
		return;
	}

	if (unread != 0) {
		handle->error_token = "FileUnrec";
		handle->mode = DISCFILE_ERROR;
		return;
	}
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

