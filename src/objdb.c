/* Copyright 2012-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: objdb.c
 *
 * Filer object database implementation.
 */

/* ANSI C header files. */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files. */

#include "oslib/os.h"
#include "oslib/osfile.h"
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

#include "discfile.h"
#include "file.h"
#include "textdump.h"


#define OBJDB_ALLOC_CHUNK 100							/**< The number of objects to allocate from memory at a time.	*/
#define OBJDB_MAX_DEPTH 255							/**< The maximum directory depth that can be handled.		*/

#define OBJDB_NULL_INDEX 0xffffffffu						/**< An index that does not exist.				*/

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

	unsigned		longest_name;					/**< The length of the longest filename string in the database.	*/
	unsigned		longest_path;					/**< The length of the longest pathname string in the database.	*/

	unsigned		key;						/**< Track new unique primary keys.				*/

	osbool			full_scan;					/**< TRUE if the database contains a full scan; FALSE if not.	*/
};

enum objdb_object_flags
{
	OBJDB_OBJECT_FLAGS_NONE = 0,						/**< There are no flags set.					*/
	OBJDB_OBJECT_FLAGS_LOST = 1,						/**< Set if the object is no longer in its original location.	*/
	OBJDB_OBJECT_FLAGS_CHANGED = 2						/**< Set if the object is on disc, but has changed somehow.	*/
};

/**
 * Data structure for a filing system object.
 */

struct object
{
	unsigned		key;						/**< Primary key to index database entries.			*/

	unsigned		parent;						/**< The key of the parent object, or OBJDB_NULL_KEY.		*/

	enum objdb_object_flags	flags;						/**< The object flags for the object in question.		*/

	bits			load_addr;					/**< The load address of the object.				*/
	bits			exec_addr;					/**< The execution address of the object.			*/
	unsigned		size;						/**< The size of the object in bytes.				*/
	fileswitch_attr		attributes;					/**< The file attributes of the object.				*/
	fileswitch_object_type	type;						/**< The fileswitch object type of the object.			*/
	unsigned		name;						/**< Textdump offset to the name of the object.			*/
};


static unsigned	objdb_find(struct objdb_block *handle, unsigned key);
static unsigned	objdb_new(struct objdb_block *handle);
static osbool	objdb_extend(struct objdb_block *handle, unsigned allocation);
static void	objdb_delete(struct objdb_block *handle, unsigned index);


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
	new->longest_name = 0;
	new->longest_path = 0;
	new->full_scan = FALSE;

	/* Claim the database flex block and a text dump for the names. */

	if (flex_alloc((flex_ptr) &(new->list),
			(new->allocation + OBJDB_ALLOC_CHUNK) * sizeof(struct object)) == 1)
		new->allocation += OBJDB_ALLOC_CHUNK;
	else
		new->list = NULL;

	new->text = textdump_create(0, 20, '\0');

	/* If either of the sub allocations failed, free the claimed memory and exit. */

	if (new->list == NULL || new->text == NULL) {
		if (new->list != NULL)
			flex_free((flex_ptr) &(new->list));

		if (new->text != NULL)
			textdump_destroy(new->text);

		heap_free(new);

		return NULL;
	}

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
	unsigned	length, name, index = objdb_new(handle);

	if (handle == NULL || path == NULL || index == OBJDB_NULL_INDEX)
		return OBJDB_NULL_KEY;

	name = textdump_store(handle->text, path);

	handle->list[index].parent = OBJDB_NULL_KEY;
	handle->list[index].name = name;

	if ((length = strlen(path)) > handle->longest_name)
		handle->longest_name = length;

	if ((length = objdb_get_name_length(handle, handle->list[index].key)) > handle->longest_path)
		handle->longest_path = length;

#ifdef DEBUG
	debug_printf("\\YAdding root details for %s with key %u", path, handle->list[index].key);
#endif

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
	unsigned	length, name, index = objdb_new(handle);

	if (handle == NULL || file == NULL || index == OBJDB_NULL_INDEX)
		return OBJDB_NULL_KEY;

	name = textdump_store(handle->text, file->name);

	handle->list[index].parent = parent;

	handle->list[index].load_addr = file->load_addr;
	handle->list[index].exec_addr = file->exec_addr;
	handle->list[index].size = file->size;
	handle->list[index].attributes = file->attr;
	handle->list[index].type = file->obj_type;
	handle->list[index].name = name;

	if ((length = strlen(file->name)) > handle->longest_name)
		handle->longest_name = length;

	if ((length = objdb_get_name_length(handle, handle->list[index].key)) > handle->longest_path)
		handle->longest_path = length;

