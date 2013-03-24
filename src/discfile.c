/* Copyright 2012-2013, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "oslib/os.h"
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

#include "datetime.h"


/**
 * File block magic words.
 */

#define DISCFILE_FILE_MAGIC_WORD (0x48435253u)
#define DISCFILE_SECTION_MAGIC_WORD (0x54434553u)
#define DISCFILE_CHUNK_MAGIC_WORD (0x4b4e4843u)

/**
 * Options chunk type ids.
 */

#define DISCFILE_OPTION_UNSIGNED (0x00000000u)
#define DISCFILE_OPTION_INT (0x00000001u)
#define DISCFILE_OPTION_STRING (0x00000002u)
#define DISCFILE_OPTION_BOOLEAN (0x00000003u)
#define DISCFILE_OPTION_DATE (0x00000004u)
#define DISCFILE_OPTION_UNSIGNED_ARRAY (0x00000005u)

/**
 * Memory alloacation.
 */

#define DISCFILE_FLEX_ALLOCATION 256

/**
 * Generic file structure handling.
 */

enum discfile_mode {
	DISCFILE_CLOSED = 0,							/**< The file isn't open, or is in an unknown state.		*/
	DISCFILE_READ,								/**< The file is open for reading.				*/
	DISCFILE_WRITE,								/**< The file is open for writing.				*/
	DISCFILE_ERROR								/**< The file is probably open, but there has been an error.	*/
};

struct discfile_section_info {
	int				ptr;					/**< Pointer to the last section found in the file.		*/
	osbool				multiple;				/**< TRUE if the last section was set as multiple; else FALSE.	*/
	unsigned			count;					/**< Count of the number of sections found.			*/
};

struct discfile_block {
	os_fw				handle;					/**< The file handle of the file.				*/
	enum discfile_mode		mode;					/**< The read-write mode of the file.				*/
	enum discfile_format		format;					/**< The format of the file.					*/
	int				section;				/**< File ptr to the current section, or 0 for none.		*/
	int				chunk;					/**< File ptr to the current chunk, or 0 for none.		*/
	int				data_size;				/**< The size of the current chunk, or 0 for none.		*/
	char				*error_token;				/**< Pointer to a MessageTrans token for an error, or NULL.	*/
	struct discfile_section_info	section_info[DISCFILE_MAX_SECTIONS];	/**< Details of the different sections in a Locate 2 file.	*/
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
 *
 * If a section has DISCFILE_SECTION_FLAGS_MULTIPLE set then more than one copy
 * can appear in a file: each copy MUST have this flag set, however. If a section
 * does not have this flag set, then only one section of the type can be in the
 * file.
 */

enum discfile_section_flags {
	DISCFILE_SECTION_FLAGS_NONE = 0,					/**< There are no section flags set.				*/
	DISCFILE_SECTION_FLAGS_MULTIPLE = 1,					/**< This section can appear multiple times in the file.	*/
	DISCFILE_SECTION_FLAGS_UNUSED = 0xfffffffeu				/**< Bitmask for all the unused flag bits which should be zero.	*/
};

struct discfile_section {
	unsigned			magic_word;				/**< The section magic word.					*/
	enum discfile_section_type	type;					/**< The section type identifier.				*/
	unsigned			size;					/**< The section size (bytes), always a multiple of four.	*/
	enum discfile_section_flags	flags;					/**< The overall section flags.					*/
};

/**
 * Each section contains a sequence of chunks. Each is rounded up to a multiple
 * of four bytes by adding up to three zero bytes if required.
 *
 * Chunk sizes are stored in the file unrounded, (and so consist of the header
 * size plus the data size). Thus they might be up to three bytes less than the
 * rounded chunk size.
 */
 
enum discfile_chunk_flags {
	DISCFILE_CHUNK_FLAGS_NONE = 0,						/**< There are no chunk flags set.				*/
	DISCFILE_CHUNK_FLAGS_UNUSED = 0xffffffffu				/**< Bitmask for all the unused flag bits which should be zero.	*/
};

struct discfile_chunk {
	unsigned			magic_word;				/**< The chunk magic word.					*/
	enum discfile_chunk_type	type;					/**< The chunk type identifier.					*/
	unsigned			size;					/**< The unrounded chunk size (bytes), before rounding up.	*/
	enum discfile_chunk_flags	flags;					/**< The overall chunk flags.					*/
};

/**
 * Option chunks can contain a sequence of option blocks. These consist
 * of an option ID containing the option type and a three-character
 * identifier, then four bytes of option-specific data.
 */

union discfile_option_data {
	unsigned			data_unsigned;				/**< The option data for unsigned options.			*/
	int				data_int;				/**< The option data for integer options.			*/
	osbool				data_osbool;				/**< The option data for boolean options.			*/
	unsigned			length;					/**< The word-aligned length for extended options.		*/
};

struct discfile_option {
	unsigned			id;					/**< The option id (0xttttttiiu).				*/
	union discfile_option_data	data;					/**< The option data.						*/
};


static void	discfile_write_header(struct discfile_block *handle);
static void	discfile_read_header(struct discfile_block *handle);
static void	discfile_legacy_validate_structure(struct discfile_block *handle);
static void	discfile_validate_structure(struct discfile_block *handle);
static int	discfile_find_option_data(struct discfile_block *handle, unsigned id);
static unsigned	discfile_make_id(unsigned type, char *code);

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
	new->data_size = 0;
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
 * \param multile		TRUE if the section can appear multiple times;
 *				FALSE if it can only appear once.
 */

void discfile_start_section(struct discfile_block *handle, enum discfile_section_type type, osbool multiple)
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
	section.flags = DISCFILE_SECTION_FLAGS_NONE;
	
