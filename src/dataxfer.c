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
 * \file: dataxfer.c
 *
 * Save dialogue and data transfer implementation.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/dragasprite.h"
#include "oslib/osbyte.h"
#include "oslib/osfscontrol.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"

/* SF-Lib header files. */

#include "sflib/icons.h"
#include "sflib/string.h"
#include "sflib/windows.h"
#include "sflib/transfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/general.h"
#include "sflib/msgs.h"
#include "sflib/config.h"

/* Application header files */

#include "dataxfer.h"

#include "ihelp.h"
#include "main.h"
#include "templates.h"

/* ==================================================================================================================
 * Global variables.
 */


/**
 * Data associated with a message exchange.
 */

enum dataxfer_message_type {
	DATAXFER_MESSAGE_NONE = 0,							/**< An unset message block.						*/
	DATAXFER_MESSAGE_SAVE = 1,							/**< A message block associated with a save operation.			*/
	DATAXFER_MESSAGE_LOAD = 2							/**< A message block associated with a load operation.			*/
};

struct dataxfer_descriptor {
	enum dataxfer_message_type	type;						/**< The type of message that the block describes.			*/

	int				my_ref;						/**< The MyRef of the sent message.					*/
	wimp_t				task;						/**< The task handle of the recipient task.				*/
	osbool				(*callback)(char *filename, void *data);	/**< The callback function to be used if a save is required.		*/
	void				*callback_data;					/**< Data to be passed to the callback function.			*/

	struct dataxfer_descriptor	*next;						/**< The next message block in the chain, or NULL.			*/
};

static struct dataxfer_descriptor	*dataxfer_descriptors = NULL;			/**< List of currently active message operations.			*/

/**
 * Data associated with incoming transfer targets.
 */

struct dataxfer_incoming_target {
	unsigned			filetype;					/**< The target filetype.						*/
	wimp_w				window;						/**< The target window (used in window and icon lists).			*/
	wimp_i				icon;						/**< The target icon (used in icon lists).				*/

	osbool				(*callback)(wimp_w w, wimp_i i,
			unsigned filetype, char *filename, void *data);			/**< The callback function to be used if a load is required.		*/
	void				*callback_data;					/**< Data to be passed to the callback function.			*/

	struct dataxfer_incoming_target	*children;					/**< Pointer to a list of child targets (window or icon lists).		*/

	struct dataxfer_incoming_target	*next;						/**< The next target in the chain, or NULL.				*/
};

struct dataxfer_incoming_target		*dataxfer_incoming_targets = NULL;		/**< List of defined incoming targets.					*/

/**
 * Data asscoiated with drag box handling.
 */

static osbool	dataxfer_dragging_sprite = FALSE;					/**< TRUE if we're dragging a sprite, FALSE if dragging a dotted-box.	*/
static void	(*dataxfer_drag_end_callback)(wimp_pointer *, void *) = NULL;		/**< The callback function to be used by the icon drag code.		*/


/**
 * Function prototypes.
 */

static void				dataxfer_terminate_user_drag(wimp_dragged *drag, void *data);
static osbool				dataxfer_message_data_save_ack(wimp_message *message);
static osbool				dataxfer_message_data_load_ack(wimp_message *message);

static osbool				dataxfer_message_data_save(wimp_message *message);
static osbool				dataxfer_message_data_load(wimp_message *message);
static osbool				dataxfer_message_data_open(wimp_message *message);

static struct dataxfer_incoming_target	*dataxfer_find_incoming_target(wimp_w w, wimp_i i, unsigned filetype);

static osbool				dataxfer_message_bounced(wimp_message *message);

static struct dataxfer_descriptor	*dataxfer_new_descriptor(void);
static struct dataxfer_descriptor	*dataxfer_find_descriptor(int ref, enum dataxfer_message_type type);
static void				dataxfer_delete_descriptor(struct dataxfer_descriptor *message);


/**
 * Initialise the data transfer system.
 */

void dataxfer_initialise(void)
{
	event_add_message_handler(message_DATA_SAVE, EVENT_MESSAGE_INCOMING, dataxfer_message_data_save);
	event_add_message_handler(message_DATA_SAVE_ACK, EVENT_MESSAGE_INCOMING, dataxfer_message_data_save_ack);
	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, dataxfer_message_data_load);
	event_add_message_handler(message_DATA_LOAD_ACK, EVENT_MESSAGE_INCOMING, dataxfer_message_data_load_ack);
	event_add_message_handler(message_DATA_OPEN, EVENT_MESSAGE_INCOMING, dataxfer_message_data_open);

	event_add_message_handler(message_DATA_SAVE, EVENT_MESSAGE_ACKNOWLEDGE, dataxfer_message_bounced);
	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_ACKNOWLEDGE, dataxfer_message_bounced);
	event_add_message_handler(message_DATA_SAVE_ACK, EVENT_MESSAGE_ACKNOWLEDGE, dataxfer_message_bounced);
}


