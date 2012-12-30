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
 * \file: saveas.h
 *
 * Save dialogue implementation.
 */

#ifndef LOCATE_SAVEAS
#define LOCATE_SAVEAS

/* ==================================================================================================================
 * Static constants
 */


/**
 * A savebox data handle.
 */

struct saveas_dialogue;

/**
 * Initialise the data transfer system.
 */

void saveas_initialise(void);


/**
 * Create a new save dialogue definition.
 *
 * \param: selection		TRUE if the dialogue has a Selection switch; FALSE if not.
 * \param: *sprite		Pointer to the sprite name for the dialogue.
 * \param: *save_callback	The callback function for saving data.
 */

struct saveas_dialogue *saveas_create_dialogue(osbool selection, char *sprite, osbool (*save_callback)(char *filename, osbool selection, void *data));


/**
 * Initialise a save dialogue definition with two filenames, and an indication of
 * the selection icon status.  This is called when opening a menu or creating a
 * dialogue from a toolbar.
 *
 * \param *handle		The handle of the save dialogue to be initialised.
 * \param *fullname		Pointer to the filename token for a full save.
 * \param *selectname		Pointer to the filename token for a selection save.
 * \param selection		TRUE if the Selection option is enabled; else FALSE.
 * \param selected		TRUE if the Selection option is selected; else FALSE.
 * \param *data			Data to pass to any save callbacks, or NULL.
 */

void saveas_initialise_dialogue(struct saveas_dialogue *handle, char *fullname, char *selectname, osbool selection, osbool selected, void *data);


/**
 * Prepare the physical save dialogue based on the current state of the given
 * dialogue data.  Any existing data will be saved into the appropriate
 * dialogue definition first.
 *
 * \param *handle		The handle of the save dialogue to prepare.
 */

void saveas_prepare_dialogue(struct saveas_dialogue *handle);

#endif
