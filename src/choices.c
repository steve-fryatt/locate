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
 * \file: choices.c
 *
 * Choices dialogue implementation.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */


/* OSLib Header files. */

#include "oslib/osbyte.h"
#include "oslib/wimp.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/string.h"

/* Application header files. */

#include "choices.h"

#include "ihelp.h"
#include "search.h"
#include "templates.h"


/* Choices window icons. */

#define CHOICE_ICON_APPLY 0
#define CHOICE_ICON_SAVE 1
#define CHOICE_ICON_CANCEL 2
#define CHOICE_ICON_SEARCH_PATH 6
#define CHOICE_ICON_STORE_ALL 7
#define CHOICE_ICON_IMAGE_FS 8
#define CHOICE_ICON_SUPPRESS_ERRORS 9
#define CHOICE_ICON_FULL_INFO 10
#define CHOICE_ICON_PLUGIN_QUIT 13
#define CHOICE_ICON_PLUGIN_WINDOW 14
#define CHOICE_ICON_AUTOSCROLL 15
#define CHOICE_ICON_VALIDATE_PATHS 16


/* Global variables */

static wimp_w			choices_window = NULL;

static void	choices_close_window(void);
static void	choices_set_window(void);
static osbool	choices_read_window(void);
static void	choices_redraw_window(void);

static void	choices_click_handler(wimp_pointer *pointer);
static osbool	choices_keypress_handler(wimp_key *key);

static osbool	handle_choices_icon_drop(wimp_message *message);


/**
 * Initialise the Choices module.
 */

void choices_initialise(void)
{
	wimp_window	*def = templates_load_window("Choices");
	int		buf_size = config_int_read("PathBufSize");

	if (def == NULL)
		error_msgs_report_fatal("BadTemplate");

	def->icons[CHOICE_ICON_SEARCH_PATH].data.indirected_text.text = heap_alloc(buf_size);
	def->icons[CHOICE_ICON_SEARCH_PATH].data.indirected_text.size = buf_size;
	choices_window = wimp_create_window(def);
	icons_printf(choices_window, CHOICE_ICON_SEARCH_PATH, "");
	free(def);
	ihelp_add_window(choices_window, "Choices", NULL);

	event_add_window_mouse_event(choices_window, choices_click_handler);
	event_add_window_key_event(choices_window, choices_keypress_handler);

	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, handle_choices_icon_drop);
}


/**
 * Open the Choices window at the mouse pointer.
 *
 * \param *pointer		The details of the pointer to open the window at.
 */

void choices_open_window(wimp_pointer *pointer)
{
	if (windows_get_open(choices_window))
		return;

	choices_set_window();

	windows_open_centred_at_pointer(choices_window, pointer);

	icons_put_caret_at_end(choices_window, CHOICE_ICON_SEARCH_PATH);
}


/**
 * Close the choices window.
 */

static void choices_close_window(void)
{
	wimp_close_window(choices_window);
}


/**
 * Set the contents of the Choices window to reflect the current settings.
 */

static void choices_set_window(void)
{
	icons_printf(choices_window, CHOICE_ICON_SEARCH_PATH, "%s", config_str_read("SearchPath"));

	icons_set_selected(choices_window, CHOICE_ICON_STORE_ALL, config_opt_read("StoreAllFiles"));
	icons_set_selected(choices_window, CHOICE_ICON_IMAGE_FS, config_opt_read("ImageFS"));
	icons_set_selected(choices_window, CHOICE_ICON_SUPPRESS_ERRORS, config_opt_read("SuppressErrors"));
	icons_set_selected(choices_window, CHOICE_ICON_FULL_INFO, config_opt_read("FullInfoDisplay"));
	icons_set_selected(choices_window, CHOICE_ICON_PLUGIN_QUIT, config_opt_read("QuitAsPlugin"));
	icons_set_selected(choices_window, CHOICE_ICON_PLUGIN_WINDOW, config_opt_read("SearchWindAsPlugin"));
	icons_set_selected(choices_window, CHOICE_ICON_AUTOSCROLL, config_opt_read("ScrollResults"));
	icons_set_selected(choices_window, CHOICE_ICON_VALIDATE_PATHS, config_opt_read("ValidatePaths"));
}


/**
 * Update the configuration settings from the values in the Choices window.
 *
 * \return			TRUE if successful; FALSE if validation errors occurred.
 */