	if (multiple == TRUE)
		section.flags |= DISCFILE_SECTION_FLAGS_MULTIPLE;

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
	chunk.flags = DISCFILE_CHUNK_FLAGS_NONE;

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
 * Write a boolean value to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the value.
 * \param value			The value to be written.
 */

void discfile_write_option_boolean(struct discfile_block *handle, char *tag, osbool value)
{
	struct discfile_option		option;

	if (tag == NULL)
		return;

	option.id = discfile_make_id(DISCFILE_OPTION_BOOLEAN, tag);
	option.data.data_osbool = value;

	discfile_write_chunk(handle, (byte *) &option, sizeof(struct discfile_option));
}


/**
 * Write an unsigned value to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the value.
 * \param value			The value to be written.
 */

void discfile_write_option_unsigned(struct discfile_block *handle, char *tag, unsigned value)
{
	struct discfile_option		option;

	if (tag == NULL)
		return;

	option.id = discfile_make_id(DISCFILE_OPTION_UNSIGNED, tag);
	option.data.data_unsigned = value;

	discfile_write_chunk(handle, (byte *) &option, sizeof(struct discfile_option));
}


/**
 * Write a text string to an open chunk in a file. Strings are always padded out
 * to a multiple of four bytes, so that options are always word-alinged inside
 * their chunk.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the text.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_option_string(struct discfile_block *handle, char *tag, char *text)
{
	unsigned			length, zero = 0;
	int				unwritten;
	struct discfile_option		option;
	os_error			*error;

	if (tag == NULL || text == NULL)
		return;

	length = strlen(text) + 1;

	option.id = discfile_make_id(DISCFILE_OPTION_STRING, tag);
	option.data.length = WORDALIGN(length);

	discfile_write_chunk(handle, (byte *) &option, sizeof(struct discfile_option));
	discfile_write_string(handle, text);

	/* If the data wasn't a multiple of four bytes, pad the option out with zeros. */

	if (option.data.length != length) {
		error = xosgbpb_writew(handle->handle, (byte *) &zero, option.data.length - length, &unwritten);
		if (error != NULL || unwritten != 0)
			return;
	}
}


/**
 * Write an unsigned array to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the text.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_option_unsigned_array(struct discfile_block *handle, char *tag, unsigned *array, unsigned terminator)
{
	unsigned			length;
	struct discfile_option		option;

	if (tag == NULL || array == NULL)
		return;

	for (length = 0; array[length] != terminator; length++);

	option.id = discfile_make_id(DISCFILE_OPTION_UNSIGNED_ARRAY, tag);
	option.data.length = length * sizeof(unsigned);

	discfile_write_chunk(handle, (byte *) &option, sizeof(struct discfile_option));
	discfile_write_chunk(handle, (byte *) array, length * sizeof(unsigned));
}


/**
 * Write an OS date to an open chunk in a file. The dates are padded out to
 * 8 bytes, so that options are always word-alinged inside their chunk.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the text.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_option_date(struct discfile_block *handle, char *tag, os_date_and_time date)
{
	unsigned			words[2];
	struct discfile_option		option;

	if (tag == NULL)
		return;

	datetime_get_date(date, &words[1], &words[0]);

	option.id = discfile_make_id(DISCFILE_OPTION_DATE, tag);
	option.data.length = 2 * sizeof(unsigned);

	discfile_write_chunk(handle, (byte *) &option, sizeof(struct discfile_option));
	discfile_write_chunk(handle, (byte *) &words, 2 * sizeof(unsigned));
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
	int			i;

	/* Allocate memory. */

	new = heap_alloc(sizeof(struct discfile_block));
	if (new == NULL)
		return NULL;

	error = xosfind_openinw(osfind_NO_PATH | osfind_ERROR_IF_DIR, filename, NULL, &(new->handle));
	if (error != NULL || new->handle == 0) {
		heap_free(new);
		return NULL;
	}

	new->section = 0;
	new->chunk = 0;
	new->data_size = 0;
	new->mode = DISCFILE_READ;
	new->format = DISCFILE_UNKNOWN_FORMAT;
	new->error_token = NULL;
	
	/* Initialise the Locate 2 file section info blocks. Each pointer is set
	 * to start from the first section, so that it will work for both multiple
	 * and non-multiple sections.
	 */
	
	for (i = 0; i < DISCFILE_MAX_SECTIONS; i++) {
		new->section_info[i].ptr = sizeof(struct discfile_header);
		new->section_info[i].multiple = FALSE;
		new->section_info[i].count = 0;
	}

	discfile_read_header(new);

	switch (new->format) {
	case DISCFILE_LOCATE0:
	case DISCFILE_LOCATE1:
		discfile_legacy_validate_structure(new);
		break;
	case DISCFILE_LOCATE2:
		discfile_validate_structure(new);
		break;
	default:
		discfile_set_error(new, "BadFile");
		break;
	}

	return new;
}