#ifdef DEBUG
	debug_printf("\\YAdding file details for %s to %u with key %u", file->name, parent, handle->list[index].key);
#endif

	return handle->list[index].key;
}


/**
 * Validate an entry in the object database by checking to see if it is still
 * present on disc in the same location.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \param retest		TRUE to check the disc; FALSE to rely on the stored data.
 * \return			The object's status.
 */

enum objdb_status objdb_validate_file(struct objdb_block *handle, unsigned key, osbool retest)
{
	unsigned		index;
	size_t			pathname_len;
	char			*pathname;
	os_error		*error;
	fileswitch_object_type	type;
	fileswitch_attr		attributes;
	bits			load_addr, exec_addr;
	int			size;


	if (handle == NULL || key == OBJDB_NULL_KEY)
		return OBJDB_STATUS_ERROR;

	index = objdb_find(handle, key);

	if (index == OBJDB_NULL_INDEX)
		return OBJDB_STATUS_ERROR;

	/* If we're not retesting, then just return the status based on the
	 * object flags.
	 */

	if (!retest) {
		if (handle->list[index].flags & OBJDB_OBJECT_FLAGS_LOST)
			return OBJDB_STATUS_MISSING;

		if (handle->list[index].flags & OBJDB_OBJECT_FLAGS_CHANGED)
			return OBJDB_STATUS_CHANGED;

		return OBJDB_STATUS_UNCHANGED;
	}

	/* Find the pathname of the object to be tested. */

	pathname_len = objdb_get_name_length(handle, key);

	pathname = malloc(pathname_len);
	if (pathname == NULL)
		return OBJDB_STATUS_ERROR;

	objdb_get_name(handle, key, pathname, pathname_len);

	/* Read the object's current details and compare them to those on file. */

	error = xosfile_read_no_path(pathname, &type, &load_addr, &exec_addr, &size, &attributes);

	free(pathname);

	if (error != NULL)
		return OBJDB_STATUS_ERROR;

	if (type == fileswitch_NOT_FOUND) {
		handle->list[index].flags |= OBJDB_OBJECT_FLAGS_LOST;
		return OBJDB_STATUS_MISSING;
	}

	if (type != handle->list[index].type || load_addr != handle->list[index].load_addr ||
			exec_addr != handle->list[index].exec_addr || size != handle->list[index].size ||
			attributes != handle->list[index].attributes) {
		handle->list[index].flags |= OBJDB_OBJECT_FLAGS_CHANGED;
		return OBJDB_STATUS_CHANGED;
	}

	/* If all else fails, the file is the same. */

	return OBJDB_STATUS_UNCHANGED;
}


/**
 * Return the parent of an object in the database.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \return			The parent object key, or OBJDB_NULL_KEY if none.
 */

unsigned objdb_get_parent(struct objdb_block *handle, unsigned key)
{
	unsigned	index;

	if (handle == NULL || key == OBJDB_NULL_KEY)
		return OBJDB_NULL_KEY;

	index = objdb_find(handle, key);

	if (index == OBJDB_NULL_INDEX)
		return OBJDB_NULL_KEY;

	return handle->list[index].parent;
}


/**
 * Return the pathname of an object in the database.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \param *buffer		Pointer to a buffer to hold the name.
 * \param len			The size of the supplied buffer.
 * \return			TRUE if successful; else FALSE.
 */

osbool objdb_get_name(struct objdb_block *handle, unsigned key, char *buffer, size_t len)
{
	unsigned	index, indexes[OBJDB_MAX_DEPTH], i;
	char		*base, *from, *to;

	if (handle == NULL || buffer == NULL)
		return FALSE;

	i = 0;

	do {
		index = (key != OBJDB_NULL_KEY) ? objdb_find(handle, key) : OBJDB_NULL_INDEX;

		if (index != OBJDB_NULL_INDEX) {
			indexes[i++] = index;
			key = handle->list[index].parent;
		}
	} while (index != OBJDB_NULL_INDEX && i < OBJDB_MAX_DEPTH);

	base = textdump_get_base(handle->text);

	to = buffer;
	buffer += (len - 1);

	while (i > 0) {
		from = base + handle->list[indexes[--i]].name;

		while (to < buffer && *from != '\0')
			*(to++) = *(from++);

		if (i > 0)
			*(to++) = '.';
	}

	*to = '\0';

	return TRUE;
}


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