/**
 * Start dragging from a window work area, creating a sprite to drag and starting
 * a drag action.  When the action completes, a callback will be made to the
 * supplied function.
 *
 * \param w		The window where the drag is starting.
 * \param *pointer	The current pointer state.
 * \param *extent	The extent of the drag box, relative to the window work area.
 * \param *sprite	Pointer to the name of the sprite to use for the drag, or NULL.
 * \param callback	A callback function
 */

void dataxfer_work_area_drag(wimp_w w, wimp_pointer *pointer, os_box *extent, char *sprite, void (* drag_end_callback)(wimp_pointer *pointer, void *data), void *drag_end_data)
{
	wimp_window_state	window;
	wimp_drag		drag;
	int			ox, oy, width, height;

	/* If there's no callback, there's no point bothering. */

	if (drag_end_callback == NULL)
		return;

	/* Get the basic information about the window and icon. */

	window.w = w;
	wimp_get_window_state(&window);

	ox = window.visible.x0 - window.xscroll;
	oy = window.visible.y1 - window.yscroll;

	/* Read CMOS RAM to see if solid drags are required; this can only
	 * happen if a sprite name is supplied.
	 */

	dataxfer_dragging_sprite = ((sprite != NULL) && ((osbyte2(osbyte_READ_CMOS, osbyte_CONFIGURE_DRAG_ASPRITE, 0) &
			osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0)) ? TRUE : FALSE;

	/* Set up the drag parameters. */

	drag.w = window.w;
	drag.type = wimp_DRAG_USER_FIXED;

	/* If the drag is a sprite, it is centred on the pointer; if a drag box,
	 * the supplied box extent is used relative to the window work area.
	 */

	if (dataxfer_dragging_sprite) {
		if (xwimpspriteop_read_sprite_size(sprite, &width, &height, NULL, NULL) != NULL) {
			width = 32;
			height = 32;
		}

		// \TODO -- This assumes a square pixel mode!

		drag.initial.x0 = pointer->pos.x - width;
		drag.initial.y0 = pointer->pos.y - height;
		drag.initial.x1 = pointer->pos.x + width;
		drag.initial.y1 = pointer->pos.y + height;
	} else {
		drag.initial.x0 = ox + extent->x0;
		drag.initial.y0 = oy + extent->y0;
		drag.initial.x1 = ox + extent->x1;
		drag.initial.y1 = oy + extent->y1;
	}

	drag.bbox.x0 = 0x80000000;
	drag.bbox.y0 = 0x80000000;
	drag.bbox.x1 = 0x7fffffff;
	drag.bbox.y1 = 0x7fffffff;

	dataxfer_drag_end_callback = drag_end_callback;

	/* Start the drag and set an eventlib callback. */

	if (dataxfer_dragging_sprite)
		dragasprite_start(dragasprite_HPOS_CENTRE | dragasprite_VPOS_CENTRE |
			dragasprite_NO_BOUND | dragasprite_BOUND_POINTER | dragasprite_DROP_SHADOW,
			wimpspriteop_AREA, sprite, &(drag.initial), &(drag.bbox));
	else
		wimp_drag_box(&drag);

	event_set_drag_handler(dataxfer_terminate_user_drag, NULL, drag_end_data);
}


/**
 * Start dragging an icon from a dialogue, creating a sprite to drag and starting
 * a drag action.  When the action completes, a callback will be made to the
 * supplied function.
 *
 * \param w		The window where the drag is starting.
 * \param i		The icon to be dragged.
 * \param callback	A callback function
 */

void dataxfer_save_window_drag(wimp_w w, wimp_i i, void (* drag_end_callback)(wimp_pointer *pointer, void *data), void *drag_end_data)
{
	wimp_window_state	window;
	wimp_icon_state		icon;
	wimp_drag		drag;
	int			ox, oy;


	/* If there's no callback, there's no point bothering. */

	if (drag_end_callback == NULL)
		return;

	/* Get the basic information about the window and icon. */

	window.w = w;
	wimp_get_window_state(&window);

	ox = window.visible.x0 - window.xscroll;
	oy = window.visible.y1 - window.yscroll;

	icon.w = window.w;
	icon.i = i;
	wimp_get_icon_state(&icon);

	/* Set up the drag parameters. */

	drag.w = window.w;
	drag.type = wimp_DRAG_USER_FIXED;

	drag.initial.x0 = ox + icon.icon.extent.x0;
	drag.initial.y0 = oy + icon.icon.extent.y0;
	drag.initial.x1 = ox + icon.icon.extent.x1;
	drag.initial.y1 = oy + icon.icon.extent.y1;

	drag.bbox.x0 = 0x80000000;
	drag.bbox.y0 = 0x80000000;
	drag.bbox.x1 = 0x7fffffff;
	drag.bbox.y1 = 0x7fffffff;

	/* Read CMOS RAM to see if solid drags are required. */

	dataxfer_dragging_sprite = ((osbyte2(osbyte_READ_CMOS, osbyte_CONFIGURE_DRAG_ASPRITE, 0) &
			osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0) ? TRUE : FALSE;

	dataxfer_drag_end_callback = drag_end_callback;

	/* Start the drag and set an eventlib callback. */

	if (dataxfer_dragging_sprite)
		dragasprite_start(dragasprite_HPOS_CENTRE | dragasprite_VPOS_CENTRE |
			dragasprite_NO_BOUND | dragasprite_BOUND_POINTER | dragasprite_DROP_SHADOW,
			wimpspriteop_AREA, icon.icon.data.indirected_text.text, &(drag.initial), &(drag.bbox));
	else
		wimp_drag_box(&drag);

	event_set_drag_handler(dataxfer_terminate_user_drag, NULL, drag_end_data);
}


/**
 * Callback handler for queue window drag termination.
 *
 * Start a data-save dialogue with the application at the other end.
 *
 * \param  *drag		The Wimp poll block from termination.
 * \param  *data		NULL (unused).
 */

static void dataxfer_terminate_user_drag(wimp_dragged *drag, void *data)
{
	wimp_pointer		pointer;

	if (dataxfer_dragging_sprite)
		dragasprite_stop();

	wimp_get_pointer_info(&pointer);

	if (dataxfer_drag_end_callback != NULL)
		dataxfer_drag_end_callback(&pointer, data);

	dataxfer_drag_end_callback = NULL;
}


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

osbool dataxfer_start_save(wimp_pointer *pointer, char *name, int size, bits type, int your_ref, osbool (*save_callback)(char *filename, void *data), void *data)
{
	struct dataxfer_descriptor	*descriptor;
	wimp_full_message_data_xfer	message;
	os_error			*error;


	if (save_callback == NULL)
		return FALSE;

	/* Allocate a block to store details of the message. */

	descriptor = dataxfer_new_descriptor();
	if (descriptor == NULL)
		return FALSE;

	descriptor->callback = save_callback;
	descriptor->callback_data = data;

	/* Set up and send the datasave message. If it fails, give an error
	 * and delete the message details as we won't need them again.
	 */

	message.size = WORDALIGN(45 + strlen(name));
	message.your_ref = your_ref;
	message.action = message_DATA_SAVE;
	message.w = pointer->w;
	message.i = pointer->i;
	message.pos = pointer->pos;
	message.est_size = size;
	message.file_type = type;

	strncpy(message.file_name, name, 212);
	message.file_name[211] = '\0';

	error = xwimp_send_message_to_window(wimp_USER_MESSAGE_RECORDED, (wimp_message *) &message, pointer->w, pointer->i, &(descriptor->task));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		dataxfer_delete_descriptor(descriptor);
		return FALSE;
	}

	/* Complete the message descriptor information. */

	descriptor->type = DATAXFER_MESSAGE_SAVE;
	descriptor->my_ref = message.my_ref;

	return TRUE;
}


/**
 * Start a data load action for another task by sending it a message containing
 * the name of the file that it should take.
 *
 * \param *pointer		The Wimp pointer details of the save target.
 * \param *name			The full pathname of the file.
 * \param size			The estimated file size.
 * \param type			The proposed file type.
 * \param your_ref		The "your ref" to use for the opening message, or 0.
 * \return			TRUE on success; FALSE on failure.
 */

osbool dataxfer_start_load(wimp_pointer *pointer, char *name, int size, bits type, int your_ref)
{
	struct dataxfer_descriptor	*descriptor;
	wimp_full_message_data_xfer	message;
	os_error			*error;


	/* Allocate a block to store details of the message. */

	descriptor = dataxfer_new_descriptor();
	if (descriptor == NULL)
		return FALSE;

	descriptor->callback = NULL;
	descriptor->callback_data = NULL;

	/* Set up and send the datasave message. If it fails, give an error
	 * and delete the message details as we won't need them again.
	 */

	message.size = WORDALIGN(45 + strlen(name));
	message.your_ref = your_ref;
	message.action = message_DATA_LOAD;
	message.w = pointer->w;
	message.i = pointer->i;
	message.pos = pointer->pos;
	message.est_size = size;
	message.file_type = type;

	strncpy(message.file_name, name, 212);
	message.file_name[211] = '\0';

	error = xwimp_send_message_to_window(wimp_USER_MESSAGE_RECORDED, (wimp_message *) &message, pointer->w, pointer->i, &(descriptor->task));
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		dataxfer_delete_descriptor(descriptor);
		return FALSE;
	}

	/* Complete the message descriptor information. */

	descriptor->type = DATAXFER_MESSAGE_SAVE;
	descriptor->my_ref = message.my_ref;

	return TRUE;
}