/**
 * Read the file format of a discfile.
 *
 * \param *handle		The handle of the file to read.
 * \return			The format of the file.
 */

enum discfile_format discfile_read_format(struct discfile_block *handle)
{
	return (handle == NULL) ? DISCFILE_UNKNOWN_FORMAT : handle->format;
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
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return;
	}

	/* Read the file header.  If this fails, give up with an unknown format. */

	error = xosgbpb_read_atw(handle->handle, (byte *) &header, sizeof(struct discfile_header), 0, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return;
	}

	/* The first word of the header should be a magic word to identify it. */

	if (header.magic_word != DISCFILE_FILE_MAGIC_WORD) {
		discfile_set_error(handle, "FileUnrec");
		return;
	}

	/* There are only three valid formats. */

	if (header.format != DISCFILE_LOCATE0 && header.format != DISCFILE_LOCATE1 && header.format != DISCFILE_LOCATE2) {
		discfile_set_error(handle, "FileUnrec");
		return;
	}

	/* The flags are reserved, and should always be zero. */

	if (header.flags != 0) {
		discfile_set_error(handle, "FileUnrec");
		return;
	}

	handle->format = header.format;
}


/**
 * Walk through a Locate 1 file, checking that the sections all add up to the
 * correct size.
 *
 * On return, handle->format will be set appropriately, while handle->error_token
 * will be pointing to an error token if an error occurred.
 *
 * \param *handle		The handle of the file block to process.
 */

static void discfile_legacy_validate_structure(struct discfile_block *handle)
{
	int				ptr, section_size, file_extent, unread;
	os_error			*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			(handle->format != DISCFILE_LOCATE0 && handle->format != DISCFILE_LOCATE1)) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return;
	}

	handle->format = DISCFILE_UNKNOWN_FORMAT;

	/* Get the file extent. */

	error = xosargs_read_extw(handle->handle, &file_extent);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return;
	}

	/* Walk through the sections of the file, checking that all of the sizes
	 * make sense.
	 */

	ptr = sizeof(struct discfile_header);

	while (ptr < file_extent) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &section_size, sizeof(int), ptr, &unread);
		if (error != NULL || unread != 0) {
			discfile_set_error(handle, "FileError");
			return;
		}

		if (section_size < 0) {
			discfile_set_error(handle, "FileUnrec");
			return;
		}

		ptr += section_size + sizeof(int);
	}

	/* The end of the last section should align exactly with the end of the
	 * file.
	 */

	if (ptr != file_extent) {
		discfile_set_error(handle, "FileUnrec");
		return;
	}

	handle->format = DISCFILE_LOCATE1;
}


/**
 * Open a section from a legacy disc file, ready for data to be read from
 * it.
 *
 * \param *handle		The discfile handle to be read from.
 * \param type			The type of the section to be opened.
 * \return			TRUE if the section was opened; else FALSE.
 */

osbool discfile_legacy_open_section(struct discfile_block *handle, enum discfile_legacy_section_type type)
{
	int				ptr, extent, section_size, unread;
	os_error			*error;
	unsigned			section;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			(handle->format != DISCFILE_LOCATE0 && handle->format != DISCFILE_LOCATE1) ||
			handle->section != 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return FALSE;
	}

	/* The first section starts immediately following the file header. */

	ptr = sizeof(struct discfile_header);

	/* Get the file extent. */

	error = xosargs_read_extw(handle->handle, &extent);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	section = 0;

	while (ptr < extent && handle->section == 0) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &section_size, sizeof(int), ptr, &unread);
		if (error != NULL || unread != 0) {
			discfile_set_error(handle, "FileError");
			return FALSE;
		}

		if (section_size < 0) {
			discfile_set_error(handle, "FileUnrec");
			return FALSE;
		}

		if (++section == type) {
			handle->section = ptr;
			handle->data_size = section_size;
		} else {
			ptr += section_size + sizeof(int);
		}
	}

	return (handle->section == 0) ? FALSE : TRUE;
}


