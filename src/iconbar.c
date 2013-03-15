/* Copyright 2012-2013, Stephen Fryatt
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
 * \file: iconbar.c
 *
 * IconBar icon implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/os.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files */

#include "iconbar.h"

#include "choices.h"
#include "dataxfer.h"
#include "dialogue.h"
#include "discfile.h"
#include "file.h"
#include "hotlist.h"
#include "ihelp.h"
#include "main.h"
#include "templates.h"


/* Iconbar menu */

#define ICONBAR_MENU_INFO 0
#define ICONBAR_MENU_HELP 1
#define ICONBAR_MENU_HOTLIST 2
#define ICONBAR_MENU_CHOICES 3
#define ICONBAR_MENU_QUIT 4

/* Program Info Window */

#define ICON_PROGINFO_AUTHOR  4
#define ICON_PROGINFO_VERSION 6
#define ICON_PROGINFO_WEBSITE 8


static void	iconbar_click_handler(wimp_pointer *pointer);
static void	iconbar_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void	iconbar_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static osbool	iconbar_proginfo_web_click(wimp_pointer *pointer);
static osbool	iconbar_icon_drop_handler(wimp_message *message);
static osbool	iconbar_load_locate_file(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data);

static wimp_menu		*iconbar_menu = NULL;				/**< The iconbar menu handle.					*/
static wimp_w			iconbar_info_window = NULL;			/**< The iconbar menu info window handle.			*/
static struct dialogue_block	*iconbar_last_search_dialogue = NULL;		/**< The handle of the last search dialogue, or NULL if none.	*/


/**
 * Initialise the iconbar icon and its associated menus and dialogues.
 */

void iconbar_initialise(void)
{
	char*			date = BUILD_DATE;
	wimp_icon_create	icon_bar;

	iconbar_menu = templates_get_menu(TEMPLATES_MENU_ICONBAR);

	iconbar_info_window = templates_create_window("ProgInfo");
	templates_link_menu_dialogue("ProgInfo", iconbar_info_window);
	ihelp_add_window(iconbar_info_window, "ProgInfo", NULL);
	icons_msgs_param_lookup(iconbar_info_window, ICON_PROGINFO_VERSION, "Version",
			BUILD_VERSION, date, NULL, NULL);
	icons_printf(iconbar_info_window, ICON_PROGINFO_AUTHOR, "\xa9 Stephen Fryatt, 2001-%s", date + 7);
	event_add_window_icon_click(iconbar_info_window, ICON_PROGINFO_WEBSITE, iconbar_proginfo_web_click);

	icon_bar.w = wimp_ICON_BAR_RIGHT;
	icon_bar.icon.extent.x0 = 0;
	icon_bar.icon.extent.x1 = 68;
	icon_bar.icon.extent.y0 = 0;
	icon_bar.icon.extent.y1 = 69;
	icon_bar.icon.flags = wimp_ICON_SPRITE | (wimp_BUTTON_CLICK << wimp_ICON_BUTTON_TYPE_SHIFT);
	msgs_lookup("TaskSpr", icon_bar.icon.data.sprite, osspriteop_NAME_LIMIT);
	wimp_create_icon(&icon_bar);

	event_add_window_mouse_event(wimp_ICON_BAR, iconbar_click_handler);
	event_add_window_menu(wimp_ICON_BAR, iconbar_menu);
	event_add_window_menu_warning(wimp_ICON_BAR, iconbar_menu_warning);
	event_add_window_menu_selection(wimp_ICON_BAR, iconbar_menu_selection);

	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, iconbar_icon_drop_handler);

	dataxfer_set_load_target(DISCFILE_LOCATE_FILETYPE, wimp_ICON_BAR, -1, iconbar_load_locate_file, NULL);
}


/**
 * Handle mouse clicks on the iconbar icon.
 *
 * \param *pointer		The Wimp mouse click event data.
 */

static void iconbar_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch (pointer->buttons) {
	case wimp_CLICK_SELECT:
		file_create_dialogue(pointer, NULL, NULL);
		break;

	case wimp_CLICK_ADJUST:
		file_create_dialogue(pointer, NULL, iconbar_last_search_dialogue);
		break;
	}
}


