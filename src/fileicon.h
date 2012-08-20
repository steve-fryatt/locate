/* Locate - fileicon.h
 * (c) Stephen Fryatt, 2012
 *
 * Track and manage icons for filetypes.
 */

#ifndef LOCATE_FILEICON
#define LOCATE_FILEICON

#include "oslib/types.h"


enum fileicon_icons {
	FILEICON_UNKNOWN = 0,							/**< The Unknown filetype icon.				*/
	FILEICON_DIRECTORY,							/**< The Directory icon.				*/
	FILEICON_APPLICATION,							/**< The Application icon.				*/
	FILEICON_UNTYPED,							/**< The Untyped filetype icon.				*/
	FILEICON_ERROR,								/**< The Error icon.					*/
	FILEICON_MAX_ICONS							/**< Placeholder to show the maximum index.		*/
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
 * \param type			The filetype of the file.
 * \param *name			The name of the file.
 * \param *small		Pointer to location to return the small flag:
 *				TRUE if a small sprite is available, else FALSE.
 * \return			Offset to the sprite name, or TEXTDUMP_NULL.
 */

unsigned fileicon_get_type_icon(unsigned type, char *name, osbool *small);
unsigned fileicon_get_special_icon(enum fileicon_icons icon, osbool *small);

#endif

