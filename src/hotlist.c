/* Copyright 2013, Stephen Fryatt
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
 * \file: hotlist.c
 *
 * Hotlist implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

//#include "oslib/os.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

//#include "sflib/debug.h"
//#include "sflib/errors.h"
#include "sflib/event.h"
//#include "sflib/icons.h"
//#include "sflib/menus.h"
//#include "sflib/msgs.h"
//#include "sflib/string.h"
//#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files */

#include "iconbar.h"

//#include "choices.h"
//#include "dataxfer.h"
#include "dialogue.h"
//#include "discfile.h"
//#include "file.h"
#include "ihelp.h"
//#include "main.h"
#include "templates.h"


/* Hotlist Add Window */

#define HOTLIST_ADD_ICON_NAME 1
#define HOTLIST_ADD_ICON_CANCEL 2
#define HOTLIST_ADD_ICON_ADD 3


static void	hotlist_add_click_handler(wimp_pointer *pointer);
static osbool	hotlist_add_keypress_handler(wimp_key *key);



static wimp_menu		*hotlist_menu = NULL;				/**< The hotlist menu handle.							*/
static wimp_w			hotlist_add_window = NULL;			/**< The add to hotlist window handle.						*/
static struct dialogue_block	*hotlist_add_dialogue_handle = NULL;		/**< The handle of the dialogue to be added to the hotlist, or NULL if none.	*/


/**
 * Initialise the iconbar icon and its associated menus and dialogues.
 */

void hotlist_initialise(void)
{
	//char*			date = BUILD_DATE;
	//wimp_icon_create	icon_bar;

	//iconbar_menu = templates_get_menu(TEMPLATES_MENU_ICONBAR);

	hotlist_add_window = templates_create_window("HotlistAdd");
	//templates_link_menu_dialogue("ProgInfo", iconbar_info_window);
	ihelp_add_window(hotlist_add_window, "HotlistAdd", NULL);
	event_add_window_mouse_event(hotlist_add_window, hotlist_add_click_handler);
	event_add_window_key_event(hotlist_add_window, hotlist_add_keypress_handler);
}


/**
 * Add a dialogue to the hotlist.
 *
 * \param *dialogue		The handle of the dialogue to be added.
 */

void hotlist_add_dialogue(struct dialogue_block *dialogue)
{
	wimp_pointer	pointer;

	hotlist_add_dialogue_handle = dialogue;

	wimp_get_pointer_info(&pointer);
	windows_open_centred_at_pointer(hotlist_add_window, &pointer);
}


/**
 * Identify whether the Hotlist Add Dialogue window is currently open.
 *
 * \return		TRUE if the window is open; else FALSE.
 */

osbool hotlist_add_window_is_open(void)
{
	return windows_get_open(hotlist_add_window);
}






/**
 * Process mouse clicks in the Hotlist Add dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void hotlist_add_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch ((int) pointer->i) {
	case HOTLIST_ADD_ICON_ADD:
		if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
/*			if (!dialogue_read_window(dialogue_data))
				break;

			dialogue_start_search(dialogue_data);

			if (pointer->buttons == wimp_CLICK_SELECT) {
				settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
				dialogue_close_window();
			}*/
		}
		break;

	case HOTLIST_ADD_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			wimp_close_window(hotlist_add_window);
			hotlist_add_dialogue_handle = NULL;
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			//dialogue_set_window(dialogue_data);
			//dialogue_redraw_window();
		}
		break;
	}
}


/**
 * Process keypresses in the Hotlist Add window.
 *
 * \param *key			The keypress event block to handle.
 * \return			TRUE if the event was handled; else FALSE.
 */

static osbool hotlist_add_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
/*		settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
		if (!dialogue_read_window(dialogue_data))
			break;
		dialogue_start_search(dialogue_data);
		dialogue_close_window();*/
		break;

	case wimp_KEY_ESCAPE:
		wimp_close_window(hotlist_add_window);
		hotlist_add_dialogue_handle = NULL;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

