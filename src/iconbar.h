/* Copyright 2012-2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Locate:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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
 * \file: iconbar.h
 *
 * IconBar icon implementation.
 */

#ifndef LOCATE_ICONBAR
#define LOCATE_ICONBAR

#include "dialogue.h"


/**
 * Initialise the iconbar icon and its associated menus and dialogues.
 */

void iconbar_initialise(void);


/**
 * Create the iconbar icon.
 */

void iconbar_create_icon(void);


/**
 * Set a dialogue as the data for the last search. The existing last search
 * dialogue will be discarded.
 *
 * \param *dialogue		The handle of the dialogue to set.
 */

void iconbar_set_last_search_dialogue(struct dialogue_block *dialogue);

#endif