size_t objdb_get_name_length(struct objdb_block *handle, unsigned key)
{
	unsigned	index, length;
	char		*base;

	if (handle == NULL)
		return 0;

	if (key == OBJDB_NULL_KEY)
		return handle->longest_path * sizeof(char);

	length = 0;

	base = textdump_get_base(handle->text);

	/* The length calculation is the length of each name plus one for the
	 * RISC OS '.' separator. This is added to every entry, so the extra
	 * one will provide space for the '\0' terminator.
	 */

	do {
		index = (key != OBJDB_NULL_KEY) ? objdb_find(handle, key) : OBJDB_NULL_INDEX;

		if (index != OBJDB_NULL_INDEX) {
			length += strlen(base + handle->list[index].name) + 1;
			key = handle->list[index].parent;
		}
	} while (index != OBJDB_NULL_INDEX);

	return length * sizeof(char);
}


/**
 * Return the filetype of an object in the database.
 *
 * \param *handle		The database to look in.
 * \param key			The key of the object to be returned.
 * \return			The filetype, or 0xffffffffu.
 */

unsigned objdb_get_filetype(struct objdb_block *handle, unsigned key)
{
	unsigned	index = objdb_find(handle, key);
	char		*name;

	if (handle == NULL || index == OBJDB_NULL_INDEX)
		return 0xffffffffu;

	name = textdump_get_base(handle->text) + handle->list[index].name;

	if (handle->list[index].type == fileswitch_IS_DIR && name[0] == '!')
		return osfile_TYPE_APPLICATION;
	else if (handle->list[index].type == fileswitch_IS_DIR)
		return osfile_TYPE_DIR;
	else if ((handle->list[index].load_addr & 0xfff00000u) != 0xfff00000u)
		return osfile_TYPE_UNTYPED;
	else
		return (handle->list[index].load_addr & osfile_FILE_TYPE) >> osfile_FILE_TYPE_SHIFT;
}


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

size_t objdb_get_info(struct objdb_block *handle, unsigned key, osgbpb_info *info, struct objdb_info *additional)
{
	char		*base;
	unsigned	index;

	if (handle == NULL)
		return 0;

	index = (key != OBJDB_NULL_KEY) ? objdb_find(handle, key) : OBJDB_NULL_INDEX;
	base = textdump_get_base(handle->text);

	if (info == NULL && additional == NULL) {
		if (index != OBJDB_NULL_INDEX)
			return 21 + strlen(base + handle->list[index].name);
		else
			return 21 + handle->longest_name;
	}

	if (info != NULL) {
		info->load_addr = handle->list[index].load_addr;
		info->exec_addr = handle->list[index].exec_addr;
		info->size = handle->list[index].size;
		info->attr = handle->list[index].attributes;
		info->obj_type = handle->list[index].type;
		strcpy(info->name, base + handle->list[index].name);
	}

	if (additional != NULL) {
		additional->filetype = objdb_get_filetype(handle, key);

		if (handle->list[index].flags & OBJDB_OBJECT_FLAGS_LOST)
			additional->status = OBJDB_STATUS_MISSING;
		else if (handle->list[index].flags & OBJDB_OBJECT_FLAGS_CHANGED)
			additional->status = OBJDB_STATUS_CHANGED;
		else
			additional->status = OBJDB_STATUS_UNCHANGED;
	}

	return 0;
}


/**
 * Load the contents of an object file into the database.
 *
 * \param *file			The file to which the database will belong.
 * \param *load			The discfile handle to load from.
 * \return			The new databse, or NULL on failure.
 */

struct objdb_block *objdb_load_file(struct file_block *file, struct discfile_block *load)
{
	struct objdb_block	*handle;
	int			size;

	if (file == NULL || load == NULL)
		return NULL;

