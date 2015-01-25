/* Copyright 2012-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "discfile.h"
#include "file.h"

/**
 * Dialogue client details: flags are set to indicate that users have a
 * dialogue data block in use.
 */

enum dialogue_client {
	DIALOGUE_CLIENT_NONE = 0,						/**< No flags indicates that nothing is using the dialogue.	*/
	DIALOGUE_CLIENT_FILE = 1,						/**< The dialogue is being used by a file.			*/
	DIALOGUE_CLIENT_LAST = 2,						/**< The dialogue is being used by the last search.		*/
	DIALOGUE_CLIENT_HOTLIST = 4,						/**< The dialogue is being used by the hotlist.			*/
	DIALOGUE_CLIENT_ALL = 0xffffffffu					/**< All flags indicates that all clients shoul be cleared.	*/
};

/**
 * Actions which can be requested from a dialogue load or save helper
 * function.
 */

enum dialogue_file_action {
	DIALOGUE_START_SECTION,							/**< Start a new section for writing and leave it open.		*/
	DIALOGUE_WRITE_DATA,							/**< Write any required data to the open section.		*/
	DIALOGUE_OPEN_SECTION,							/**< Open a new section for reading, and leave it open.		*/
	DIALOGUE_READ_DATA							/**< Read any required data from the open section.		*/
};

/**
 * Initialise the Dialogue module.
 */

void dialogue_initialise(void);


/**
 * Create a new set of dialogue data with the default values.
 *
 * \param *file			The file to which the dialogue belongs, or NULL
 *				for none.
 * \param *path			The search path to use, or NULL for default.
 * \param *template		A dialogue to copy the settings from, or NULL for
 *				default values.
 * \return			Pointer to the new block, or NULL on failure.
 */

struct dialogue_block *dialogue_create(struct file_block *file, char *path, struct dialogue_block *template);


/**
 * Destroy a dialogue and its data. The client will be removed from the list
 * of users for the dialogue, but it will only be destroyed if there are no
 * other clients registered.
 *
 * \param *dialogue		The dialogue to be destroyed.
 * \param client		The client trying to destroy the dialogue.
 */

void dialogue_destroy(struct dialogue_block *dialogue, enum dialogue_client client);


/**
 * Add a client to a dialogue, so that dialogue_destroy knows that it is
 * being used.
 */

void dialogue_add_client(struct dialogue_block *dialogue, enum dialogue_client client);


/**
 * Save a dialogue's settings to an open disc file. This can either be a dialogue
 * block, if the name is NULL, or a hotlist entry, if a pointer to a name is
 * supplied.
 *
 * \param *dialogue		The dialogue to be saved.
 * \param *out			The handle of the file to save to.
 * \param *helper		A helper function to be called once the
 *				options chunk is open, or NULL for none.
 * \param *data			Data to be passed to the helper function,
 *				or NULL for none.
 */

void dialogue_save_file(struct dialogue_block *dialogue, struct discfile_block *out,
		void (*helper)(struct discfile_block *out, enum dialogue_file_action action, void *data), void *data);


/**
 * Load a dialogue's settings from an open disc file and create a new
 * dialogue structure from them.
 *
 * \param *file			The file to which the dialogue will belong.
 * \param *load			The handle of the file to load from.
 * \param *helper		A helper function to be called once the
 *				options chunk is open, or NULL for none.
 * \param *data			Data to be passed to the helper function,
 *				or NULL for none.
 * \return			The handle of the new dialogue, or NULL.
 */

struct dialogue_block *dialogue_load_file(struct file_block *file, struct discfile_block *load,
		osbool (*helper)(struct discfile_block *out, enum dialogue_file_action action, void *data), void *data);


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

