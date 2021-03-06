/* Copyright 2013-2017, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: hotlist.c
 *
 * Hotlist implementation.
 */

/* ANSI C header files */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Acorn C header files */

/* OSLib header files */

#include "oslib/hourglass.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

#include "sflib/config.h"
#include "sflib/dataxfer.h"
#include "sflib/debug.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/ihelp.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/windows.h"

/* Application header files */

#include "iconbar.h"

//#include "choices.h"
#include "dialogue.h"
#include "discfile.h"
#include "file.h"
//#include "main.h"


/* Hotlist Window */

#define HOTLIST_TOOLBAR_HEIGHT 60						/**< The height of the toolbar in OS units.				*/
#define HOTLIST_LINE_HEIGHT 56							/**< The height of a results line, in OS units.				*/
#define HOTLIST_WINDOW_MARGIN 4							/**< The margin around the edge of the window, in OS units.		*/
#define HOTLIST_LINE_OFFSET 4							/**< The offset from the base of a line to the base of the icon.	*/
#define HOTLIST_ICON_HEIGHT 52							/**< The height of an icon in the results window, in OS units.		*/

#define HOTLIST_MIN_LINES 10							/**< The minimum number of lines to show in the hotlist window.		*/

#define HOTLIST_AUTOSCROLL_BORDER 80						/**< The autoscroll border for the window, in OS units.			*/

#define HOTLIST_ICON_FILE 0

/* Hotlist Toolbar */

#define HOTLIST_TOOLBAR_ICON_SAVE 0
#define HOTLIST_TOOLBAR_ICON_SELECT 1
#define HOTLIST_TOOLBAR_ICON_RUN 2
#define HOTLIST_TOOLBAR_ICON_RENAME 3
#define HOTLIST_TOOLBAR_ICON_DELETE 4
#define HOTLIST_TOOLBAR_ICON_DEFAULT 5


/* Hotlist Window Menu */

#define HOTLIST_MENU_ITEM 0
#define HOTLIST_MENU_SELECT_ALL 1
#define HOTLIST_MENU_CLEAR_SELECTION 2
#define HOTLIST_MENU_SAVE_HOTLIST 3

#define HOTLIST_MENU_ITEM_SAVE 0
#define HOTLIST_MENU_ITEM_DEFAULT 1
#define HOTLIST_MENU_ITEM_RENAME 2
#define HOTLIST_MENU_ITEM_DELETE 3

/* Hotlist Add Window */

#define HOTLIST_ADD_ICON_NAME 1
#define HOTLIST_ADD_ICON_CANCEL 2
#define HOTLIST_ADD_ICON_ADD 3

/* Hotlist memory allocation. */

#define HOTLIST_NAME_LENGTH 48							/**< The maximum length of a hotlist entry name.				*/

#define HOTLIST_ALLOCATION 10							/**< The number of hotlist entries to be allocated in one go.			*/

/**
 * An invalid or non-existant Hotlist entry.
 */

#define HOTLIST_NULL_ENTRY ((int) -1)

/**
 * The structure to contain details of a hotlist entry.
 */

enum hotlist_block_flags {
	HOTLIST_FLAG_NONE = 0x00,
	HOTLIST_FLAG_SELECTABLE = 0x01,						/**< Indicates that the hotlist line can be selected.				*/
	HOTLIST_FLAG_SELECTED = 0x02,						/**< Indicates that the hotlist line is selected.				*/
	HOTLIST_FLAG_DEFAULT = 0x04						/**< Indicates that the hotlist line is the defult entry.			*/
};

struct hotlist_block {
	char				name[HOTLIST_NAME_LENGTH];		/**< The name of the hotlist entry.						*/
	struct dialogue_block		*dialogue;				/**< The data associated with the hotlist entry.				*/
	enum hotlist_block_flags	flags;					/**< The flags for the hotlist entry.						*/
};

/* Global variables. */

static struct hotlist_block	*hotlist = NULL;				/**< The hotlist entries.							*/
static int			hotlist_allocation = 0;				/**< The number of entries for which hotlist memory is allocated.		*/
static int			hotlist_entries = 0;				/**< The number of entries in the hotlist.					*/

/* Hotlist Menu. */

static wimp_menu		*hotlist_menu = NULL;				/**< The hotlist menu handle.							*/

/* Hotlist Window. */

static wimp_window		*hotlist_window_def = NULL;			/**< The definition of the hotlist window.					*/
static wimp_w			hotlist_window = NULL;				/**< The hotlist window handle.							*/
static wimp_w			hotlist_window_pane = NULL;			/**< The hotlist window toolbar pane handle.					*/
static int			hotlist_window_width = 0;			/**< The X extent of the hotlist window.					*/
static int			hotlist_selection_count = 0;			/**< The number of items selected in the hotlist window.			*/
static int			hotlist_selection_row = -1;			/**< The selected row, if there is only one.					*/
static osbool			hotlist_selection_from_menu = FALSE;		/**< TRUE if the hotlist selection came via the menu opening; else FALSE.	*/
static wimp_menu		*hotlist_window_menu = NULL;			/**< The hotlist window menu handle.						*/
static wimp_menu		*hotlist_window_menu_item = NULL;		/**< THe hotlist window menu's item submenu handle.				*/
static struct saveas_block	*hotlist_saveas_hotlist = NULL;			/**< The Save Hotlist savebox data handle.					*/
static struct saveas_block	*hotlist_saveas_search = NULL;			/**< The Save Search savebox data handle.					*/

static unsigned			hotlist_select_drag_row = -1		;	/**< The row in which the selection drag started.				*/
static unsigned			hotlist_select_drag_pos = 0;			/**< The position within the row where the selection drag started.		*/
static osbool			hotlist_select_drag_adjust = FALSE;		/**< TRUE if the selection drag is with Adjust; FALSE for Select.		*/

/* Add/Edit Window. */

static wimp_w			hotlist_add_window = NULL;			/**< The add to hotlist window handle.						*/
static struct dialogue_block	*hotlist_add_dialogue_handle = NULL;		/**< The handle of the dialogue to be added to the hotlist, or NULL if none.	*/
static int			hotlist_add_entry = -1;				/**< The hotlist entry associated with the add window, or -1 for none.		*/

/* Function prototypes. */