	if (discfile_read_format(load) != DISCFILE_LOCATE2)
		return NULL;

	/* Open the object database section of the file. */

	if (!discfile_open_section(load, DISCFILE_SECTION_OBJECTDB))
		return NULL;

	/* Create a new object database. */

	handle = objdb_create(file);

	if (handle == NULL) {
		discfile_set_error(load, "FileMem");
		return NULL;
	}

	/* Load the settings chunk into memory. */

	if (discfile_open_chunk(load, DISCFILE_CHUNK_OPTIONS)) {
		if (!discfile_read_option_unsigned(load, "OBJ", &handle->objects) ||
				!discfile_read_option_unsigned(load, "KEY", &handle->key) ||
				!discfile_read_option_unsigned(load, "LEN", &handle->longest_name) ||
				!discfile_read_option_unsigned(load, "PTH", &handle->longest_path) ||
				!discfile_read_option_boolean(load, "FUL", &handle->full_scan)) {
			discfile_set_error(load, "FileUnrec");
			objdb_destroy(handle);
			return NULL;
		}

		discfile_close_chunk(load);

		if (handle->objects > handle->allocation)
			objdb_extend(handle, handle->objects);

		if (handle->objects > handle->allocation) {
			discfile_set_error(load, "FileMem");
			objdb_destroy(handle);
			return NULL;
		}
	} else {
		discfile_set_error(load, "FileUnrec");
		objdb_destroy(handle);
		return NULL;
	}

	/* Load the database contents into memory. */

	if (discfile_open_chunk(load, DISCFILE_CHUNK_OBJECTS)) {
		size = discfile_chunk_size(load);

		if ((size <= handle->allocation * sizeof(struct object)) && (size == handle->objects * sizeof(struct object))){
			discfile_read_chunk(load, (byte *) handle->list, size);
		} else {
			discfile_set_error(load, "FileUnrec");
			objdb_destroy(handle);
			return NULL;
		}

		discfile_close_chunk(load);
	} else {
		discfile_set_error(load, "FileUnrec");
		objdb_destroy(handle);
		return NULL;
	}

	/* Load the textdump contents into memory. */

	if (!textdump_load_file(handle->text, load)) {
		discfile_set_error(load, "FileUnrec");
		objdb_destroy(handle);
		return NULL;
	}

	/* Close the database section of the file. */

	discfile_close_section(load);

	return handle;
}


/**
 * Save the contents of the database into a discfile.
 *
 * \param *handle		The database to be saved.
 * \param *file			The discfile handle to save to.
 * \return			TRUE on success; else FALSE.
 */

osbool objdb_save_file(struct objdb_block *handle, struct discfile_block *file)
{
	if (handle == NULL || file == NULL)
		return FALSE;

	/* Open the database section of the file. */

	discfile_start_section(file, DISCFILE_SECTION_OBJECTDB, FALSE);

	/* Write the database settings. */

	discfile_start_chunk(file, DISCFILE_CHUNK_OPTIONS);
	discfile_write_option_unsigned(file, "OBJ", handle->objects);
	discfile_write_option_unsigned(file, "LEN", handle->longest_name);
	discfile_write_option_unsigned(file, "PTH", handle->longest_path);
	discfile_write_option_unsigned(file, "KEY", handle->key);
	discfile_write_option_boolean(file, "FUL", handle->full_scan);
	discfile_end_chunk(file);

	/* Write the database object data. */

	discfile_start_chunk(file, DISCFILE_CHUNK_OBJECTS);
	discfile_write_chunk(file, (byte *) handle->list, handle->objects * sizeof(struct object));
	discfile_end_chunk(file);

	/* Write the textdump contents. */

	textdump_save_file(handle->text, file);

	/* Close the database section of the file. */

	discfile_end_section(file);

	return TRUE;
}


/**
 * Create a new, empty entry in the database and return its key.
 *
 * \param *handle		The database in which to create a new key.
 * \return			The new key, or OBJDB_NULL_KEY.
 */

unsigned objdb_create_key(struct objdb_block *handle)
{
	unsigned	key = OBJDB_NULL_KEY;
	unsigned	index = objdb_new(handle);

	if (handle != NULL && index != OBJDB_NULL_INDEX)
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
	unsigned	index;

	if (handle == NULL || key == OBJDB_NULL_KEY)
		return;

	index = objdb_find(handle, key);

	if (index != OBJDB_NULL_INDEX)
		objdb_delete(handle, index);
}


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