/**
 * Handle the receipt of a Message_DataSaveAck in response our starting a
 * save action.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool dataxfer_message_data_save_ack(wimp_message *message)
{
	struct dataxfer_descriptor	*descriptor;
	wimp_full_message_data_xfer	*datasaveack = (wimp_full_message_data_xfer *) message;
	os_error			*error;


	descriptor = dataxfer_find_descriptor(message->your_ref, DATAXFER_MESSAGE_SAVE);
	if (descriptor == NULL)
		return FALSE;

	/* We now know the message in question, so see if our client wants to
	 * make use of the data.  If there's a callback, call it.  If it
	 * returns FALSE then we don't want to send a Message_DataLoad so
	 * free the information block and quit marking the incoming
	 * message as handled.
	 */

	if (descriptor->callback != NULL && !descriptor->callback(datasaveack->file_name, descriptor->callback_data)) {
		dataxfer_delete_descriptor(descriptor);
		return TRUE;
	}

	/* The client saved something, so finish off the data transfer. */

	datasaveack->your_ref = datasaveack->my_ref;
	datasaveack->action = message_DATA_LOAD;

	error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) datasaveack, datasaveack->sender);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);

		dataxfer_delete_descriptor(descriptor);
		return TRUE;
	}

	descriptor->my_ref = datasaveack->my_ref;

	return TRUE;
}