static void	hotlist_redraw_handler(wimp_draw *redraw);
static void	hotlist_click_handler(wimp_pointer *pointer);
static void	hotlist_toolbar_click_handler(wimp_pointer *pointer);
static void	hotlist_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void	hotlist_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void	hotlist_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void	hotlist_menu_close(wimp_w w, wimp_menu *menu);
static void	hotlist_update_extent(void);
static void	hotlist_drag_select(unsigned row, wimp_pointer *pointer, wimp_window_state *state, osbool ctrl_pressed);
static void	hotlist_xfer_drag_end_handler(wimp_pointer *pointer, void *data);
static void	hotlist_select_drag_end_handler(wimp_dragged *drag, void *data);
static void	hotlist_select_click_select(int row);
static void	hotlist_select_click_adjust(int row);
static void	hotlist_select_all(void);
static void	hotlist_select_none(void);
static void	hotlist_update_toolbar(void);
static int	hotlist_calculate_window_click_row(os_coord *pos, wimp_window_state *state);
static osbool	hotlist_load_locate_file(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data);
static void	hotlist_delete_selection(void);
static void	hotlist_rename_entry(int entry);
static void	hotlist_add_click_handler(wimp_pointer *pointer);
static osbool	hotlist_add_keypress_handler(wimp_key *key);
static void	hotlist_set_add_window(int entry);
static void	hotlist_redraw_add_window(void);
static osbool	hotlist_read_add_window(void);
static osbool	hotlist_add_new_entry(char *name, enum hotlist_block_flags flags, struct dialogue_block *dialogue);
static void	hotlist_delete_entry(int entry);
static int	hotlist_move_entry(int entry, int insert_before);
static osbool	hotlist_save_hotlist(char *filename, osbool selection, void *data);
static osbool	hotlist_saveas_save_search(char *filename, osbool selection, void *data);
static osbool	hotlist_save_search(char *filename, void *data);
static osbool	hotlist_save_choices(void);
static osbool	hotlist_save_file(char *filename, osbool selection);
static void	hotlist_save_file_callback(struct discfile_block *out, enum dialogue_file_action action, void *data);
static osbool	hotlist_load_choices(void);
static osbool	hotlist_load_file(struct discfile_block *load);
static osbool	hotlist_load_file_callback(struct discfile_block *load, enum dialogue_file_action action, void *data);
static osbool	hotlist_extend(int allocation);
static void	hotlist_open_entry(int entry);
static void	hotlist_set_default_dialogue(int entry);
static int	hotlist_find_default_entry(void);


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
 * Initialise the hotlist system and its associated menus and dialogues.
 *
 * \param *sprites		Pointer to the sprite area to be used.
 */

void hotlist_initialise(osspriteop_area *sprites)
{
	static wimp_window	*pane_def;
	//char*			date = BUILD_DATE;
	//wimp_icon_create	icon_bar;

	hotlist_window_menu = templates_get_menu("HotlistWindowMenu");
	ihelp_add_menu(hotlist_window_menu, "HotlistMenu");
	hotlist_window_menu_item = templates_get_menu("HotlistItemMenu");

	hotlist_saveas_search = saveas_create_dialogue(FALSE, "file_1a1", hotlist_saveas_save_search);
	hotlist_saveas_hotlist = saveas_create_dialogue(TRUE, "file_1a1", hotlist_save_hotlist);


	/* Allocate some memory for the initial hotlist entries. */

	hotlist = heap_alloc(sizeof(struct hotlist_block) * HOTLIST_ALLOCATION);
	if (hotlist != NULL)
		hotlist_allocation = HOTLIST_ALLOCATION;

	/* Initialise the hotlist window. */

	hotlist_window_def = templates_load_window("Hotlist");
	hotlist_window_def->sprite_area = sprites;
	hotlist_window_def->extent.y1 = 0;
	hotlist_window_def->extent.y0 = -((HOTLIST_MIN_LINES * HOTLIST_LINE_HEIGHT) + HOTLIST_TOOLBAR_HEIGHT);
	hotlist_window_def->icon_count = 0;
	hotlist_window = wimp_create_window(hotlist_window_def);
	ihelp_add_window(hotlist_window, "Hotlist", NULL);
	event_add_window_redraw_event(hotlist_window, hotlist_redraw_handler);
	event_add_window_mouse_event(hotlist_window, hotlist_click_handler);
	hotlist_window_width = hotlist_window_def->extent.x1 - hotlist_window_def->extent.x0;

	event_add_window_menu(hotlist_window, hotlist_window_menu);
	event_add_window_menu_prepare(hotlist_window, hotlist_menu_prepare);
	event_add_window_menu_warning(hotlist_window, hotlist_menu_warning);
	event_add_window_menu_selection(hotlist_window, hotlist_menu_selection);
	event_add_window_menu_close(hotlist_window, hotlist_menu_close);

	dataxfer_set_drop_target(dataxfer_TYPE_LOCATE, hotlist_window, -1, NULL, hotlist_load_locate_file, NULL);

	/* Initialise the hotlist pane window. */

	pane_def = templates_load_window("HotlistPane");
	pane_def->sprite_area = sprites;
	hotlist_window_pane = wimp_create_window(pane_def);
	free(pane_def);
	ihelp_add_window(hotlist_window_pane, "HotlistPane", NULL);
	event_add_window_mouse_event(hotlist_window_pane, hotlist_toolbar_click_handler);

	event_add_window_menu(hotlist_window_pane, hotlist_window_menu);
	event_add_window_menu_prepare(hotlist_window_pane, hotlist_menu_prepare);
	event_add_window_menu_warning(hotlist_window_pane, hotlist_menu_warning);
	event_add_window_menu_selection(hotlist_window_pane, hotlist_menu_selection);
	event_add_window_menu_close(hotlist_window_pane, hotlist_menu_close);

	dataxfer_set_drop_target(dataxfer_TYPE_LOCATE, hotlist_window_pane, -1, NULL, hotlist_load_locate_file, NULL);

	/* Initialise the add/edit window. */

	hotlist_add_window = templates_create_window("HotlistAdd");
	ihelp_add_window(hotlist_add_window, "HotlistAdd", NULL);
	event_add_window_mouse_event(hotlist_add_window, hotlist_add_click_handler);
	event_add_window_key_event(hotlist_add_window, hotlist_add_keypress_handler);

	/* Load the hotlist from disc. */

	hotlist_load_choices();
}


/**
 * Terminate the hotlist system.
 */

void hotlist_terminate(void)
{
	hotlist_save_choices();
}


/**
 * Open the hotlist window.
 *
 * \param *pointer		The location at which to open the window.
 */

void hotlist_open(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	hotlist_update_toolbar();
	windows_open_centred_at_pointer(hotlist_window, pointer);
	windows_open_nested_as_toolbar(hotlist_window_pane, hotlist_window, HOTLIST_TOOLBAR_HEIGHT, FALSE);
}


/**
 * Callback to handle redraw events on a hotlist window.
 *
 * \param  *redraw		The Wimp redraw event block.
 */

static void hotlist_redraw_handler(wimp_draw *redraw)
{
	int			oy, top, bottom, y;
	osbool			more;
	wimp_icon		*icon;


	icon = hotlist_window_def->icons;

	/* Redraw the window. */

	more = wimp_redraw_window(redraw);

	// ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		icon[HOTLIST_ICON_FILE].extent.x0 = HOTLIST_WINDOW_MARGIN;
		icon[HOTLIST_ICON_FILE].extent.x1 = hotlist_window_width - HOTLIST_WINDOW_MARGIN;

		top = (oy - redraw->clip.y1 - HOTLIST_TOOLBAR_HEIGHT) / HOTLIST_LINE_HEIGHT;
		if (top < 0)
			top = 0;

                bottom = ((HOTLIST_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0 - HOTLIST_TOOLBAR_HEIGHT) / HOTLIST_LINE_HEIGHT;
		if (bottom > hotlist_entries)
			bottom = hotlist_entries;

		for (y = top; y < bottom; y++) {
			icon[HOTLIST_ICON_FILE].extent.y0 = LINE_Y0(y);
			icon[HOTLIST_ICON_FILE].extent.y1 = LINE_Y1(y);

			icon[HOTLIST_ICON_FILE].data.indirected_text.text = hotlist[y].name;

			if (hotlist[y].flags & HOTLIST_FLAG_DEFAULT)
				icon[HOTLIST_ICON_FILE].data.indirected_text.validation = "Sdflthot";
			else
				icon[HOTLIST_ICON_FILE].data.indirected_text.validation = "Ssmall_1a1";

			if (hotlist[y].flags & HOTLIST_FLAG_SELECTED)
				icon[HOTLIST_ICON_FILE].flags |= wimp_ICON_SELECTED;
			else
				icon[HOTLIST_ICON_FILE].flags &= ~wimp_ICON_SELECTED;

			wimp_plot_icon(&(icon[HOTLIST_ICON_FILE]));
		}

		more = wimp_get_rectangle(redraw);
	}
}


