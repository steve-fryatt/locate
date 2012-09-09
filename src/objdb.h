/* Locate - objdb.h
 * (c) Stephen Fryatt, 2012
 *
 * Filer Object Database.
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






unsigned objdb_add_file(struct objdb_block *handle, unsigned parent, osgbpb_info *file);











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

#endif

