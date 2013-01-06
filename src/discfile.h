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
	DISCFILE_SECTION_SEARCH = 3						/**< The section contains search settings.		*/
};

enum discfile_chunk_type {
	DISCFILE_CHUNK_UNKNOWN = 0,						/**< The chunk type is unknown.				*/
	DISCFILE_CHUNK_TEXTDUMP = 1,						/**< The chunk contains the contents of a textdump.	*/
	DISCFILE_CHUNK_OBJECTS = 2,						/**< The chunk contains objects from an ObjectDB.	*/
	DISCFILE_CHUNK_RESULTS = 3						/**< The chunk contains entries from a results window.	*/
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
 * Write a string chunk to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *id			The ID (1-4 characters) for the chunk.
 * \param *text			Pointer to the text to be written.
 * \return			Pointer to the byte after the string terminator.
 */

char *discfile_write_string(struct discfile_block *handle, char *text);


/**
 * Write generic chunk data to disc, into an already open chunk of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *id			The ID (1-4 characters) for the chunk.
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
 * Close a discfile and free any memory associated with it.
 *
 * \param *handle		The discfile handle to be closed.
 */

void discfile_close(struct discfile_block *handle);

#endif