/**
 * Process mouse clicks in the hotlist window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void hotlist_click_handler(wimp_pointer *pointer)
{
	wimp_window_state	state;
	int			row;
	osbool			ctrl_pressed;

	if (pointer == NULL)
		return;

	ctrl_pressed = ((osbyte1(osbyte_IN_KEY, 0xf0, 0xff) == 0xff) || (osbyte1(osbyte_IN_KEY, 0xfb, 0xff) == 0xff)) ? TRUE : FALSE;

	state.w = pointer->w;
	if (xwimp_get_window_state(&state) != NULL)
		return;

	row = hotlist_calculate_window_click_row(&(pointer->pos), &state);

	switch(pointer->buttons) {
	case wimp_SINGLE_SELECT:
		if (!ctrl_pressed)
			hotlist_select_click_select(row);
		break;

	case wimp_SINGLE_ADJUST:
		if (!ctrl_pressed)
			hotlist_select_click_adjust(row);
		break;

	case wimp_DOUBLE_SELECT:
		if (!ctrl_pressed) {
			hotlist_select_none();
			hotlist_open_entry(row);
		}
		break;

	case wimp_DOUBLE_ADJUST:
		if (!ctrl_pressed) {
			hotlist_select_click_adjust(row);
			/* Start the search directly... */
		}
		break;

	case wimp_DRAG_SELECT:
	case wimp_DRAG_ADJUST:
		hotlist_drag_select(row, pointer, &state, ctrl_pressed);
		break;
	}
}


/**
 * Process mouse clicks in the hotlist toolbar.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void hotlist_toolbar_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch(pointer->i) {
	case HOTLIST_TOOLBAR_ICON_SAVE:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			saveas_initialise_dialogue(hotlist_saveas_hotlist, NULL, "HotlistName", "SelectName", hotlist_selection_count > 0, hotlist_selection_count > 0, NULL);
			saveas_prepare_dialogue(hotlist_saveas_hotlist);
			saveas_open_dialogue(hotlist_saveas_hotlist, pointer);
		} else if (pointer->buttons == wimp_CLICK_ADJUST && hotlist_selection_count == 1) {
			saveas_initialise_dialogue(hotlist_saveas_search, NULL, "SrchName", "SelectName", FALSE, FALSE, NULL);
			saveas_prepare_dialogue(hotlist_saveas_search);
			saveas_open_dialogue(hotlist_saveas_search, pointer);
		}
		break;

	case HOTLIST_TOOLBAR_ICON_SELECT:
		if (pointer->buttons == wimp_CLICK_SELECT)
			hotlist_select_all();
		else if (pointer->buttons == wimp_CLICK_ADJUST)
			hotlist_select_none();
		break;

	case HOTLIST_TOOLBAR_ICON_RUN:
		if (pointer->buttons == wimp_CLICK_SELECT && hotlist_selection_count == 1)
			hotlist_open_entry(hotlist_selection_row);
		break;

	case HOTLIST_TOOLBAR_ICON_RENAME:
		if (pointer->buttons == wimp_CLICK_SELECT && hotlist_selection_count == 1)
			hotlist_rename_entry(hotlist_selection_row);
		break;

	case HOTLIST_TOOLBAR_ICON_DELETE:
		if (pointer->buttons == wimp_CLICK_SELECT && hotlist_selection_count >= 1)
			hotlist_delete_selection();
		break;

	case HOTLIST_TOOLBAR_ICON_DEFAULT:
		if (pointer->buttons == wimp_CLICK_SELECT && hotlist_selection_count == 1)
			hotlist_set_default_dialogue((hotlist_selection_row != hotlist_find_default_entry()) ? hotlist_selection_row : HOTLIST_NULL_ENTRY);
		break;
	}
}


/**
 * Prepare the hotlist menu for (re)-opening.
 *
 * \param  w			The handle of the menu's parent window.
 * \param  *menu		Pointer to the menu being opened.
 * \param  *pointer		Pointer to the Wimp Pointer event block.
 */

static void hotlist_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	wimp_window_state	state;
	unsigned		row;

	if (menu != hotlist_window_menu)
		return;

	if (pointer != NULL) {
		state.w = pointer->w;
		if (xwimp_get_window_state(&state) != NULL)
			return;

		row = hotlist_calculate_window_click_row(&(pointer->pos), &state);
		if (hotlist_selection_count == 0) {
			hotlist_select_click_select(row);
			hotlist_selection_from_menu = TRUE;
		} else {
			hotlist_selection_from_menu = FALSE;
		}

		saveas_initialise_dialogue(hotlist_saveas_search, NULL, "SrchName", "SelectName", FALSE, FALSE, NULL);
		saveas_initialise_dialogue(hotlist_saveas_hotlist, NULL, "HotlistName", "SelectName", hotlist_selection_count > 0, hotlist_selection_count > 0, NULL);
	}

	menus_shade_entry(hotlist_window_menu, HOTLIST_MENU_ITEM, hotlist_selection_count == 0);
	menus_shade_entry(hotlist_window_menu, HOTLIST_MENU_CLEAR_SELECTION, hotlist_selection_count == 0);

	menus_shade_entry(hotlist_window_menu_item, HOTLIST_MENU_ITEM_SAVE, hotlist_selection_count != 1);
	menus_shade_entry(hotlist_window_menu_item, HOTLIST_MENU_ITEM_DEFAULT, hotlist_selection_count != 1);
	menus_shade_entry(hotlist_window_menu_item, HOTLIST_MENU_ITEM_RENAME, hotlist_selection_count != 1);
	menus_shade_entry(hotlist_window_menu_item, HOTLIST_MENU_ITEM_DELETE, hotlist_selection_count == 0);

	menus_tick_entry(hotlist_window_menu_item, HOTLIST_MENU_ITEM_DEFAULT,
			(hotlist_selection_count == 1 && hotlist_selection_row == hotlist_find_default_entry()));
}


/**
 * Process submenu warning events in the hotlist menu.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void hotlist_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	if (menu != hotlist_window_menu)
		return;

	switch (warning->selection.items[0]) {
	case HOTLIST_MENU_ITEM:
		switch (warning->selection.items[1]) {
		case HOTLIST_MENU_ITEM_SAVE:
			saveas_prepare_dialogue(hotlist_saveas_search);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		}
		break;

	case HOTLIST_MENU_SAVE_HOTLIST:
		saveas_prepare_dialogue(hotlist_saveas_hotlist);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Handle selections from the hotlist menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param  *selection		Pointer to the Wimp menu selction block.
 */