/**
 * Close a section from a legacy disc file after reading from it.
 *
 * \param *handle		The discfile handle to be read from.
 */

void discfile_legacy_close_section(struct discfile_block *handle)
{
	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			(handle->format != DISCFILE_LOCATE0 && handle->format != DISCFILE_LOCATE1) ||
			handle->section == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return;
	}

	handle->section = 0;
	handle->data_size = 0;
}


/**
 * Return the size of the currently open section from a legacy disc file.
 *
 * \param *handle		The discfile handle to be read from.
 * \return			The number of bytes of data in the section.
 */

int discfile_legacy_section_size(struct discfile_block *handle)
{
	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			(handle->format != DISCFILE_LOCATE0 && handle->format != DISCFILE_LOCATE1) ||
			handle->section == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return 0;
	}

	return handle->data_size;
}


/**
 * Read an integer from the currently open section of a legacy disc file.
 *
 * \param *handle		The discfile handle to be read from.
 * \return			The integer that was read, or 0.
 */

int discfile_legacy_read_word(struct discfile_block *handle)
{
	int		data, ptr, unread;
	os_error	*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			(handle->format != DISCFILE_LOCATE0 && handle->format != DISCFILE_LOCATE1) ||
			handle->section == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return 0;
	}

	/* Get the curent file position. */

	error = xosargs_read_ptrw(handle->handle, &ptr);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return 0;
	}

	/* Check that there are enough bytes left in the section. */

	if (sizeof(int) > (handle->section + handle->data_size + sizeof(int) - ptr)) {
		discfile_set_error(handle, "FileUnrec");
		return 0;
	}

	/* Read the contents. */

	error = xosgbpb_readw(handle->handle, (byte *) &data, sizeof(int), &unread);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return 0;
	}

	if (unread != 0) {
		discfile_set_error(handle, "FileUnrec");
		return 0;
	}

	return data;
}


/**
 * Read a string from the currently open section of a legacy disc file.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *text			Pointer to a buffer to contain the string.
 * \param size			The number of bytes available in the buffer.
 * \return			Pointer to the start of the buffer.
 */

char *discfile_legacy_read_string(struct discfile_block *handle, char *text, size_t size)
{
	int		ptr;
	unsigned	max_bytes, read;
	bits		psr = 0;
	os_error	*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			(handle->format != DISCFILE_LOCATE0 && handle->format != DISCFILE_LOCATE1) ||
			handle->section == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return text;
	}

	if (text == NULL)
		return NULL;

	/* Get the curent file position. */

	error = xosargs_read_ptrw(handle->handle, &ptr);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return text;
	}

	/* Work out how many bytes can be read, based on the size of the
	 * supplied buffer and the number of bytes left in the chunk.
	 */

	max_bytes = handle->section + handle->data_size + sizeof(int) - ptr;

	if (max_bytes <= 0) {
		discfile_set_error(handle, "FileUnrec");
		return text;
	}

	if (max_bytes > size)
		max_bytes = size;

	/* Read the bytes in from disc. */

	read = 0;

	while ((psr & 0x20000000) == 0 && read < max_bytes && error == NULL && (read == 0 || text[read - 1] != '\r')) {
		error = xos_bgetw(handle->handle, text + read, &psr);

		if (error == NULL && text[read] != '\0' && text[read] != '\n')
			read++;
	}

	if (error != NULL || read == 0) {
		text[0] = '\0';
		discfile_set_error(handle, "FileError");
		return text;
	}

	if (text[read - 1] != '\r') {
		text[read - 1] = '\0';
		discfile_set_error(handle, "FileUnrec");
		return text;
	}

	text[read - 1] = '\0';

	return text;
}


/**
 * Read a string from the currently open section of a legacy disc file, storing
 * it in a flex block.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *text			Pointer to a buffer to contain the string.
 * \param string_ptr		Pointer to a flex block to take the text.
 * \return			Pointer to the next free byte in the buffer.
 */