/**
 * Handle the receipt of a Message_DataLoadAck in response our starting a
 * save action.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool dataxfer_message_data_load_ack(wimp_message *message)
{
	struct dataxfer_descriptor	*descriptor;

	/* This is the tail end of a data save, so just clean up and claim the
	 * message.
	 */

	descriptor = dataxfer_find_descriptor(message->your_ref, DATAXFER_MESSAGE_SAVE);
	if (descriptor == NULL)
		return FALSE;

	dataxfer_delete_descriptor(descriptor);
	return TRUE;
}


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
 * \param filetype		The filetype to register as a target.
 * \param w			The target window, or NULL.
 * \param i			The target icon, or -1.
 * \param *callback		The load callback function.
 * \param *data			Data to be passed to load functions, or NULL.
 * \return			TRUE if successfully registered; else FALSE.
 */

osbool dataxfer_set_load_target(unsigned filetype, wimp_w w, wimp_i i, osbool (*callback)(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data), void *data)
{
	struct dataxfer_incoming_target		*type, *window, *icon;

	/* Validate the input: if there's an icon, there must be a window. */

	if (w == NULL && i != -1)
		return FALSE;

	/* Set up the top-level filetype target. */

	type = dataxfer_incoming_targets;

	while (type != NULL && type->filetype != filetype)
		type = type->next;

	if (type == NULL) {
		type = malloc(sizeof(struct dataxfer_incoming_target));
		if (type == NULL)
			return FALSE;

		type->filetype = filetype;
		type->window = 0;
		type->icon = 0;

		type->callback = NULL;
		type->callback_data = NULL;

		type->children = NULL;

		type->next = dataxfer_incoming_targets;
		dataxfer_incoming_targets = type;
	}

	if (w == NULL) {
		type->callback = callback;
		type->callback_data = data;
		return TRUE;
	}

	/* Set up the window target. */

	window = type->children;

	while (window != NULL && window->window != w)
		window = window->next;

	if (window == NULL) {
		window = malloc(sizeof(struct dataxfer_incoming_target));
		if (window == NULL)
			return FALSE;

		window->filetype = filetype;
		window->window = w;
		window->icon = 0;

		window->callback = NULL;
		window->callback_data = NULL;

		window->children = NULL;

		window->next = type->children;
		type->children = window;
	}

	if (i == -1) {
		window->callback = callback;
		window->callback_data = data;
		return TRUE;
	}

	/* Set up the icon target. */

	icon = window->children;

	while (icon != NULL && icon->icon != i)
		icon = icon->next;

	if (icon == NULL) {
		icon = malloc(sizeof(struct dataxfer_incoming_target));
		if (icon == NULL)
			return FALSE;

		icon->filetype = filetype;
		icon->window = w;
		icon->icon = i;

		icon->callback = NULL;
		icon->callback_data = NULL;

		icon->children = NULL;

		icon->next = window->children;
		window->children = icon;
	}

	icon->callback = callback;
	icon->callback_data = data;

	return TRUE;
}