static void hotlist_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	wimp_pointer	pointer;


	if (menu != hotlist_window_menu)
		return;

	wimp_get_pointer_info(&pointer);

	switch (selection->items[0]) {
	case HOTLIST_MENU_ITEM:
		switch (selection->items[1]) {
		case HOTLIST_MENU_ITEM_SAVE:
			break;

		case HOTLIST_MENU_ITEM_DEFAULT:
			if (hotlist_selection_count != 1)
				break;

			hotlist_set_default_dialogue((hotlist_selection_row != hotlist_find_default_entry()) ? hotlist_selection_row : HOTLIST_NULL_ENTRY);
			break;

		case HOTLIST_MENU_ITEM_RENAME:
			if (hotlist_selection_count == 1)
				hotlist_rename_entry(hotlist_selection_row);
			break;

		case HOTLIST_MENU_ITEM_DELETE:
			hotlist_delete_selection();
			break;
		}

		break;

	case HOTLIST_MENU_SELECT_ALL:
		hotlist_select_all();
		hotlist_selection_from_menu = FALSE;
		break;

	case HOTLIST_MENU_CLEAR_SELECTION:
		hotlist_select_none();
		hotlist_selection_from_menu = FALSE;
		break;

	case HOTLIST_MENU_SAVE_HOTLIST:
		hotlist_save_choices();
		break;
	}
}


/**
 * Handle the closure of a menu.
 *
 * \param  w			The handle of the menu's parent window.
 * \param  *menu		Pointer to the menu being closee.
 */

static void hotlist_menu_close(wimp_w w, wimp_menu *menu)
{
	if (menu != hotlist_window_menu || hotlist_selection_from_menu == FALSE)
		return;

	hotlist_select_none();
	hotlist_selection_from_menu = FALSE;
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
 * Process drags in a hotlist window. Depending on whether the drag began on a
 * selectable row or not, the drag will either start an object drag or start
 * a selection dragbox.
 *
 * \param row			The row under the click, or RESULTS_ROW_NONE.
 * \param *pointer		Pointer to the current mouse info.
 * \param *state		Pointer to the current window state.
 * \param ctrl_pressed		TRUE if a Ctrl key is down; else FALSE.
 */

static void hotlist_drag_select(unsigned row, wimp_pointer *pointer, wimp_window_state *state, osbool ctrl_pressed)
{
	int			y;
	os_box			extent;
	wimp_drag		drag;
	wimp_auto_scroll_info	scroll;
	char			*sprite = NULL;

	if (pointer == NULL || state == NULL)
		return;

	// x = pointer->pos.x - state->visible.x0 + state->xscroll;
	y = pointer->pos.y - state->visible.y1 + state->yscroll;

	if ((row != -1) && (row < hotlist_entries) && (pointer->buttons == wimp_DRAG_SELECT) &&
			(hotlist[row].flags & HOTLIST_FLAG_SELECTABLE) && !ctrl_pressed) {
		extent.x0 = state->xscroll + HOTLIST_WINDOW_MARGIN;
		extent.x1 = state->xscroll + (state->visible.x1 - state->visible.x0) - HOTLIST_WINDOW_MARGIN;
		extent.y0 = LINE_Y0(row);
		extent.y1 = LINE_Y1(row);

		if (hotlist_selection_count == 1 && hotlist_selection_row == row)
			sprite = "file_1a1";
		else
			sprite = "package";

		dataxfer_work_area_drag(hotlist_window, pointer, &extent, sprite, hotlist_xfer_drag_end_handler, NULL);
	} else {
		hotlist_select_drag_row = ROW(y);
		hotlist_select_drag_pos = ROW_Y_POS(y);
		hotlist_select_drag_adjust = (pointer->buttons == wimp_DRAG_ADJUST) ? TRUE : FALSE;

		drag.w = hotlist_window;
		drag.type = wimp_DRAG_USER_RUBBER;

		drag.initial.x0 = pointer->pos.x;
		drag.initial.y0 = pointer->pos.y;
		drag.initial.x1 = pointer->pos.x;
		drag.initial.y1 = pointer->pos.y;

		drag.bbox.x0 = state->visible.x0;
		drag.bbox.y0 = state->visible.y0 ;
		drag.bbox.x1 = state->visible.x1;
		drag.bbox.y1 = state->visible.y1 - HOTLIST_TOOLBAR_HEIGHT;

		scroll.w = hotlist_window;

		scroll.pause_zone_sizes.x0 = HOTLIST_AUTOSCROLL_BORDER;
		scroll.pause_zone_sizes.y0 = HOTLIST_AUTOSCROLL_BORDER;
		scroll.pause_zone_sizes.x1 = HOTLIST_AUTOSCROLL_BORDER;
		scroll.pause_zone_sizes.y1 = HOTLIST_AUTOSCROLL_BORDER + HOTLIST_TOOLBAR_HEIGHT;

		scroll.pause_duration = 0;
		scroll.state_change = wimp_AUTO_SCROLL_DEFAULT_HANDLER;

		wimp_drag_box_with_flags(&drag, wimp_DRAG_BOX_KEEP_IN_LINE | wimp_DRAG_BOX_CLIP);
		wimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_VERTICAL, &scroll);

		event_set_drag_handler(hotlist_select_drag_end_handler, NULL, NULL);
	}
}


/**
 * Process the termination of transfer drags from a hotlist window. Drags within
 * the window move entries in the list, while drags outside issue a
 * Message_DataSave to try and start a save action.
 *
 * \param *pointer		The pointer location at the end of the drag.
 * \param *data			The results_window data for the drag.
 */

static void hotlist_xfer_drag_end_handler(wimp_pointer *pointer, void *data)
{
	int			y, row, row_y_pos, i;
	wimp_window_state	state;
	os_error		*error;
	char			leafname[HOTLIST_NAME_LENGTH], *from, *to;

	if (pointer->w == hotlist_window) {
		/* Move items within the window. */

		state.w = hotlist_window;
		error = xwimp_get_window_state(&state);
		if (error != NULL)
			return;

		/* Calculate the row stats for the drag end. */

		y = pointer->pos.y - state.visible.y1 + state.yscroll;

		row = ROW(y);
		row_y_pos = ROW_Y_POS(y);

		if (row_y_pos > (HOTLIST_LINE_HEIGHT / 2))
			row++;

		if (row < 0)
			row = 0;

		if (row > hotlist_entries)
			row = hotlist_entries;

		/* Repeatedly move and deselect selections until there are none left. */

		do {
			for (i = 0; i < hotlist_entries; i++) {
				if (hotlist[i].flags & HOTLIST_FLAG_SELECTED) {
					hotlist[i].flags &= ~HOTLIST_FLAG_SELECTED;
					row = hotlist_move_entry(i, row);
					break;
				}
			}
		} while (i < hotlist_entries);

		windows_redraw(hotlist_window);

		hotlist_update_toolbar();
	} else {
		/* Issue Message_DataSave for selected items. */

		for (row = 0; row < hotlist_entries; row++) {
			if (!(hotlist[row].flags & HOTLIST_FLAG_SELECTED))
				continue;

			from = hotlist[row].name;
			to = leafname;

			while (*from != '\0') {
				if (isgraph(*from) && (strchr(".:*#$&@^%\\", *from) == NULL))
					*(to++) = *from;

				from++;

			}

			*from = '\0';

			dataxfer_start_save(pointer, leafname, 0, dataxfer_TYPE_LOCATE, 0, hotlist_save_search, hotlist[row].dialogue);
		}
	}
}


