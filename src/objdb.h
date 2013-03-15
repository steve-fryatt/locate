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
 * \file: objdb.h
 *
 * Filer object database implementation.
 */

#ifndef LOCATE_OBJDB
#define LOCATE_OBJDB

#include "oslib/osgbpb.h"

#include "discfile.h"
#include "file.h"

#define OBJDB_NULL_KEY 0xffffffffu

#define OBJDB_NAME_LENGTH 64
#define OBJDB_SPRITE_LENGTH 20
#define OBJDB_COMMAND_LENGTH 1024


struct objdb_block;

/**
 * Object status values.
 */

enum objdb_status {
	OBJDB_STATUS_ERROR = 0,							/**< There was an error in making the call.			*/
	OBJDB_STATUS_UNCHANGED,							/**< The object is present and unchanged.			*/
	OBJDB_STATUS_CHANGED,							/**< The object is present but its catalogue info has changed.	*/
	OBJDB_STATUS_MISSING							/**< The object is no longer in its recorded location.		*/
};

/**
 * File information structure.
 */

struct objdb_info {
	enum objdb_status	status;						/**< The status of the object in question.			*/
	unsigned		filetype;					/**< The object's filetype.					*/
};

/**
 * Create a new object database, returning the handle.
 *
 * \param *file			The file to which the database will belong.
 * \return 			The new database handle, or NULL on failure.
 */

struct objdb_block *objdb_create(struct file_block *file);


/**
 * Destroy an object database block and release all of the memory that
 * it uses.
 *
 * \param *handle		The database block to be destroyed.
 */

void objdb_destroy(struct objdb_block *handle);


/**
 * Add a search root to an object database.
 *
 * \param *handle		The handle of the database to add the root to.
 * \param *path			The path of the root to be added.
 * \return			The key of the new object.
 */

unsigned objdb_add_root(struct objdb_block *handle, char *path);


/**
 * Store a file in the object database, using its OS_GBPB file descriptor block
 * to supply the information.
 *
 * \param *handle		The handle of the database to add the file to.
 * \param parent		The key of the parent object in the database.
 * \param *file			The file data to be added.
 * \return			The key of the new object.
 */

unsigned objdb_add_file(struct objdb_block *handle, unsigned parent, osgbpb_info *file);


/**
 * Validate an entry in the object database by checking to see if it is still
 * present on disc in the same location.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \param retest		TRUE to check the disc; FALSE to rely on the stored data.
 * \return			The object's status.
 */

enum objdb_status objdb_validate_file(struct objdb_block *handle, unsigned key, osbool retest);


/**
 * Return the parent of an object in the database.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \return			The parent object key, or OBJDB_NULL_KEY if none.
 */

unsigned objdb_get_parent(struct objdb_block *handle, unsigned key);


/**
 * Return the pathname of an object in the database.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \param *buffer		Pointer to a buffer to hold the name.
 * \param len			The size of the supplied buffer.
 * \return			TRUE if successful; else FALSE.
 */

osbool objdb_get_name(struct objdb_block *handle, unsigned key, char *buffer, size_t len);


/**
 * Return the size of the buffer required to store the pathname of an object
 * in the database, including terminator. If key is OBJDB_NULL_KEY then the
 * size required for the longest path is returned.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object of interest, or
 *				OBJDB_NULL_KEY for the longest.
 * \return			The required buffer size including terminator.
 */

size_t objdb_get_name_length(struct objdb_block *handle, unsigned key);


/**
 * Return the filetype of an object in the database.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \return			The filetype, or 0xffffffffu.
 */

unsigned objdb_get_filetype(struct objdb_block *handle, unsigned key);


/**
 * Return information on an object in the database, or details of the memory
 * buffer required to return those details.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned, or OBJDB_NULL_KEY.
 * \param *info			A block to take the information, or NULL to get required size.
 * \param *additional		A block to return addition info, or NULL to get the required info size.
 * \return			The required block size.
 */

size_t objdb_get_info(struct objdb_block *handle, unsigned key, osgbpb_info *info, struct objdb_info *additional);


/**
 * Load the contents of an object file into the database.
 *
 * \param *file			The file to which the database will belong.
 * \param *load			The discfile handle to load from.
 * \return			The new databse, or NULL on failure.
 */

struct objdb_block *objdb_load_file(struct file_block *file, struct discfile_block *load);


/**
 * Save the contents of the database into a discfile.
 *
 * \param *handle		The database to be saved.
 * \param *file			The discfile handle to save to.
 * \return			TRUE on success; else FALSE.
 */

osbool objdb_save_file(struct objdb_block *handle, struct discfile_block *file);


/**
 * Create a new, empty entry in the database and return its key.
 *
 * \param *handle		The database in which to create a new key.
 * \return			The new key, or OBJDB_NULL_KEY.
 */

unsigned objdb_create_key(struct objdb_block *handle);


/**
 * Delete an entry from the database.
 *
 * \param *handle		The database which to delete the entry.
 * \param key			The key of the entry to delete.
 */

void objdb_delete_key(struct objdb_block *handle, unsigned key);


/**
 * Delete an entry from the database, if it was the last one to be added.
 * This allows a recursive filesearch routine to add all nodes to the database,
 * then delete the unused ones once it knows that they won't be required.  This
 * only works if all of the chidren for a node are added immediately followng
 * the node.
 *
 * \param *handle		The database which to delete the entry.
 * \param key			The key of the entry to delete.
 */

void objdb_delete_last_key(struct objdb_block *handle, unsigned key);


/**
 * Given a database key, return the next key from the database.
 *
 * \param *handle		The database to iterate through.
 * \param key			The current key, or OBJDB_NULL_KEY to start sequence.
 * \return			The next key, or OBJDB_NULL_KEY.
 */

unsigned objdb_get_next_key(struct objdb_block *handle, unsigned key);

#endif

