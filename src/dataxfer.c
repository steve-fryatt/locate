/* Locate - dataxfer.c
 *
 * (C) Stephen Fryatt, 2012
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/dragasprite.h"
#include "oslib/osbyte.h"
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


#define DATAXFER_SAVEAS_ICON_SAVE 0
#define DATAXFER_SAVEAS_ICON_CANCEL 1
#define DATAXFER_SAVEAS_ICON_FILENAME 2
#define DATAXFER_SAVEAS_ICON_FILE 3
#define DATAXFER_SAVEAS_ICON_SELECTION 4


#define DATAXFER_MAX_FILENAME 256
#define DATAXFER_MAX_SPRNAME 16

/* ==================================================================================================================
 * Global variables.
 */


/**
 * Data associated with a message exchange.
 */

enum dataxfer_message_type {
	DATAXFER_MESSAGE_NONE = 0,							/**< An unset message block.						*/
	DATAXFER_MESSAGE_SAVE,								/**< A message block associated with a Message_DataSave.		*/
	DATAXFER_MESSAGE_LOAD								/**< A message block associated with a Message_DataLoad.		*/
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

/* Data associated with the Save As window. */

struct dataxfer_savebox {
	char				full_filename[DATAXFER_MAX_FILENAME];		/**< The full filename to be used in the savebox.			*/
	char				selection_filename[DATAXFER_MAX_FILENAME];	/**< The selection filename to be used in the savebox.			*/
	char				sprite[DATAXFER_MAX_SPRNAME];			/**< The sprite to be used in the savebox.				*/

	wimp_w				window;						/**< The window handle of the savebox to be used.			*/
	osbool				selection;					/**< TRUE if the selection icon is enabled; else FALSE.			*/
	osbool				selected;					/**< TRUE if the selection icon is ticked; else FALSE.			*/

	osbool				(*callback)(char *filename, osbool selection);	/**< The callback function to be used if a save is required.		*/
};


static wimp_w				dataxfer_saveas_window = NULL;			/**< The Save As window handle.						*/
static wimp_w				dataxfer_saveas_sel_window = NULL;		/**< The Save As window with selection icon.				*/

/* Data asscoiated with drag box handling. */

static osbool	dataxfer_dragging_sprite = FALSE;					/**< TRUE if we're dragging a sprite, FALSE if dragging a dotted-box.	*/
static void	(*dataxfer_drag_end_callback)(wimp_pointer *, void *) = NULL;		/**< The callback function to be used by the icon drag code.		*/


/**
 * Function prototypes.
 */

static void				dataxfer_saveas_click_handler(wimp_pointer *pointer);
static osbool				dataxfer_saveas_keypress_handler(wimp_key *key);

static void				dataxfer_drag_end_handler(wimp_pointer *pointer, void *data);
static osbool				dataxfer_save_handler(char *filename, void *data);
static void				dataxfer_immediate_save(struct dataxfer_savebox *handle);

static void				dataxfer_terminate_user_drag(wimp_dragged *drag, void *data);
static osbool				dataxfer_message_data_save_ack(wimp_message *message);
static osbool				dataxfer_message_data_load_ack(wimp_message *message);
static osbool				dataxfer_message_bounced(wimp_message *message);

static struct dataxfer_descriptor	*dataxfer_new_descriptor(void);
static struct dataxfer_descriptor	*dataxfer_find_descriptor(int ref, enum dataxfer_message_type type);
static void				dataxfer_delete_descriptor(struct dataxfer_descriptor *message);

//static osbool		message_data_save_reply(wimp_message *message);
//static osbool		message_data_load_reply(wimp_message *message);


/**
 * Initialise the data transfer system.
 */

void dataxfer_initialise(void)
{
	dataxfer_saveas_window = templates_create_window("SaveAs");
	ihelp_add_window(dataxfer_saveas_window, "SaveAs", NULL);
	event_add_window_mouse_event(dataxfer_saveas_window, dataxfer_saveas_click_handler);
	event_add_window_key_event(dataxfer_saveas_window, dataxfer_saveas_keypress_handler);
	templates_link_menu_dialogue("save_as", dataxfer_saveas_window);
	event_add_window_user_data(dataxfer_saveas_window, NULL);

	dataxfer_saveas_sel_window = templates_create_window("SaveAsSel");
	ihelp_add_window(dataxfer_saveas_sel_window, "SaveAs", NULL);
	event_add_window_mouse_event(dataxfer_saveas_sel_window, dataxfer_saveas_click_handler);
	event_add_window_key_event(dataxfer_saveas_sel_window, dataxfer_saveas_keypress_handler);
	templates_link_menu_dialogue("save_as_sel", dataxfer_saveas_sel_window);
	event_add_window_user_data(dataxfer_saveas_sel_window, NULL);


	//event_add_message_handler(message_DATA_SAVE, EVENT_MESSAGE_INCOMING, message_data_save_reply);
	event_add_message_handler(message_DATA_SAVE_ACK, EVENT_MESSAGE_INCOMING, dataxfer_message_data_save_ack);
	//event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, message_data_load_reply);
	event_add_message_handler(message_DATA_LOAD_ACK, EVENT_MESSAGE_INCOMING, dataxfer_message_data_load_ack);

	event_add_message_handler(message_DATA_SAVE, EVENT_MESSAGE_ACKNOWLEDGE, dataxfer_message_bounced);
	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_ACKNOWLEDGE, dataxfer_message_bounced);
}