/**
 * Process the termination of selection drags from a hotlist window.
 *
 * \param  *drag		The Wimp poll block from termination.
 * \param  *data		NULL (unused).
 */

static void hotlist_select_drag_end_handler(wimp_dragged *drag, void *data)
{
	int			y, row_y_pos;
	int			row, start, end;
	wimp_pointer		pointer;
	wimp_window_state	state;
	os_error		*error;


	/* Terminate the scroll process. */

	error = xwimp_auto_scroll(NONE, NULL, NULL);
	if (error != NULL)
		return;

	/* Get details of the position of the drag end. */

	error = xwimp_get_pointer_info(&pointer);
	if (error != NULL)
		return;

	state.w = hotlist_window;
	error = xwimp_get_window_state(&state);
	if (error != NULL)
		return;

	/* Calculate the row stats for the drag end. */

	y = pointer.pos.y - state.visible.y1 + state.yscroll;

	row = ROW(y);
	row_y_pos = ROW_Y_POS(y);

	/* Work out the range of rows to be affected. */

	if (row > hotlist_select_drag_row) {
		start = hotlist_select_drag_row;
		if (ROW_BELOW(hotlist_select_drag_pos))
			start++;

		end = row;
		if (ROW_ABOVE(row_y_pos))
			end --;
	} else if (row < hotlist_select_drag_row) {
		start = row;
		if (ROW_BELOW(row_y_pos))
			start++;

		end = hotlist_select_drag_row;
		if (ROW_ABOVE(hotlist_select_drag_pos))
			end --;
	} else if (!((ROW_ABOVE(row_y_pos) && ROW_ABOVE(hotlist_select_drag_pos)) || (ROW_BELOW(row_y_pos) && ROW_BELOW(hotlist_select_drag_pos)))) {
		start = row;
		end = row;
	} else {
		start = -1;
		end = -1;
	}

	/* Get out if the range isn't valid. The (end < start) state implies
	 * adjacent ABOVE/BELOW bands, which means no icons have been included.
	 */

	if (start == -1 || end == -1 || end < start)
		return;

	if (!hotlist_select_drag_adjust)
		hotlist_select_none();

	for (row = start; row <= end && row < hotlist_entries; row++) {
		if (!(hotlist[row].flags & HOTLIST_FLAG_SELECTABLE))
			continue;

		if (hotlist[row].flags & HOTLIST_FLAG_SELECTED) {
			hotlist[row].flags &= ~HOTLIST_FLAG_SELECTED;
			hotlist_selection_count--;
		} else {
			hotlist[row].flags |= HOTLIST_FLAG_SELECTED;
			hotlist_selection_count++;
		}

		wimp_force_redraw(state.w, state.xscroll, LINE_BASE(row),
			state.xscroll + (state.visible.x1 - state.visible.x0), LINE_Y1(row));
	}

	if (hotlist_selection_count == 1) {
		for (row = 0; row < hotlist_entries; row++) {
			if (hotlist[row].flags & HOTLIST_FLAG_SELECTED) {
				hotlist_selection_row = row;
				break;
			}
		}
	}

	hotlist_update_toolbar();
}


/**
 * Update the current selection based on a select click over a row of the
 * hotlist.
 *
 * \param row			The row under the click, or -1.
 */

static void hotlist_select_click_select(int row)
{
	wimp_window_state	window;

	/* If the click is on a selection, nothing changes. */

	if ((row != -1) && (row < hotlist_entries) && (hotlist[row].flags & HOTLIST_FLAG_SELECTED))
		return;

	/* Clear everything and then try to select the clicked line. */

	hotlist_select_none();

	window.w = hotlist_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	if ((row < hotlist_entries) && (hotlist[row].flags & HOTLIST_FLAG_SELECTABLE)) {
		hotlist[row].flags |= HOTLIST_FLAG_SELECTED;
		hotlist_selection_count++;
		if (hotlist_selection_count == 1)
			hotlist_selection_row = row;

		wimp_force_redraw(window.w, window.xscroll, LINE_BASE(row),
				window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(row));
	}

	hotlist_update_toolbar();
}


/**
 * Update the current selection based on an adjust click over a row of the
 * hotlist.
 *
 * \param row			The row under the click, or -1.
 */

static void hotlist_select_click_adjust(int row)
{
	int			i;
	wimp_window_state	window;

	if (row == -1 || row >= hotlist_entries || (hotlist[row].flags & HOTLIST_FLAG_SELECTABLE) == 0)
		return;

	window.w = hotlist_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	if (hotlist[row].flags & HOTLIST_FLAG_SELECTED) {
		hotlist[row].flags &= ~HOTLIST_FLAG_SELECTED;
		hotlist_selection_count--;
		if (hotlist_selection_count == 1) {
			for (i = 0; i < hotlist_entries; i++) {
				if (hotlist[i].flags & HOTLIST_FLAG_SELECTED) {
					hotlist_selection_row = i;
					break;
				}
			}
		}
	} else {
		hotlist[row].flags |= HOTLIST_FLAG_SELECTED;
		hotlist_selection_count++;
		if (hotlist_selection_count == 1)
			hotlist_selection_row = row;
	}

	wimp_force_redraw(window.w, window.xscroll, LINE_BASE(row),
			window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(row));

	hotlist_update_toolbar();
}


/**
 * Select all of the rows in the hotlist window.
 */

static void hotlist_select_all(void)
{
	int			i;
	wimp_window_state	window;

	if (hotlist_selection_count == hotlist_entries)
		return;

	window.w = hotlist_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	for (i = 0; i < hotlist_entries; i++) {
		if ((hotlist[i].flags & (HOTLIST_FLAG_SELECTABLE | HOTLIST_FLAG_SELECTED)) == HOTLIST_FLAG_SELECTABLE) {
			hotlist[i].flags |= HOTLIST_FLAG_SELECTED;

			hotlist_selection_count++;
			if (hotlist_selection_count == 1)
				hotlist_selection_row = i;

			wimp_force_redraw(window.w, window.xscroll, LINE_BASE(i),
					window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(i));
		}
	}

	hotlist_update_toolbar();
}


/**
 * Clear the selection in the hotlist window.
 */