char *discfile_legacy_read_flex_string(struct discfile_block *handle, flex_ptr string_ptr)
{
	char		*text = NULL;
	int		ptr;
	unsigned	max_bytes, size, read;
	bits		psr = 0;
	os_error	*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			(handle->format != DISCFILE_LOCATE0 && handle->format != DISCFILE_LOCATE1) ||
			handle->section == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return NULL;
	}

	if (string_ptr == NULL || *string_ptr == NULL)
		return NULL;

	/* Get the curent file position. */

	error = xosargs_read_ptrw(handle->handle, &ptr);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return text;
	}

	max_bytes = handle->section + handle->data_size + sizeof(int) - ptr;

	if (max_bytes <= 0) {
		discfile_set_error(handle, "FileUnrec");
		return text;
	}

	/* Find the current flex memory allocation, and extend it to the
	 * minimum block size if necessary.
	 */

	size = flex_size(string_ptr);

	if (size < DISCFILE_FLEX_ALLOCATION) {
		if (flex_extend(string_ptr, DISCFILE_FLEX_ALLOCATION) == 0) {
			discfile_set_error(handle, "FileMem");
			return FALSE;
		}

		size = DISCFILE_FLEX_ALLOCATION;
	}

	text = (char *) *string_ptr;

	/* Read the bytes in from disc, extending the block as we go. */

	read = 0;

	while ((psr & 0x20000000) == 0 && read < size && max_bytes > 0 && error == NULL && (read == 0 || text[read - 1] != '\r')) {
		error = xos_bgetw(handle->handle, text + read, &psr);

		if (error == NULL && text[read] != '\0' && text[read] != '\n')
			read++;

		if (read >= size) {
			if (flex_extend(string_ptr, size + DISCFILE_FLEX_ALLOCATION) == 0) {
				discfile_set_error(handle, "FileMem");
				return FALSE;
			}

			size += DISCFILE_FLEX_ALLOCATION;

			text = (char *) *string_ptr;
		}

		max_bytes--;
	}

	if (error != NULL || read == 0) {
		text[0] = '\0';
		discfile_set_error(handle, "FileError");
		return text + 1;
	}

	if (text[read - 1] != '\r') {
		text[read - 1] = '\0';
		discfile_set_error(handle, "FileUnrec");
		return text + read;
	}

	/* Terminate the string, and free any extra memory. */

	text[read - 1] = '\0';

	if (read < size)
		flex_extend(string_ptr, read);

	return text + read;
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
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return;
	}

	handle->format = DISCFILE_UNKNOWN_FORMAT;

	/* Get the file extent. */

	error = xosargs_read_extw(handle->handle, &file_extent);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return;
	}

	/* Walk through the sections of the file, checking that all of the sizes
	 * make sense.
	 */

	section_ptr = sizeof(struct discfile_header);

	while (section_ptr < file_extent) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), section_ptr, &unread);
		if (error != NULL || unread != 0) {
			discfile_set_error(handle, "FileError");
			return;
		}

		if (section.magic_word != DISCFILE_SECTION_MAGIC_WORD) {
			discfile_set_error(handle, "FileUnrec");
			return;
		}
		
		if (section.type < 0 || section.type >= DISCFILE_MAX_SECTIONS) {
			discfile_set_error(handle, "FileUnrec");
			return;
		}

		if ((section.flags & DISCFILE_SECTION_FLAGS_UNUSED) != 0) {
			discfile_set_error(handle, "FileUnrec");
			return;
		}
		
		if (handle->section_info[section.type].count > 0 &&
				(handle->section_info[section.type].multiple == FALSE || (section.flags & DISCFILE_SECTION_FLAGS_MULTIPLE) == 0)) {
			discfile_set_error(handle, "FileUnrec");
			return;
		}
		
		handle->section_info[section.type].count++;
		
		if (section.flags & DISCFILE_SECTION_FLAGS_MULTIPLE)
			handle->section_info[section.type].multiple = TRUE;

		/* Walk through the chunks in the section, checking that all of
		 * the sizes make sense.
		 */

		chunk_ptr = section_ptr + sizeof(struct discfile_section);

		while (chunk_ptr < section_ptr + section.size) {
			error = xosgbpb_read_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), chunk_ptr, &unread);
			if (error != NULL || unread != 0) {
				discfile_set_error(handle, "FileError");
				return;
			}

			if (chunk.magic_word != DISCFILE_CHUNK_MAGIC_WORD) {
				discfile_set_error(handle, "FileUnrec");
				return;
			}

			if ((chunk.flags & DISCFILE_CHUNK_FLAGS_UNUSED) != 0) {
				discfile_set_error(handle, "FileUnrec");
				return;
			}

			chunk_ptr += WORDALIGN(chunk.size);
		}

		/* The end of the last chunk should align exactly with the end
		 * of the current section.
		 */

		if (chunk_ptr != (section_ptr + section.size)) {
			discfile_set_error(handle, "FileUnrec");
			return;
		}

		section_ptr += section.size;
	}

	/* The end of the last section should align exactly with the end of the
	 * file.
	 */

	if (section_ptr != file_extent) {
		discfile_set_error(handle, "FileUnrec");
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
			handle->format != DISCFILE_LOCATE2 ||
			handle->section != 0 || handle->chunk != 0 ||
			type >= DISCFILE_MAX_SECTIONS) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return FALSE;
	}
	
	ptr = handle->section_info[type].ptr;

	/* Get the file extent. */

	error = xosargs_read_extw(handle->handle, &extent);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	while (ptr < extent && handle->section == 0) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), ptr, &unread);
		if (error != NULL || unread != 0) {
			discfile_set_error(handle, "FileError");
			return FALSE;
		}

		if (section.magic_word != DISCFILE_SECTION_MAGIC_WORD || (section.flags & DISCFILE_SECTION_FLAGS_UNUSED) != 0) {
			discfile_set_error(handle, "FileUnrec");
			return FALSE;
		}

		if (section.type == type) {
			handle->section = ptr;
			
			/* If this is a "multiple" section, then on the next pass,
			 * start looking from the next section in the file.
			 */ 
			
			if (handle->section_info[type].multiple)
				handle->section_info[type].ptr = ptr + section.size;
		} else {
			ptr += section.size;
		}
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
			handle->format != DISCFILE_LOCATE2 ||
			handle->section == 0 || handle->chunk != 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
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
			handle->format != DISCFILE_LOCATE2 ||
			handle->section == 0 || handle->chunk != 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return FALSE;
	}

	/* The first chunk starts immediately following the section header. */

	ptr = handle->section + sizeof(struct discfile_section);

	/* Get the section extent. */

	error = xosgbpb_read_atw(handle->handle, (byte *) &section, sizeof(struct discfile_section), handle->section, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	if (section.magic_word != DISCFILE_SECTION_MAGIC_WORD || (section.flags & DISCFILE_SECTION_FLAGS_UNUSED) != 0) {
		discfile_set_error(handle, "FileUnrec");
		return FALSE;
	}

	extent = handle->section + section.size;

	while (ptr < extent && handle->chunk == 0) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &chunk, sizeof(struct discfile_chunk), ptr, &unread);
		if (error != NULL || unread != 0) {
			discfile_set_error(handle, "FileError");
			return FALSE;
		}

		if (chunk.magic_word != DISCFILE_CHUNK_MAGIC_WORD || (chunk.flags & DISCFILE_CHUNK_FLAGS_UNUSED) != 0) {
			discfile_set_error(handle, "FileUnrec");
			return FALSE;
		}

		if (chunk.type == type) {
			handle->chunk = ptr;
			handle->data_size = chunk.size;
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
			handle->format != DISCFILE_LOCATE2 ||
			handle->section == 0 || handle->chunk == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return;
	}

	handle->chunk = 0;
	handle->data_size = 0;
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
			handle->format != DISCFILE_LOCATE2 ||
			handle->section == 0 || handle->chunk == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return 0;
	}

	return handle->data_size - sizeof(struct discfile_chunk);
}


