/* Copyright 2013, Stephen Fryatt (info@stevefryatt.org.uk)
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
#include "sflib/heap.h"
//#include "sflib/icons.h"
//#include "sflib/menus.h"
#include "sflib/msgs.h"
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

#define HOTLIST_NAME_LENGTH 48


/**
 * The structure to contain details of a hotlist entry.
 */

struct hotlist_data {
	char			name[HOTLIST_NAME_LENGTH];			/**< The name of the hotlist entry.						*/
	struct dialogue_block	*dialogue;					/**< The data associated with the hotlist entry.				*/
};

/* Global variables. */

static wimp_menu		*hotlist_menu = NULL;				/**< The hotlist menu handle.							*/
static wimp_w			hotlist_add_window = NULL;			/**< The add to hotlist window handle.						*/
static struct dialogue_block	*hotlist_add_dialogue_handle = NULL;		/**< The handle of the dialogue to be added to the hotlist, or NULL if none.	*/

/* Function prototypes. */

static void	hotlist_add_click_handler(wimp_pointer *pointer);
static osbool	hotlist_add_keypress_handler(wimp_key *key);


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
	dialogue_add_client(dialogue, DIALOGUE_CLIENT_HOTLIST);

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
			dialogue_destroy(hotlist_add_dialogue_handle, DIALOGUE_CLIENT_HOTLIST);
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
		dialogue_destroy(hotlist_add_dialogue_handle, DIALOGUE_CLIENT_HOTLIST);
		hotlist_add_dialogue_handle = NULL;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Build a hotlist menu and return a pointer to it.
 *
 * \return			A pointer to the menu block.
 */

wimp_menu *hotlist_build_menu(void)
{
	int			length, context = 0, line, width = 0;
	//char			buffer[TYPEMENU_NAME_LENGTH], *sprite_base, *validation, *var_name;
	os_var_type		type;
	os_error		*error;


	/* Allocate space for the Wimp menu block. */

	if (hotlist_menu != NULL)
		heap_free(hotlist_menu);

	hotlist_menu = heap_alloc(28 + 24 * 1);

	if (hotlist_menu == NULL)
		return NULL;

	/* Build the menu from the collected types. */

//	if (strlen(typemenu_types[line].name) > width)
//		width = strlen(typemenu_types[line].name);

	/* Set the menu and icon flags up. */

	line = 0;

	hotlist_menu->entries[line].menu_flags = NONE;

	hotlist_menu->entries[line].sub_menu = (wimp_menu *) -1;
	hotlist_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED |
			wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
			wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

	/* Set the menu icon contents up. */

	msgs_lookup("HotlistEdit", hotlist_menu->entries[line].data.text, 12);
	//hotlist_menu->entries[line].data.indirected_text.validation = typemenu_types[line].validation;
	//hotlist_menu->entries[line].data.indirected_text.size = TYPEMENU_NAME_LENGTH;


	#if 0

	for (line = 0; line < typemenu_entries; line++) {
		if (strlen(typemenu_types[line].name) > width)
			width = strlen(typemenu_types[line].name);

		/* Set the menu and icon flags up. */

		hotlist_menu->entries[line].menu_flags = NONE;

		hotlist_menu->entries[line].sub_menu = (wimp_menu *) -1;
		hotlist_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
				wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
				wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

		/* Set the menu icon contents up. */

		hotlist_menu->entries[line].data.indirected_text.text = typemenu_types[line].name;
		hotlist_menu->entries[line].data.indirected_text.validation = typemenu_types[line].validation;
		hotlist_menu->entries[line].data.indirected_text.size = TYPEMENU_NAME_LENGTH;
	}
	#endif
	
	line++;

	//if (typemenu_entries > 1)
	//	typemenu_menu->entries[0].menu_flags |= wimp_MENU_SEPARATE;

	hotlist_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	msgs_lookup("Hotlist", hotlist_menu->title_data.text, 12);
	hotlist_menu->title_fg = wimp_COLOUR_BLACK;
	hotlist_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	hotlist_menu->work_fg = wimp_COLOUR_BLACK;
	hotlist_menu->work_bg = wimp_COLOUR_WHITE;

	hotlist_menu->width = 40 + (width + 1) * 16;
	hotlist_menu->height = 44;
	hotlist_menu->gap = 0;

	return hotlist_menu;
}


/**
 * Process a selection from the hotlist menu.
 *
 * \param selection		The menu selection.
 */

void hotlist_process_menu_selection(int selection)
{
	unsigned	*types;
	int		entries = 0;
	osbool		found = FALSE;

	if (selection < 0 || selection >= 1 /*typemenu_entries*/)
		return;
		
	debug_printf("Selected hotlist menu item %d", selection);
}


