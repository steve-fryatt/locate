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
 * \file: file.h
 *
 * Locate disc file format implementation.
 */

#ifndef LOCATE_DISCFILE
#define LOCATE_DISCFILE

#include "oslib/os.h"

#include "flex.h"

#define DISCFILE_LOCATE_FILETYPE (0x1a1)

/**
 * The known file formats.
 */

enum discfile_format {
	DISCFILE_UNKNOWN_FORMAT = 0,						/**< The format has not been recognised.		*/
	DISCFILE_LOCATE1 = 1,							/**< The file is from Locate 1.				*/
	DISCFILE_LOCATE2 = 2							/**< The file is from Locate 2.				*/
};

enum discfile_section_type {
	DISCFILE_SECTION_UNKNOWN = 0,						/**< The section type is unknown.			*/
	DISCFILE_SECTION_OBJECTDB = 1,						/**< The section contains an object database.		*/
	DISCFILE_SECTION_RESULTS = 2,						/**< The section contains a results window definition.	*/
	DISCFILE_SECTION_DIALOGUE = 3						/**< The section contains dialogue settings.		*/
};

enum discfile_chunk_type {
	DISCFILE_CHUNK_UNKNOWN = 0,						/**< The chunk type is unknown.				*/
	DISCFILE_CHUNK_TEXTDUMP = 1,						/**< The chunk contains the contents of a textdump.	*/
	DISCFILE_CHUNK_OBJECTS = 2,						/**< The chunk contains objects from an ObjectDB.	*/
	DISCFILE_CHUNK_RESULTS = 3,						/**< The chunk contains entries from a results window.	*/
	DISCFILE_CHUNK_OPTIONS = 4						/**< The chunk contains a series of option values.	*/
};


struct discfile_block;

/**
 * Open a new file for writing and return its handle.
 *
 * \param *filename		Pointer to the filename to open.
 * \return			Pointer to an discfile handle, or NULL.
 */

struct discfile_block *discfile_open_write(char *filename);


/**
 * Open a new section in a disc file, ready for chunks of data to be written to
 * it.
 *
 * \param *handle		The discfile handle to be written to.
 * \param type			The section type for the new section.
 */

void discfile_start_section(struct discfile_block *handle, enum discfile_section_type type);


/**
 * Close an already open section of a discfile, updating the section header
 * appropriately.
 *
 * \param *handle		The discfile handle to be written to.
 */

void discfile_end_section(struct discfile_block *handle);


/**
 * Open a new chunk in a disc file, ready for data to be written to it.
 *
 * \param *handle		The discfile handle to be written to.
 * \param type			The section type for the new section.
 */

void discfile_start_chunk(struct discfile_block *handle, enum discfile_chunk_type type);


/**
 * Close an already open chunk of a discfile, updating the chunk header
 * appropriately.
 *
 * \param *handle		The discfile handle to be written to.
 */

void discfile_end_chunk(struct discfile_block *handle);


/**
 * Write a boolean value to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the value.
 * \param value			The value to be written.
 */

void discfile_write_option_boolean(struct discfile_block *handle, char *tag, osbool value);


/**
 * Write an unsigned value to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the value.
 * \param value			The value to be written.
 */

void discfile_write_option_unsigned(struct discfile_block *handle, char *tag, unsigned value);


/**
 * Write a text string to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the text.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_option_string(struct discfile_block *handle, char *tag, char *text);


/**
 * Write an unsigned array to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the text.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_option_unsigned_array(struct discfile_block *handle, char *tag, unsigned *array, unsigned terminator);


/**
 * Write an OS date to an open chunk in a file.
 *
 * \param *handle		The handle to be written to.
 * \param *tag			The tag to give to the text.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_option_date(struct discfile_block *handle, char *tag, os_date_and_time date);


/**
 * Write a string chunk to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *text			Pointer to the text to be written.
 * \return			Pointer to the byte after the string terminator.
 */

char *discfile_write_string(struct discfile_block *handle, char *text);


