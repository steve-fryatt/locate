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
 * \file: fileicon.h
 *
 * Track and manage names and icons for filetypes.
 */

#ifndef LOCATE_FILEICON
#define LOCATE_FILEICON

#include "oslib/types.h"

#include "textdump.h"


enum fileicon_icons {
	FILEICON_UNKNOWN = 0,							/**< The Unknown filetype icon.				*/
	FILEICON_DIRECTORY,							/**< The Directory icon.				*/
	FILEICON_APPLICATION,							/**< The Application icon.				*/
	FILEICON_UNTYPED,							/**< The Untyped filetype icon.				*/
	FILEICON_INCOMPLETE,							/**< The Incomplete (dead) filetype icon.		*/
	FILEICON_ERROR,								/**< The Error icon.					*/
	FILEICON_MAX_ICONS							/**< Placeholder to show the maximum index.		*/
};

/**
 * filetype information block.
 */

struct fileicon_info {
	unsigned	type;
	unsigned	name;
	unsigned	large;
	unsigned	small;
};


/**
 * Initialise the fileicon module, set up storage and sort out icons for the
 * basic file types.
 */

void fileicon_initialise(void);


/**
 * Terminate the fileicon module and free all memory usage.
 */

void fileicon_terminate(void);


/**
 * Return the offset base for the fileicon text block. The returned value is
 * only guaranteed to be correct unitl the Flex heap is altered.
 *
 * \return			The block base, or NULL on error.
 */

char *fileicon_get_base(void);


/**
 * Return the offset of a sprite name suitable for the given file with
 * a type and name as specified.
 *
 * This call could move the flex heap if memory has to be allocated internally.
 *
 * \param type			The filetype of the file.
 * \param *name			The name of the file.
 * \info *icon			Pointer to a block to take the filetype details.
 * \return			TRUE on success; else FALSE.
 * \return			Offset to the sprite name, or TEXTDUMP_NULL.
 */

osbool fileicon_get_type_icon(unsigned type, char *name, struct fileicon_info *info);


/**
 * Return the offset of one of the special icon sprite names.
 *
 * \param icon			The icon to return a sprite for.
 * \info *icon			Pointer to a block to take the filetype details.
 * \return			TRUE on success; else FALSE.
 */

osbool fileicon_get_special_icon(enum fileicon_icons icon, struct fileicon_info *info);

#endif

