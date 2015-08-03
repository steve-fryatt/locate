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
 * \file: plugin.c
 *
 * Support FilerAction plugins on RISC OS Select.
 */

/* ANSI C Header files. */

#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/config.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"

/* OSLib Header files. */

#include "oslib/filer.h"

/* Application header files. */

#include "plugin.h"

#include "file.h"

/**
 * The allowed states of the module's state machine.
 */

enum plugin_state {
	PLUGIN_STATE_IDLE = 0,							/**< The module is idle and no negotaions are underway.			*/
	PLUGIN_STATE_WAITING,							/**< A negotiation has started but no details have been received.	*/
	PLUGIN_STATE_RECEIVED_DIRECTORY,					/**< The directory info has been received; waiting for files.		*/
	PLUGIN_STATE_RECEIVED_FILES						/**< At least one batch of filenames has been received.			*/
};

/* Global Variables.
 */

static enum plugin_state	plugin_current_state = PLUGIN_STATE_IDLE;	/**< The current state of the state machine.				*/

static char			*plugin_buffer = NULL;				/**< A buffer to hold the assembled search paths.			*/
static size_t			plugin_buffer_length = 0;			/**< The size of the assembled path buffer.				*/
static size_t			plugin_buffer_position = 0;			/**< The current insert position in the assmbled path buffer.		*/
static char			*plugin_directory = NULL;			/**< Pointer to a string holding the current directory name.		*/

static osbool plugin_message_filer_action(wimp_message *message);
static osbool plugin_message_filer_selection_dir(wimp_message *message);
static osbool plugin_message_filer_add_selection(wimp_message *message);
static osbool plugin_allocate_buffer(void);
static void plugin_release_buffer(void);


/**
 * Initialise the plugin support
 */

void plugin_initialise(void)
{
	event_add_message_handler(message_FILER_ACTION, EVENT_MESSAGE_INCOMING, plugin_message_filer_action);
	event_add_message_handler(message_FILER_SELECTION_DIR, EVENT_MESSAGE_INCOMING, plugin_message_filer_selection_dir);
	event_add_message_handler(message_FILER_ADD_SELECTION, EVENT_MESSAGE_INCOMING, plugin_message_filer_add_selection);
}


/**
 * Notify the plugin support that we have been laucnched by FilerAction, and
 * therefore are already in the middle of a message exchange.
 */

void plugin_filer_launched(void)
{
	if (plugin_current_state != PLUGIN_STATE_IDLE)
		return;
	
	if (!plugin_allocate_buffer())
		return;

	plugin_current_state = PLUGIN_STATE_WAITING;
	
}


/**
 * Respond to a Message_FilerAction, if the required action is FIND, by
 * starting or completing the process of negotiating the parameters for the
 * search.
 *
 * \param *message		The message to reply to.
 * \return			TRUE if the message was handled; else FALSE.
 */

static osbool plugin_message_filer_action(wimp_message *message)
{
	os_error				*error;
	filer_full_message_action		*filer_action = (filer_full_message_action *) message;
	wimp_full_message_task_close_down	close_down;
	wimp_pointer				pointer;

	
	if (filer_action->operation != fileraction_FIND)
		return FALSE;
		
	switch (plugin_current_state) {
	case PLUGIN_STATE_IDLE:
		debug_printf("Message Exchange Started");
		
		message->your_ref = message->my_ref;
		error = xwimp_send_message(wimp_USER_MESSAGE, message, message->sender);
		
		if (error == NULL && plugin_allocate_buffer())
			plugin_current_state = PLUGIN_STATE_WAITING;
		break;
		
	case PLUGIN_STATE_RECEIVED_FILES:
		debug_printf("Message Exchange completed.");
		plugin_current_state = PLUGIN_STATE_IDLE;
		
		close_down.size = 20;
		close_down.your_ref = 0;
		close_down.action = message_TASK_CLOSE_DOWN;
		wimp_send_message(wimp_USER_MESSAGE, (wimp_message *) &close_down, message->sender);

		if (plugin_buffer != NULL) {
			wimp_get_pointer_info(&pointer);
			file_create_dialogue(&pointer, plugin_buffer, NULL);
		}

		plugin_release_buffer();
		break;
	
	default:
		debug_printf("Unexpected Message_FilerAction");
		plugin_current_state = PLUGIN_STATE_IDLE;
		plugin_release_buffer();
		break;
	}

	return TRUE;
}


