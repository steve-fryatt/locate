/* Copyright 2012, Stephen Fryatt
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
 * \file: results.c
 *
 * Search result and status window implementation.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* OSLib Header files. */

#include "oslib/fileswitch.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/osspriteop.h"
#include "oslib/territory.h"
#include "oslib/wimp.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/string.h"

/* Application header files. */

#include "results.h"

#include "dataxfer.h"
#include "datetime.h"
#include "dialogue.h"
#include "file.h"
#include "fileicon.h"
#include "ihelp.h"
#include "objdb.h"
#include "templates.h"
#include "textdump.h"


#define STATUS_LENGTH 128							/**< The maximum size of the status bar text field.			*/
#define TITLE_LENGTH 256							/**< The maximum size of the undefined title bar text field. 		*/

#define RESULTS_TOOLBAR_HEIGHT 0						/**< The height of the toolbar in OS units.				*/
#define RESULTS_LINE_HEIGHT 56							/**< The height of a results line, in OS units.				*/
#define RESULTS_WINDOW_MARGIN 4
#define RESULTS_LINE_OFFSET 4
#define RESULTS_ICON_HEIGHT 52							/**< The height of an icon in the results window, in OS units.		*/
#define RESULTS_STATUS_HEIGHT 60						/**< The height of the status bar in OS units.				*/
#define RESULTS_ICON_WIDTH 50							/**< The width of a small file icon in OS units.			*/

#define RESULTS_MIN_LINES 10							/**< The minimum number of lines to show in the results window.		*/

#define RESULTS_ALLOC_REDRAW 50							/**< Number of window redraw blocks to allocate at a time.		*/
#define RESULTS_ALLOC_FILES 50							/**< Number of file blocks to allocate at a time.			*/
#define RESULTS_ALLOC_TEXT 1024							/**< Number of bytes of text storage to allocate at a time.		*/

#define RESULTS_NULL 0xffffffff							/**< 'NULL' value for use with the unsigned flex block offsets.		*/

#define RESULTS_ROW_NONE ((unsigned) 0xffffffff)				/**< Row selection value to indicate "No Row".				*/

/* Results window icons. */

#define RESULTS_ICON_FILE 0
#define RESULTS_ICON_INFO 1
#define RESULTS_ICON_SIZE 1
#define RESULTS_ICON_TYPE 2
#define RESULTS_ICON_ATTRIBUTES 3
#define RESULTS_ICON_DATE 4

#define RESULTS_ICON_STATUS 1

/* Object Info window icons. */

#define RESULTS_OBJECT_ICON_NAME 0
#define RESULTS_OBJECT_ICON_TYPE 2
#define RESULTS_OBJECT_ICON_SIZE 4
#define RESULTS_OBJECT_ICON_ACCESS 6
#define RESULTS_OBJECT_ICON_DATE 8
#define RESULTS_OBJECT_ICON_ICON 9

/* Results Window Menu. */

#define RESULTS_MENU_DISPLAY 0
#define RESULTS_MENU_SAVE 1
#define RESULTS_MENU_SELECT_ALL 2
#define RESULTS_MENU_CLEAR_SELECTION 3
#define RESULTS_MENU_OBJECT_INFO 4
#define RESULTS_MENU_OPEN_PARENT 5
#define RESULTS_MENU_COPY_NAMES 6
#define RESULTS_MENU_MODIFY_SEARCH 7
#define RESULTS_MENU_STOP_SEARCH 8

#define RESULTS_MENU_DISPLAY_PATH_ONLY 0
#define RESULTS_MENU_DISPLAY_FULL_INFO 1

#define RESULTS_MENU_SAVE_RESULTS 0
#define RESULTS_MENU_SAVE_PATH_NAMES 1
#define RESULTS_MENU_SAVE_SEARCH_OPTIONS 2


/* Data structures. */

enum results_line_type {
	RESULTS_LINE_NONE = 0,							/**< A blank line (nothing plotted).					*/
	RESULTS_LINE_TEXT,							/**< A line containing unspecific text.					*/
	RESULTS_LINE_FILENAME,							/**< A line containing a filename.					*/
	RESULTS_LINE_FILEINFO,							/**< A line containing file information.				*/
	RESULTS_LINE_CONTENTS							/**< A line containing a file contents match.				*/
};

enum results_line_flags {
	RESULTS_FLAG_NONE = 0,							/**< No flags set.							*/
	RESULTS_FLAG_HALFSIZE = 1,						/**< Sprite should be plotted at half-size.				*/
	RESULTS_FLAG_SELECTABLE = 2,						/**< The row can be selected by the user.				*/
	RESULTS_FLAG_SELECTED = 4						/**< The row is currently selected.					*/
};


/** A line definition for the results window. */

struct results_line {
	enum results_line_type	type;						/**< The type of line.							*/

	enum results_line_flags	flags;						/**< Any flags relating to the line.					*/

	unsigned		parent;						/**< The parent line for a group (points to itself for the parent).	*/

	unsigned		text;						/**< Text offset for text-based lines (RESULTS_NULL if not used).	*/
	unsigned		sprite;						/**< Text offset for the display icon's sprite name.			*/
	unsigned		file;						/**< Object key for file objects.					*/

	unsigned		truncate;					/**< Non-zero indicates first character of text to be displayed.	*/
	wimp_colour		colour;						/**< The foreground colour of the text.					*/

	unsigned		index;						/**< Sort indirection index. UNCONNECTED TO REMAINING DATA.		*/
};

/** A data structure defining a results window. */

struct results_window {
	struct file_block	*file;						/**< The file block to which the window belongs.			*/

	unsigned		format_width;					/**< The currently formatted window width.				*/

	/* Results Window line data. */

	struct results_line	*redraw;					/**< The array of redraw data for the window.				*/
	unsigned		redraw_lines;					/**< The number of lines in the window.					*/
	unsigned		redraw_size;					/**< The number of redraw lines claimed.				*/

	unsigned		formatted_lines;				/**< The number of lines currently formatted;				*/

	unsigned		display_lines;					/**< The number of lines currently indexed into the window.		*/

	osbool			full_info;					/**< TRUE if full info mode is on; else FALSE.				*/

	/* Selection handling */

	unsigned		selection_count;				/**< The number of selected rows in the window.				*/
	unsigned		selection_row;					/**< The selected row, iff selection_count == 1.			*/

	osbool			selection_from_menu;				/**< TRUE if the selection came about because of a menu opening.	*/

	/* Generic text string storage. */

	struct textdump_block	*text;						/**< The general text string dump.					*/

	/* Object database. */

	struct objdb_block	*objects;					/**< The object database associated with the search results.		*/

	/* Window handles */

	wimp_w			window;						/**< The window handle.							*/
	wimp_w			status;						/**< The status bar handle.						*/
};


/* Global variables. */

