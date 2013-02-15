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
 * \file: fileicon.c
 *
 * Track and manage names and icons for filetypes.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/msgs.h"
#include "sflib/string.h"

/* OSLib Header files. */

#include "oslib/osfile.h"
#include "oslib/osgbpb.h"
#include "oslib/types.h"
#include "oslib/wimpspriteop.h"

/* Application header files. */

#include "fileicon.h"

#include "textdump.h"


enum fileicon_status {
	FILEICON_UNCHECKED = 0,							/**< The filetype's sprite hasn't been checked.				*/
	FILEICON_NONE,								/**< The filetype's sprite has been checked and found missing.		*/
	FILEICON_LARGE,								/**< The filetype's sprite has been checked and a large version found.	*/
	FILEICON_SMALL								/**< The filetype's sprite has been checked and a small version found.	*/
};


struct fileicon_icon {
	enum fileicon_status	status;						/**< The status of the icon.						*/
	unsigned		name;						/**< Offset to the filetype's textual name.				*/
	unsigned		large;						/**< Offset to the small icon's name.					*/
	unsigned		small;						/**< Offset to the large icon's name.					*/
};


static struct textdump_block		*fileicon_text;				/**< The text dump for filetype names and icons.			*/

static struct fileicon_icon		fileicon_types[4096];			/**< Array of details for the 4096 filetype icons.			*/
static struct fileicon_icon		fileicon_specials[FILEICON_MAX_ICONS];	/**< Array of details for the special icons.				*/

static unsigned				fileicon_large_fixed_allocation;	/**< Text dump offset of a fixed allocation for large sprite names.	*/
static unsigned				fileicon_small_fixed_allocation;	/**< Text dump offset of a fixed allocation for small sprite names.	*/

static void	fileicon_find_sprites(struct fileicon_icon *icon, char *small, char *large, osbool allocate);


/**
 * Initialise the fileicon module, set up storage and sort out icons for the
 * basic file types.
 */

void fileicon_initialise(void)
{
	int	i;
	char	name[20];

	fileicon_text = textdump_create(0, 0, '\0');

	if (fileicon_text == NULL)
		return;

	/* Initialise the icon data. */

	for (i = 0; i < 4096; i++) {
		fileicon_types[i].status = FILEICON_UNCHECKED;
		fileicon_types[i].name = TEXTDUMP_NULL;
		fileicon_types[i].large = TEXTDUMP_NULL;
		fileicon_types[i].small = TEXTDUMP_NULL;
	}

	for (i = 0; i < FILEICON_MAX_ICONS; i++) {
		fileicon_specials[i].status = FILEICON_UNCHECKED;
		fileicon_specials[i].name = TEXTDUMP_NULL;
		fileicon_specials[i].large = TEXTDUMP_NULL;
		fileicon_specials[i].small = TEXTDUMP_NULL;
	}

	fileicon_specials[FILEICON_UNKNOWN].name = textdump_store(fileicon_text, msgs_lookup("Unknown", name, sizeof(name)));
	fileicon_specials[FILEICON_DIRECTORY].name = textdump_store(fileicon_text, msgs_lookup("Dir", name, sizeof(name)));
	fileicon_specials[FILEICON_APPLICATION].name = textdump_store(fileicon_text, msgs_lookup("App", name, sizeof(name)));
	fileicon_specials[FILEICON_UNTYPED].name = textdump_store(fileicon_text, msgs_lookup("File", name, sizeof(name)));
	fileicon_specials[FILEICON_INCOMPLETE].name = textdump_store(fileicon_text, msgs_lookup("Unf", name, sizeof(name)));

	fileicon_find_sprites(fileicon_specials + FILEICON_UNKNOWN, "small_xxx", "file_xxx", TRUE);
	fileicon_find_sprites(fileicon_specials + FILEICON_DIRECTORY, "small_dir", "directory", TRUE);
	fileicon_find_sprites(fileicon_specials + FILEICON_APPLICATION, "small_app", "application", TRUE);
	fileicon_find_sprites(fileicon_specials + FILEICON_UNTYPED, "small_lxa", "file_lxa", TRUE);
	fileicon_find_sprites(fileicon_specials + FILEICON_INCOMPLETE, "small_unf", "file_unf", TRUE);

	/* The error sprite is in our own sprite area, so won't show up in searches
	 * of the Wimp Sprite Pool.
	 */

	fileicon_specials[FILEICON_ERROR].status = FILEICON_SMALL;
	fileicon_specials[FILEICON_ERROR].name = textdump_store(fileicon_text, msgs_lookup("Err", name, sizeof(name)));
	fileicon_specials[FILEICON_ERROR].large = TEXTDUMP_NULL;
	fileicon_specials[FILEICON_ERROR].small = textdump_store(fileicon_text, "error");

	/* The custom application icon is a special case that gets created on the
	 * fly as required.
	 */

	fileicon_specials[FILEICON_CUSTOM_APPLICATION].status = FILEICON_UNCHECKED;
	fileicon_specials[FILEICON_CUSTOM_APPLICATION].name = fileicon_specials[FILEICON_APPLICATION].name;
	fileicon_specials[FILEICON_CUSTOM_APPLICATION].large = TEXTDUMP_NULL;
	fileicon_specials[FILEICON_CUSTOM_APPLICATION].small = TEXTDUMP_NULL;

	/* Create two 12 character blocks to hold custom sprite names. */

	fileicon_large_fixed_allocation = textdump_store(fileicon_text, "123456789012");
	fileicon_small_fixed_allocation = textdump_store(fileicon_text, "123456789012");
}