/**
 * Read a boolean option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param *value		Pointer to an osbool variable to take the value.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_boolean(struct discfile_block *handle, char *tag, osbool *value)
{
	unsigned			id;
	int				ptr, unread;
	os_error			*error;
	struct discfile_option		option;

	if (handle == NULL || tag == NULL || value == NULL)
		return FALSE;

	id = discfile_make_id(DISCFILE_OPTION_BOOLEAN, tag);
	ptr = discfile_find_option_data(handle, id);

	if (ptr == 0)
		return FALSE;

	error = xosgbpb_read_atw(handle->handle, (byte *) &option, sizeof(struct discfile_option), ptr, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	*value = option.data.data_osbool;

	return TRUE;
}


/**
 * Read an unsigned option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param *value		Pointer to an unsigned variable to take the value.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_unsigned(struct discfile_block *handle, char *tag, unsigned *value)
{
	unsigned			id;
	int				ptr, unread;
	os_error			*error;
	struct discfile_option		option;

	if (handle == NULL || tag == NULL || value == NULL)
		return FALSE;

	id = discfile_make_id(DISCFILE_OPTION_UNSIGNED, tag);
	ptr = discfile_find_option_data(handle, id);

	if (ptr == 0)
		return FALSE;

	error = xosgbpb_read_atw(handle->handle, (byte *) &option, sizeof(struct discfile_option), ptr, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	*value = option.data.data_unsigned;

	return TRUE;
}


/**
 * Read a string option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param *value		Pointer to an string buffer to take the text.
 * \param length		The size of the supplied buffer.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_string(struct discfile_block *handle, char *tag, char *value, size_t length)
{
	unsigned			id;
	int				ptr, unread;
	os_error			*error;
	struct discfile_option		option;

	if (handle == NULL || tag == NULL || value == NULL)
		return FALSE;

	id = discfile_make_id(DISCFILE_OPTION_STRING, tag);
	ptr = discfile_find_option_data(handle, id);

	if (ptr == 0) {
		value[0] = '\0';
		return FALSE;
	}

	error = xosgbpb_read_atw(handle->handle, (byte *) &option, sizeof(struct discfile_option), ptr, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		value[0] = '\0';
		return FALSE;
	}

	if (option.data.length > length) {
		value[0] = '\0';
		return FALSE;
	}

	discfile_read_string(handle, value, length);
	return TRUE;
}


/**
 * Read a string option from an open chunk in a discfile, storing it in a
 * string within a flex block.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param string_ptr		Pointer to a flex block to take the text.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_flex_string(struct discfile_block *handle, char *tag, flex_ptr string_ptr)
{
	unsigned			id, length;
	int				ptr, unread;
	os_error			*error;
	struct discfile_option		option;

	if (handle == NULL || tag == NULL || string_ptr == NULL || *string_ptr == NULL)
		return FALSE;

	id = discfile_make_id(DISCFILE_OPTION_STRING, tag);
	ptr = discfile_find_option_data(handle, id);

	if (ptr == 0)
		return FALSE;

	/* If the incloming flex block has space, terminate the string now so
	 * that there's something valid in there if an error occurs.
	 */

	if (flex_size(string_ptr) > 0)
		((char *) *string_ptr)[0] = '\0';

	error = xosgbpb_read_atw(handle->handle, (byte *) &option, sizeof(struct discfile_option), ptr, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	/* Allocate enough memory to store the string. */

	if (flex_extend(string_ptr, option.data.length) == 0) {
		discfile_set_error(handle, "FileMem");
		return FALSE;
	}

	discfile_read_string(handle, *string_ptr, option.data.length);
	length = strlen(*string_ptr) + 1;

	if (length < option.data.length)
		flex_extend(string_ptr, length);

	return TRUE;
}