static void hotlist_select_none(void)
{
	int			i;
	wimp_window_state	window;

	if (hotlist_selection_count == 0)
		return;

	window.w = hotlist_window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	/* If there's just one row selected, we can avoid looping through the lot
	 * by just clearing that one line.
	 */

	if (hotlist_selection_count == 1) {
		if (hotlist_selection_row < hotlist_entries)
			hotlist[hotlist_selection_row].flags &= ~HOTLIST_FLAG_SELECTED;
		hotlist_selection_count = 0;

		wimp_force_redraw(window.w, window.xscroll, LINE_BASE(hotlist_selection_row),
				window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(hotlist_selection_row));

		hotlist_update_toolbar();

		return;
	}

	/* If there is more than one row selected, we must loop through the lot
	 * to clear them all.
	 */

	for (i = 0; i < hotlist_entries; i++) {
		if (hotlist[i].flags & HOTLIST_FLAG_SELECTED) {
			hotlist[i].flags &= ~HOTLIST_FLAG_SELECTED;

			wimp_force_redraw(window.w, window.xscroll, LINE_BASE(i),
					window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(i));
		}
	}

	hotlist_selection_count = 0;

	hotlist_update_toolbar();
}


/**
 * Update the selected state of the toolbar icons.
 */

static void hotlist_update_toolbar(void)
{
	icons_set_shaded(hotlist_window_pane, HOTLIST_TOOLBAR_ICON_RENAME, hotlist_selection_count != 1);
	icons_set_shaded(hotlist_window_pane, HOTLIST_TOOLBAR_ICON_DELETE, hotlist_selection_count == 0);
	icons_set_shaded(hotlist_window_pane, HOTLIST_TOOLBAR_ICON_RUN, hotlist_selection_count != 1);
	icons_set_shaded(hotlist_window_pane, HOTLIST_TOOLBAR_ICON_DEFAULT, hotlist_selection_count != 1);
	
	icons_set_selected(hotlist_window_pane, HOTLIST_TOOLBAR_ICON_DEFAULT,
			(hotlist_selection_count == 1 && hotlist_selection_row == hotlist_find_default_entry()));
}


/**
 * Calculate the row that the mouse was clicked over in the hotlist window.
 *
 * \param  *pointer		The Wimp pointer data.
 * \param  *state		The results window state.
 * \return			The row (from 0) or -1 if none.
 */

static int hotlist_calculate_window_click_row(os_coord *pos, wimp_window_state *state)
{
	int		y, row_y_pos;
	unsigned	row;

	if (state == NULL)
		return -1;

	y = pos->y - state->visible.y1 + state->yscroll;

	row = ROW(y);
	row_y_pos = ROW_Y_POS(y);

	if (row >= hotlist_entries || ROW_ABOVE(row_y_pos) || ROW_BELOW(row_y_pos))
		row = -1;

	return row;
}


/**
 * Handle attempts to load Locate files to the hotlist window.
 *
 * \param w			The target window handle.
 * \param i			The target icon handle.
 * \param filetype		The filetype being loaded.
 * \param *filename		The name of the file being loaded.
 * \param *data			Unused NULL pointer.
 * \return			TRUE on loading; FALSE on passing up.
 */

static osbool hotlist_load_locate_file(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data)
{
	struct dialogue_block	*dialogue;
	struct discfile_block	*load;

	if (filetype != dataxfer_TYPE_LOCATE)
		return FALSE;

	load = discfile_open_read(filename);
	if (load == NULL)
		return FALSE;

	hourglass_on();

	hotlist_load_file(load);

	dialogue = dialogue_load_file(NULL, load, NULL, NULL);

	hourglass_off();

	/* If an error is raised when the file closes, or there wasn't any
	 * dialogue data in the file, give up.
	 */

	if (discfile_close(load) || dialogue == NULL) {
		dialogue_destroy(dialogue, DIALOGUE_CLIENT_HOTLIST);
		return FALSE;
	}

	dialogue_add_client(dialogue, DIALOGUE_CLIENT_HOTLIST);

	if (!hotlist_add_new_entry(string_find_leafname(filename), HOTLIST_FLAG_NONE, dialogue))
		dialogue_destroy(dialogue, DIALOGUE_CLIENT_HOTLIST);

	return TRUE;
}


/**
 * Delete the selected items from the hotlist.
 */

static void hotlist_delete_selection(void)
{

	int i = 0;

	while (i < hotlist_entries) {
		if (hotlist[i].flags & HOTLIST_FLAG_SELECTED)
			hotlist_delete_entry(i);
		else
			i++;
	}

	windows_redraw(hotlist_window);

	hotlist_update_toolbar();
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

	windows_title_msgs_lookup(hotlist_add_window, "HotlistAddT");
	icons_msgs_lookup(hotlist_add_window, HOTLIST_ADD_ICON_ADD, "HotlistAddB");

	wimp_get_pointer_info(&pointer);
	windows_open_centred_at_pointer(hotlist_add_window, &pointer);
	icons_put_caret_at_end(hotlist_add_window, HOTLIST_ADD_ICON_NAME);
}


/**
 * Rename an entry in the hotlist.
 *
 * \param index			The index of the .
 */

static void hotlist_rename_entry(int entry)
{
	wimp_pointer	pointer;

	hotlist_add_dialogue_handle = NULL;

	hotlist_set_add_window(entry);

	windows_title_msgs_lookup(hotlist_add_window, "HotlistRenameT");
	icons_msgs_lookup(hotlist_add_window, HOTLIST_ADD_ICON_ADD, "HotlistRenameB");

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
	char			*new_name;
	wimp_window_state	window;
	int			i;

	new_name = icons_get_indirected_text_addr(hotlist_add_window, HOTLIST_ADD_ICON_NAME);
	string_ctrl_zero_terminate(new_name);

	if (new_name == NULL || strlen(new_name) == 0) {
		error_msgs_report_info("HotlistNoName");
		return FALSE;
	}

	for (i = 0; i < hotlist_entries; i++) {
		if (string_nocase_strcmp(new_name, hotlist[i].name) == 0 &&
				(hotlist_add_entry == 0 || hotlist_add_entry != i)) {
			error_msgs_report_info("HotlistDupName");
			return FALSE;
		}
	}

	if (hotlist_add_dialogue_handle == NULL && hotlist_add_entry >= 0 && hotlist_add_entry < hotlist_entries) {
		string_copy(hotlist[hotlist_add_entry].name, new_name, HOTLIST_NAME_LENGTH);

		window.w = hotlist_window;
		if (xwimp_get_window_state(&window) == NULL) {
			wimp_force_redraw(window.w, window.xscroll, LINE_BASE(hotlist_add_entry),
				window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(hotlist_add_entry));
		}

		return TRUE;
	} else if (hotlist_add_dialogue_handle != NULL && hotlist_add_entry == -1) {
		return hotlist_add_new_entry(new_name, HOTLIST_FLAG_NONE, hotlist_add_dialogue_handle);
	}

	return FALSE;
}


/**
 * Add a new entry to the hotlist.
 *
 * \param *name			The name to give the entry.
 * \param flags			Any flags to set for the entry.
 * \param *dialogue		The dialogue data to associate with the entry.
 * \return			TRUE if successful; FALSE if errors occurred.
 */

