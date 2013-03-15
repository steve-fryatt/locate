/* Copyright 2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: typemenu.h
 *
 * Filetype menu implementation.
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