/**
 * Process submenu warning events for the iconbar menu.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void iconbar_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	wimp_menu	*hotlist;

	switch (warning->selection.items[0]) {
	case ICONBAR_MENU_HOTLIST:
		hotlist = hotlist_build_menu();
		
		if (hotlist != NULL)
			wimp_create_sub_menu(hotlist, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Handle selections from the iconbar menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param  *selection		Pointer to the Wimp menu selction block.
 */

static void iconbar_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	wimp_pointer		pointer;
	os_error		*error;

	wimp_get_pointer_info(&pointer);

	switch(selection->items[0]) {
	case ICONBAR_MENU_HELP:
		error = xos_cli("%Filer_Run <Locate$Dir>.!Help");
		if (error != NULL)
			error_report_os_error(error, wimp_ERROR_BOX_OK_ICON);
		break;

	case ICONBAR_MENU_HOTLIST:
		hotlist_process_menu_selection(selection->items[1]);
		break;

	case ICONBAR_MENU_CHOICES:
		choices_open_window(&pointer);
		break;

	case ICONBAR_MENU_QUIT:
		main_quit_flag = TRUE;
		break;
	}
}


/**
 * Handle clicks on the Website action button in the program info window.
 *
 * \param *pointer	The Wimp Event message block for the click.
 * \return		TRUE if we handle the click; else FALSE.
 */

static osbool iconbar_proginfo_web_click(wimp_pointer *pointer)
{
	char		temp_buf[256];

	msgs_lookup("SupportURL:http://www.stevefryatt.org.uk/software/", temp_buf, sizeof(temp_buf));
	url_launch(temp_buf);

	if (pointer->buttons == wimp_CLICK_SELECT)
		wimp_create_menu((wimp_menu *) -1, 0, 0);

	return TRUE;
}


/**
 * Check incoming Message_DataSave to see if it's a file being dropped into the
 * the iconbar icon.
 *
 * \param *message		The incoming message block.
 * \return			TRUE if we claim the message as intended for us; else FALSE.
 */

static osbool iconbar_icon_drop_handler(wimp_message *message)
{
	wimp_full_message_data_xfer	*datasave = (wimp_full_message_data_xfer *) message;
	wimp_pointer			pointer;
	char				path[256];

	/* If it isn't our window, don't claim the message as someone else
	 * might want it.
	 *
	 * Also pass up Locate files, as they need to be handled centrally
	 * via the dataxfer module.
	 */

	if (datasave == NULL || datasave->w != wimp_ICON_BAR || datasave->file_type == DISCFILE_LOCATE_FILETYPE)
		return FALSE;

	/* It's our iconbar icon, so start by finding the pointer and then copy
	 * the filename for manipulation.
	 */

	wimp_get_pointer_info(&pointer);

	strcpy(path, datasave->file_name);

	/* If it's a folder, take just the pathname. */

	if (datasave->file_type <= 0xfff)
		string_find_pathname(path);

	file_create_dialogue(&pointer, path, NULL);

	return TRUE;
}


/**
 * Handle attempts to load Locate files to the iconbar.
 *
 * \param w			The target window handle.
 * \param i			The target icon handle.
 * \param filetype		The filetype being loaded.
 * \param *filename		The name of the file being loaded.
 * \param *data			Unused NULL pointer.
 * \return			TRUE on loading; FALSE on passing up.
 */

static osbool iconbar_load_locate_file(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data)
{
	if (filetype != DISCFILE_LOCATE_FILETYPE)
		return FALSE;

	file_create_from_saved(filename);

	return TRUE;
}


/**
 * Set a dialogue as the data for the last search. The existing last search
 * dialogue will be discarded.
 *
 * \param *dialogue		The handle of the dialogue to set.
 */

void iconbar_set_last_search_dialogue(struct dialogue_block *dialogue)
{
	if (iconbar_last_search_dialogue != NULL)
		dialogue_destroy(iconbar_last_search_dialogue, DIALOGUE_CLIENT_LAST);

	iconbar_last_search_dialogue = dialogue;

	if (dialogue != NULL)
		dialogue_add_client(dialogue, DIALOGUE_CLIENT_LAST);
}

