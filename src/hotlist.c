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

#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

//#include "oslib/os.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
//#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/string.h"
//#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files */

#include "iconbar.h"

//#include "choices.h"
//#include "dataxfer.h"
#include "dialogue.h"
//#include "discfile.h"
#include "file.h"
#include "ihelp.h"
//#include "main.h"
#include "templates.h"


/* Hotlist Window */

#define HOTLIST_TOOLBAR_HEIGHT 60						/**< The height of the toolbar in OS units.				*/
#define HOTLIST_LINE_HEIGHT 56							/**< The height of a results line, in OS units.				*/
#define HOTLIST_WINDOW_MARGIN 4							/**< The margin around the edge of the window, in OS units.		*/
#define HOTLIST_LINE_OFFSET 4							/**< The offset from the base of a line to the base of the icon.	*/
#define HOTLIST_ICON_HEIGHT 52							/**< The height of an icon in the results window, in OS units.		*/

#define HOTLIST_MIN_LINES 10							/**< The minimum number of lines to show in the hotlist window.		*/

#define HOTLIST_ICON_FILE 0

/* Hotlist Add Window */

#define HOTLIST_ADD_ICON_NAME 1
#define HOTLIST_ADD_ICON_CANCEL 2
#define HOTLIST_ADD_ICON_ADD 3

/* Hotlist Menu */

#define HOTLIST_MENU_EDIT 0

/* Hotlist memory allocation. */

#define HOTLIST_NAME_LENGTH 48							/**< The maximum length of a hotlist entry name.				*/

#define HOTLIST_ALLOCATION 10							/**< The number of hotlist entries to be allocated in one go.			*/


/**
 * The structure to contain details of a hotlist entry.
 */

struct hotlist_block {
	char			name[HOTLIST_NAME_LENGTH];			/**< The name of the hotlist entry.						*/
	struct dialogue_block	*dialogue;					/**< The data associated with the hotlist entry.				*/
};

/* Global variables. */

static struct hotlist_block	*hotlist = NULL;				/**< The hotlist entries -- \TODO -- Fix allocation!				*/
static int			hotlist_allocation = 0;				/**< The number of entries for which hotlist memory is allocated.		*/
static int			hotlist_entries = 0;				/**< The number of entries in the hotlist.					*/

/* Hotlist Menu. */

static wimp_menu		*hotlist_menu = NULL;				/**< The hotlist menu handle.							*/

/* Hotlist Window. */

static wimp_window		*hotlist_window_def = NULL;			/**< The definition of the hotlist window.					*/
static wimp_w			hotlist_window = NULL;				/**< The hotlist window handle.							*/
static wimp_w			hotlist_window_pane = NULL;			/**< The hotlist window toolbar pane handle.					*/

/* Add/Edit Window. */

static wimp_w			hotlist_add_window = NULL;			/**< The add to hotlist window handle.						*/
static struct dialogue_block	*hotlist_add_dialogue_handle = NULL;		/**< The handle of the dialogue to be added to the hotlist, or NULL if none.	*/
static int			hotlist_add_entry = -1;				/**< The hotlist entry associated with the add window, or -1 for none.		*/

/* Function prototypes. */

static void	hotlist_redraw_handler(wimp_draw *redraw);
static void	hotlist_update_extent(void);
static void	hotlist_add_click_handler(wimp_pointer *pointer);
static osbool	hotlist_add_keypress_handler(wimp_key *key);
static void	hotlist_set_add_window(int entry);
static void	hotlist_redraw_add_window(void);
static osbool	hotlist_read_add_window(void);
static osbool	hotlist_extend(int allocation);


/* Line position calculations.
 *
 * NB: These can be called with lines < 0 to give lines off the top of the window!
 */

#define LINE_BASE(x) (-((x)+1) * HOTLIST_LINE_HEIGHT - HOTLIST_TOOLBAR_HEIGHT - HOTLIST_WINDOW_MARGIN)
#define LINE_Y0(x) (LINE_BASE(x) + HOTLIST_LINE_OFFSET)
#define LINE_Y1(x) (LINE_BASE(x) + HOTLIST_LINE_OFFSET + HOTLIST_ICON_HEIGHT)

