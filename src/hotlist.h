/* Copyright 2013-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: hotlist.h
 *
 * Hotlist implementation.
 */

#ifndef LOCATE_HOTLIST
#define LOCATE_HOTLIST

#include "dialogue.h"


/**
 * Initialise the hotlist system and its associated menus and dialogues.
 *
 * \param *sprites		Pointer to the sprite area to be used.
 */

void hotlist_initialise(osspriteop_area *sprites);


/**
 * Terminate the hotlist system.
 */

void hotlist_terminate(void);


/**
 * Open the hotlist window.
 *
 * \param *pointer		The location at which to open the window.
 */

void hotlist_open(wimp_pointer *pointer);


/**
 * Add a dialogue to the hotlist.
 *
 * \param *dialogue		The handle of the dialogue to be added.
 */

void hotlist_add_dialogue(struct dialogue_block *dialogue);


/**
 * Identify whether the Hotlist Add Dialogue window is currently open.
 *
 * \return		TRUE if the window is open; else FALSE.
 */

osbool hotlist_add_window_is_open(void);


/**
 * Build a hotlist menu.
 *
 * \return			A pointer to the menu block.
 */

wimp_menu *hotlist_build_menu(void);


/**
 * Process a selection from the hotlist menu.
 *
 * \param selection		The menu selection.
 */

void hotlist_process_menu_selection(int selection);


/**
 * Return the handle of the dialogue to use for default search settings,
 * or NULL if none has been set.
 *
 * \return			The handle of the default dialogue, or NULL.
 */

struct dialogue_block *hotlist_get_default_dialogue(void);

#endif

