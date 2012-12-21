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

/* OSLib Header files. */

#include "oslib/osfile.h"
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


static struct textdump_block		*fileicon_text;				/**< The text dump for filetype icons.					*/

static struct fileicon_icon		fileicon_types[4096];			/**< Array of details for the 4096 filetype icons.			*/
static struct fileicon_icon		fileicon_specials[FILEICON_MAX_ICONS];	/**< Array of details for the special icons.				*/


static void	fileicon_find_sprites(struct fileicon_icon *icon, char *small, char *large);


/**
 * Initialise the fileicon module, set up storage and sort out icons for the
 * basic file types.
 */

void fileicon_initialise(void)
{
	int	i;
	char	name[20];

	fileicon_text = textdump_create(0, 0);

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
	fileicon_specials[FILEICON_UNTYPED].name = textdump_store(fileicon_text, msgs_lookup("Untyped", name, sizeof(name)));

	fileicon_find_sprites(fileicon_specials + FILEICON_UNKNOWN, "small_xxx", "file_xxx");
	fileicon_find_sprites(fileicon_specials + FILEICON_DIRECTORY, "small_dir", "directory");
	fileicon_find_sprites(fileicon_specials + FILEICON_APPLICATION, "small_app", "application");
	fileicon_find_sprites(fileicon_specials + FILEICON_UNTYPED, "small_lxa", "file_lxa");

	/* The error sprite is in our own sprite area, so won't show up in searches
	 * of the Wimp Sprite Pool.
	 */

	fileicon_specials[FILEICON_ERROR].status = FILEICON_SMALL;
	fileicon_specials[FILEICON_ERROR].name = textdump_store(fileicon_text, msgs_lookup("Err", name, sizeof(name)));
	fileicon_specials[FILEICON_ERROR].large = TEXTDUMP_NULL;
	fileicon_specials[FILEICON_ERROR].small = textdump_store(fileicon_text, "error");
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
 * Return the offset of a sprite name suitable for the given file with
 * a type and name as specified.
 *
 * \param type			The filetype of the file.
 * \param *name			The name of the file.
 * \info *icon			Pointer to a block to take the filetype details.
 * \return			TRUE on success; else FALSE.
 * \return			Offset to the sprite name, or TEXTDUMP_NULL.
 */

osbool fileicon_get_type_icon(unsigned type, char *name, struct fileicon_info *info)
{
	char		smallname[20], largename[20];

	if (info == NULL)
		return FALSE;

	/* Deal with special cases first. */

	if (type > 0xfffu) {
		switch (type) {
		case osfile_TYPE_DIR:
			return fileicon_get_special_icon(FILEICON_DIRECTORY, info);
			break;
		case osfile_TYPE_APPLICATION:
			return fileicon_get_special_icon(FILEICON_APPLICATION, info);
			break;
		case osfile_TYPE_UNTYPED:
			return fileicon_get_special_icon(FILEICON_UNTYPED, info);
			break;
		default:
			return fileicon_get_special_icon(FILEICON_UNKNOWN, info);
			break;
		}

		return TRUE;
	}

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

		fileicon_find_sprites(fileicon_types + type, smallname, largename);
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
 */

static void fileicon_find_sprites(struct fileicon_icon *icon, char *small, char *large)
{
	os_error	*error;

	if (icon == NULL)
		return;

	if (small != NULL && icon->small == TEXTDUMP_NULL) {
		error = xwimpspriteop_read_sprite_info(small, NULL, NULL, NULL, NULL);

		if (error == NULL) {
			icon->status = FILEICON_SMALL;
			icon->small = textdump_store(fileicon_text, small);
		}
	}

	if (large != NULL && icon->large == TEXTDUMP_NULL) {
		error = xwimpspriteop_read_sprite_info(large, NULL, NULL, NULL, NULL);

		if (error == NULL) {
			if (icon->status != FILEICON_SMALL)
				icon->status = FILEICON_LARGE;
			icon->large = textdump_store(fileicon_text, large);
		}
	}

	if (icon->status == FILEICON_UNCHECKED)
		icon->status = FILEICON_NONE;
}

