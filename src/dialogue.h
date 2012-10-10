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
 * \file: dialogue.h
 *
 * Search dialogue implementation.
 */

#ifndef LOCATE_DIALOGUE
#define LOCATE_DIALOGUE

struct dialogue_block;

#include "file.h"

/**
 * Initialise the Dialogue module.
 */

void dialogue_initialise(void);


/**
 * Create a new set of dialogue data with the default values.
 *
 * \param *file			The file to which the dialogue belongs.
 * \param *path			The search path to use, or NULL for default.
 * \param *template		A dialogue to copy the settings from, or NULL for
 *				default values.
 * \return			Pointer to the new block, or NULL on failure.
 */

struct dialogue_block *dialogue_create(struct file_block *file, char *path, struct dialogue_block *template);


/**
 * Destroy a dialogue and its data.
 *
 * \param *dialogue		The dialogue to be destroyed.
 */

void dialogue_destroy(struct dialogue_block *dialogue);


/**
 * Open the Search Dialogue window at the mouse pointer.
 *
 * \param *dialogue		The dialogue details to use to open the window.
 * \param *pointer		The details of the pointer to open the window at.
 */

void dialogue_open_window(struct dialogue_block *dialogue, wimp_pointer *pointer);


/**
 * Identify whether the Search Dialogue window is currently open.
 *
 * \return		TRUE if the window is open; else FALSE.
 */

osbool dialogue_window_is_open(void);

#endif