static osbool choices_read_window(void)
{
	wimp_error_box_selection	selection;

	/* Validate the search paths. */

	if (!search_validate_paths(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SEARCH_PATH), FALSE)) {
		selection = error_msgs_report_question("BadConfigPaths", "BadConfigPathsB");

		if (selection == 1)
			return FALSE;
	}

	/* Read the main window. */

	config_str_set("SearchPath", icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SEARCH_PATH));

	config_opt_set("StoreAllFiles", icons_get_selected(choices_window, CHOICE_ICON_STORE_ALL));
	config_opt_set("ImageFS", icons_get_selected(choices_window, CHOICE_ICON_IMAGE_FS));
	config_opt_set("SuppressErrors", icons_get_selected(choices_window, CHOICE_ICON_SUPPRESS_ERRORS));
	config_opt_set("FullInfoDisplay", icons_get_selected(choices_window, CHOICE_ICON_FULL_INFO));
	config_opt_set("QuitAsPlugin", icons_get_selected(choices_window, CHOICE_ICON_PLUGIN_QUIT));
	config_opt_set("SearchWindAsPlugin", icons_get_selected(choices_window, CHOICE_ICON_PLUGIN_WINDOW));
	config_opt_set("ScrollResults", icons_get_selected(choices_window, CHOICE_ICON_AUTOSCROLL));
	config_opt_set("ValidatePaths", icons_get_selected(choices_window, CHOICE_ICON_VALIDATE_PATHS));

	return TRUE;
}


/**
 * Refresh the Choices dialogue, to reflech changed icon states.
 */

static void choices_redraw_window(void)
{
	wimp_set_icon_state(choices_window, CHOICE_ICON_SEARCH_PATH, 0, 0);

	icons_replace_caret_in_window(choices_window);
}


/**
 * Process mouse clicks in the Choices dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void choices_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch ((int) pointer->i) {
	case CHOICE_ICON_APPLY:
		if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
			if (!choices_read_window())
				break;

			if (pointer->buttons == wimp_CLICK_SELECT)
				choices_close_window();
		}
		break;

	case CHOICE_ICON_SAVE:
		if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
			if (!choices_read_window())
				break;
			config_save();

			if (pointer->buttons == wimp_CLICK_SELECT)
				choices_close_window();
		}
		break;

	case CHOICE_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			choices_close_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			choices_set_window();
			choices_redraw_window();
		}
		break;
	}
}


/**
 * Process keypresses in the Choices window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool choices_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		choices_read_window();
		config_save();
		choices_close_window();
		break;

	case wimp_KEY_ESCAPE:
		choices_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Check incoming Message_DataSave to see if it's a file being dropped into the
 * the search path icon.
 *
 * \param *message		The incoming message block.
 * \return			TRUE if we claim the message as intended for us; else FALSE.
 */

static osbool handle_choices_icon_drop(wimp_message *message)
{
	wimp_full_message_data_xfer	*datasave = (wimp_full_message_data_xfer *) message;

	char				*insert, *end, path[256], *p;

	/* If it isn't our window, don't claim the message as someone else
	 * might want it.
	 */

	if (datasave == NULL || datasave->w != choices_window)
		return FALSE;

	/* If it is our window, but not the icon we care about, claim
	 * the message.
	 */

	if (datasave->i != CHOICE_ICON_SEARCH_PATH)
		return TRUE;

	/* It's our window and the correct icon, so start by copying the filename. */

	strcpy(path, datasave->file_name);

	/* If it's a folder, take just the pathname. */

	if (datasave->file_type <= 0xfff)
		string_find_pathname(path);

	insert = icons_get_indirected_text_addr(datasave->w, datasave->i);
	end = insert + icons_get_indirected_text_length(datasave->w, datasave->i) - 1;

	/* Unless Shift is pressed, find the end of the current buffer. */

	if (osbyte1(osbyte_IN_KEY, 0xfc, 0xff) == 0 && osbyte1(osbyte_IN_KEY, 0xf9, 0xff) == 0) {
		while (*insert != '\0' && insert < end)
			insert++;

		if (insert < end)
			*insert++ = ',';

		*insert = '\0';
	}

	/* Copy the new path across. */

	p = path;

	while (*p != '\0' && insert < end)
		*insert++ = *p++;

	*insert = '\0';

	if (*p != '\0')
		error_msgs_report_error("PathBufFull");

	icons_replace_caret_in_window(datasave->w);
	wimp_set_icon_state(datasave->w, datasave->i, 0, 0);

	return TRUE;
}

