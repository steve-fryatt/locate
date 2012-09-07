/* Locate - fileicon.c
 * (c) Stephen Fryatt, 2012
 *
 * Track and manage icons for filetypes.
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
	unsigned		name;						/**< The offset to the icon's name.					*/
};


static struct textdump_block		*fileicon_text;				/**< The text dump for filetype icons.					*/

static struct fileicon_icon		fileicon_types[4096];			/**< Array of details for the 4096 filetype icons.			*/
static struct fileicon_icon		fileicon_specials[FILEICON_MAX_ICONS];	/**< Array of details for the special icons.				*/


static void	fileicon_find_sprite(struct fileicon_icon *icon, char *small, char *large);


/**
 * Initialise the fileicon module, set up storage and sort out icons for the
 * basic file types.
 */

void fileicon_initialise(void)
{
	int	i;

	fileicon_text = textdump_create(0);

	if (fileicon_text == NULL)
		return;

	/* Initialise the icon data. */

	for (i = 0; i < 4096; i++) {
		fileicon_types[i].status = FILEICON_UNCHECKED;
		fileicon_types[i].name = TEXTDUMP_NULL;
	}

	for (i = 0; i < FILEICON_MAX_ICONS; i++) {
		fileicon_specials[i].status = FILEICON_UNCHECKED;
		fileicon_specials[i].name = TEXTDUMP_NULL;
	}

	fileicon_find_sprite(fileicon_specials + FILEICON_UNKNOWN, "small_xxx", "file_xxx");
	fileicon_find_sprite(fileicon_specials + FILEICON_DIRECTORY, "small_dir", "directory");
	fileicon_find_sprite(fileicon_specials + FILEICON_APPLICATION, "small_app", "application");
	fileicon_find_sprite(fileicon_specials + FILEICON_UNTYPED, "small_lxa", "file_lxa");

	/* The error sprite is in our own sprite area, so won't show up in searches
	 * of the Wimp Sprite Pool.
	 */

	fileicon_specials[FILEICON_ERROR].status = FILEICON_SMALL;
	fileicon_specials[FILEICON_ERROR].name = textdump_store(fileicon_text, "error");
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
 * \param *small		Pointer to location to return the small flag:
 *				TRUE if a small sprite is available, else FALSE.
 * \return			Offset to the sprite name, or TEXTDUMP_NULL.
 */

unsigned fileicon_get_type_icon(unsigned type, char *name, osbool *small)
{
	char		smallname[20], largename[20];

	/* Deal with special cases first. */

	if (type > 0xfffu) {
		switch (type) {
		case osfile_TYPE_DIR:
			return fileicon_get_special_icon(FILEICON_DIRECTORY, small);
			break;
		case osfile_TYPE_APPLICATION:
			return fileicon_get_special_icon(FILEICON_APPLICATION, small);
			break;
		case osfile_TYPE_UNTYPED:
			return fileicon_get_special_icon(FILEICON_UNTYPED, small);
			break;
		default:
			return fileicon_get_special_icon(FILEICON_UNKNOWN, small);
			break;
		}

		return TRUE;
	}

	/* For filetypes, check the cache and then start testing sprites. */

	switch (fileicon_types[type].status) {
	case FILEICON_LARGE:
	case FILEICON_SMALL:
		if (small != NULL)
			*small = (fileicon_types[type].status == FILEICON_SMALL) ? TRUE : FALSE;
		return fileicon_types[type].name;

	case FILEICON_NONE:
		return fileicon_get_special_icon(FILEICON_UNKNOWN, small);

	default:
		snprintf(smallname, sizeof(smallname), "small_%3x", type);
		snprintf(largename, sizeof(largename), "file_%3x", type);

		fileicon_find_sprite(fileicon_types + type, smallname, largename);
		if (fileicon_types[type].status != FILEICON_NONE) {
			if (small != NULL)
				*small = (fileicon_types[type].status == FILEICON_SMALL) ? TRUE : FALSE;
			return fileicon_types[type].name;
		}
		break;
	}

	return fileicon_get_special_icon(FILEICON_UNKNOWN, small);
}


/**
 * Return the offset of one of the special icon sprite names.
 *
 * \param icon			The icon to return a sprite for.
 * \param *small		Pointer to location to return the small flag:
 *				TRUE if a small sprite is available, else FALSE.
 * \return			Offset to the sprite name, or TEXTDUMP_NULL.
 */

unsigned fileicon_get_special_icon(enum fileicon_icons icon, osbool *small)
{
	if (icon < 0 || icon >= FILEICON_MAX_ICONS)
		return TEXTDUMP_NULL;

	if (small != NULL)
		*small = (fileicon_specials[icon].status == FILEICON_SMALL) ? TRUE : FALSE;

	return fileicon_specials[icon].name;
}


/**
 * Identify an icon for a filetype, setting the supplied record accordingly.
 *
 * \param *icon			The icon record to update.
 * \param *small		The name of the small icon to be used.
 * \param *large		The name of the large icon to be used.
 */

static void fileicon_find_sprite(struct fileicon_icon *icon, char *small, char *large)
{
	os_error	*error;

	if (icon == NULL)
		return;

	if (small != NULL) {
		error = xwimpspriteop_read_sprite_info(small, NULL, NULL, NULL, NULL);

		if (error == NULL) {
			icon->status = FILEICON_SMALL;
			icon->name = textdump_store(fileicon_text, small);
			return;
		}
	}

	if (large != NULL) {
		error = xwimpspriteop_read_sprite_info(large, NULL, NULL, NULL, NULL);

		if (error == NULL) {
			icon->status = FILEICON_LARGE;
			icon->name = textdump_store(fileicon_text, large);

			return;
		}
	}

	icon->status = FILEICON_NONE;
}

