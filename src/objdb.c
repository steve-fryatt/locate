/* Locate - objdb.c
 * (c) Stephen Fryatt, 2012
 *
 * Filer Object Database.
 */

/* ANSI C header files. */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files. */

#include "oslib/os.h"
#include "oslib/osgbpb.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files. */

#include "objdb.h"

#include "file.h"
#include "textdump.h"


#define OBJDB_ALLOC_CHUNK 100

/**
 * Data structure for an object database instance.
 */

struct objdb_block
{
	struct file_block	*file;						/**< The file to which the object database belongs.		*/

	struct object		*list;						/**< Array of object data.					*/
	struct textdump_block	*text;						/**< Textdump for object names.					*/

	unsigned		objects;					/**< The number of objects stored in the database.		*/
	unsigned		allocation;					/**< The number of objects for which space is allocated.	*/

	unsigned		key;						/**< Track new unique primary keys.				*/
};

/**
 * Data structure for a filing system object.
 */

struct object
{
	unsigned		key;						/**< Primary key to index database entries.			*/

	unsigned		parent;						/**< The key of the parent object, or OBJDB_NULL_KEY.		*/

	bits			load_addr;					/**< The load address of the object.				*/
	bits			exec_addr;					/**< The execution address of the object.			*/
	unsigned		size;						/**< The size of the object in bytes.				*/
	fileswitch_attr		attributes;					/**< The file attributes of the object.				*/
	fileswitch_object_type	type;						/**< The fileswitch object type of the object.			*/
	unsigned		name;						/**< Textdump offset to the name of the object.			*/
};


//static struct object		*objdb_list = NULL;			/**< Array of application data.					*/

//static int				objdb_apps = 0;				/**< The number of applications stored in the database.		*/
//static int				objdb_allocation = 0;			/**< The number of applications for which space is allocated.	*/

//static unsigned				objdb_key = 0;				/**< Track new unique primary keys.				*/


static int	objdb_find(struct objdb_block *handle, unsigned key);
static int	objdb_new(struct objdb_block *handle);
static void	objdb_delete(struct objdb_block *handle, int index);


/**
 * Create a new object database, returning the handle.
 *
 * \param *file			The file to which the database will belong.
 * \return 			The new database handle, or NULL on failure.
 */

struct objdb_block *objdb_create(struct file_block *file)
{
	struct objdb_block	*new;


	if (file == NULL)
		return NULL;

	/* Claim the required memory and initialise the contents. */

	new = heap_alloc(sizeof(struct objdb_block));
	if (new == NULL)
		return NULL;

	new->file = file;

	new->objects = 0;
	new->allocation = 0;
	new->key = 0;

	/* Claim the database flex block and a text dump for the names. */

	if (flex_alloc((flex_ptr) &(new->list),
			(new->allocation + OBJDB_ALLOC_CHUNK) * sizeof(struct object)) == 1)
		new->allocation += OBJDB_ALLOC_CHUNK;
	else
		new->list = NULL;

	new->text = textdump_create(0, 20);

	/* If either of the sub allocations failed, free the claimed memory and exit. */

	if (new->list == NULL || new->text == NULL) {
		if (new->list != NULL)
			flex_free((flex_ptr) &(new->list));

		if (new->text != NULL)
			textdump_destroy(new->text);

		heap_free(new);

		return NULL;
	}

	debug_printf("New object database 0x%x created", new);

	return new;
}


/**
 * Destroy an object database block and release all of the memory that
 * it uses.
 *
 * \param *handle		The database block to be destroyed.
 */

void objdb_destroy(struct objdb_block *handle)
{
	if (handle == NULL)
		return;

	if (handle->text != NULL)
		textdump_destroy(handle->text);

	if (handle->list != NULL)
		flex_free((flex_ptr) &(handle->list));

	debug_printf("Object database 0x%x destroyed.", handle);

	heap_free(handle);
}


/*
 * There's no need to set the root at this stage: just create a new root as a
 * type of object and allocate object types...
 * OBJDB_ROOT
 * OBJDB_OBJECT?
 */


/**
 * Add a search root to an object database.
 *
 * \param *handle		The handle of the database to add the root to.
 * \param *path			The path of the root to be added.
 * \return			The key of the new object.
 */

unsigned objdb_add_root(struct objdb_block *handle, char *path)
{
	int	index = objdb_new(handle);

	if (handle == NULL || path == NULL || index == -1)
		return OBJDB_NULL_KEY;

	handle->list[index].parent = OBJDB_NULL_KEY;
	handle->list[index].name = textdump_store(handle->text, path);

	debug_printf("\\YAdding root details for %s with key %u", path, handle->list[index].key);

	return handle->list[index].key;
}