static osbool hotlist_add_new_entry(char *name, enum hotlist_block_flags flags, struct dialogue_block *dialogue)
{
	wimp_window_state	window;

	if (hotlist_entries >= hotlist_allocation)
		hotlist_extend(hotlist_allocation + HOTLIST_ALLOCATION);

	if (hotlist_entries >= hotlist_allocation) {
		error_msgs_report_error("HotlistNoMem");
		return FALSE;
	}

	string_copy(hotlist[hotlist_entries].name, name, HOTLIST_NAME_LENGTH);
	hotlist[hotlist_entries].dialogue = dialogue;
	hotlist[hotlist_entries].flags = flags | HOTLIST_FLAG_SELECTABLE;

	window.w = hotlist_window;
	if (xwimp_get_window_state(&window) == NULL) {
		wimp_force_redraw(window.w, window.xscroll, LINE_BASE(hotlist_entries),
				window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(hotlist_entries));
	}

	hotlist_entries++;
	hotlist_update_extent();

	return TRUE;
}


/**
 * Delete an entry from the hotlist. The entry is removed, but no memory is
 * freed up.
 *
 * \param entry			The entry to be deleted.
 */

static void hotlist_delete_entry(int entry)
{
	if (entry < 0 || entry >=hotlist_entries)
		return;

	/* Keep the selection info in step. */

	if (hotlist[entry].flags & HOTLIST_FLAG_SELECTED)
		hotlist_selection_count--;

	if (hotlist_selection_row > entry)
		hotlist_selection_row--;

	/* Remove the entry if it isn't the last in the list. */

	if (entry < (hotlist_entries - 1))
		memmove(hotlist + entry, hotlist + entry + 1, sizeof(struct hotlist_block) * (hotlist_entries - entry - 1));

	hotlist_entries--;
}


/**
 * Move a hotlist entry, inserting it before a given point in the list.
 *
 * \param entry			The entry to be moved.
 * \param insert_before		The location to insert the moved block. This can
 *				be up to one more than the number of entries in
 *				the hotlist.
 * \return			The updated index of the insertion point, taking
 *				into account effects of the move.
 */

static int hotlist_move_entry(int entry, int insert_before)
{
	int			insert_at;
	struct hotlist_block	temp;


	if (entry < 0 || entry >= hotlist_entries || insert_before < 0 || insert_before > hotlist_entries)
		return insert_before;

	/* These two operations would result in no change. */

	if (entry == insert_before || entry + 1 == insert_before)
		return insert_before;

	/* Copy the block to be moved. */

	string_copy(temp.name, hotlist[entry].name, HOTLIST_NAME_LENGTH);
	temp.dialogue = hotlist[entry].dialogue;
	temp.flags = hotlist[entry].flags;

	/* Shuffle the hotlist contents to open up the destination. */

	if (entry < insert_before) {
		/* Move the intermediate items down the list. */

		memmove(hotlist + entry, hotlist + entry + 1, sizeof(struct hotlist_block) * (insert_before - entry - 1));
		insert_at = insert_before - 1;
	} else {
		/* Move the intermediate items up the list. */

		memmove(hotlist + insert_before + 1, hotlist + insert_before, sizeof(struct hotlist_block) * (entry - insert_before));
		insert_at = insert_before;
		insert_before++;
	}

	/* Put the copy back into the hotlist. */

	string_copy(hotlist[insert_at].name, temp.name, HOTLIST_NAME_LENGTH);
	hotlist[insert_at].dialogue = temp.dialogue;
	hotlist[insert_at].flags = temp.flags;

	return insert_before;
}


/**
 * Save the current hotlist settings to file.  Used as a DataXfer callback, so
 * must return TRUE on success or FALSE on failure.
 *
 * \param *filename		The filename to save to.
 * \param selection		TRUE to save just the selection, else FALSE.
 * \param *data			Context data: the handle of the parent dialogue.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool hotlist_save_hotlist(char *filename, osbool selection, void *data)
{
	return hotlist_save_file(filename, selection);
}


/**
 * Save a hotlist entry's search settings to file from a saveas dialogue.  Used
 * as a dialogue callback, so must return TRUE on success or FALSE on failure.
 *
 * \param *filename		The filename to save to.
 * \param selection		TRUE to save just the selection, else FALSE.
 * \param *data			Context data: unused and always NULL.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool hotlist_saveas_save_search(char *filename, osbool selection, void *data)
{
	if (filename == NULL || hotlist_selection_row < 0 || hotlist_selection_row >= hotlist_entries)
		return FALSE;

	return hotlist_save_search(filename, hotlist[hotlist_selection_row].dialogue);
}


/**
 * Save a hotlits entry's search settings for file, as a generic dataxfer callback.
 *
 * \param *filename		The filename to save to.
 * \param *data			Context data: the handle of the dialogue data to save.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool hotlist_save_search(char *filename, void *data)
{
	struct dialogue_block *dialogue = data;
	struct discfile_block		*out;

	if (filename == NULL || dialogue == NULL)
		return FALSE;

	out = discfile_open_write(filename);
	if (out == NULL)
		return FALSE;

	hourglass_on();

	dialogue_save_file(data, out, NULL, NULL);

	hourglass_off();

	discfile_close(out);

	osfile_set_type(filename, dataxfer_TYPE_LOCATE);

	return TRUE;
}


/**
 * Save the hotlist to a hotlist file in the default location.
 *
 * \return			TRUE if successful; FALSE if errors occurred.
 */

static osbool hotlist_save_choices(void)
{
	char	filename[1024];

	config_find_save_file(filename, 1024, "Hotlist");

#ifdef DEBUG
	debug_printf("Saving hotlist to '%s'", filename);
#endif

	return hotlist_save_file(filename, FALSE);
}


/**
 * Save the hotlist to a hotlist file.
 *
 * \param *filename		Pointer to the filename to save the hotlist to.
 * \param selection		TRUE to save just selected items; FALSE to save all.
 * \return			TRUE if successful; FALSE if errors occurred.
 */

static osbool hotlist_save_file(char *filename, osbool selection)
{
	struct discfile_block		*out;
	int				entry;

	if (filename == NULL)
		return FALSE;

	out = discfile_open_write(filename);
	if (out == NULL)
		return FALSE;

	hourglass_on();

	for (entry = 0; entry < hotlist_entries; entry++)
		if (!selection || (hotlist[entry].flags & HOTLIST_FLAG_SELECTED))
			dialogue_save_file(hotlist[entry].dialogue, out, hotlist_save_file_callback, &(hotlist[entry]));

	hourglass_off();

	discfile_close(out);

	osfile_set_type(filename, dataxfer_TYPE_LOCATE);

	return TRUE;
}


/**
 * Helper callback for hotlist saving, passed to dialogue_save_file() to
 * provide the hotlist-specific bits of the options chunk.
 *
 * \param *out			The handle of the discfile being written to.
 * \param action		The action being requested.
 * \param *data			Pointer to the data from hotlist_save_file(),
 *				which is a pointer to a struct hotlist_block.
 */

static void hotlist_save_file_callback(struct discfile_block *out, enum dialogue_file_action action, void *data)
{
	struct hotlist_block	*hotlist = data;

	if (out == NULL || data == NULL)
		return;

	switch (action) {
	case DIALOGUE_START_SECTION:
		discfile_start_section(out, DISCFILE_SECTION_HOTLIST, TRUE);
		break;

	case DIALOGUE_WRITE_DATA:
		discfile_write_option_string(out, "HNM", hotlist->name);
		discfile_write_option_unsigned(out, "HFG", (unsigned) (hotlist->flags & HOTLIST_FLAG_DEFAULT));
		break;

	default:
		break;
	}
}