static wimp_window			*results_window_def = NULL;		/**< Definition for the main results window.				*/
static wimp_window			*results_status_def = NULL;		/**< Definition for the results status pane.				*/

static wimp_w				results_object_window = NULL;		/**< Handle of the object info window.					*/

static wimp_menu			*results_window_menu = NULL;		/**< The results window menu.						*/
static wimp_menu			*results_window_menu_display = NULL;	/**< The results window display submenu.				*/

static osspriteop_area			*results_sprite_area = NULL;		/**< The application sprite area.					*/

static struct dataxfer_savebox		*results_save_results = NULL;		/**< The Save Results savebox data handle.				*/
static struct dataxfer_savebox		*results_save_paths = NULL;		/**< The Save Paths savebox data handle.				*/
static struct dataxfer_savebox		*results_save_options = NULL;		/**< The Save Options savebox data handle.				*/


/* Local function prototypes. */

static void	results_click_handler(wimp_pointer *pointer);
static osbool	results_keypress_handler(wimp_key *key);
static void	results_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void	results_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void	results_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static void	results_menu_close(wimp_w w, wimp_menu *menu);
static void	results_redraw_handler(wimp_draw *redraw);
static void	results_close_handler(wimp_close *close);
static void	results_set_display_mode(struct results_window *handle, osbool full_info);
static void	results_update_extent(struct results_window *handle);
static unsigned	results_add_line(struct results_window *handle, osbool show);
static unsigned	results_calculate_window_click_row(struct results_window *handle, os_coord *pos, wimp_window_state *state);
static void	results_select_click_select(struct results_window *handle, unsigned row);
static void	results_select_click_adjust(struct results_window *handle, unsigned row);
static void	results_select_all(struct results_window *handle);
static void	results_select_none(struct results_window *handle);
static void	results_run_object(struct results_window *handle, unsigned row);
static void	results_open_parent(struct results_window *handle, unsigned row);
static void	results_object_info_prepare(struct results_window *handle);
static char	*results_create_attributes_string(fileswitch_attr attributes, char *buffer, size_t length);
static char	*results_create_address_string(unsigned load_addr, unsigned exec_addr, char *buffer, size_t length);


//static unsigned	results_add_fileblock(struct results_window *handle);


/* Line position calculations.
 *
 * NB: These can be called with lines < 0 to give lines off the top of the window!
 */

#define LINE_BASE(x) (-((x)+1) * RESULTS_LINE_HEIGHT - RESULTS_TOOLBAR_HEIGHT - RESULTS_WINDOW_MARGIN)
#define LINE_Y0(x) (LINE_BASE(x) + RESULTS_LINE_OFFSET)
#define LINE_Y1(x) (LINE_BASE(x) + RESULTS_LINE_OFFSET + RESULTS_ICON_HEIGHT)


/**
 * Initialise the Results module.
 *
 * \param *sprites		Pointer to the sprite area to be used.
 */

void results_initialise(osspriteop_area *sprites)
{
	results_window_menu = templates_get_menu(TEMPLATES_MENU_RESULTS);
	results_window_menu_display = templates_get_menu(TEMPLATES_MENU_RESULTS_DISPLAY);

	results_window_def = templates_load_window("Results");
	results_window_def->icon_count = 0;

	results_status_def = templates_load_window("ResultsPane");

	if (results_window_def == NULL || results_status_def == NULL)
		error_msgs_report_fatal("BadTemplate");

	results_object_window = templates_create_window("ObjectInfo");
	templates_link_menu_dialogue("ObjectInfo", results_object_window);
	ihelp_add_window(results_object_window, "ObjectInfo", NULL);

	results_sprite_area = sprites;

	results_save_results = dataxfer_new_savebox(FALSE, "file_1a1", NULL);
	results_save_paths = dataxfer_new_savebox(TRUE, "file_fff", NULL);
	results_save_options = dataxfer_new_savebox(FALSE, "file_1a1", NULL);
}


/**
 * Create and open a new results window.
 *
 * \param *file			The file block to which the window belongs.
 * \param *objects		The object database from which file data should be taken.
 * \param *title		The title to use for the window, or NULL to allocate
 *				an empty buffer of TITLE_LENGTH.
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_create(struct file_block *file, struct objdb_block *objects, char *title)
{
	struct results_window	*new = NULL;
	char			*title_block = FALSE;
	char			*status_block = FALSE;
	int			status_height;
	osbool			mem_ok = TRUE;

	/* Allocate all the memory that we require. */

	if ((new = heap_alloc(sizeof(struct results_window))) == NULL)
		mem_ok = FALSE;

	if (new != NULL) {
		new->redraw = NULL;
		new->text = NULL;
		new->objects = NULL;
	}

	if (mem_ok) {
		if (title != NULL) {
			if ((title_block = heap_strdup(title)) == NULL)
				mem_ok = FALSE;
		} else {
			if ((title_block = heap_alloc(TITLE_LENGTH)) == NULL)
				mem_ok = FALSE;
		}
	}

	if (mem_ok) {
		if ((status_block = heap_alloc(STATUS_LENGTH)) == NULL)
			mem_ok = FALSE;
	}

	if (mem_ok) {
		if (flex_alloc((flex_ptr) &(new->redraw), RESULTS_ALLOC_REDRAW * sizeof(struct results_line)) == 0)
			mem_ok = FALSE;
	}

	if (mem_ok) {
		if ((new->text = textdump_create(RESULTS_ALLOC_TEXT, 0)) == NULL)
			mem_ok = FALSE;
	}

	/* If any memory allocations failed, free anything that did get
	 * claimed and exit with an error.
	 */

	if (!mem_ok) {
		if (new != NULL && new->redraw != NULL)
			flex_free((flex_ptr) &(new->redraw));
		if (new != NULL && new->text != NULL)
			textdump_destroy(new->text);

		if (new != NULL)
			heap_free(new);
		if (title_block != NULL)
			heap_free(title_block);
		if (status_block != NULL)
			heap_free(status_block);

		error_msgs_report_error("NoMemResultsCreate");
		return NULL;
	}

	/* Populate the data in the block. */

	new->file = file;
	new->objects = objects;

	new->redraw_size = RESULTS_ALLOC_REDRAW;
	new->redraw_lines = 0;

	new->formatted_lines = 0;

	new->display_lines = 0;

	new->full_info = FALSE;

	new->selection_count = 0;
	new->selection_row = 0;
	new->selection_from_menu = FALSE;

	new->format_width = results_window_def->visible.x1 - results_window_def->visible.x0;

	/* Create the window and open it on screen. */

	status_height = results_status_def->visible.y1 - results_status_def->visible.y0;

	windows_place_as_footer(results_window_def, results_status_def, status_height);

	results_window_def->title_data.indirected_text.text = title_block;
	results_window_def->title_data.indirected_text.size = (title == NULL) ? TITLE_LENGTH : strlen(title_block) + 1;
	results_window_def->sprite_area = results_sprite_area;

	results_status_def->icons[RESULTS_ICON_STATUS].data.indirected_text.text = status_block;
	results_status_def->icons[RESULTS_ICON_STATUS].data.indirected_text.size = STATUS_LENGTH;
	*status_block = '\0';
	if (title == NULL)
		*title_block = '\0';

	new->window = wimp_create_window(results_window_def);
	new->status = wimp_create_window(results_status_def);

	ihelp_add_window(new->window, "Results", NULL);
	ihelp_add_window(new->status, "ResultsStatus", NULL);

	event_add_window_user_data(new->window, new);
	event_add_window_user_data(new->status, new);

	event_add_window_redraw_event(new->window, results_redraw_handler);
	event_add_window_close_event(new->window, results_close_handler);
	event_add_window_mouse_event(new->window, results_click_handler);
	//event_add_window_key_event(new->window, results_keypress_handler);
	event_add_window_menu(new->window, results_window_menu);
	event_add_window_menu_prepare(new->window, results_menu_prepare);
	event_add_window_menu_warning(new->window, results_menu_warning);
	event_add_window_menu_selection(new->window, results_menu_selection);
	event_add_window_menu_close(new->window, results_menu_close);

	event_add_window_menu(new->status, results_window_menu);
	event_add_window_menu_prepare(new->status, results_menu_prepare);
	event_add_window_menu_warning(new->status, results_menu_warning);
	event_add_window_menu_selection(new->status, results_menu_selection);
	event_add_window_menu_close(new->status, results_menu_close);

	windows_open(new->window);
	windows_open_nested_as_footer(new->status, new->window, status_height);

	return new;
}