/**
 * Store a file in the object database, using its OS_GBPB file descriptor block
 * to supply the information.
 *
 * \param *handle		The handle of the database to add the file to.
 * \param parent		The key of the parent object in the database.
 * \param *file			The file data to be added.
 * \return			The key of the new object.
 */

unsigned objdb_add_file(struct objdb_block *handle, unsigned parent, osgbpb_info *file)
{
	int	index = objdb_new(handle);

	if (handle == NULL || file == NULL || index == -1)
		return OBJDB_NULL_KEY;

	handle->list[index].parent = parent;

	handle->list[index].load_addr = file->load_addr;
	handle->list[index].exec_addr = file->exec_addr;
	handle->list[index].size = file->size;
	handle->list[index].attributes = file->attr;
	handle->list[index].type = file->obj_type;
	handle->list[index].name = textdump_store(handle->text, file->name);

	debug_printf("\\YAdding file details for %s to %u with key %u", file->name, parent, handle->list[index].key);

	return handle->list[index].key;
}




#if 0


/**
 * Load the contents of a button file into the buttons database.
 *
 * \param *leaf_name		The file leafname to load.
 * \return			TRUE on success; else FALSE.
 */

osbool objdb_load_file(char *leaf_name)
{
	int	result, current = -1;
	char	token[1024], contents[1024], section[1024], filename[1024];
	FILE	*file;

	/* Find a buttons file somewhere in the usual config locations. */

	config_find_load_file(filename, sizeof(filename), leaf_name);

	if (*filename == '\0')
		return FALSE;

	/* Open the file and work through it using the config file handling library. */

	file = fopen(filename, "r");

	if (file == NULL)
		return FALSE;

	while ((result = config_read_token_pair(file, token, contents, section)) != sf_READ_CONFIG_EOF) {

		/* A new section of the file, so create, initialise and link in a new button object. */

		if (result == sf_READ_CONFIG_NEW_SECTION) {
			current = objdb_new();

			if (current != -1)
				strncpy(objdb_list[current].name, section, OBJDB_NAME_LENGTH);
		}

		/* If there is a current button object, add the current piece of data to it. */

		if (current != -1) {
			if (strcmp(token, "XPos") == 0)
				objdb_list[current].x = atoi(contents);
			else if (strcmp(token, "YPos") == 0)
				objdb_list[current].y = atoi(contents);
			else if (strcmp(token, "Sprite") == 0)
				strncpy(objdb_list[current].sprite, contents, OBJDB_SPRITE_LENGTH);
			else if (strcmp(token, "RunPath") == 0)
				strncpy(objdb_list[current].command, contents, OBJDB_COMMAND_LENGTH);
			else if (strcmp(token, "Boot") == 0)
				objdb_list[current].filer_boot = config_read_opt_string(contents);
		}
	}

	fclose(file);

	return TRUE;
}


/**
 * Save the contents of the buttons database into a buttons file.
 *
 * \param *leaf_name		The file leafname to save to.
 * \return			TRUE on success; else FALSE.
 */

osbool objdb_save_file(char *leaf_name)
{
	char	filename[1024];
	int	current;
	FILE	*file;


	/* Find a buttons file to write somewhere in the usual config locations. */

	config_find_save_file(filename, sizeof(filename), leaf_name);

	if (*filename == '\0')
		return FALSE;

	/* Open the file and work through it using the config file handling library. */

	file = fopen(filename, "w");

	if (file == NULL)
		return FALSE;

	fprintf(file, "# >Buttons\n#\n# Saved by Launcher.\n");

	for (current = 0; current < objdb_apps; current++) {
		fprintf(file, "\n[%s]\n", objdb_list[current].name);
		fprintf(file, "XPos: %d\n", objdb_list[current].x);
		fprintf(file, "YPos: %d\n", objdb_list[current].y);
		fprintf(file, "Sprite: %s\n", objdb_list[current].sprite);
		fprintf(file, "RunPath: %s\n", objdb_list[current].command);
		fprintf(file, "Boot: %s\n", config_return_opt_string(objdb_list[current].filer_boot));
	}

	fclose(file);

	return TRUE;
}
#endif

/**
 * Create a new, empty entry in the database and return its key.
 *
 * \param *handle		The database in which to create a new key.
 * \return			The new key, or OBJDB_NULL_KEY.
 */

unsigned objdb_create_key(struct objdb_block *handle)
{
	unsigned	key = OBJDB_NULL_KEY;
	int		index = objdb_new(handle);

	if (handle != NULL && index != -1)
		key = handle->list[index].key;

	return key;
}


/**
 * Delete an entry from the database.
 *
 * \param *handle		The database which to delete the entry.
 * \param key			The key of the entry to delete.
 */

