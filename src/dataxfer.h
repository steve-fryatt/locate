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
 * \file: dataxfer.h
 *
 * Save dialogue and data transfer implementation.
 */

#ifndef LOCATE_DATAXFER
#define LOCATE_DATAXFER

/* ==================================================================================================================
 * Static constants
 */


#define LOCATE_FILE_TYPE 0x1a1

/**
 * Initialise the data transfer system.
 */

void dataxfer_initialise(void);


/**
 * Start dragging an icon from a dialogue, creating a sprite to drag and starting
 * a drag action.  When the action completes, a callback will be made to the
 * supplied function.
 *
 * \param w		The window where the drag is starting.
 * \param i		The icon to be dragged.
 * \param callback	A callback function
 */

void dataxfer_save_window_drag(wimp_w w, wimp_i i, void (* drag_end_callback)(wimp_pointer *pointer, void *data), void *drag_end_data);


/**
 * Start a data save action by sending a message to another task.  The data
 * transfer protocol will be started, and at an appropriate time a callback
 * will be made to save the data.
 *
 * \param *pointer		The Wimp pointer details of the save target.
 * \param *name			The proposed file leafname.
 * \param size			The estimated file size.
 * \param type			The proposed file type.
 * \param your_ref		The "your ref" to use for the opening message, or 0.
 * \param *save_callback	The function to be called with the full pathname
 *				to save the file.
 * \param *data			Data to be passed to the callback function.
 * \return			TRUE on success; FALSE on failure.
 */

osbool dataxfer_start_save(wimp_pointer *pointer, char *name, int size, bits type, int your_ref, osbool (*save_callback)(char *filename, void *data), void *data);


/**
 * Specify a handler for files which are double-clicked or dragged into a window.
 * Files which match on type, target window and target icon are passed to the
 * appropriate handler for attention.
 *
 * To specify a generic handler for a type, set window to NULL and icon to -1.
 * To specify a generic handler for all the icons in a window, set icon to -1.
 *
 * Double-clicked files (Message_DataOpen) will be passed to a generic type
 * handler or a type handler for a window with the handle wimp_ICON_BAR.
 *
 * \param filetype		The window to register as a target.
 * \param w			The target window, or NULL.
 * \param i			The target icon, or -1.
 * \param *callback		The load callback function.
 * \param *data			Data to be passed to load functions, or NULL.
 * \return			TRUE if successfully registered; else FALSE.
 */

osbool dataxfer_set_load_target(unsigned filetype, wimp_w w, wimp_i i, osbool (*callback)(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data), void *data);

#endif