/* Row calculations: taking a positive offset from the top of the window, return
 * the raw row number and the position within a row.
 */

#define ROW(y) (((-(y)) - HOTLIST_TOOLBAR_HEIGHT - HOTLIST_WINDOW_MARGIN) / HOTLIST_LINE_HEIGHT)
#define ROW_Y_POS(y) (((-(y)) - HOTLIST_TOOLBAR_HEIGHT - HOTLIST_WINDOW_MARGIN) % HOTLIST_LINE_HEIGHT)

/* Return true or false if a ROW_Y_POS() value is above or below the icon
 * area of the row.
 */

#define ROW_ABOVE(y) ((y) < (HOTLIST_LINE_HEIGHT - (HOTLIST_LINE_OFFSET + HOTLIST_ICON_HEIGHT)))
#define ROW_BELOW(y) ((y) > (HOTLIST_LINE_HEIGHT - HOTLIST_LINE_OFFSET))


/**
 * Initialise the iconbar icon and its associated menus and dialogues.
 */

void hotlist_initialise(void)
{
	//char*			date = BUILD_DATE;
	//wimp_icon_create	icon_bar;

	//iconbar_menu = templates_get_menu(TEMPLATES_MENU_ICONBAR);
	
	/* Allocate some memory for the initial hotlist entries. */
	
	hotlist = heap_alloc(sizeof(struct hotlist_block) * HOTLIST_ALLOCATION);
	if (hotlist != NULL)
		hotlist_allocation = HOTLIST_ALLOCATION;

	/* Initialise the hotlist window. */

	hotlist_window_def = templates_load_window("Hotlist");
	hotlist_window_def->extent.y1 = 0;
	hotlist_window_def->extent.y0 = -((HOTLIST_MIN_LINES * HOTLIST_LINE_HEIGHT) + HOTLIST_TOOLBAR_HEIGHT);
	hotlist_window_def->icon_count = 0;
	hotlist_window = wimp_create_window(hotlist_window_def);
	ihelp_add_window(hotlist_window, "Hotlist", NULL);
	event_add_window_redraw_event(hotlist_window, hotlist_redraw_handler);
	
	/* Initialise the hotlist pane window. */
	
	hotlist_window_pane = templates_create_window("HotlistPane");
	ihelp_add_window(hotlist_window_pane, "HotlistPane", NULL);

	/* Initialise the add/edit window. */

	hotlist_add_window = templates_create_window("HotlistAdd");
	//templates_link_menu_dialogue("ProgInfo", iconbar_info_window);
	ihelp_add_window(hotlist_add_window, "HotlistAdd", NULL);
	event_add_window_mouse_event(hotlist_add_window, hotlist_add_click_handler);
	event_add_window_key_event(hotlist_add_window, hotlist_add_keypress_handler);
}


/**
 * Callback to handle redraw events on a hotlist window.
 *
 * \param  *redraw		The Wimp redraw event block.
 */