void objdb_delete_key(struct objdb_block *handle, unsigned key)
{
	int		index;

	if (handle == NULL || key == OBJDB_NULL_KEY)
		return;

	index = objdb_find(handle, key);

	if (index != -1)
		objdb_delete(handle, index);
}


/**
 * Given a database key, return the next key from the database.
 *
 * \param *handle		The database to iterate through.
 * \param key			The current key, or OBJDB_NULL_KEY to start sequence.
 * \return			The next key, or OBJDB_NULL_KEY.
 */

unsigned objdb_get_next_key(struct objdb_block *handle, unsigned key)
{
	int index;

	if (handle == NULL)
		return OBJDB_NULL_KEY;

	if (key == OBJDB_NULL_KEY && handle->objects > 0)
		return handle->list[0].key;

	index = objdb_find(handle, key);

	return (index != -1 && index < (handle->objects - 1)) ? handle->list[index + 1].key : OBJDB_NULL_KEY;
}

#if 0
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

osbool objdb_get_button_info(unsigned key, int *x_pos, int *y_pos, char **name, char **sprite,
		char **command, osbool *local_copy, osbool *filer_boot)
{
	int index;

	index = objdb_find(key);

	if (index == -1)
		return FALSE;

	if (x_pos != NULL)
		*x_pos = objdb_list[index].x;

	if (y_pos != NULL)
		*y_pos = objdb_list[index].y;

	if (name != NULL)
		*name = objdb_list[index].name;

	if (sprite != NULL)
		*sprite = objdb_list[index].sprite;

	if (command != NULL)
		*command = objdb_list[index].command;

	if (local_copy != NULL)
		*local_copy = objdb_list[index].local_copy;

	if (filer_boot != NULL)
		*filer_boot = objdb_list[index].filer_boot;

	return TRUE;
}


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

osbool objdb_set_button_info(unsigned key, int x_pos, int y_pos, char *name, char *sprite,
		char *command, osbool local_copy, osbool filer_boot)
{
	int index;

	index = objdb_find(key);

	if (index == -1)
		return FALSE;

	objdb_list[index].x = x_pos;
	objdb_list[index].y = y_pos;
	objdb_list[index].local_copy = local_copy;
	objdb_list[index].filer_boot = filer_boot;

	if (name != NULL)
		strncpy(objdb_list[index].name, name, OBJDB_NAME_LENGTH);
	if (sprite != NULL)
		strncpy(objdb_list[index].sprite, sprite, OBJDB_SPRITE_LENGTH);
	if (command != NULL)
		strncpy(objdb_list[index].command, command, OBJDB_COMMAND_LENGTH);

	return TRUE;
}
#endif


/**
 * Find the index of an application based on its key.
 *
 * \param *handle		The database in which to locate the object.
 * \param key			The key to locate.
 * \return			The current index, or -1 if not found.
 */

static int objdb_find(struct objdb_block *handle, unsigned key)
{
	int index;

	if (handle == NULL)
		return -1;

	/* We know that keys are allocated in ascending order, possibly
	 * with gaps in the sequence.  Therefore we can hit the list
	 * at the corresponding index (or the end) and count back until
	 * we pass the key we're looking for.
	 */

	index = (key >= handle->objects) ? handle->objects - 1 : key;

	while (index >= 0 && handle->list[index].key > key)
		index--;

	if (index != -1 && handle->list[index].key != key)
		index = -1;

	return index;
}


/**
 * Claim a block for a new application, fill in the unique key and set
 * default values for the data.
 *
 * \param *handle		The database to claim the new block from.
 * \return			The new block number, or -1 on failure.
 */

static int objdb_new(struct objdb_block *handle)
{
	if (handle == NULL)
		return -1;

	if (handle->objects >= handle->allocation && flex_extend((flex_ptr) &(handle->list),
			(handle->allocation + OBJDB_ALLOC_CHUNK) * sizeof(struct object)) == 1)
		handle->allocation += OBJDB_ALLOC_CHUNK;

	if (handle->objects >= handle->allocation)
		return -1;

	handle->list[handle->objects].key = handle->key++;
	handle->list[handle->objects].load_addr = 0;
	handle->list[handle->objects].exec_addr = 0;
	handle->list[handle->objects].size = 0;
	handle->list[handle->objects].attributes = 0;
	handle->list[handle->objects].type = 0;
	handle->list[handle->objects].name = 0;

	return handle->objects++;
}


/**
 * Delete an application block, given its index.
 *
 * \param *handle		The database to delete the block from.
 * \param index			The index of the block to be deleted.
 */

static void objdb_delete(struct objdb_block *handle, int index)
{
	if (handle == NULL || index < 0 || index >= handle->objects)
		return;

	flex_midextend((flex_ptr) &(handle->list), (index + 1) * sizeof(struct object),
			-sizeof(struct object));
}