/**
 * Read a date option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param date			A date structure to take the date.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_date(struct discfile_block *handle, char *tag, os_date_and_time date)
{
	unsigned			id, words[2];
	int				ptr, unread;
	os_error			*error;
	struct discfile_option		option;

	if (handle == NULL || tag == NULL)
		return FALSE;

	id = discfile_make_id(DISCFILE_OPTION_DATE, tag);
	ptr = discfile_find_option_data(handle, id);

	if (ptr == 0)
		return FALSE;

	error = xosgbpb_read_atw(handle->handle, (byte *) &option, sizeof(struct discfile_option), ptr, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	if (option.data.length != 2 * sizeof(unsigned)) {
		discfile_set_error(handle, "FileUnrec");
		return FALSE;
	}

	error = xosgbpb_readw(handle->handle, (byte *) words, 2 * sizeof(unsigned), &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	datetime_set_date(date, words[1], words[0]);

	return TRUE;
}


/**
 * Read an unsigned array option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param array_ptr		A flex pointer to use for the array.
 * \param terminator		The terminator to place at the end of the array.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_unsigned_array(struct discfile_block *handle, char *tag, flex_ptr array_ptr, unsigned terminator)
{
	unsigned			id, *array, length;
	int				ptr, unread;
	os_error			*error;
	struct discfile_option		option;

	if (handle == NULL || tag == NULL || array_ptr == NULL)
		return FALSE;

	id = discfile_make_id(DISCFILE_OPTION_UNSIGNED_ARRAY, tag);
	ptr = discfile_find_option_data(handle, id);

	if (ptr == 0)
		return FALSE;

	error = xosgbpb_read_atw(handle->handle, (byte *) &option, sizeof(struct discfile_option), ptr, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		return FALSE;
	}

	if (option.data.length % sizeof(unsigned) != 0) {
		discfile_set_error(handle, "FileUnrec");
		return FALSE;
	}

	length = option.data.length / sizeof(unsigned);

	/* Allocate enough memory to store all the entries, plus a terminator. */

	if (flex_extend(array_ptr, sizeof(unsigned) * (length + 1)) == 0) {
		discfile_set_error(handle, "FileMem");
		return FALSE;
	}

	array = (unsigned *) *array_ptr;

	error = xosgbpb_readw(handle->handle, (byte *) array, option.data.length, &unread);
	if (error != NULL || unread != 0) {
		discfile_set_error(handle, "FileError");
		array[0] = terminator;
		return FALSE;
	}

	array[length] = terminator;

	return TRUE;
}


/**
 * Locate an option with the given ID word in the currently open chunk, returning
 * a pointer to its option block.
 *
 * \param *handle		The discfile handle being read from.
 * \param id			The ID word to look for.
 * \return			The file pointer to the option block, or 0.
 */

