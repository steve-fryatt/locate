/* Copyright 2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: clipboard.c
 *
 * Global Clipboard support.
 */

/* ANSI C header files */

#include <stdlib.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/osfile.h"
#include "oslib/types.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/dataxfer.h"
#include "sflib/errors.h"
#include "sflib/event.h"

/* Application header files */

#include "clipboard.h"

#include "main.h"


static osbool	clipboard_held = FALSE;						/**< TRUE if we hold the clipboard; else FALSE.		*/
static void	*clipboard_user_data = NULL;					/**< Data to be supplied to callback functions.		*/
static void	*(*clipboard_callback_find)(void *);				/**< Callback to get the clipboard address.		*/
static size_t	(*clipboard_callback_size)(void *);				/**< Callback to get the size of the clipboard.		*/
static void	(*clipboard_callback_release)(void *);				/**< Callback to release the clipboard.			*/

static osbool	clipboard_message_claim_entity(wimp_message *message);
static osbool	clipboard_message_data_request(wimp_message *message);
static osbool clipboard_save_file(char *filename, void *data);


/**
 * Initialise the global clipboard system.
 */

void clipboard_initialise(void)
{
	event_add_message_handler(message_CLAIM_ENTITY, EVENT_MESSAGE_INCOMING, clipboard_message_claim_entity);
	event_add_message_handler(message_DATA_REQUEST, EVENT_MESSAGE_INCOMING, clipboard_message_data_request);
}


/**
 * Claim the cliboard contents, supplying a set of callback functions which will
 * provide details in the event that they are required.
 *
 * \param *find			Callback function to supply address of data.
 * \param *size			Callback function to supply size of data.
 * \param *release		Callback function to notify of loss of clipboard.
 * \param *data			Data to be passed to the callback functions.
 */

osbool clipboard_claim(void *(*find)(void *), size_t (*size)(void *), void (*release)(void *), void *data)
{
	wimp_full_message_claim_entity	claimblock;
	os_error			*error;


	clipboard_held = TRUE;

	clipboard_callback_find = find;
	clipboard_callback_size = size;
	clipboard_callback_release = release;
	clipboard_user_data = data;

	/* Send out Message_CliamClipboard. */

	claimblock.size = 24;
	claimblock.your_ref = 0;
	claimblock.action = message_CLAIM_ENTITY;
	claimblock.flags = wimp_CLAIM_CLIPBOARD;

	error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) &claimblock, wimp_BROADCAST);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);

		clipboard_held = FALSE;

		clipboard_callback_find = NULL;
		clipboard_callback_size = NULL;
		clipboard_callback_release = NULL;
		clipboard_user_data = NULL;

		return FALSE;
	}

	return TRUE;
}


/**
 * Handle the receipt of a Message_ClaimEntity. If our client owns the clipboard
 * at this stage then we notify it of the loss and clear all of our data
 * records.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE if the message was handled; else FALSE.
 */

static osbool clipboard_message_claim_entity(wimp_message *message)
{
	wimp_full_message_claim_entity	*claim = (wimp_full_message_claim_entity *) message;

	if (claim->sender == main_task_handle || !clipboard_held || !(claim->flags & wimp_CLAIM_CLIPBOARD))
		return FALSE;

	clipboard_held = FALSE;

	if (clipboard_callback_release != NULL)
		clipboard_callback_release(clipboard_user_data);

	clipboard_user_data = NULL;
	clipboard_callback_find = FALSE;
	clipboard_callback_size = FALSE;
	clipboard_callback_release = FALSE;

	return TRUE;
}


/**
 * Handle the receipt of a Message_DataRequest. If our client owns the clipboard
 * at this stage, we respond accordingly to start the data transfer process.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE if the message was handled; else FALSE.
 */

static osbool clipboard_message_data_request(wimp_message *message)
{
	wimp_full_message_data_request	*request = (wimp_full_message_data_request *) message;
	wimp_pointer			pointer;
	size_t				data_size;

	if (!clipboard_held || !(request->flags & wimp_DATA_REQUEST_CLIPBOARD))
		return FALSE;

	pointer.pos.x = request->pos.x;
	pointer.pos.y = request->pos.y;
	pointer.buttons = NONE;
	pointer.w = request->w;
	pointer.i = request->i;

	if (clipboard_callback_size != NULL)
		data_size = clipboard_callback_size(clipboard_user_data);
	else
		data_size = 0;

	dataxfer_start_save(&pointer, "Clipboard", data_size, osfile_TYPE_TEXT, request->my_ref, clipboard_save_file, NULL);

	return TRUE;
}


/**
 * Callback for saving the clipboard data as part of the data transfer protocol.
 *
 * \param *filename		The name of the file to save.
 * \param *data			NULL data.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool clipboard_save_file(char *filename, void *data)
{
	os_error	*error;
	const byte 	*start, *end;

	start = (clipboard_callback_find != NULL) ? clipboard_callback_find(clipboard_user_data) : NULL;
	end = start + ((clipboard_callback_size != NULL) ? clipboard_callback_size(clipboard_user_data) : 0);

	if (start == NULL)
		return FALSE;

	error = xosfile_save_stamped(filename, osfile_TYPE_TEXT, start, end);

	if (error != NULL)
		return FALSE;

	return TRUE;
}

