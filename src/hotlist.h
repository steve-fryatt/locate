/* Copyright 2013, Stephen Fryatt
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
 */

void hotlist_initialise(void);


/**
 * Add a dialogue to the hotlist.
 *
 * \param *dialogue		The handle of the dialogue to be added.
 */

void hotlist_add_dialogue(struct dialogue_block *dialogue);

#endif