/**
 * Open the Save As dialogue at the pointer.
 *
 * \param *pointer		The pointer location to open the dialogue.
 */

void dataxfer_open_saveas_window(wimp_pointer *pointer)
{
	//menus_create_standard_menu((wimp_menu *) dataxfer_saveas_window, pointer);
}


/**
 * Create a new save dialogue definition.
 *
 * \param: selection		TRUE if the dialogue has a Selection switch; FALSE if not.
 * \param: *sprite		Pointer to the sprite name for the dialogue.
 * \param: *save_callback	The callback function for saving data.
 * \return			The handle to use for the new save dialogue.
 */

struct dataxfer_savebox *dataxfer_new_savebox(osbool selection, char *sprite, osbool (*save_callback)(char *filename, osbool selection))
{
	struct dataxfer_savebox		*new;

	new = malloc(sizeof(struct dataxfer_savebox));
	if (new == NULL)
		return NULL;

	new->full_filename[0] = '\0';
	new->selection_filename[0] = '\0';
	strncpy(new->sprite, (sprite != NULL) ? sprite : "", DATAXFER_MAX_SPRNAME);

	new->window = (selection) ? dataxfer_saveas_sel_window : dataxfer_saveas_window;
	new->selection = FALSE;
	new->selected = FALSE;
	new->callback = save_callback;

	return new;
}


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
 */

void dataxfer_savebox_initialise(struct dataxfer_savebox *handle, char *fullname, char *selectname, osbool selection, osbool selected)
{
	if (handle == NULL)
		return;

	if (fullname != NULL)
		msgs_lookup(fullname, handle->full_filename, DATAXFER_MAX_FILENAME);
	else
		handle->full_filename[0] = '\0';

	if (selectname != NULL)
		msgs_lookup(selectname, handle->selection_filename, DATAXFER_MAX_FILENAME);
	else
		handle->selection_filename[0] = '\0';

	handle->selection = selection;
	handle->selected = (selection) ? selected : FALSE;

	event_add_window_user_data(dataxfer_saveas_window, NULL);
	event_add_window_user_data(dataxfer_saveas_sel_window, NULL);
}


/**
 * Prepare the physical save dialogue based on the current state of the given
 * dialogue data.  Any existing data will be saved into the appropriate
 * dialogue definition first.
 *
 * \param *handle		The handle of the save dialogue to prepare.
 */

void dataxfer_savebox_prepare(struct dataxfer_savebox *handle)
{
	struct dataxfer_savebox		*old_handle;

	if (handle == NULL)
		return;

	old_handle = event_get_window_user_data(handle->window);
	if (old_handle != NULL) {
		if (old_handle->window == dataxfer_saveas_window) {
			icons_copy_text(old_handle->window, DATAXFER_SAVEAS_ICON_FILENAME, old_handle->full_filename);
			old_handle->selected = FALSE;
		} else {
			old_handle->selected = icons_get_selected(old_handle->window, DATAXFER_SAVEAS_ICON_SELECTION);
			icons_copy_text(old_handle->window, DATAXFER_SAVEAS_ICON_FILENAME, (old_handle->selected) ? old_handle->selection_filename : old_handle->full_filename);
		}
	}

	event_add_window_user_data(handle->window, handle);

	icons_printf(handle->window, DATAXFER_SAVEAS_ICON_FILE, handle->sprite);

	if (handle->window == dataxfer_saveas_window) {
		icons_printf(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, handle->full_filename);
	} else {
		icons_set_shaded(handle->window, DATAXFER_SAVEAS_ICON_SELECTION, !handle->selection);
		icons_set_selected(handle->window, DATAXFER_SAVEAS_ICON_SELECTION, handle->selected);
		icons_printf(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, (handle->selected) ? handle->selection_filename : handle->full_filename);
	}
}