/**
 * Terminate the fileicon module and free all memory usage.
 */

void fileicon_terminate(void)
{
	textdump_destroy(fileicon_text);
}


/**
 * Return the offset base for the fileicon text block. The returned value is
 * only guaranteed to be correct unitl the Flex heap is altered.
 *
 * \return			The block base, or NULL on error.
 */

char *fileicon_get_base(void)
{
	return textdump_get_base(fileicon_text);
}


/**
 * Return the offset of a sprite name suitable for the given object with
 * the details as given.
 *
 * This call could move the flex heap if memory has to be allocated internally.
 *
 * \param *file			The file details.
 * \param *info			Pointer to block to take the filetype details.
 * \return			TRUE on success; else FALSE.
 */

osbool fileicon_get_object_icon(osgbpb_info *file, struct fileicon_info *info)
{
	char	smallname[13], largename[13];

	if (file == NULL || info == NULL)
		return FALSE;

	/* Deal with special cases first. */

	if (file->obj_type == fileswitch_IS_DIR && file->name[0] == '!') {
		snprintf(largename, 12, "%s", file->name);
		largename[12] = '\0';
		string_tolower(largename);

		snprintf(smallname, 12, "sm%s", file->name);
		smallname[12] = '\0';
		string_tolower(smallname);

		fileicon_specials[FILEICON_CUSTOM_APPLICATION].status = FILEICON_UNCHECKED;
		fileicon_specials[FILEICON_CUSTOM_APPLICATION].large = TEXTDUMP_NULL;
		fileicon_specials[FILEICON_CUSTOM_APPLICATION].small = TEXTDUMP_NULL;
		fileicon_find_sprites(fileicon_specials + FILEICON_CUSTOM_APPLICATION, smallname, largename, FALSE);
		if (fileicon_specials[FILEICON_CUSTOM_APPLICATION].status != FILEICON_NONE)
			return fileicon_get_special_icon(FILEICON_CUSTOM_APPLICATION, info);
		else
			return fileicon_get_special_icon(FILEICON_APPLICATION, info);
	} else if (file->obj_type == fileswitch_IS_DIR) {
		return fileicon_get_special_icon(FILEICON_DIRECTORY, info);
	} else if (file->load_addr == 0xdeaddead && file->exec_addr == 0xdeaddead) {
		return fileicon_get_special_icon(FILEICON_INCOMPLETE, info);
	} else if ((file->load_addr & 0xfff00000u) != 0xfff00000u) {
		return fileicon_get_special_icon(FILEICON_UNTYPED, info);
	}

	if (file->obj_type != fileswitch_IS_FILE && file->obj_type != fileswitch_IS_IMAGE)
		return fileicon_get_special_icon(FILEICON_UNKNOWN, info);

	/* We now have a typed file. */

	return fileicon_get_type_icon((file->load_addr & osfile_FILE_TYPE) >> osfile_FILE_TYPE_SHIFT, info);
}


/**
 * Return the offset of a sprite name suitable for the given file with
 * a type specified.
 *
 * This call could move the flex heap if memory has to be allocated internally.
 *
 * \param type			The filetype of the file.
 * \param *info			Pointer to block to take the filetype details.
 * \return			TRUE on success; else FALSE.
 */