/**
 * Write generic chunk data to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *data			Pointer to the first byte of data to be written.
 * \param size			The number of bytes to be written.
 */

void discfile_write_chunk(struct discfile_block *handle, byte *data, unsigned size);


/**
 * Open an existing file for reading and return its handle.
 *
 * \param *filename		Pointer to the filename to open.
 * \return			Pointer to an discfile handle, or NULL.
 */

struct discfile_block *discfile_open_read(char *filename);


/**
 * Open a section from a disc file, ready for chunks of data to be read from
 * it.
 *
 * \param *handle		The discfile handle to be read from.
 * \param type			The type of the section to be opened.
 * \return			TRUE if the section was opened; else FALSE.
 */

osbool discfile_open_section(struct discfile_block *handle, enum discfile_section_type type);


/**
 * Close a section from a disc file after reading from it.
 *
 * \param *handle		The discfile handle to be read from.
 */

void discfile_close_section(struct discfile_block *handle);


/**
 * Open a chunk from a disc file, ready for data to be read from it.
 *
 * \param *handle		The discfile handle to be read from.
 * \param type			The type of the chunk to be opened.
 * \return			TRUE if the section was opened; else FALSE.
 */

osbool discfile_open_chunk(struct discfile_block *handle, enum discfile_chunk_type type);


/**
 * Close a chunk from a disc file after reading from it.
 *
 * \param *handle		The discfile handle to be read from.
 */

void discfile_close_chunk(struct discfile_block *handle);


/**
 * Return the size of the currently open chunk from a disc file.
 *
 * \param *handle		The discfile handle to be read from.
 * \return			The number of bytes of data in the chunk.
 */

int discfile_chunk_size(struct discfile_block *handle);


/**
 * Read a boolean option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param *value		Pointer to an osbool variable to take the value.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_boolean(struct discfile_block *handle, char *tag, osbool *value);


/**
 * Read an unsigned option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param *value		Pointer to an unsigned variable to take the value.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_unsigned(struct discfile_block *handle, char *tag, unsigned *value);


/**
 * Read an string option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param *value		Pointer to an string buffer to take the text.
 * \param length		The size of the supplied buffer.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_string(struct discfile_block *handle, char *tag, char *value, size_t length);


/**
 * Read a string option from an open chunk in a discfile, storing it in a
 * string within a flex block.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param *value		Pointer to an string buffer to take the text.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_flex_string(struct discfile_block *handle, char *tag, flex_ptr string_ptr);


/**
 * Read a date option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param date			A date structure to take the date.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_date(struct discfile_block *handle, char *tag, os_date_and_time date);


/**
 * Read an unsigned array option from an open chunk in a discfile.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *tag			The tag of the option to be read.
 * \param array_ptr		A flex pointer to use for the array.
 * \param terminator		The terminator to place at the end of the array.
 * \return			TRUE if a value was found; else FALSE.
 */

osbool discfile_read_option_unsigned_array(struct discfile_block *handle, char *tag, flex_ptr array_ptr, unsigned terminator);


/**
 * Read a string chunk from disc, out of an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be read from.
 * \param *text			Pointer to the text to be written.
 * \return			Pointer to the next free byte in the buffer.
 */

char *discfile_read_string(struct discfile_block *handle, char *text, size_t size);


/**
 * Read a generic chunk data from disc, out of an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *data			Pointer to the buffer into which to read the data.
 * \param size			The number of bytes to be read.
 */

void discfile_read_chunk(struct discfile_block *handle, byte *data, unsigned size);


/**
 * Set an error state on an open disc file.
 *
 * \param *handle		The discfile handle to be closed.
 * \param *token		The token to use for an error message.
 */

void discfile_set_error(struct discfile_block *handle, char *token);


/**
 * Close a discfile and free any memory associated with it.
 *
 * \param *handle		The discfile handle to be closed.
 * \return			TRUE if the file had an error flagged; else FALSE.
 */

osbool discfile_close(struct discfile_block *handle);

#endif

