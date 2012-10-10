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
 * \file: objdb.h
 *
 * Filer object database implementation.
 */

#ifndef LOCATE_OBJDB
#define LOCATE_OBJDB

#include "oslib/osgbpb.h"

#include "file.h"

#define OBJDB_NULL_KEY 0xffffffffu

#define OBJDB_NAME_LENGTH 64
#define OBJDB_SPRITE_LENGTH 20
#define OBJDB_COMMAND_LENGTH 1024


struct objdb_block;


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
 * Return the filetype of an object in the database.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \return			The filetype, or 0xffffffffu.
 */

unsigned objdb_get_filetype(struct objdb_block *handle, unsigned key);









/**
 * Load the contents of a button file into the buttons database.
 *
 * \param *leaf_name		The file leafname to load.
 * \return			TRUE on success; else FALSE.
 */

osbool objdb_load_file(char *leaf_name);


/**
 * Save the contents of the buttons database into a buttons file.
 *
 * \param *leaf_name		The file leafname to save to.
 * \return			TRUE on success; else FALSE.
 */

osbool objdb_save_file(char *leaf_name);


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
 * Given a database key, return the next key from the database.
 *
 * \param *handle		The database to iterate through.
 * \param key			The current key, or OBJDB_NULL_KEY to start sequence.
 * \return			The next key, or OBJDB_NULL_KEY.
 */

unsigned objdb_get_next_key(struct objdb_block *handle, unsigned key);


/**
 * Given a key, return details of the button associated with the application.
 * Any parameters passed as NULL will not be returned. String pointers will only
 * remain valid until the memory heap is disturbed.
 *
 * \param key			The key of the netry to be returned.
 * \param *x_pos		Place to return the X coordinate of the button.
 * \param *y_pos		Place to return the Y coordinate of the button.
 * \param **name		Place to return a pointer to the button name.
 * \param **sprite		Place to return a pointer to the sprite name
 *				used in the button.
 * \param **command		Place to return a pointer to the command associated
 *				with a button.
 * \param *local_copy		Place to return the local copy flag.
 * \param *filer_boot		Place to return the filer boot flag.
 * \return			TRUE if an entry was found; else FALSE.
 */

//osbool objdb_get_button_info(unsigned key, int *x_pos, int *y_pos, char **name, char **sprite,
//		char **command, osbool *local_copy, osbool *filer_boot);


/**
 * Given a key, set details of the button associated with the application.
 *
 * \param key			The key of the entry to be updated.
 * \param *x_pos		The new X coordinate of the button.
 * \param *y_pos		The new Y coordinate of the button.
 * \param **name		Pointer to the new button name.
 * \param **sprite		Pointer to the new sprite name to be used in the button.
 * \param **command		Pointer to the command associated with the button.
 * \param *local_copy		The new local copy flag.
 * \param *filer_boot		The new filer boot flag.
 * \return			TRUE if an entry was updated; else FALSE.
 */

//osbool objdb_set_button_info(unsigned key, int x_pos, int y_pos, char *name, char *sprite,
//		char *command, osbool local_copy, osbool filer_boot);


/**
 * Dump details of a database's text hash to Reporter for debugging.
 *
 * \TODO -- Remove once unused.
 *
 * \param *handle		The database to dump the details from.
 */

void objdb_output_hash_data(struct objdb_block *handle);

#endif