/**
 * Process mouse clicks in the Save As dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void dataxfer_saveas_click_handler(wimp_pointer *pointer)
{
	struct dataxfer_savebox		*handle;

	if (pointer == NULL || (pointer->w != dataxfer_saveas_window && pointer->w != dataxfer_saveas_sel_window))
		return;

	handle = event_get_window_user_data(pointer->w);
	if (handle == NULL)
		return;

	switch (pointer->i) {
	case DATAXFER_SAVEAS_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT)
			wimp_create_menu(NULL, 0, 0);
		break;

	case DATAXFER_SAVEAS_ICON_SAVE:
		if (pointer->buttons == wimp_CLICK_SELECT)
			dataxfer_immediate_save(handle);
		break;

	case DATAXFER_SAVEAS_ICON_FILE:
		if (pointer->buttons == wimp_DRAG_SELECT)
			dataxfer_save_window_drag(pointer->w, DATAXFER_SAVEAS_ICON_FILE, dataxfer_drag_end_handler, handle);
		break;

	case DATAXFER_SAVEAS_ICON_SELECTION:
		handle->selected = icons_get_selected(handle->window, DATAXFER_SAVEAS_ICON_SELECTION);
		icons_copy_text(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, (handle->selected) ? handle->full_filename : handle->selection_filename);
		icons_strncpy(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, (handle->selected) ? handle->selection_filename : handle->full_filename);
		wimp_set_icon_state(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, 0, 0);
		icons_replace_caret_in_window(handle->window);
		break;
	}
}


/**
 * Process keypresses in the Save As dialogue.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool dataxfer_saveas_keypress_handler(wimp_key *key)
{
	struct dataxfer_savebox		*handle;

	handle = event_get_window_user_data(key->w);
	if (handle == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		dataxfer_immediate_save(handle);
		break;

	case wimp_KEY_ESCAPE:
		wimp_create_menu(NULL, 0, 0);
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process the termination of icon drags from the Save dialogues.
 *
 * \param *pointer		The pointer location at the end of the drag.
 * \param *data			The dataxfer_savebox data for the drag.
 */

static void dataxfer_drag_end_handler(wimp_pointer *pointer, void *data)
{
	struct dataxfer_savebox		*handle = data;

	if (handle == NULL)
		return;

	if (handle->window == dataxfer_saveas_window) {
		icons_copy_text(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, handle->full_filename);
		handle->selected = FALSE;
	} else {
		handle->selected = icons_get_selected(handle->window, DATAXFER_SAVEAS_ICON_SELECTION);
		icons_copy_text(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, (handle->selected) ? handle->selection_filename : handle->full_filename);
	}


	dataxfer_start_save(pointer, (handle->selected) ? handle->selection_filename : handle->full_filename, 0, 0xffffffffu, dataxfer_save_handler, handle);

	wimp_create_menu(NULL, 0, 0);
}


/**
 * Process data transfer results for Save As dialogues.
 *
 * \param *filename		The destination of the dragged file.
 * \param *data			Context data.
 * \return			TRUE if the save succeeded; FALSE if it failed.
 */

static osbool dataxfer_save_handler(char *filename, void *data)
{
	struct dataxfer_savebox		*handle = data;

	if (handle == NULL || handle->callback == NULL)
		return FALSE;

	return handle->callback(filename, handle->selected);
}


/**
 * Perform an immediate save based on the information in a save
 * dialogue.
 *
 * \param *handle		The handle of the save dialogue.
 */