/**
 * Handle the receipt of a Message_DataSave due to another application trying
 * to transfer data in to us.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool dataxfer_message_data_save(wimp_message *message)
{
	wimp_full_message_data_xfer	*datasave = (wimp_full_message_data_xfer *) message;
	struct dataxfer_descriptor	*descriptor;
	os_error			*error;
	struct dataxfer_incoming_target	*target;


	/* We don't want to respond to our own save requests. */

	if (message->sender == main_task_handle)
		return TRUE;

	/* See if the window is one of the registered targets. */

	target = dataxfer_find_incoming_target(datasave->w, datasave->i, datasave->file_type);

	if (target == NULL || target->callback == NULL)
		return FALSE;

	/* If we've got a target, get a descriptor to track the message exchange. */

	descriptor = dataxfer_new_descriptor();
	if (descriptor == NULL)
		return FALSE;

	descriptor->callback = NULL;
	descriptor->callback_data = target;

	/* Update the message block and send an acknowledgement. */

	datasave->your_ref = datasave->my_ref;
	datasave->action = message_DATA_SAVE_ACK;
	datasave->est_size = -1;
	strcpy(datasave->file_name, "<Wimp$Scrap>");
	datasave->size = WORDALIGN(45 + strlen(datasave->file_name));

	error = xwimp_send_message(wimp_USER_MESSAGE_RECORDED, (wimp_message *) datasave, datasave->sender);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);

		dataxfer_delete_descriptor(descriptor);
		return TRUE;
	}

	descriptor->type = DATAXFER_MESSAGE_LOAD;
	descriptor->my_ref = datasave->my_ref;

	return TRUE;
}


/**
 * Handle the receipt of a Message_DataLoad due to a filer load or an ongoing
 * data transfer process.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool dataxfer_message_data_load(wimp_message *message)
{
	wimp_full_message_data_xfer	*dataload = (wimp_full_message_data_xfer *) message;
	struct dataxfer_descriptor	*descriptor = NULL;
	os_error			*error;
	struct dataxfer_incoming_target	*target;


	/* We don't want to respond to our own save requests. */

	if (message->sender == main_task_handle)
		return TRUE;

	/* See if we know about this transfer already.  If we do, this is the
	 * closing stage of a full transfer; if not, we must ask the client
	 * if we need to proceed.
	 */

	descriptor = dataxfer_find_descriptor(message->your_ref, DATAXFER_MESSAGE_LOAD);
	if (descriptor == NULL) {
		/* See if the window is one of the registered targets. */

		target = dataxfer_find_incoming_target(dataload->w, dataload->i, dataload->file_type);

		if (target == NULL || target->callback == NULL)
			return FALSE;
	} else {
		target = descriptor->callback_data;
	}

	/* If there's no load callback function, abandon the transfer here. */

	if (target->callback == NULL)
		return FALSE;

	/* If the load failed, abandon the transfer here. */

	if (target->callback(dataload->w, dataload->i, dataload->file_type, dataload->file_name, target->callback_data) == FALSE)
		return FALSE;

	/* If this was an inter-application transfer, tidy up. */

	if (descriptor != NULL) {
		xosfscontrol_wipe(dataload->file_name, NONE, 0, 0, 0, 0);
		dataxfer_delete_descriptor(descriptor);
	}

	/* Update the message block and send an acknowledgement. */

	dataload->your_ref = dataload->my_ref;
	dataload->action = message_DATA_LOAD_ACK;

	error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) dataload, dataload->sender);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return TRUE;
	}

	return TRUE;
}