void objdb_delete_last_key(struct objdb_block *handle, unsigned key)
{
	unsigned	index;

	if (handle == NULL || key == OBJDB_NULL_KEY || handle->objects == 0)
		return;

	index = handle->objects - 1;
	if (handle->list[index].key != key)
		return;

	/* Don't bother to free up memory; just release the last allocated
	 * block for reuse, and also free up the key value if it is also the
	 * last one to be allocated, to keep the two in step and speed up
	 * accesses.
	 */

#ifdef DEBUG
	debug_printf("\\ODeleting key %u", handle->list[index].key);
#endif

	if (handle->list[index].key + 1 == handle->key)
		handle->key--;

	handle->objects--;
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
	unsigned	index;

	if (handle == NULL)
		return OBJDB_NULL_KEY;

	if (key == OBJDB_NULL_KEY && handle->objects > 0)
		return handle->list[0].key;

	index = objdb_find(handle, key);

	return (index != OBJDB_NULL_INDEX && index < (handle->objects - 1)) ? handle->list[index + 1].key : OBJDB_NULL_KEY;
}


/**
 * Find the index of an application based on its key.
 *
 * \param *handle		The database in which to locate the object.
 * \param key			The key to locate.
 * \return			The current index, or OBJDB_NULL_INDEX if not found.
 */

static unsigned objdb_find(struct objdb_block *handle, unsigned key)
{
	unsigned	index;

	if (handle == NULL)
		return OBJDB_NULL_INDEX;

	/* We know that keys are allocated in ascending order, possibly
	 * with gaps in the sequence.  Therefore we can hit the list
	 * at the corresponding index (or the end) and count back until
	 * we pass the key we're looking for.
	 */

	index = (key >= handle->objects) ? handle->objects - 1 : key;

	while (index > 0 && handle->list[index].key > key)
		index--;

	if (handle->list[index].key != key)
		index = OBJDB_NULL_INDEX;

	return index;
}


/**
 * Claim a block for a new application, fill in the unique key and set
 * default values for the data.
 *
 * \param *handle		The database to claim the new block from.
 * \return			The new block index, or OBJDB_NULL_INDEX on failure.
 */

static unsigned objdb_new(struct objdb_block *handle)
{
	if (handle == NULL || handle->list == NULL)
		return OBJDB_NULL_INDEX;

	if (handle->objects >= handle->allocation)
		objdb_extend(handle, handle->allocation + OBJDB_ALLOC_CHUNK);

	if (handle->objects >= handle->allocation)
		return OBJDB_NULL_INDEX;

	handle->list[handle->objects].key = handle->key++;
	handle->list[handle->objects].load_addr = 0;
	handle->list[handle->objects].exec_addr = 0;
	handle->list[handle->objects].size = 0;
	handle->list[handle->objects].attributes = 0;
	handle->list[handle->objects].type = 0;
	handle->list[handle->objects].name = 0;
	handle->list[handle->objects].flags = OBJDB_OBJECT_FLAGS_NONE;

	return handle->objects++;
}


/**
 * Extend the memory allocaton for a database by the given number of objects.
 *
 * \param *handle		The database to extend.
 * \param allocation		The required number of objects in the database.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool objdb_extend(struct objdb_block *handle, unsigned allocation)
{
	if (handle == NULL || handle->list == NULL || handle->allocation > allocation)
		return FALSE;

	if (flex_extend((flex_ptr) &(handle->list), allocation * sizeof(struct object)) != 1)
		return FALSE;

	handle->allocation = allocation;

	return TRUE;
}


/**
 * Delete an application block, given its index.
 *
 * \param *handle		The database to delete the block from.
 * \param index			The index of the block to be deleted.
 */

static void objdb_delete(struct objdb_block *handle, unsigned index)
{
	if (handle == NULL || handle->list == NULL || index >= handle->objects)
		return;

	if (flex_midextend((flex_ptr) &(handle->list), (index + 1) * sizeof(struct object),
			-sizeof(struct object)) == 1) {
		handle->objects--;
		handle->allocation--;
	}
}

