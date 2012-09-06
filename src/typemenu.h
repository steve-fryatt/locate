/* Locate - typemenu.h
 * (c) Stephen Fryatt, 2012
 *
 * Implement a menu of filetypes.
 */

#ifndef LOCATE_TYPEMENU
#define LOCATE_TYPEMENU

#include "oslib/types.h"
#include "oslib/wimp.h"


/**
 * Build a typemenu.
 *
 * \return			A pointer to the menu block.
 */

wimp_menu *typemenu_build(void);


/**
 * Process a selection from the type menu, adding the selected type into
 * a list of filetypes if it isn't already there.
 *
 * \param selection		The menu selection.
 * \param type_list		A flex block containing a type list.
 */

void typemenu_process_selection(int selection, flex_ptr type_list);

#endif