osbool fileicon_get_type_icon(unsigned type, struct fileicon_info *info)
{
	char		smallname[20], largename[20], typename[20], typevar[20], buffer[20];
	int		length;

	if (info == NULL)
		return FALSE;

	/* Process the filetype name. */

	if (fileicon_types[type].name == TEXTDUMP_NULL) {
		snprintf(typevar, sizeof(typevar), "File$Type_%03X", type);
		if (xos_read_var_val(typevar, buffer, sizeof(buffer), 0, os_VARTYPE_STRING, &length, NULL, NULL) == NULL) {
			buffer[(length < sizeof(buffer)) ? length : (sizeof(buffer) - 1)] = '\0';
			snprintf(typename, sizeof(typename), "%s", buffer);
		} else {
			snprintf(typename, sizeof(typename), "&%03x", type);
		}

		fileicon_types[type].name = textdump_store(fileicon_text, typename);
	}

	info->name = fileicon_types[type].name;

	/* For filetypes, check the cache and then start testing sprites. */

	switch (fileicon_types[type].status) {
	case FILEICON_LARGE:
	case FILEICON_SMALL:
		info->type = type;
		info->name = fileicon_types[type].name;
		info->large = fileicon_types[type].large;
		info->small = fileicon_types[type].small;

		return TRUE;

	case FILEICON_NONE:
		return fileicon_get_special_icon(FILEICON_UNKNOWN, info);

	default:
		snprintf(smallname, sizeof(smallname), "small_%3x", type);
		snprintf(largename, sizeof(largename), "file_%3x", type);

		fileicon_find_sprites(fileicon_types + type, smallname, largename, TRUE);
		if (fileicon_types[type].status != FILEICON_NONE) {
			info->type = type;
			info->name = fileicon_types[type].name;
			info->large = fileicon_types[type].large;
			info->small = fileicon_types[type].small;

			return TRUE;
		}
		break;
	}

	return fileicon_get_special_icon(FILEICON_UNKNOWN, info);
}


/**
 * Return the offset of one of the special icon sprite names.
 *
 * \param icon			The icon to return a sprite for.
 * \info *icon			Pointer to a block to take the filetype details.
 * \return			TRUE on success; else FALSE.
 */

osbool fileicon_get_special_icon(enum fileicon_icons icon, struct fileicon_info *info)
{
	if (info == NULL || icon < 0 || icon >= FILEICON_MAX_ICONS)
		return FALSE;

	info->name = fileicon_specials[icon].name;
	info->large = fileicon_specials[icon].large;
	info->small = fileicon_specials[icon].small;

	return TRUE;
}


/**
 * Identify an icon for a filetype, setting the supplied record accordingly.
 *
 * \param *icon			The icon record to update.
 * \param *small		The name of the small icon to be used.
 * \param *large		The name of the large icon to be used.
 * \param allocate		TRUE to allocate new storage for the sprite
 *				names; FALSE to copy them into the supplied
 *				buffers.
 */

static void fileicon_find_sprites(struct fileicon_icon *icon, char *small, char *large, osbool allocate)
{
	os_error	*error;

	if (icon == NULL)
		return;

	if (small != NULL && icon->small == TEXTDUMP_NULL) {
		error = xwimpspriteop_read_sprite_info(small, NULL, NULL, NULL, NULL);

		if (error == NULL) {
			icon->status = FILEICON_SMALL;
			if (allocate) {
				icon->small = textdump_store(fileicon_text, small);
			} else {
				icon->small = fileicon_small_fixed_allocation;
				strcpy(textdump_get_base(fileicon_text) + icon->small, small);
			}
		}
	}

	if (large != NULL && icon->large == TEXTDUMP_NULL) {
		error = xwimpspriteop_read_sprite_info(large, NULL, NULL, NULL, NULL);

		if (error == NULL) {
			if (icon->status != FILEICON_SMALL)
				icon->status = FILEICON_LARGE;
			if (allocate) {
				icon->large = textdump_store(fileicon_text, large);
			} else {
				icon->large = fileicon_large_fixed_allocation;
				strcpy(textdump_get_base(fileicon_text) + icon->large, large);
			}
		}
	}

	if (icon->status == FILEICON_UNCHECKED)
		icon->status = FILEICON_NONE;
}