static int discfile_find_option_data(struct discfile_block *handle, unsigned id)
{
	int				ptr, location, unread;
	struct discfile_option		option;
	os_error			*error;

	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->format != DISCFILE_LOCATE2 ||
			handle->section == 0 || handle->chunk == 0) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return 0;
	}

	ptr = handle->chunk + sizeof(struct discfile_chunk);
	location = 0;

	while (ptr < handle->chunk + handle->data_size && location == 0) {
		error = xosgbpb_read_atw(handle->handle, (byte *) &option, sizeof(struct discfile_option), ptr, &unread);
		if (error != NULL || unread != 0) {
			discfile_set_error(handle, "FileError");
			return 0;
		}

		if (option.id == id) {
			location = ptr;
		} else {
			switch (option.id & 0xffu) {
			case DISCFILE_OPTION_STRING:
			case DISCFILE_OPTION_DATE:
			case DISCFILE_OPTION_UNSIGNED_ARRAY:
				ptr += sizeof(struct discfile_option) + option.data.length;
				break;

			default:
				ptr += sizeof(struct discfile_option);
				break;
			}
		}
	}

	return location;
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
	bits		psr = 0;
	os_error	*error;


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->format != DISCFILE_LOCATE2 ||
			handle->section == 0 || handle->chunk == 0 || text == NULL) {
		if (handle != NULL) {
			if (text != NULL)
				text[0] = '\0';
			discfile_set_error(handle, "FileError");
		}
		return text;
	}

	/* Get the curent file position. */

	error = xosargs_read_ptrw(handle->handle, &ptr);
	if (error != NULL) {
		text[0] = '\0';
		discfile_set_error(handle, "FileError");
		return text;
	}

	/* Work out how many bytes can be read, based on the size of the
	 * supplied buffer and the number of bytes left in the chunk.
	 */

	max_bytes = handle->chunk + handle->data_size - ptr;

	if (max_bytes > size)
		max_bytes = size;

	/* Read the bytes in from disc. */

	read = 0;

	while ((psr & 0x20000000) == 0 && read < max_bytes && error == NULL && (read == 0 || text[read - 1] != '\0'))
		error = xos_bgetw(handle->handle, text + (read++), &psr);

	if (error != NULL || read == 0) {
		text[0] = '\0';
		discfile_set_error(handle, "FileError");
		return text + 1;
	}

	if (text[read - 1] != '\0') {
		text[read - 1] = '\0';
		discfile_set_error(handle, "FileUnrec");
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


	if (handle == NULL || handle->handle == 0 || handle->mode != DISCFILE_READ ||
			handle->format != DISCFILE_LOCATE2 ||
			handle->section == 0 || handle->chunk == 0 || data == NULL) {
		if (handle != NULL)
			discfile_set_error(handle, "FileError");
		return;
	}

	/* Get the curent file position. */

	error = xosargs_read_ptrw(handle->handle, &ptr);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return;
	}

	/* Check that there are enough bytes left in the chunk. */

	if (size > (handle->chunk + handle->data_size - ptr)) {
		discfile_set_error(handle, "FileUnrec");
		return;
	}

	/* Read the contents. */

	error = xosgbpb_readw(handle->handle, (byte *) data, size, &unread);
	if (error != NULL) {
		discfile_set_error(handle, "FileError");
		return;
	}

	if (unread != 0) {
		discfile_set_error(handle, "FileUnrec");
		return;
	}
}


/**
 * Set an error state on an open disc file.
 *
 * \param *handle		The discfile handle to be closed.
 * \param *token		The token to use for an error message.
 */

void discfile_set_error(struct discfile_block *handle, char *token)
{
	if (handle == NULL || token == NULL)
		return;

	if (handle->error_token == NULL)
		handle->error_token = token;
	handle->mode = DISCFILE_ERROR;

	handle->section = 0;
	handle->chunk = 0;
	handle->data_size = 0;
}


/**
 * Close a discfile and free any memory associated with it.
 *
 * \param *handle		The discfile handle to be closed.
 * \return			TRUE if the file had an error flagged; else FALSE.
 */

osbool discfile_close(struct discfile_block *handle)
{
	osbool	error;

	if (handle == NULL)
		return FALSE;

	/* Close the RISC OS file, if open. */

	if (handle->handle != 0)
		xosfind_closew(handle->handle);

	/* Handle any error that is pending. */

	error = (handle->mode == DISCFILE_ERROR) ? TRUE : FALSE;

	if (error == TRUE)
		error_msgs_report_error(handle->error_token);

	/* Free the discfile handle storage. */

	heap_free(handle);

	return error;
}


/**
 * Return a four-byte word containing a data type and ID code, made up from
 * the id byte in byte 0 and the first three characters in a string in bytes
 * 1 to 3.
 *
 * \param type			The block type.
 * \param *code			The string to use for the code.
 * \return			The numeric ID code.
 */

static unsigned discfile_make_id(unsigned type, char *code)
{
	unsigned	id = 0;
	int		i;

	id = type & 0xffu;

	for (i = 0; i < 3 && code[i] != '\0'; i++)
		id += (code[i] << (8 * (i + 1)));

	return id;
}