/**
 * Respond to a Message_FilerSelectionDir
 *
 * \param *message		The message to reply to.
 * \return			TRUE if the message was handled; else FALSE.
 */

static osbool plugin_message_filer_selection_dir(wimp_message *message)
{
	filer_full_message_selection_dir	*selection_dir = (filer_full_message_selection_dir *) message;

	debug_printf("Received Message_FilerSelectionDir: '%s'", selection_dir->dir_name);

	plugin_current_state = PLUGIN_STATE_RECEIVED_DIRECTORY;

	if (plugin_directory != NULL)
		heap_free(plugin_directory);

	plugin_directory = heap_strdup(selection_dir->dir_name);

	return TRUE;
}


/**
 * Respond to a Message_FilerAddSelection
 *
 * \param *message		The message to reply to.
 * \return			TRUE if the message was handled; else FALSE.
 */

static osbool plugin_message_filer_add_selection(wimp_message *message)
{
	char	*copy = NULL, *name;
	int	p;

	filer_full_message_add_selection	*add_selection = (filer_full_message_add_selection *) message;

	debug_printf("Received Message_FilerAddSelection: '%s'", add_selection->leaf_list);

	plugin_current_state = PLUGIN_STATE_RECEIVED_FILES;

	/* Take a copy of the string before breaking it up. */

	copy = strdup(add_selection->leaf_list);
	if (copy == NULL)
		return TRUE;

	/* Process each name in the string, adding it to the list with the
	 * current directory prefixed to make it into a full path.
	 */

	name = strtok(copy, " ");

	while (name != NULL) {
		if ((plugin_buffer_position > 0) && (plugin_buffer_position < plugin_buffer_length))
			plugin_buffer[plugin_buffer_position++] = ',';

		for (p = 0; (plugin_directory[p] != '\0') && (plugin_buffer_position < plugin_buffer_length); )
			plugin_buffer[plugin_buffer_position++] = plugin_directory[p++];
	
		if (plugin_buffer_position < plugin_buffer_length)
			plugin_buffer[plugin_buffer_position++] = '.';

		for (p = 0; (name[p] != '\0') && (plugin_buffer_position < plugin_buffer_length); )
			plugin_buffer[plugin_buffer_position++] = name[p++];
	
		name = strtok(NULL, " ");
	}

	/* Terminate the string in the buffer. Don't increment the position,
	 * as a subsequent Message_FilerAddSelection will just add to the end
	 * of the buffer.
	 */

	if (plugin_buffer_position < plugin_buffer_length) {
		plugin_buffer[plugin_buffer_position] = '\0';
	} else {
		plugin_buffer[plugin_buffer_length] = '\0';
		error_msgs_report_error("PathBufFull");
	}

	/* Free the copy of the list of filenames. */

	free(copy);

	return TRUE;
}


/**
 * Allocate the space required for the plugin path buffer.
 *
 * \return			TRUE if successful; FALSE on error.
 */

static osbool plugin_allocate_buffer(void)
{
	if (plugin_buffer != NULL)
		plugin_release_buffer();

	plugin_buffer_length = config_int_read("PathBufSize");
	plugin_buffer = heap_alloc(plugin_buffer_length);

	if (plugin_buffer == NULL) {
		plugin_buffer_length = 0;
		error_msgs_report_error("NoMemSearchCreate");
		return FALSE;
	}

	*plugin_buffer = '\0';
	plugin_buffer_position = 0;

	return TRUE;
}


/**
 * Free any memory held for the plugin buffer.
 */

static void plugin_release_buffer(void)
{
	if (plugin_buffer != NULL) {
		heap_free(plugin_buffer);
		plugin_buffer = NULL;
		plugin_buffer_length = 0;
		plugin_buffer_position = 0;
	}
	
	if (plugin_directory != NULL) {
		heap_free(plugin_directory);
		plugin_directory = NULL;
	}
}