/**
 * Handle the receipt of a Message_DataOpen due to a double-click in the Filer.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool dataxfer_message_data_open(wimp_message *message)
{
	wimp_full_message_data_xfer	*dataopen = (wimp_full_message_data_xfer *) message;
	os_error			*error;
	struct dataxfer_incoming_target	*target;


	target = dataxfer_find_incoming_target(wimp_ICON_BAR, -1, dataopen->file_type);

	if (target == NULL || target->callback == NULL)
		return FALSE;

	/* If there's no load callback function, abandon the transfer here. */

	if (target->callback == NULL)
		return FALSE;

	/* Update the message block and send an acknowledgement. Do this before
	 * calling the load callback, to meet the requirements of the PRM.
	 */

	dataopen->your_ref = dataopen->my_ref;
	dataopen->action = message_DATA_LOAD_ACK;

	error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) dataopen, dataopen->sender);
	if (error != NULL) {
		error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
		return TRUE;
	}

	/* Call the load callback. */

	target->callback(NULL, -1, dataopen->file_type, dataopen->file_name, target->callback_data);

	return TRUE;
}


/**
 * Find an incoming transfer target based on the filetype and its target
 * window and icon handle.
 *
 * \param w			The window into which the file was dropped.
 * \param i			The icon onto which the file was dropped.
 * \param filetype		The filetype of the incoming file.
 * \return			The appropriate target, or NULL if none found.
 */

static struct dataxfer_incoming_target *dataxfer_find_incoming_target(wimp_w w, wimp_i i, unsigned filetype)
{
	struct dataxfer_incoming_target		*type, *window, *icon;

	/* Search for a filetype. */

	type = dataxfer_incoming_targets;

	while (type != NULL && type->filetype != filetype)
		type = type->next;

	if (type == NULL)
		return NULL;

	/* Now search for a window. */

	window = type->children;

	while (w != NULL && window != NULL && window->window != w)
		window = window->next;

	if (window == NULL)
		return type;

	/* Now search for an icon. */

	icon = window->children;

	while (i != -1 && icon != NULL && icon->icon != i)
		icon = icon->next;

	if (icon == NULL)
		return window;

	return icon;
}


/**
 * Handle the bounce of a Message during a load or save operation.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool dataxfer_message_bounced(wimp_message *message)
{
	struct dataxfer_descriptor	*descriptor;

	/* The message has bounced, so just clean up. */

	descriptor = dataxfer_find_descriptor(message->your_ref, DATAXFER_MESSAGE_SAVE | DATAXFER_MESSAGE_LOAD);
	if (descriptor != NULL) {
		if (message->action == message_DATA_LOAD) {
			wimp_full_message_data_xfer *dataload = (wimp_full_message_data_xfer *) message;

			xosfscontrol_wipe(dataload->file_name, NONE, 0, 0, 0, 0);
			error_msgs_report_error("XferFail");
		}

		dataxfer_delete_descriptor(descriptor);
		return TRUE;
	}

	return FALSE;
}


/**
 * Create a new message descriptor with no data and return a pointer.
 *
 * \return			The new block, or NULL on failure.
 */

static struct dataxfer_descriptor *dataxfer_new_descriptor(void)
{
	struct dataxfer_descriptor		*new;

	new = malloc(sizeof(struct dataxfer_descriptor));
	if (new != NULL) {
		new->type = DATAXFER_MESSAGE_NONE;

		new->next = dataxfer_descriptors;
		dataxfer_descriptors = new;
	}

	return new;
}


/**
 * Find a record for a message, based on type and reference.
 *
 * \param ref			The message reference field to match.
 * \param type			The message type(s) to match.
 * \return			The message descriptor, or NULL if not found.
 */

static struct dataxfer_descriptor *dataxfer_find_descriptor(int ref, enum dataxfer_message_type type)
{
	struct dataxfer_descriptor		*list = dataxfer_descriptors;

	while (list != NULL && (list->type & type) != 0 && list->my_ref != ref)
		list = list->next;

	return list;
}


/**
 * Delete a message descriptor.
 *
 * \param *message		The message descriptor to be deleted.
 */

static void dataxfer_delete_descriptor(struct dataxfer_descriptor *message)
{
	struct dataxfer_descriptor		*list = dataxfer_descriptors;

	if (message == NULL)
		return;

	if (dataxfer_descriptors == message) {
		dataxfer_descriptors = message->next;
		free(message);
		return;
	}

	while (list != NULL && list->next != message)
		list = list->next;

	if (list != NULL)
		list->next = message->next;

	free(message);
}