/**
 * Close and destroy a results window.
 *
 * \param *handle		The handle of the results window to destroy.
 */

void results_destroy(struct results_window *handle)
{
	char	*title, *status;

	if (handle == NULL)
		return;

	title = windows_get_indirected_title_addr(handle->window);
	status = icons_get_indirected_text_addr(handle->status, RESULTS_ICON_STATUS);

	ihelp_remove_window(handle->window);
	event_delete_window(handle->window);
	wimp_delete_window(handle->window);

	ihelp_remove_window(handle->window);
	event_delete_window(handle->status);
	wimp_delete_window(handle->status);

	flex_free((flex_ptr) &(handle->redraw));

	if (handle->text != NULL)
		textdump_destroy(handle->text);

	heap_free(title);
	heap_free(status);
	heap_free(handle);
}


/**
 * Process mouse clicks in results window.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void results_click_handler(wimp_pointer *pointer)
{
	struct results_window	*handle = event_get_window_user_data(pointer->w);
	wimp_window_state	state;
	unsigned		row;

	if (handle == NULL || pointer== NULL)
		return;

	state.w = pointer->w;
	if (xwimp_get_window_state(&state) != NULL)
		return;

	row = results_calculate_window_click_row(handle, &(pointer->pos), &state);

	switch(pointer->buttons) {
	case wimp_SINGLE_SELECT:
		results_select_click_select(handle, row);
		break;

	case wimp_SINGLE_ADJUST:
		results_select_click_adjust(handle, row);
		break;

	case wimp_DOUBLE_SELECT:
		results_select_none(handle);
		results_run_object(handle, row);
		break;

	case wimp_DOUBLE_ADJUST:
		results_select_click_adjust(handle, row);
		results_open_parent(handle, row);
		break;
	}

/*
	if (pointer->w == dialogue_window) {
		switch ((int) pointer->i) {
		case DIALOGUE_ICON_SEARCH:
			if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
				dialogue_read_window(dialogue_data);
				dialogue_start_search(dialogue_data);

				if (pointer->buttons == wimp_CLICK_SELECT) {
					settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
					dialogue_close_window();
				}
			}
			break;

		case DIALOGUE_ICON_CANCEL:
			if (pointer->buttons == wimp_CLICK_SELECT) {
				if (dialogue_data != NULL)
					file_destroy(dialogue_data->file);
				settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
				dialogue_close_window();
			} else if (pointer->buttons == wimp_CLICK_ADJUST) {
				dialogue_set_window(dialogue_data);
				dialogue_redraw_window();
			}
			break;

		case DIALOGUE_ICON_SIZE:
		case DIALOGUE_ICON_DATE:
		case DIALOGUE_ICON_TYPE:
		case DIALOGUE_ICON_ATTRIBUTES:
		case DIALOGUE_ICON_CONTENTS:
			dialogue_change_pane(icons_get_radio_group_selected(dialogue_window, DIALOGUE_PANES,
				DIALOGUE_ICON_SIZE, DIALOGUE_ICON_DATE, DIALOGUE_ICON_TYPE,
				DIALOGUE_ICON_ATTRIBUTES, DIALOGUE_ICON_CONTENTS));
			break;

		case DIALOGUE_ICON_SHOW_OPTS:
			dialogue_toggle_size(icons_get_selected(dialogue_window, DIALOGUE_ICON_SHOW_OPTS));
			break;

		case DIALOGUE_ICON_DRAG:
			if (pointer->buttons == wimp_DRAG_SELECT)
				dataxfer_save_window_drag(dialogue_window, DIALOGUE_ICON_DRAG, dialogue_drag_end_handler, NULL);
			break;
		}
	} else if (pointer->w == dialogue_panes[DIALOGUE_PANE_DATE]) {
		dialogue_shade_date_pane();
		if (pointer->i == DIALOGUE_DATE_ICON_DATE_FROM_SET || pointer->i == DIALOGUE_DATE_ICON_DATE_TO_SET)
			settime_open(pointer->w, (pointer->i == DIALOGUE_DATE_ICON_DATE_FROM_SET) ? DIALOGUE_DATE_ICON_DATE_FROM : DIALOGUE_DATE_ICON_DATE_TO, pointer);
	} else if (pointer->w == dialogue_panes[DIALOGUE_PANE_TYPE])
		dialogue_shade_type_pane();
	else if (pointer->w == dialogue_panes[DIALOGUE_PANE_ATTRIBUTES])
		dialogue_shade_attributes_pane();

		*/
}


/**
 * Process keypresses in a results window.
 *
 * \param *key			The keypress event block to handle.
 * \return			TRUE if the event was handled; else FALSE.
 */

static osbool results_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Prepare the results menu for (re)-opening.
 *
 * \param  w			The handle of the menu's parent window.
 * \param  *menu		Pointer to the menu being opened.
 * \param  *pointer		Pointer to the Wimp Pointer event block.
 */

