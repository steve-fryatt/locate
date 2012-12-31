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
	DISCFILE_UNKNOWN_SECTION = 0,						/**< The section type is unknown.			*/
	DISCFILE_OBJECTDB_SECTION = 1,						/**< The section contains an object database.		*/
	DISCFILE_RESULTS_SECTION = 2,						/**< The section contains a results window definition.	*/
	DISCFILE_SEARCH_SECTION = 3						/**< The section contains search settings.		*/
};

enum discfile_chunk_type {
	DISCFILE_UNKNOWN_CHUNK = 0,						/**< The chunk type is unknown.				*/
	DISCFILE_BLOB_CHUNK = 1,						/**< The chunk type is a binary object.			*/
	DISCFILE_TEXT_CHUNK = 2							/**< The chunk type is a text string.			*/
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
 * Write a binary chunk to disc, into an already open section of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *id			The ID (1-4 characters) for the chunk.
 * \param *data			Pointer to the first byte of data to be written.
 * \param size			The number of bytes to be written.
 */

void discfile_write_blob(struct discfile_block *handle, char *id, byte *data, unsigned size);


/**
 * Write a string chunk to disc, into an already open section of a file.
 *
 * \param *handle		The discfile handle to be written to.
 * \param *id			The ID (1-4 characters) for the chunk.
 * \param *text			Pointer to the text to be written.
 */

void discfile_write_string(struct discfile_block *handle, char *id, char *text);


/**
 * Open an existing file for reading and return its handle.
 *
 * \param *filename		Pointer to the filename to open.
 * \return			Pointer to an discfile handle, or NULL.
 */

struct discfile_block *discfile_open_read(char *filename);


/**
 * Close a discfile and free any memory associated with it.
 *
 * \param *handle		The discfile handle to be closed.
 */

void discfile_close(struct discfile_block *handle);

#endif