static void dataxfer_immediate_save(struct dataxfer_savebox *handle)
{
	char	*filename;

	if (handle == NULL)
		return;

	if (handle->window == dataxfer_saveas_window) {
		icons_copy_text(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, handle->full_filename);
		handle->selected = FALSE;
	} else {
		handle->selected = icons_get_selected(handle->window, DATAXFER_SAVEAS_ICON_SELECTION);
		icons_copy_text(handle->window, DATAXFER_SAVEAS_ICON_FILENAME, (handle->selected) ? handle->selection_filename : handle->full_filename);
	}

	filename = (handle->selected) ? handle->selection_filename : handle->full_filename;

	if (strchr(filename, '.') == NULL) {
		error_msgs_report_info("BadSave");
		return;
	}

	if (handle->callback !=NULL)
		handle->callback(filename, handle->selected);
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
			osbyte_CONFIGURE_DRAG_ASPRITE_MASK) != 0);

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
 * \param *save_callback	The function to be called with the full pathname
 *				to save the file.
 * \param *data			Data to be passed to the callback function.
 * \return			TRUE on success; FALSE on failure.
 */

osbool dataxfer_start_save(wimp_pointer *pointer, char *name, int size, bits type, osbool (*save_callback)(char *filename, void *data), void *data)
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
	message.your_ref = 0;
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

	debug_printf("Sent Message_DataSave with descriptor 0x%x", descriptor);

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

	debug_printf("Received Message_DataSaveAck with descriptor 0x%x", descriptor);

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

	debug_printf("Sent Message_DataLoad with descriptor 0x%x", descriptor);

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

	debug_printf("Received Message_DataLoadAck with descriptor 0x%x", descriptor);

	dataxfer_delete_descriptor(descriptor);
	return TRUE;
}

/**
 * Handle the bounce of a Message_DataSave or Message_DataLoad.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool dataxfer_message_bounced(wimp_message *message)
{
	struct dataxfer_descriptor	*descriptor;

	/* The message has bounced, so just clean up. */

	descriptor = dataxfer_find_descriptor(message->your_ref, DATAXFER_MESSAGE_SAVE);
	if (descriptor != NULL) {
		debug_printf("Save message bounced with descriptor 0x%x", descriptor);

		dataxfer_delete_descriptor(descriptor);
		return TRUE;
	}

	return FALSE;
}



















#if 0

/**
 * Handle the receipt of a Message_DataSaveAck, usually in response to another
 * application trying to save a file to our icon.  Supply the location of the
 * queue head file, which allows postscript printer drivers to be set up by
 * dragging their save icon to our iconbar.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show that the message was handled.
 */

static osbool message_data_save_reply(wimp_message *message)
{
	wimp_full_message_data_xfer	*datasave = (wimp_full_message_data_xfer *) message;
	os_error			*error;

	if (message->sender == main_task_handle) /* We don't want to respond to our own save requests. */
		return TRUE;


	if (datasave->w == wimp_ICON_BAR && datasave->file_type == TEXT_FILE_TYPE) {
		datasave->your_ref = datasave->my_ref;
		datasave->action = message_DATA_SAVE_ACK;

		switch (datasave->file_type) {
		case TEXT_FILE_TYPE:
			strcpy(datasave->file_name, "<Wimp$Scrap>");
			break;
		}

		datasave->size = WORDALIGN(45 + strlen(datasave->file_name));

		error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) datasave, datasave->sender);
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);
	}

	return TRUE;
}


/**
 * Handle the receipt of a Message_DataLoad, generally as a result of a file
 * being dragged from the Filer to one of our windows or icons.
 *
 * \param *message		The associated Wimp message block.
 * \return			TRUE to show the message was handled.
 */

static osbool message_data_load_reply(wimp_message *message)
{
	wimp_full_message_data_xfer	*dataload = (wimp_full_message_data_xfer *) message;
	os_error			*error;
	osbool				handled = FALSE;

	if (dataload->w == wimp_ICON_BAR && dataload->file_type == TEXT_FILE_TYPE) {
		switch (dataload->file_type) {
		case TEXT_FILE_TYPE:
			//convert_load_file(dataload->file_name);
			break;
		}

		/* Reply with a Message_DataLoadAck. */

		dataload->your_ref = dataload->my_ref;
		dataload->action = message_DATA_LOAD_ACK;

		error = xwimp_send_message(wimp_USER_MESSAGE, (wimp_message *) dataload, dataload->sender);
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_CANCEL_ICON);

		handled = TRUE;
	}

	return handled;
}

#endif


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
 * \param type			The message type to match.
 * \return			The message descriptor, or NULL if not found.
 */


static struct dataxfer_descriptor *dataxfer_find_descriptor(int ref, enum dataxfer_message_type type)
{
	struct dataxfer_descriptor		*list = dataxfer_descriptors;

	while (list != NULL && list->type != type && list->my_ref != ref)
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