static void results_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	struct results_window	*handle = event_get_window_user_data(w);
	wimp_window_state	state;
	unsigned		row;

	if (handle == NULL)
		return;

	if (pointer != NULL) {
		state.w = pointer->w;
		if (xwimp_get_window_state(&state) != NULL)
			return;

		row = results_calculate_window_click_row(handle, &(pointer->pos), &state);
		if (handle->selection_count == 0) {
			results_select_click_select(handle, row);
			handle->selection_from_menu = TRUE;
		} else {
			handle->selection_from_menu = FALSE;
		}
	}

	menus_shade_entry(results_window_menu, RESULTS_MENU_CLEAR_SELECTION, handle->selection_count == 0);
	menus_shade_entry(results_window_menu, RESULTS_MENU_OBJECT_INFO, handle->selection_count != 1);
	menus_shade_entry(results_window_menu, RESULTS_MENU_OPEN_PARENT, handle->selection_count != 1);
	menus_shade_entry(results_window_menu, RESULTS_MENU_COPY_NAMES, handle->selection_count == 0);
	menus_shade_entry(results_window_menu, RESULTS_MENU_MODIFY_SEARCH, dialogue_window_is_open());
	menus_shade_entry(results_window_menu, RESULTS_MENU_STOP_SEARCH, !file_search_active(handle->file));

	menus_tick_entry(results_window_menu_display, RESULTS_MENU_DISPLAY_PATH_ONLY, !handle->full_info);
	menus_tick_entry(results_window_menu_display, RESULTS_MENU_DISPLAY_FULL_INFO, handle->full_info);

	dataxfer_savebox_initialise(results_save_results, "FileName", NULL, TRUE, FALSE, NULL);
	dataxfer_savebox_initialise(results_save_paths, "ExptName", "SelectName", TRUE, FALSE, NULL);
	dataxfer_savebox_initialise(results_save_options, "SrchName", NULL, FALSE, FALSE, NULL);
}


/**
 * Process submenu warning events for the results menu.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void results_menu_warning(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	struct results_window	*handle = event_get_window_user_data(w);

	if (handle == NULL)
		return;

	switch (warning->selection.items[0]) {
	case RESULTS_MENU_SAVE:
		switch (warning->selection.items[1]) {
		case RESULTS_MENU_SAVE_RESULTS:
			dataxfer_savebox_prepare(results_save_results);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		case RESULTS_MENU_SAVE_PATH_NAMES:
			dataxfer_savebox_prepare(results_save_paths);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		case RESULTS_MENU_SAVE_SEARCH_OPTIONS:
			dataxfer_savebox_prepare(results_save_options);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		}
		break;

	case RESULTS_MENU_OBJECT_INFO:
		results_object_info_prepare(handle);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Handle selections from the results menu.
 *
 * \param  w			The window to which the menu belongs.
 * \param  *menu		Pointer to the menu itself.
 * \param  *selection		Pointer to the Wimp menu selction block.
 */

static void results_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection)
{
	struct results_window	*handle = event_get_window_user_data(w);
	wimp_pointer		pointer;

	if (handle == NULL)
		return;

	wimp_get_pointer_info(&pointer);

	switch(selection->items[0]) {
	case RESULTS_MENU_DISPLAY:
		switch(selection->items[1]) {
		case RESULTS_MENU_DISPLAY_PATH_ONLY:
			results_set_display_mode(handle, FALSE);
			break;

		case RESULTS_MENU_DISPLAY_FULL_INFO:
			results_set_display_mode(handle, TRUE);
			break;
		}
		break;

	case RESULTS_MENU_SELECT_ALL:
		results_select_all(handle);
		break;

	case RESULTS_MENU_CLEAR_SELECTION:
		results_select_none(handle);
		break;

	case RESULTS_MENU_OPEN_PARENT:
		if (handle->selection_count == 1)
			results_open_parent(handle, handle->selection_row);
		break;

	case RESULTS_MENU_MODIFY_SEARCH:
		file_create_dialogue(&pointer, NULL, handle->file);
		break;

	case RESULTS_MENU_STOP_SEARCH:
		file_stop_search(handle->file);
		break;
	}
}


/**
 * Handle the closure of a menu.
 *
 * \param  w			The handle of the menu's parent window.
 * \param  *menu		Pointer to the menu being closee.
 */

static void results_menu_close(wimp_w w, wimp_menu *menu)
{
	struct results_window	*handle = event_get_window_user_data(w);

	if (handle == NULL || handle->selection_from_menu == FALSE)
		return;

	results_select_none(handle);
	handle->selection_from_menu = FALSE;
}


/**
 * Callback to handle a results window closing.  This is terminal, and simply
 * calls the file destructor for the associated file data (which in turn calls
 * results_destroy() for the window in question.
 *
 * \param *close		The wimp close event data block.
 */

static void results_close_handler(wimp_close *close)
{
	struct results_window *block;

	block = event_get_window_user_data(close->w);
	if (block == NULL)
		return;

	file_destroy(block->file);
}


/**
 * Callback to handle redraw events on a results window.
 *
 * \param  *redraw		The Wimp redraw event block.
 */