/**
 * Load the hotlist from a hotlist file in the default location.
 *
 * \return			TRUE if successful; FALSE if errors occurred.
 */

static osbool hotlist_load_choices(void)
{
	struct discfile_block	*load;
	char			filename[1024];

	config_find_load_file(filename, 1024, "Hotlist");

	/* \TODO -- There's no check that this is a Locate filetype. */

	load = discfile_open_read(filename);
	if (load == NULL)
		return FALSE;

	hourglass_on();

	hotlist_load_file(load);

	hourglass_off();

	if (discfile_close(load))
		return FALSE;

	return TRUE;
}


/**
 * Load any hotlist entries contained in an open file.
 *
 * \param *load			The handle of the file to load from.
 * \return			TRUE if successful; FALSE if errors occurred.
 */

static osbool hotlist_load_file(struct discfile_block *load)
{
	struct dialogue_block	*dialogue;
	struct hotlist_block	hotlist_data;


	if (load == NULL)
		return FALSE;

	do {
		*hotlist_data.name = '\0';
		hotlist_data.flags = HOTLIST_FLAG_NONE;

		dialogue = dialogue_load_file(NULL, load, hotlist_load_file_callback, &hotlist_data);

		if (dialogue == NULL)
			continue;

		dialogue_add_client(dialogue, DIALOGUE_CLIENT_HOTLIST);

		if (!hotlist_add_new_entry(hotlist_data.name, hotlist_data.flags, dialogue))
			dialogue_destroy(dialogue, DIALOGUE_CLIENT_HOTLIST);
	} while (dialogue != NULL);

	return TRUE;
}


/**
 * Helper callback for hotlist loading, passed to dialogue_load_file() to
 * read the hotlist-specific bits of the options chunk.
 *
 * \param *load			The handle of the discfile being read from.
 * \param action		The action being requested.
 * \param *data			Pointer to the data from hotlist_save_file(),
 *				which is a pointer to a struct hotlist_block.
 * \return			TRUE if successful; FALSE on error.
 */

static osbool hotlist_load_file_callback(struct discfile_block *load, enum dialogue_file_action action, void *data)
{
	struct hotlist_block	*hotlist = data;
	osbool			result = TRUE;


	if (load == NULL || data == NULL)
		return FALSE;

#ifdef DEBUG
	debug_printf("Calling load helper: %u", action);
#endif

	switch (action) {
	case DIALOGUE_OPEN_SECTION:
		result = discfile_open_section(load, DISCFILE_SECTION_HOTLIST) && discfile_open_chunk(load, DISCFILE_CHUNK_OPTIONS);
		break;

	case DIALOGUE_READ_DATA:
		discfile_read_option_string(load, "HNM", hotlist->name, HOTLIST_NAME_LENGTH);
		if (!discfile_read_option_unsigned(load, "HFG", (unsigned *) &(hotlist->flags)))
			hotlist->flags = HOTLIST_FLAG_NONE;
		break;

	default:
		break;
	}

	return result;
}


/**
 * Build a hotlist menu and return a pointer to it.
 *
 * \return			A pointer to the menu block.
 */

wimp_menu *hotlist_build_menu(void)
{
	int	line = 0, width = 0;


	/* Allocate space for the Wimp menu block. */

	if (hotlist_menu != NULL)
		heap_free(hotlist_menu);

	hotlist_menu = heap_alloc(28 + 24 * ((hotlist_entries == 0) ? 1 : hotlist_entries));

	if (hotlist_menu == NULL)
		return NULL;



	/* Enter the default Edit entry at the top of the menu. */



	/* Add any hotlist entries that have been defined. */

	if (hotlist_entries > 0) {
		for (line = 0; line < hotlist_entries; line++) {
			if (strlen(hotlist[line].name) > width)
				width = strlen(hotlist[line].name);

			/* Set the menu and icon flags up. */

			hotlist_menu->entries[line].menu_flags = NONE;

			hotlist_menu->entries[line].sub_menu = (wimp_menu *) -1;
			hotlist_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
					wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
					wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

			/* Set the menu icon contents up. */

			hotlist_menu->entries[line].data.indirected_text.text = hotlist[line].name;
			hotlist_menu->entries[line].data.indirected_text.validation = "";
			hotlist_menu->entries[line].data.indirected_text.size = HOTLIST_NAME_LENGTH;
		}
	} else {
		hotlist_menu->entries[line].menu_flags = NONE;

		hotlist_menu->entries[line].sub_menu = (wimp_menu *) -1;
		hotlist_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_FILLED | wimp_ICON_SHADED |
			wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
			wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

		msgs_lookup("HotlistNone", hotlist_menu->entries[line].data.text, 12);

		if (strlen(hotlist_menu->entries[line].data.text) > width)
			width = strlen(hotlist_menu->entries[line].data.text);

		line++;
	}

	/* Finish off the menu definition. */

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
	if (selection < 0 || selection >= hotlist_entries)
		return;

#ifdef DEBUG
	debug_printf("Selected hotlist menu item %d", selection);
#endif

	if (hotlist[selection].dialogue != NULL)
		hotlist_open_entry(selection);
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


/**
 * Open a hotlist entry in a new search dialogue.
 *
 * \param entry			The hotlist entry to open.
 */

static void hotlist_open_entry(int entry)
{
	wimp_pointer	pointer;

	if (entry < 0 || entry >= hotlist_entries || hotlist[entry].dialogue == NULL)
		return;

	if (xwimp_get_pointer_info(&pointer) == NULL)
		file_create_dialogue(&pointer, NULL, NULL, hotlist[entry].dialogue);
}


/**
 * Set or unset the default dialogue in the hotlist.
 *
 * \param entry			The hotlist entry to set to default (if not
 *				within range, or -1, the current default
 *				will be unset).
 */

static void hotlist_set_default_dialogue(int entry)
{
	int	i;


	for (i = 0; i < hotlist_entries; i++) {
		if (i == entry)
			hotlist[i].flags |= HOTLIST_FLAG_DEFAULT;
		else
			hotlist[i].flags &= ~HOTLIST_FLAG_DEFAULT;
	}

	windows_redraw(hotlist_window);
	hotlist_update_toolbar();
}


/**
 * Find the index of the (first) hotlist entry to have the default
 * flag set.
 *
 * \return			The index of the first default entry, or
 *				HOTLIST_NULL_ENTRY if none found.
 */

static int hotlist_find_default_entry(void)
{
	int	i, entry = HOTLIST_NULL_ENTRY;

	for (i = 0; i < hotlist_entries; i++) {
		if (hotlist[i].flags & HOTLIST_FLAG_DEFAULT) {
			entry = i;
			break;
		}
	}

	return entry;
}


/**
 * Return the handle of the dialogue to use for default search settings,
 * or NULL if none has been set.
 *
 * \return			The handle of the default dialogue, or NULL.
 */

struct dialogue_block *hotlist_get_default_dialogue(void)
{
	int			entry;

	entry = hotlist_find_default_entry();
	if (entry == HOTLIST_NULL_ENTRY)
		return NULL;

	return hotlist[entry].dialogue;
}