static void hotlist_redraw_handler(wimp_draw *redraw)
{
	int			ox, oy, top, bottom, y, i;
	osbool			more;
	wimp_icon		*icon;
	char			validation[255];
	char			*truncation, *size, *attributes, *date;
	size_t			truncation_len;


	icon = hotlist_window_def->icons;

	/* Set up the validation string buffer for text+sprite icons. */

	//*validation = 'S';
	//icon[HOTLIST_ICON_FILE].data.indirected_text.validation = validation;

	/* Set up the truncation line. */

	//if (truncation != NULL)
	//	strcpy(truncation, "...");

	/* Redraw the window. */

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		icon[HOTLIST_ICON_FILE].extent.x1 = 1000 - HOTLIST_WINDOW_MARGIN; // \TODO -- 1000 ought to be window width.

		top = (oy - redraw->clip.y1 - HOTLIST_TOOLBAR_HEIGHT) / HOTLIST_LINE_HEIGHT;
		if (top < 0)
			top = 0;

                bottom = ((HOTLIST_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0 - HOTLIST_TOOLBAR_HEIGHT) / HOTLIST_LINE_HEIGHT;
		if (bottom > hotlist_entries)
			bottom = hotlist_entries;

		for (y = top; y < bottom; y++) {
			//i = handle->redraw[y].index;

//			switch (handle->redraw[i].type) {
//			case RESULTS_LINE_FILENAME:
				icon[HOTLIST_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[HOTLIST_ICON_FILE].extent.y1 = LINE_Y1(y);
				
				icon[HOTLIST_ICON_FILE].data.indirected_text.text = hotlist[y].name;

				/*if (handle->redraw[i].flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SELECTED;*/

				wimp_plot_icon(&(icon[HOTLIST_ICON_FILE]));
			//	break;

//			default:
//				break;
//			}
		}

		more = wimp_get_rectangle(redraw);
	}
}


/**
 * Update the window extent to hold all of the hotlist entries.
 */

static void hotlist_update_extent(void)
{
	wimp_window_info	info;
	unsigned		lines;
	int			new_y_extent;
	osbool			reopen = TRUE;

	lines = (hotlist_entries > HOTLIST_MIN_LINES) ? hotlist_entries : HOTLIST_MIN_LINES;
	new_y_extent = -((lines * HOTLIST_LINE_HEIGHT) + HOTLIST_TOOLBAR_HEIGHT);

	info.w = hotlist_window;
	if (xwimp_get_window_info_header_only(&info) != NULL)
		return;

	if (new_y_extent > (info.visible.y0 - info.visible.y1))
		info.visible.y0 = info.visible.y1 + new_y_extent;
	else if ((new_y_extent > (info.visible.y0 - info.visible.y1 + info.yscroll)))
		info.yscroll = new_y_extent - (info.visible.y0 - info.visible.y1);
	else
		reopen = FALSE;

	if (reopen && (xwimp_open_window((wimp_open *) &info) != NULL))
		return;

	info.extent.y0 = info.extent.y1 + new_y_extent;

	xwimp_set_extent(hotlist_window, &(info.extent));
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
	
	hotlist_set_add_window(-1);

	wimp_get_pointer_info(&pointer);
	windows_open_centred_at_pointer(hotlist_add_window, &pointer);
	icons_put_caret_at_end(hotlist_add_window, HOTLIST_ADD_ICON_NAME);
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
			if (!hotlist_read_add_window())
				break;

			if (pointer->buttons == wimp_CLICK_SELECT)
				wimp_close_window(hotlist_add_window);
		}
		break;

	case HOTLIST_ADD_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			wimp_close_window(hotlist_add_window);
			dialogue_destroy(hotlist_add_dialogue_handle, DIALOGUE_CLIENT_HOTLIST);
			hotlist_add_dialogue_handle = NULL;
			hotlist_add_entry = -1;
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			hotlist_set_add_window(hotlist_add_entry);
			hotlist_redraw_add_window();
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
		if (!hotlist_read_add_window())
			break;
		wimp_close_window(hotlist_add_window);
		break;

	case wimp_KEY_ESCAPE:
		wimp_close_window(hotlist_add_window);
		dialogue_destroy(hotlist_add_dialogue_handle, DIALOGUE_CLIENT_HOTLIST);
		hotlist_add_dialogue_handle = NULL;
		hotlist_add_entry = -1;
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Set the contents of the hotlist add window to either the given hotlist entry
 * or the default settings for a new entry.
 *
 * \param entry			The hotlist entry to use, or -1 for none.
 */

static void hotlist_set_add_window(int entry)
{
	if (entry < -1 || entry >= hotlist_entries)
		return;

	hotlist_add_entry = entry;
	
	if (entry == -1)
		icons_msgs_lookup(hotlist_add_window, HOTLIST_ADD_ICON_NAME, "HotlistNew");
	else
		icons_printf(hotlist_add_window, HOTLIST_ADD_ICON_NAME, "%s", hotlist[entry].name);
}


/**
 * Refresh the hotlist add window to reflect any changes in state.
 */

static void hotlist_redraw_add_window(void)
{
	wimp_set_icon_state(hotlist_add_window, HOTLIST_ADD_ICON_NAME, 0, 0);
	icons_replace_caret_in_window(hotlist_add_window);
}


/**
 * Process the contents of the hotlist add window, either updating an entry or
 * creating a new one.
 *
 * \return			TRUE if successful; FALSE if errors occurred.
 */

static osbool hotlist_read_add_window(void)
{
	char	*new_name;
	
	new_name = icons_get_indirected_text_addr(hotlist_add_window, HOTLIST_ADD_ICON_NAME);
	string_ctrl_zero_terminate(new_name);
	
	if (new_name == NULL || strlen(new_name) == 0) {
		error_msgs_report_info("HotlistNoName");
		return FALSE;
	}
	
	if (hotlist_entries >= hotlist_allocation)
		hotlist_extend(hotlist_allocation + HOTLIST_ALLOCATION);
	
	if (hotlist_entries >= hotlist_allocation) {
		error_msgs_report_error("HotlistNoMem");
		return FALSE;
	}

	strncpy(hotlist[hotlist_entries].name, new_name, HOTLIST_NAME_LENGTH);
	hotlist[hotlist_entries].dialogue = hotlist_add_dialogue_handle;
	hotlist_entries++;
	hotlist_update_extent();

	return TRUE;
}


/**
 * Build a hotlist menu and return a pointer to it.
 *
 * \return			A pointer to the menu block.
 */

wimp_menu *hotlist_build_menu(void)
{
	int			line, width = 0;


	/* Allocate space for the Wimp menu block. */

	if (hotlist_menu != NULL)
		heap_free(hotlist_menu);

	hotlist_menu = heap_alloc(28 + 24 * (hotlist_entries + 1));

	if (hotlist_menu == NULL)
		return NULL;

	/* Enter the default Edit entry at the top of the menu. */

	line = 0;

	hotlist_menu->entries[line].menu_flags = NONE;

	hotlist_menu->entries[line].sub_menu = (wimp_menu *) -1;
	hotlist_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED |
			wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
			wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

	msgs_lookup("HotlistEdit", hotlist_menu->entries[line].data.text, 12);

	if (strlen(hotlist_menu->entries[line].data.text) > width)
		width = strlen(hotlist_menu->entries[line].data.text);

	/* Add any hotlist entries that have been defined. */

	for (line++; line <= hotlist_entries; line++) {
		if (strlen(hotlist[line - 1].name) > width)
			width = strlen(hotlist[line - 1].name);

		/* Set the menu and icon flags up. */

		hotlist_menu->entries[line].menu_flags = NONE;

		hotlist_menu->entries[line].sub_menu = (wimp_menu *) -1;
		hotlist_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
				wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
				wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

		/* Set the menu icon contents up. */

		hotlist_menu->entries[line].data.indirected_text.text = hotlist[line - 1].name;
		hotlist_menu->entries[line].data.indirected_text.validation = "";
		hotlist_menu->entries[line].data.indirected_text.size = HOTLIST_NAME_LENGTH;
	}

	/* Finish off the menu definition. */

	if (hotlist_entries >= 1)
		hotlist_menu->entries[0].menu_flags |= wimp_MENU_SEPARATE;

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
	wimp_pointer	pointer;

	if (selection < 0 || selection > hotlist_entries)
		return;
		
	debug_printf("Selected hotlist menu item %d", selection);
	
	if (selection == HOTLIST_MENU_EDIT) {
		windows_open(hotlist_window);
		windows_open_nested_as_toolbar(hotlist_window_pane, hotlist_window, HOTLIST_TOOLBAR_HEIGHT);
	} else if (selection > 0 && hotlist[selection - 1].dialogue != NULL) {
		if (xwimp_get_pointer_info(&pointer) == NULL)
			file_create_dialogue(&pointer, NULL, hotlist[selection - 1].dialogue);
	}
}


/**
 * Extend the memory allocaton for the hotlist to the given number of entries.
 *
 * \param allocation		The required number of entries in the hotlist.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool hotlist_extend(int allocation)
{
	struct hotlist_block	*new;

	if (hotlist == NULL)
		return FALSE;

	if (hotlist_allocation > allocation)
		return TRUE;

	new = heap_extend(hotlist, allocation * sizeof(struct hotlist_block));
	if (new == NULL)
		return FALSE;
	
	hotlist = new;
	hotlist_allocation = allocation;

	return TRUE;
}