static void results_redraw_handler(wimp_draw *redraw)
{
	int			ox, oy, top, bottom, y;
	osbool			more;
	struct results_window	*res;
	struct results_line	*line;
	struct fileicon_info	typeinfo;
	unsigned		filetype;
	bits			data[256];
	osgbpb_info		*file = (osgbpb_info *) data;
	wimp_icon		*icon;
	char			*text, *fileicon;
	char			validation[255];
	char			truncation[1024]; // \TODO -- Allocate properly.
	char			*size = truncation, *attributes = truncation + 32, *date = truncation + 64;
	os_t			start_time;

	start_time = os_read_monotonic_time();

	res = (struct results_window *) event_get_window_user_data(redraw->w);

	if (res == NULL)
		return;

	icon = results_window_def->icons;

	/* Set up the validation string buffer for text+sprite icons. */

	*validation = 'S';
	icon[RESULTS_ICON_FILE].data.indirected_text.validation = validation;

	/* Set up the truncation line. */

	strcpy(truncation, "...");

	/* Redraw the window. */

	text = textdump_get_base(res->text);
	fileicon = fileicon_get_base();

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		top = (oy - redraw->clip.y1 - RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (top < 0)
			top = 0;

                bottom = ((RESULTS_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0 - RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (bottom > res->display_lines)
			bottom = res->display_lines;

		for (y = top; y < bottom; y++) {
			line = &(res->redraw[res->redraw[y].index]);

			switch (line->type) {
			case RESULTS_LINE_FILENAME:
				icon[RESULTS_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_FILE].extent.y1 = LINE_Y1(y);

				filetype = objdb_get_filetype(res->objects, line->file);
				fileicon_get_type_icon(filetype, "", &typeinfo);

				if (typeinfo.small != TEXTDUMP_NULL) {
					strcpy(validation + 1, fileicon + typeinfo.small);
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;
				} else if (typeinfo.large != TEXTDUMP_NULL) {
					strcpy(validation + 1, fileicon + typeinfo.large);
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_HALF_SIZE;
				} else {
					strcpy(validation + 1, "small_xxx");
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;
				}

				objdb_get_name(res->objects, line->file, truncation + 3, sizeof(truncation) - 3);
				if (line->truncate > 0) {
					truncation[line->truncate] = '.';
					truncation[line->truncate + 1] = '.';
					truncation[line->truncate + 2] = '.';
					icon[RESULTS_ICON_FILE].data.indirected_text.text = truncation + line->truncate;
				} else {
					icon[RESULTS_ICON_FILE].data.indirected_text.text = truncation + 3;
				}

				icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_FILE].flags |= line->colour << wimp_ICON_FG_COLOUR_SHIFT;
				if (line->flags & RESULTS_FLAG_HALFSIZE)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_HALF_SIZE;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;

				if (line->flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SELECTED;

				wimp_plot_icon(&(icon[RESULTS_ICON_FILE]));
				break;


			case RESULTS_LINE_FILEINFO:
				icon[RESULTS_ICON_TYPE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_TYPE].extent.y1 = LINE_Y1(y);

				filetype = objdb_get_filetype(res->objects, line->file);
				fileicon_get_type_icon(filetype, "", &typeinfo);

				if (typeinfo.name != TEXTDUMP_NULL) {
					icon[RESULTS_ICON_TYPE].data.indirected_text.text = fileicon + typeinfo.name;
				} else {
					icon[RESULTS_ICON_TYPE].data.indirected_text.text = "";
				}

				icon[RESULTS_ICON_SIZE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_SIZE].extent.y1 = LINE_Y1(y);
				icon[RESULTS_ICON_ATTRIBUTES].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_ATTRIBUTES].extent.y1 = LINE_Y1(y);
				icon[RESULTS_ICON_DATE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_DATE].extent.y1 = LINE_Y1(y);

				objdb_get_info(res->objects, line->file, file);

				if (xos_convert_file_size(file->size, size, 32, NULL) != NULL)
					*size = '\0';

				results_create_attributes_string(file->attr, attributes, 32);

				results_create_address_string(file->load_addr, file->exec_addr, date, 100);

				icon[RESULTS_ICON_SIZE].data.indirected_text.text = size;
				icon[RESULTS_ICON_ATTRIBUTES].data.indirected_text.text = attributes;
				icon[RESULTS_ICON_DATE].data.indirected_text.text = date;

				wimp_plot_icon(&(icon[RESULTS_ICON_SIZE]));
				wimp_plot_icon(&(icon[RESULTS_ICON_TYPE]));
				wimp_plot_icon(&(icon[RESULTS_ICON_ATTRIBUTES]));
				wimp_plot_icon(&(icon[RESULTS_ICON_DATE]));
				break;


			case RESULTS_LINE_TEXT:
				icon[RESULTS_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_FILE].extent.y1 = LINE_Y1(y);

				strcpy(validation + 1, fileicon + line->sprite);

				if (line->truncate > 0) {
					strcpy(truncation + 3, text + line->text + line->truncate);
					icon[RESULTS_ICON_FILE].data.indirected_text.text = truncation;
				} else {
					icon[RESULTS_ICON_FILE].data.indirected_text.text = text + line->text;
				}
				icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_FILE].flags |= line->colour << wimp_ICON_FG_COLOUR_SHIFT;
				if (line->flags & RESULTS_FLAG_HALFSIZE)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_HALF_SIZE;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_HALF_SIZE;

				if (line->flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SELECTED;

				wimp_plot_icon(&(icon[RESULTS_ICON_FILE]));
				break;

			default:
				break;
			}
		}

		more = wimp_get_rectangle(redraw);
	}

	debug_printf("Redraw time: %d cs", os_read_monotonic_time() - start_time);
}


/**
 * Update the status bar text for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *status		The text to be copied into the status bar.
 */

void results_set_status(struct results_window *handle, char *status)
{
	if (handle != NULL && status != NULL)
		icons_strncpy(handle->status, RESULTS_ICON_STATUS, status);

	xwimp_set_icon_state(handle->status, RESULTS_ICON_STATUS, 0, 0);
}


/**
 * Update the status bar text for a results window using a template and truncating
 * the inset text to fit if necessary.
 *
 * \param *handle		The handle of the results window to update.
 * \param *token		The messagetrans token of the template.
 * \param *text 		The text to be inserted into placeholder %0.
 */

void results_set_status_template(struct results_window *handle, char *token, char *text)
{
	char			status[STATUS_LENGTH], truncate[STATUS_LENGTH], *pos;
	int			i;
	wimp_icon_state		icon;
	os_error		*error;


	if (handle == NULL || token == NULL || text == NULL)
		return;

	/* If text is longer than the buffer, truncate it to the buffer length. */

	i = (strlen(text) + 1) - STATUS_LENGTH;

	if (i <= 0) {
		strncpy(truncate, text, sizeof(truncate));
	} else {
		i += 3;

		while (i-- > 0 && *text != '\0')
			text++;

		strncpy(truncate, "...", sizeof(truncate));
		strncpy(truncate + 3, text, sizeof(truncate) - 3);
	}

	icon.w = handle->status;
	icon.i = RESULTS_ICON_STATUS;
	error = xwimp_get_icon_state(&icon);
	if (error != NULL)
		return;

	pos = truncate;

	msgs_param_lookup(token, status, sizeof(status), pos, NULL, NULL, NULL);

	while (wimptextop_string_width(status, 0) > (icon.icon.extent.x1 - icon.icon.extent.x0)) {
		*(pos + 3) = '.';
		pos++;

		msgs_param_lookup(token, status, sizeof(status), pos, NULL, NULL, NULL);
	}

	icons_strncpy(handle->status, RESULTS_ICON_STATUS, status);

	xwimp_set_icon_state(handle->status, RESULTS_ICON_STATUS, 0, 0);
}


/**
 * Update the title text for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *title		The text to be copied into the title.
 */

void results_set_title(struct results_window *handle, char *title)
{
	if (handle != NULL && title != NULL)
		windows_title_strncpy(handle->window, title);

	xwimp_force_redraw_title(handle->window);
}


/**
 * Add an error message to the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *message		The error message text.
 * \param *path			The path of the folder where the error occurred,
 *				or NULL if not applicable.
 */

void results_add_error(struct results_window *handle, char *message, char *path)
{
	unsigned		line, offv, offt;
	struct fileicon_info	icon;

	if (handle == NULL)
		return;

	line = results_add_line(handle, TRUE);
	if (line == RESULTS_NULL)
		return;

	offt = textdump_store(handle->text, message);
	fileicon_get_special_icon(FILEICON_ERROR, &icon);

	if (icon.small != TEXTDUMP_NULL) {
		offv = icon.small;
	} else if (icon.large != TEXTDUMP_NULL) {
		offv = icon.large;
		handle->redraw[line].flags |= RESULTS_FLAG_HALFSIZE;
	} else {
		offv = TEXTDUMP_NULL;
	}

	if (offt == TEXTDUMP_NULL || offv == TEXTDUMP_NULL)
		return;

	handle->redraw[line].type = RESULTS_LINE_TEXT;
	handle->redraw[line].text = offt;
	handle->redraw[line].sprite = offv;
	handle->redraw[line].colour = wimp_COLOUR_RED;
}


/**
 * Add a file to the end of the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param key			The database key for the file.
 */

void results_add_file(struct results_window *handle, unsigned key)
{
	unsigned		line, offv;
	unsigned		type;
	struct fileicon_info	icon;

	if (handle == NULL)
		return;

	/* Add the core filename line */

	line = results_add_line(handle, TRUE);
	if (line == RESULTS_NULL)
		return;

	//objdb_get_name(handle->objects, key, name, sizeof(name));
	type = objdb_get_filetype(handle->objects, key);

	//offt = textdump_store(handle->text, name);
	fileicon_get_type_icon(type, "", &icon);

	if (icon.small != TEXTDUMP_NULL) {
		offv = icon.small;
	} else if (icon.large != TEXTDUMP_NULL) {
		offv = icon.large;
		handle->redraw[line].flags |= RESULTS_FLAG_HALFSIZE;
	} else {
		offv = TEXTDUMP_NULL;
	}

	if (offv == TEXTDUMP_NULL)
		return;

	handle->redraw[line].type = RESULTS_LINE_FILENAME;
	handle->redraw[line].file = key;
	handle->redraw[line].sprite = offv;
	handle->redraw[line].flags |= RESULTS_FLAG_SELECTABLE;

	/* Add the file info line. */

	line = results_add_line(handle, handle->full_info);
	if (line == RESULTS_NULL)
		return;

	handle->redraw[line].type = RESULTS_LINE_FILEINFO;
	handle->redraw[line].file = key;

	/* \TODO
	 *
	 * Add size, date and attributes to the local text dump;
	 * Add filetype to the types dump.
	 */
}


/**
 * Reformat lines in the results window to take into account the current
 * display width.
 *
 * \param *handle		The handle of the results window to update.
 * \param all			TRUE to format all lines; FALSE to format
 *				only those added since the last update.
 */

void results_reformat(struct results_window *handle, osbool all)
{
	os_error		*error;
	int			line, width, length, pos;
	char			*text;
	char			truncate[1024]; // \TODO -- Allocate properly.

	if (handle == NULL)
		return;

	strcpy(truncate, "...");

	text = textdump_get_base(handle->text);

	width = handle->format_width - (2 * RESULTS_WINDOW_MARGIN) - RESULTS_ICON_WIDTH;

	for (line = (all) ? 0 : handle->formatted_lines; line < handle->redraw_lines; line++) {
		switch (handle->redraw[line].type) {
		case RESULTS_LINE_FILENAME:
			objdb_get_name(handle->objects, handle->redraw[line].file, truncate + 3, sizeof(truncate) - 3);

			if (wimptextop_string_width(truncate + 3, 0) <= width)
				break;

			length = strlen(truncate + 3);
			pos = 0;

			while (pos < length &&
					wimptextop_string_width(truncate + pos, 0) > width) {
					*(truncate + pos + 3) = '.';
					pos++;
			}

			if (pos > 0)
				handle->redraw[line].truncate = pos;
			break;

		case RESULTS_LINE_TEXT:
			if (wimptextop_string_width(text + handle->redraw[line].text, 0) <= width)
				break;

			strcpy(truncate + 3, text + handle->redraw[line].text);
			length = strlen(truncate + 3);
			pos = 0;

			while (pos < length &&
					wimptextop_string_width(truncate + pos, 0) > width) {
					*(truncate + pos + 3) = '.';
					pos++;
			}

			if (pos > 0)
				handle->redraw[line].truncate = pos;
			break;

		default:
			break;
		}
	}

	error = xwimp_force_redraw(handle->window,
			0, LINE_Y0(handle->display_lines - 1),
			handle->format_width, (all) ? LINE_Y1(0) : LINE_Y1(handle->formatted_lines));

	handle->formatted_lines = handle->redraw_lines;

	results_update_extent(handle);
}


/**
 * Update a results window index to show or hide the various categories of
 * line.
 *
 * \param *handle		The handle of the results window to update.
 * \param full_info		TRUE to include full info lines; else FALSE.
 */

static void results_set_display_mode(struct results_window *handle, osbool full_info)
{
	unsigned line, selection;

	if (handle == NULL || handle->full_info == full_info)
		return;

	/* Zero the displayed line count. */

	handle->display_lines = 0;

	/* If there's a single selection, get the real line that is currently
	 * selected.
	 */

	if (handle->selection_count == 1)
		selection = handle->redraw[handle->selection_row].index;
	else
		selection = RESULTS_ROW_NONE;

	/* Re-index the window contents. */

	for (line = 0; line < handle->redraw_lines; line++) {
		if (line == selection)
			handle->selection_row = handle->display_lines;

		switch (handle->redraw[line].type) {
		case RESULTS_LINE_TEXT:
		case RESULTS_LINE_FILENAME:
		case RESULTS_LINE_CONTENTS:
			handle->redraw[handle->display_lines++].index = line;
			break;

		case RESULTS_LINE_FILEINFO:
			if (full_info)
				handle->redraw[handle->display_lines++].index = line;
			break;

		default:
			break;
		}
	}

	handle->full_info = full_info;

	results_update_extent(handle);
	windows_redraw(handle->window);
}


/**
 * Update the window extent to hold all of the defined lines.
 *
 * \param *handle		The handle of the results window to update.
 */

static void results_update_extent(struct results_window *handle)
{
	wimp_window_info	info;
	unsigned		lines;

	if (handle == NULL)
		return;

	info.w = handle->window;
	if (xwimp_get_window_info_header_only(&info) != NULL)
		return;

	lines = (handle->display_lines > RESULTS_MIN_LINES) ? handle->display_lines : RESULTS_MIN_LINES;
	info.extent.y0 = -((lines * RESULTS_LINE_HEIGHT) + RESULTS_TOOLBAR_HEIGHT + RESULTS_STATUS_HEIGHT);

	xwimp_set_extent(handle->window, &(info.extent));
}


/**
 * Claim a new line from the redraw array, allocating more memory if required,
 * set it up and return its offset.
 *
 * \param *handle		The handle of the results window.
 * \param show			TRUE if the line is always visible; FALSE if
 *				only to be shown on 'Full Info' display.
 * \return			The offset of the new line, or RESULTS_NULL.
 */

static unsigned results_add_line(struct results_window *handle, osbool show)
{
	unsigned offset;

	if (handle == NULL)
		return RESULTS_NULL;

	/* Make sure that there is enough space in the block to take the new
	 * line, allocating more if required.
	 */

	if (handle->redraw_lines >= handle->redraw_size) {
		if (flex_extend((flex_ptr) &(handle->redraw), (handle->redraw_size + RESULTS_ALLOC_REDRAW) * sizeof(struct results_line)) == 0)
			return RESULTS_NULL;

		handle->redraw_size += RESULTS_ALLOC_REDRAW;
	}

	/* Get the new line and initialise it. */

	offset = handle->redraw_lines++;

	handle->redraw[offset].type = RESULTS_LINE_NONE;
	handle->redraw[offset].flags = RESULTS_FLAG_NONE;
	handle->redraw[offset].parent = RESULTS_NULL;
	handle->redraw[offset].text = RESULTS_NULL;
	handle->redraw[offset].file = OBJDB_NULL_KEY;
	handle->redraw[offset].sprite = RESULTS_NULL;
//	handle->redraw[offset].size = RESULTS_NULL;
//	handle->redraw[offset].type = RESULTS_NULL;
//	handle->redraw[offset].attributes = RESULTS_NULL;
//	handle->redraw[offset].date = RESULTS_NULL;
	handle->redraw[offset].truncate = 0;
	handle->redraw[offset].colour = wimp_COLOUR_BLACK;

	/* If the line is for immediate display, add it to the index. */

	if (show || handle->full_info)
		handle->redraw[handle->display_lines++].index = offset;

	return offset;
}


/**
 * Calculate the row that the mouse was clicked over in a results window.
 *
 * \param  *handle		The handle of the results window.
 * \param  *pointer		The Wimp pointer data.
 * \param  *state		The results window state.
 * \return			The row (from 0) or RESULTS_ROW_NONE if none.
 */

static unsigned results_calculate_window_click_row(struct results_window *handle, os_coord *pos, wimp_window_state *state)
{
	int		y, row_y_pos;
	unsigned	row;

	if (handle == NULL || state == NULL)
		return -1;

	y = state->visible.y1 - pos->y - state->yscroll;

	row = (unsigned) (y - RESULTS_TOOLBAR_HEIGHT - RESULTS_WINDOW_MARGIN) / RESULTS_LINE_HEIGHT;
	row_y_pos = ((y - RESULTS_TOOLBAR_HEIGHT - RESULTS_WINDOW_MARGIN) % RESULTS_LINE_HEIGHT);

	if (row >= handle->display_lines ||
			row_y_pos < (RESULTS_LINE_HEIGHT - (RESULTS_LINE_OFFSET + RESULTS_ICON_HEIGHT)) ||
			row_y_pos > (RESULTS_LINE_HEIGHT - RESULTS_LINE_OFFSET))
		row = RESULTS_ROW_NONE;

	return row;
}


/**
 * Update the current selection based on a select click over a row of the
 * table.
 *
 * \param *handle		The handle of the results window.
 * \param row			The row under the click, or RESULTS_ROW_NONE.
 */

static void results_select_click_select(struct results_window *handle, unsigned row)
{
	wimp_window_state	window;

	if (handle == NULL)
		return;

	/* If the click is on a selection, nothing changes. */

	if ((row < handle->display_lines) && (handle->redraw[handle->redraw[row].index].flags & RESULTS_FLAG_SELECTED))
		return;

	/* Clear everything and then try to select the clicked line. */

	results_select_none(handle);

	window.w = handle->window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	if ((row < handle->display_lines) && (handle->redraw[handle->redraw[row].index].flags & RESULTS_FLAG_SELECTABLE)) {
		handle->redraw[handle->redraw[row].index].flags |= RESULTS_FLAG_SELECTED;
		handle->selection_count++;
		if (handle->selection_count == 1)
			handle->selection_row = row;

		wimp_force_redraw(window.w, window.xscroll, LINE_BASE(row),
				window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(row));
	}
}


/**
 * Update the current selection based on an adjust click over a row of the
 * table.
 *
 * \param *handle		The handle of the results window.
 * \param row			The row under the click, or RESULTS_ROW_NONE.
 */

static void results_select_click_adjust(struct results_window *handle, unsigned row)
{
	int			i;
	wimp_window_state	window;

	if (handle == NULL || row >= handle->display_lines || (handle->redraw[handle->redraw[row].index].flags & RESULTS_FLAG_SELECTABLE) == 0)
		return;

	window.w = handle->window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	if (handle->redraw[handle->redraw[row].index].flags & RESULTS_FLAG_SELECTED) {
		handle->redraw[handle->redraw[row].index].flags &= ~RESULTS_FLAG_SELECTED;
		handle->selection_count--;
		if (handle->selection_count == 1) {
			for (i = 0; i < handle->display_lines; i++) {
				if (handle->redraw[handle->redraw[i].index].flags & RESULTS_FLAG_SELECTED) {
					handle->selection_row = i;
					break;
				}
			}
		}
	} else {
		handle->redraw[handle->redraw[row].index].flags |= RESULTS_FLAG_SELECTED;
		handle->selection_count++;
		if (handle->selection_count == 1)
			handle->selection_row = row;
	}

	wimp_force_redraw(window.w, window.xscroll, LINE_BASE(row),
			window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(row));
}


/**
 * Select all of the rows in a results window.
 *
 * \param *handle		The handle of the results window.
 */

static void results_select_all(struct results_window *handle)
{
	int			i;
	wimp_window_state	window;

	if (handle == NULL || handle->selection_count == handle->display_lines)
		return;

	window.w = handle->window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	for (i = 0; i < handle->display_lines; i++) {
		if ((handle->redraw[handle->redraw[i].index].flags & (RESULTS_FLAG_SELECTABLE | RESULTS_FLAG_SELECTED)) == RESULTS_FLAG_SELECTABLE) {
			handle->redraw[handle->redraw[i].index].flags |= RESULTS_FLAG_SELECTED;

			handle->selection_count++;
			if (handle->selection_count == 1)
				handle->selection_row = i;

			wimp_force_redraw(window.w, window.xscroll, LINE_BASE(i),
					window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(i));
		}
	}
}


/**
 * Clear the selection in a results window.
 *
 * \param *handle		The handle of the results window.
 */

static void results_select_none(struct results_window *handle)
{
	int			i;
	wimp_window_state	window;

	if (handle == NULL || handle->selection_count == 0)
		return;

	window.w = handle->window;
	if (xwimp_get_window_state(&window) != NULL)
		return;

	/* If there's just one row selected, we can avoid looping through the lot
	 * by just clearing that one line.
	 */

	if (handle->selection_count == 1) {
		if (handle->selection_row < handle->display_lines)
			handle->redraw[handle->redraw[handle->selection_row].index].flags &= ~RESULTS_FLAG_SELECTED;
		handle->selection_count = 0;

		wimp_force_redraw(window.w, window.xscroll, LINE_BASE(handle->selection_row),
				window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(handle->selection_row));

		return;
	}

	/* If there is more than one row selected, we must loop through the lot
	 * to clear them all.
	 */

	for (i = 0; i < handle->display_lines; i++) {
		if (handle->redraw[handle->redraw[i].index].flags & RESULTS_FLAG_SELECTED) {
			handle->redraw[handle->redraw[i].index].flags &= ~RESULTS_FLAG_SELECTED;

			wimp_force_redraw(window.w, window.xscroll, LINE_BASE(i),
					window.xscroll + (window.visible.x1 - window.visible.x0), LINE_Y1(i));
		}
	}

	handle->selection_count = 0;
}


/**
 * Filer_Run an object in the results window.
 *
 * \param *handle		The handle of the results window to use.
 * \param row			The row of the window to use.
 */

static void results_run_object(struct results_window *handle, unsigned row)
{
	char		filename[256];

	if (handle == NULL || row >= handle->display_lines)
		return;

	row = handle->redraw[row].index;

	if (row >= handle->redraw_lines || handle->redraw[row].type != RESULTS_LINE_FILENAME || handle->redraw[row].file == OBJDB_NULL_KEY)
		return;

	strcpy(filename, "Filer_Run ");

	if (!objdb_get_name(handle->objects, handle->redraw[row].file, filename + strlen(filename), sizeof(filename) - strlen(filename)))
		return;

	xos_cli(filename);
}


/**
 * Open the directory containing an object in the results window.
 *
 * \param *handle		The handle of the results window to use.
 * \param row			The row of the window to use.
 */

static void results_open_parent(struct results_window *handle, unsigned row)
{
	char		filename[256];
	unsigned	key;

	if (handle == NULL || row >= handle->display_lines)
		return;

	row = handle->redraw[row].index;

	if (row >= handle->redraw_lines || handle->redraw[row].type != RESULTS_LINE_FILENAME)
		return;

	key = objdb_get_parent(handle->objects, handle->redraw[row].file);

	if (key == OBJDB_NULL_KEY)
		return;

	strcpy(filename, "Filer_OpenDir ");

	if (!objdb_get_name(handle->objects, key, filename + strlen(filename), sizeof(filename) - strlen(filename)))
		return;

	xos_cli(filename);
}


/**
 * Prepare the contents of the object info dialogue.
 *
 * \param *handle		The handle of the results window to prepare for.
 */

static void results_object_info_prepare(struct results_window *handle)
{
	char			*base, *end;
	unsigned		row, type;
	bits			data[256];
	osgbpb_info		*file = (osgbpb_info *) data;
	struct fileicon_info	info;


	if (handle == NULL || handle->selection_count != 1 || handle->selection_row >= handle->display_lines)
		return;

	row = handle->redraw[handle->selection_row].index;

	if (row >= handle->redraw_lines || handle->redraw[row].type != RESULTS_LINE_FILENAME)
		return;

	/* Get the data. */

	objdb_get_info(handle->objects, handle->redraw[row].file, file);
	type = objdb_get_filetype(handle->objects, handle->redraw[row].file);
	fileicon_get_type_icon(type, "", &info);

	base = fileicon_get_base();

	/* Complete the fields. */

	icons_printf(results_object_window, RESULTS_OBJECT_ICON_NAME, "%s", file->name);

	if (xos_convert_file_size(file->size,
			icons_get_indirected_text_addr(results_object_window, RESULTS_OBJECT_ICON_SIZE),
			icons_get_indirected_text_length(results_object_window, RESULTS_OBJECT_ICON_SIZE),
			&end) != NULL)
		icons_printf(results_object_window, RESULTS_OBJECT_ICON_SIZE, "");

	results_create_attributes_string(file->attr,
			icons_get_indirected_text_addr(results_object_window, RESULTS_OBJECT_ICON_ACCESS),
			icons_get_indirected_text_length(results_object_window, RESULTS_OBJECT_ICON_ACCESS));

	if (info.name != TEXTDUMP_NULL)
		icons_printf(results_object_window, RESULTS_OBJECT_ICON_TYPE, "%s", base + info.name);

	if (info.large != TEXTDUMP_NULL)
		icons_printf(results_object_window, RESULTS_OBJECT_ICON_ICON, "%s", base + info.large);

	results_create_address_string(file->load_addr, file->exec_addr,
				icons_get_indirected_text_addr(results_object_window, RESULTS_OBJECT_ICON_DATE),
				icons_get_indirected_text_length(results_object_window, RESULTS_OBJECT_ICON_DATE));
}


/**
 * Turn fileswitch object attributes into a human-readable string.
 *
 * \param attributes		The file attributes to be decoded.
 * \param *buffer		A buffer into which to write the decoded string.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			Pointer to the start of the supplied buffer.
 */

static char *results_create_attributes_string(fileswitch_attr attributes, char *buffer, size_t length)
{
	snprintf(buffer, length, "%s%s%s%s/%s%s%s%s",
			(attributes & fileswitch_ATTR_OWNER_WRITE) ? "W" : "",
			(attributes & fileswitch_ATTR_OWNER_READ) ? "R" : "",
			(attributes & fileswitch_ATTR_OWNER_LOCKED) ? "L" : "",
			(attributes & fileswitch_ATTR_OWNER_SPECIAL) ? "S" : "",
			(attributes & fileswitch_ATTR_WORLD_WRITE) ? "w" : "",
			(attributes & fileswitch_ATTR_WORLD_READ) ? "r" : "",
			(attributes & fileswitch_ATTR_WORLD_LOCKED) ? "l" : "",
			(attributes & fileswitch_ATTR_WORLD_SPECIAL) ? "s" : "");

	return buffer;
}


/**
 * Turn load and exec address object attributes into a human-readable string
 * showing either date or addresses.
 *
 * \param attributes		The file attributes to be decoded.
 * \param *buffer		A buffer into which to write the decoded string.
 * \param length		The size of the supplied buffer, in bytes.
 * \return			Pointer to the start of the supplied buffer.
 */

static char *results_create_address_string(unsigned load_addr, unsigned exec_addr, char *buffer, size_t length)
{
	os_date_and_time	date;
	char			*end;

	if ((load_addr & 0xfff00000u) == 0xfff00000u) {
		datetime_set_date(date, load_addr, exec_addr);

		if (xterritory_convert_standard_date_and_time(territory_CURRENT, (const os_date_and_time *) date,
				buffer, length, &end) == NULL)
			*end = '\0';
		else
			*buffer = '\0';
	} else {
		snprintf(buffer, length, "%08X %08X", load_addr, exec_addr);
	}

	return buffer;
}

