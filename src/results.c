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

#include "clipboard.h"
#include "dataxfer.h"
#include "datetime.h"
#include "dialogue.h"
#include "discfile.h"
#include "file.h"
#include "fileicon.h"
#include "ihelp.h"
#include "objdb.h"
#include "saveas.h"
#include "templates.h"
#include "textdump.h"


#define STATUS_LENGTH 128							/**< The maximum size of the status bar text field.			*/
#define TITLE_LENGTH 256							/**< The maximum size of the undefined title bar text field. 		*/

#define RESULTS_TOOLBAR_HEIGHT 0						/**< The height of the toolbar in OS units.				*/
#define RESULTS_LINE_HEIGHT 56							/**< The height of a results line, in OS units.				*/
#define RESULTS_WINDOW_MARGIN 4							/**< The margin around the edge of the window, in OS units.		*/
#define RESULTS_LINE_OFFSET 4							/**< The offset from the base of a line to the base of the icon.	*/
#define RESULTS_ICON_HEIGHT 52							/**< The height of an icon in the results window, in OS units.		*/
#define RESULTS_STATUS_HEIGHT 60						/**< The height of the status bar in OS units.				*/
#define RESULTS_ICON_WIDTH 50							/**< The width of a small file icon in OS units.			*/

#define RESULTS_MIN_LINES 10							/**< The minimum number of lines to show in the results window.		*/

#define RESULTS_AUTOSCROLL_BORDER 80						/**< The autoscroll border for the window, in OS units.			*/

#define RESULTS_ALLOC_REDRAW 50							/**< Number of window redraw blocks to allocate at a time.		*/
#define RESULTS_ALLOC_FILES 50							/**< Number of file blocks to allocate at a time.			*/
#define RESULTS_ALLOC_TEXT 1024							/**< Number of bytes of text storage to allocate at a time.		*/
#define RESULTS_ALLOC_CLIPBOARD 1024						/**< Number of bytes of clipboard storage to allocate at a time.	*/

#define RESULTS_ROW_NONE ((unsigned) 0xffffffff)				/**< Row selection value to indicate "No Row".				*/

#define RESULTS_REDRAW_SIZE_LEN 32						/**< Space allocated for building textual file size strings.		*/
#define RESULTS_REDRAW_ATTRIBUTES_LEN 32					/**< Space allocated for building textual attribute strings.		*/
#define RESULTS_REDRAW_DATE_LEN 64						/**< Space allocated for building textual dates.			*/

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


/**
 * Data structures.
 *
 * enum results_line_type gets saved into files, so values must remain constant
 * between builds.
 */

enum results_line_type {
	RESULTS_LINE_NONE = 0,							/**< A blank line (nothing plotted).					*/
	RESULTS_LINE_TEXT = 1,							/**< A line containing unspecific text.					*/
	RESULTS_LINE_FILENAME = 2,						/**< A line containing a filename.					*/
	RESULTS_LINE_FILEINFO = 3,						/**< A line containing file information.				*/
	RESULTS_LINE_CONTENTS = 4						/**< A line containing a file contents match.				*/
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
	unsigned		file;						/**< Object key for file objects.					*/

	enum fileicon_icons	sprite;						/**< Fileicon sprite id for the icon's sprite.				*/

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

	unsigned		longest_line;					/**< The length of the longest text to be redrawn directly.		*/

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


/** File data structure for saving results lines. */

struct results_file_block {
	enum results_line_type	type;						/**< The type of line.							*/

	enum results_line_flags	flags;						/**< Any flags relating to the line.					*/

	unsigned		parent;						/**< The parent line for a group (points to itself for the parent).	*/

	unsigned		data;						/**< Data (text offset; object key) for the line.			*/
	enum fileicon_icons	sprite;						/**< Fileicon ID for the display icon's sprite.				*/

	wimp_colour		colour;						/**< The foreground colour of the text.					*/
};


/* Global variables. */

static wimp_window		*results_window_def = NULL;			/**< Definition for the main results window.				*/
static wimp_window		*results_status_def = NULL;			/**< Definition for the results status pane.				*/

static wimp_w			results_object_window = NULL;			/**< Handle of the object info window.					*/

static wimp_menu		*results_window_menu = NULL;			/**< The results window menu.						*/
static wimp_menu		*results_window_menu_display = NULL;		/**< The results window display submenu.				*/

static osspriteop_area		*results_sprite_area = NULL;			/**< The application sprite area.					*/

static struct saveas_block	*results_save_results = NULL;			/**< The Save Results savebox data handle.				*/
static struct saveas_block	*results_save_paths = NULL;			/**< The Save Paths savebox data handle.				*/
static struct saveas_block	*results_save_options = NULL;			/**< The Save Options savebox data handle.				*/

static struct textdump_block	*results_clipboard = NULL;			/**< Text Dump for the clipboard contents.				*/

static struct results_window	*results_select_drag_handle = NULL;		/**< The handle of the results window contining the selection drag.	*/
static unsigned			results_select_drag_row = RESULTS_ROW_NONE;	/**< The row in which the selection drag started.			*/
static unsigned			results_select_drag_pos = 0;			/**< The position within the row where the selection drag started.	*/
static osbool			results_select_drag_adjust = FALSE;		/**< TRUE if the selection drag is with Adjust; FALSE for Select.	*/


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
static void	results_add_raw(struct results_window *handle, enum results_line_type type, unsigned message, wimp_colour colour, enum fileicon_icons sprite);
static unsigned	results_add_line(struct results_window *handle, osbool show);
static osbool	results_extend(struct results_window *handle, unsigned lines);
static unsigned	results_calculate_window_click_row(struct results_window *handle, os_coord *pos, wimp_window_state *state);
static void	results_drag_select(struct results_window *handle, unsigned row, wimp_pointer *pointer, wimp_window_state *state, osbool ctrl_pressed);
static void	results_xfer_drag_end_handler(wimp_pointer *pointer, void *data);
static void	results_select_drag_end_handler(wimp_dragged *drag, void *data);
static void	results_select_click_select(struct results_window *handle, unsigned row);
static void	results_select_click_adjust(struct results_window *handle, unsigned row);
static void	results_select_all(struct results_window *handle);
static void	results_select_none(struct results_window *handle);
static void	results_run_object(struct results_window *handle, unsigned row);
static void	results_open_parent(struct results_window *handle, unsigned row);
static void	results_object_info_prepare(struct results_window *handle);
static char	*results_create_attributes_string(fileswitch_attr attributes, char *buffer, size_t length);
static char	*results_create_address_string(unsigned load_addr, unsigned exec_addr, char *buffer, size_t length);
static osbool	results_save_result_data(char *filename, osbool selection, void *data);
static osbool	results_save_dialogue_data(char *filename, osbool selection, void *data);
static osbool	results_save_filenames(char *filename, osbool selection, void *data);
static void	results_clipboard_copy_filenames(struct results_window *handle);
static void	*results_clipboard_find(void *data);
static size_t	results_clipboard_size(void *data);
static void	results_clipboard_release(void *data);


//static unsigned	results_add_fileblock(struct results_window *handle);


/* Line position calculations.
 *
 * NB: These can be called with lines < 0 to give lines off the top of the window!
 */

#define LINE_BASE(x) (-((x)+1) * RESULTS_LINE_HEIGHT - RESULTS_TOOLBAR_HEIGHT - RESULTS_WINDOW_MARGIN)
#define LINE_Y0(x) (LINE_BASE(x) + RESULTS_LINE_OFFSET)
#define LINE_Y1(x) (LINE_BASE(x) + RESULTS_LINE_OFFSET + RESULTS_ICON_HEIGHT)

/* Row calculations: taking a positive offset from the top of the window, return
 * the raw row number and the position within a row.
 */

#define ROW(y) (((-(y)) - RESULTS_TOOLBAR_HEIGHT - RESULTS_WINDOW_MARGIN) / RESULTS_LINE_HEIGHT)
#define ROW_Y_POS(y) (((-(y)) - RESULTS_TOOLBAR_HEIGHT - RESULTS_WINDOW_MARGIN) % RESULTS_LINE_HEIGHT)

/* Return true or false if a ROW_Y_POS() value is above or below the icon
 * area of the row.
 */

#define ROW_ABOVE(y) ((y) < (RESULTS_LINE_HEIGHT - (RESULTS_LINE_OFFSET + RESULTS_ICON_HEIGHT)))
#define ROW_BELOW(y) ((y) > (RESULTS_LINE_HEIGHT - RESULTS_LINE_OFFSET))



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

	results_save_results = saveas_create_dialogue(FALSE, "file_1a1", results_save_result_data);
	results_save_paths = saveas_create_dialogue(TRUE, "file_fff", results_save_filenames);
	results_save_options = saveas_create_dialogue(FALSE, "file_1a1", results_save_dialogue_data);

	results_clipboard = textdump_create(RESULTS_ALLOC_CLIPBOARD, 0, '\n');
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
		if ((new->text = textdump_create(RESULTS_ALLOC_TEXT, 0, '\0')) == NULL)
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

	new->longest_line = 0;

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
 * Save the contents of a results window into an open discfile.
 *
 * \param *handle		The results window to be saved.
 * \param *out			The discfile to write to.
 * \return			TRUE on success; FALSE on failure.
 */

osbool results_save_file(struct results_window *handle, struct discfile_block *out)
{
	struct results_file_block	block;
	int				i;
	char				*title;

	if (handle == NULL || out == NULL)
		return FALSE;

	title = windows_get_indirected_title_addr(handle->window);

	if (title == NULL)
		return FALSE;

	discfile_start_section(out, DISCFILE_SECTION_RESULTS);

	/* Write out the results options. */

	discfile_start_chunk(out, DISCFILE_CHUNK_OPTIONS);
	discfile_write_option_unsigned(out, "LIN", handle->redraw_lines);
	discfile_write_option_boolean(out, "FUL", handle->full_info);
	discfile_write_option_unsigned(out, "LEN", handle->longest_line);
	discfile_write_option_string(out, "TIT", title);
	discfile_end_chunk(out);

	/* Write the results line data. */

	discfile_start_chunk(out, DISCFILE_CHUNK_RESULTS);
	for (i = 0; i < handle->redraw_lines; i++) {
		if (handle->redraw[i].type != RESULTS_LINE_TEXT && handle->redraw[i].type != RESULTS_LINE_FILENAME)
			continue;

		block.type = handle->redraw[i].type;
		block.flags = handle->redraw[i].flags;
		block.parent = handle->redraw[i].parent;
		block.colour = handle->redraw[i].colour;

		switch (handle->redraw[i].type) {
		case RESULTS_LINE_TEXT:
			block.data = handle->redraw[i].text;
			block.sprite = handle->redraw[i].sprite;
			break;

		case RESULTS_LINE_FILENAME:
			block.data = handle->redraw[i].file;
			block.sprite = handle->redraw[i].sprite;
			break;

		default:
			block.data = RESULTS_NULL;
			block.sprite = RESULTS_NULL;
			break;
		}

		discfile_write_chunk(out, (byte *) &block, sizeof(struct results_file_block));
	}
	discfile_end_chunk(out);

	/* Write out the textdump data. */

	textdump_save_file(handle->text, out);

	discfile_end_section(out);

	return TRUE;
}

/**
 * Load results data from a file and create a results window from it.
 *
 * \param *file			The file block to which the window belongs.
 * \param *objects		The boject database from which file data should be taken.
 * \param *load			The discfile from which to load the details
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_load_file(struct file_block *file, struct objdb_block *objects, struct discfile_block *load)
{
	struct results_window		*new;
	struct results_file_block	data;
	char				title[TITLE_LENGTH];
	int				i, size, position;
	unsigned			lines;

	if (file == NULL || objects == NULL || load == NULL)
		return NULL;

	if (discfile_read_format(load) != DISCFILE_LOCATE2)
		return NULL;

	/* Open the results window section of the file. */

	if (!discfile_open_section(load, DISCFILE_SECTION_RESULTS))
		return NULL;

	/* Create a new results window. */

	new = results_create(file, objects, NULL);

	if (new == NULL) {
		discfile_set_error(load, "FileMem");
		return NULL;
	}

	/* Load the results window settings details. */

	if (discfile_open_chunk(load, DISCFILE_CHUNK_OPTIONS)) {
		if (!discfile_read_option_unsigned(load, "LIN", &lines) ||
				!discfile_read_option_string(load, "TIT", title, TITLE_LENGTH) ||
				!discfile_read_option_boolean(load, "FUL", &new->full_info) ||
				!discfile_read_option_unsigned(load, "LEN", &new->longest_line)) {
			discfile_set_error(load, "FileUnrec");
			results_destroy(new);
			return NULL;
		}

		discfile_close_chunk(load);

		if (lines > new->redraw_size)
			results_extend(new, lines);

		if (lines > new->redraw_size) {
			discfile_set_error(load, "FileMem");
			results_destroy(new);
			return NULL;
		}

		results_set_title(new, title);
	} else {
		discfile_set_error(load, "FileUnrec");
		results_destroy(new);
		return NULL;
	}

	/* Load the window lines into the window. */

	if (discfile_open_chunk(load, DISCFILE_CHUNK_RESULTS)) {
		size = discfile_chunk_size(load);
		if ((size % sizeof(struct results_file_block)) != 0) {
			discfile_set_error(load, "FileUnrec");
			results_destroy(new);
			return NULL;
		}

		position = sizeof(struct results_file_block);

		for (i = 0; i < lines && position <= size; i++) {
			discfile_read_chunk(load, (byte *) &data, sizeof(struct results_file_block));

			switch (data.type) {
			case RESULTS_LINE_FILENAME:
				results_add_file(new, data.data);
				break;
			case RESULTS_LINE_TEXT:
				results_add_raw(new, RESULTS_LINE_TEXT, data.data, data.colour, data.sprite);
				break;
			default:
				break;
			}

			position += sizeof(struct results_file_block);
		}

		discfile_close_chunk(load);
	} else {
		discfile_set_error(load, "FileUnrec");
		results_destroy(new);
		return NULL;
	}

	/* Load the textdump contents into memory. */

	if (!textdump_load_file(new->text, load)) {
		discfile_set_error(load, "FileUnrec");
		results_destroy(new);
		return NULL;
	}

	/* Close the results section of the file. */

	discfile_close_section(load);

	/* Reformat the loaded lines in the window. */

	results_reformat(new, TRUE);

	return new;
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
	osbool			ctrl_pressed;

	if (handle == NULL || pointer== NULL)
		return;

	ctrl_pressed = ((osbyte1(osbyte_IN_KEY, 0xf0, 0xff) == 0xff) || (osbyte1(osbyte_IN_KEY, 0xfb, 0xff) == 0xff)) ? TRUE : FALSE;

	state.w = pointer->w;
	if (xwimp_get_window_state(&state) != NULL)
		return;

	row = results_calculate_window_click_row(handle, &(pointer->pos), &state);

	switch(pointer->buttons) {
	case wimp_SINGLE_SELECT:
		if (!ctrl_pressed)
			results_select_click_select(handle, row);
		break;

	case wimp_SINGLE_ADJUST:
		if (!ctrl_pressed)
			results_select_click_adjust(handle, row);
		break;

	case wimp_DOUBLE_SELECT:
		if (!ctrl_pressed) {
			results_select_none(handle);
			results_run_object(handle, row);
		}
		break;

	case wimp_DOUBLE_ADJUST:
		if (!ctrl_pressed) {
			results_select_click_adjust(handle, row);
			results_open_parent(handle, row);
		}
		break;

	case wimp_DRAG_SELECT:
	case wimp_DRAG_ADJUST:
		results_drag_select(handle, row, pointer, &state, ctrl_pressed);
		break;
	}
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
	menus_shade_entry(results_window_menu, RESULTS_MENU_MODIFY_SEARCH, dialogue_window_is_open() || !file_has_dialogue(handle->file));
	menus_shade_entry(results_window_menu, RESULTS_MENU_STOP_SEARCH, !file_search_active(handle->file));

	menus_tick_entry(results_window_menu_display, RESULTS_MENU_DISPLAY_PATH_ONLY, !handle->full_info);
	menus_tick_entry(results_window_menu_display, RESULTS_MENU_DISPLAY_FULL_INFO, handle->full_info);

	saveas_initialise_dialogue(results_save_results, "FileName", NULL, TRUE, FALSE, handle);
	saveas_initialise_dialogue(results_save_paths, "ExptName", "SelectName", handle->selection_count > 0, handle->selection_count > 0, handle);
	saveas_initialise_dialogue(results_save_options, "SrchName", NULL, FALSE, FALSE, handle);
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
			saveas_prepare_dialogue(results_save_results);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		case RESULTS_MENU_SAVE_PATH_NAMES:
			saveas_prepare_dialogue(results_save_paths);
			wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
			break;
		case RESULTS_MENU_SAVE_SEARCH_OPTIONS:
			saveas_prepare_dialogue(results_save_options);
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

	case RESULTS_MENU_COPY_NAMES:
		results_clipboard_copy_filenames(handle);
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
	struct results_window	*handle;
	struct results_line	*line;
	struct fileicon_info	typeinfo;
	struct objdb_info	object;
	osgbpb_info		*file;
	wimp_icon		*icon;
	char			*text, *fileicon;
	char			validation[255];
	char			*truncation, *size, *attributes, *date;
	size_t			truncation_len;

	handle = (struct results_window *) event_get_window_user_data(redraw->w);

	if (handle == NULL)
		return;

	file = malloc(objdb_get_info(handle->objects, OBJDB_NULL_KEY, NULL, NULL));

	/* Identify the amount of buffer space required, by finding the largest of
	 * all of the uses it will be put to.
	 */

	truncation_len = objdb_get_name_length(handle->objects, OBJDB_NULL_KEY) + 3;
	if (truncation_len < handle->longest_line + 4)
		truncation_len = handle->longest_line + 4;
	if (truncation_len < (RESULTS_REDRAW_SIZE_LEN + RESULTS_REDRAW_ATTRIBUTES_LEN + RESULTS_REDRAW_DATE_LEN))
		truncation_len = RESULTS_REDRAW_SIZE_LEN + RESULTS_REDRAW_ATTRIBUTES_LEN + RESULTS_REDRAW_DATE_LEN;
	truncation = malloc(truncation_len);

	/* If file == NULL or truncation == NULL the redraw must go on, but some bits won't work. */

	if (truncation != NULL) {
		size = truncation;
		attributes = truncation + RESULTS_REDRAW_SIZE_LEN;
		date = attributes + RESULTS_REDRAW_ATTRIBUTES_LEN;
	} else {
		size = NULL;
		attributes = NULL;
		date = NULL;
	}

	icon = results_window_def->icons;

	/* Set up the validation string buffer for text+sprite icons. */

	*validation = 'S';
	icon[RESULTS_ICON_FILE].data.indirected_text.validation = validation;

	/* Set up the truncation line. */

	if (truncation != NULL)
		strcpy(truncation, "...");

	/* Redraw the window. */

	text = textdump_get_base(handle->text);
	fileicon = fileicon_get_base();

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	while (more) {
		top = (oy - redraw->clip.y1 - RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (top < 0)
			top = 0;

                bottom = ((RESULTS_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0 - RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (bottom > handle->display_lines)
			bottom = handle->display_lines;

		for (y = top; y < bottom; y++) {
			line = &(handle->redraw[handle->redraw[y].index]);

			switch (line->type) {
			case RESULTS_LINE_FILENAME:
				icon[RESULTS_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_FILE].extent.y1 = LINE_Y1(y);

				objdb_get_info(handle->objects, line->file, NULL, &object);
				fileicon_get_type_icon(object.filetype, "", &typeinfo);

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

				if (object.status == OBJDB_STATUS_UNCHANGED || object.status == OBJDB_STATUS_CHANGED)
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SHADED;
				else
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SHADED;

				if (truncation != NULL)
					objdb_get_name(handle->objects, line->file, truncation + 3, truncation_len - 3);

				if (truncation == NULL) {
					icon[RESULTS_ICON_FILE].data.indirected_text.text = "Redraw Error";
				} else if (line->truncate > 0) {
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

				objdb_get_info(handle->objects, line->file, file, &object);
				fileicon_get_type_icon(object.filetype, "", &typeinfo);

				if (typeinfo.name != TEXTDUMP_NULL) {
					icon[RESULTS_ICON_TYPE].data.indirected_text.text = fileicon + typeinfo.name;
				} else {
					icon[RESULTS_ICON_TYPE].data.indirected_text.text = "";
				}

				if (file == NULL)
					break;

				icon[RESULTS_ICON_SIZE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_SIZE].extent.y1 = LINE_Y1(y);
				icon[RESULTS_ICON_ATTRIBUTES].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_ATTRIBUTES].extent.y1 = LINE_Y1(y);
				icon[RESULTS_ICON_DATE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_DATE].extent.y1 = LINE_Y1(y);

				if (size != NULL && xos_convert_file_size(file->size, size, 32, NULL) != NULL)
					*size = '\0';

				if (attributes != NULL)
					results_create_attributes_string(file->attr, attributes, 32);

				if (date != NULL)
					results_create_address_string(file->load_addr, file->exec_addr, date, 100);

				icon[RESULTS_ICON_SIZE].data.indirected_text.text = (size != NULL) ? size : "Redraw Error";
				icon[RESULTS_ICON_ATTRIBUTES].data.indirected_text.text = (attributes != NULL) ? attributes : "Redraw Error";
				icon[RESULTS_ICON_DATE].data.indirected_text.text = (date != NULL) ? date : "Redraw Error";


				if (object.status == OBJDB_STATUS_UNCHANGED) {
					icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SHADED;
					icon[RESULTS_ICON_TYPE].flags &= ~wimp_ICON_SHADED;
					icon[RESULTS_ICON_ATTRIBUTES].flags &= ~wimp_ICON_SHADED;
					icon[RESULTS_ICON_DATE].flags &= ~wimp_ICON_SHADED;
				} else {
					icon[RESULTS_ICON_SIZE].flags |= wimp_ICON_SHADED;
					icon[RESULTS_ICON_TYPE].flags |= wimp_ICON_SHADED;
					icon[RESULTS_ICON_ATTRIBUTES].flags |= wimp_ICON_SHADED;
					icon[RESULTS_ICON_DATE].flags |= wimp_ICON_SHADED;
				}

				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_SIZE].flags |= line->colour << wimp_ICON_FG_COLOUR_SHIFT;
				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SELECTED;

				wimp_plot_icon(&(icon[RESULTS_ICON_SIZE]));
				wimp_plot_icon(&(icon[RESULTS_ICON_TYPE]));
				wimp_plot_icon(&(icon[RESULTS_ICON_ATTRIBUTES]));
				wimp_plot_icon(&(icon[RESULTS_ICON_DATE]));
				break;


			case RESULTS_LINE_CONTENTS:
				icon[RESULTS_ICON_SIZE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_SIZE].extent.y1 = LINE_Y1(y);

				if (truncation != NULL && line->truncate > 0) {
					strcpy(truncation + 3, text + line->text + line->truncate);
					icon[RESULTS_ICON_SIZE].data.indirected_text.text = truncation;
				} else {
					icon[RESULTS_ICON_SIZE].data.indirected_text.text = text + line->text;
				}
				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_SIZE].flags |= line->colour << wimp_ICON_FG_COLOUR_SHIFT;

				if (line->flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_SIZE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SELECTED;

				icon[RESULTS_ICON_SIZE].flags &= ~wimp_ICON_SHADED;

				wimp_plot_icon(&(icon[RESULTS_ICON_SIZE]));
				break;


			case RESULTS_LINE_TEXT:
				icon[RESULTS_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_FILE].extent.y1 = LINE_Y1(y);

				fileicon_get_special_icon(line->sprite, &typeinfo);

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

				if (truncation != NULL && line->truncate > 0) {
					strcpy(truncation + 3, text + line->text + line->truncate);
					icon[RESULTS_ICON_FILE].data.indirected_text.text = truncation;
				} else {
					icon[RESULTS_ICON_FILE].data.indirected_text.text = text + line->text;
				}
				icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_FG_COLOUR;
				icon[RESULTS_ICON_FILE].flags |= line->colour << wimp_ICON_FG_COLOUR_SHIFT;

				if (line->flags & RESULTS_FLAG_SELECTED)
					icon[RESULTS_ICON_FILE].flags |= wimp_ICON_SELECTED;
				else
					icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SELECTED;

				icon[RESULTS_ICON_FILE].flags &= ~wimp_ICON_SHADED;

				wimp_plot_icon(&(icon[RESULTS_ICON_FILE]));
				break;

			default:
				break;
			}
		}

		more = wimp_get_rectangle(redraw);
	}

	if (truncation != NULL)
		free(truncation);

	if (file != NULL)
		free(file);
}


/**
 * Set options for a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param full_info		TRUE to display the window in full info mode.
 */

void results_set_options(struct results_window *handle, osbool full_info)
{
	if (handle == NULL)
		return;

	results_set_display_mode(handle, full_info);
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
 * Add a raw line to the results window. This is for use by the file load
 * routines, and does not update variables such as the maximum line length.
 *
 * \param *handle		The handle of the results window to update.
 * \param type			The type of line to be added.
 * \param message		The error message text, as a textdump offset.
 * \param colour		The colour of the text.
 * \param sprite		The icon sprite to use.
 */

static void results_add_raw(struct results_window *handle, enum results_line_type type, unsigned message, wimp_colour colour, enum fileicon_icons sprite)
{
	unsigned		line;

	if (handle == NULL || message == TEXTDUMP_NULL)
		return;

	line = results_add_line(handle, TRUE);
	if (line == RESULTS_NULL)
		return;

	handle->redraw[line].type = type;
	handle->redraw[line].text = message;
	handle->redraw[line].sprite = sprite;
	handle->redraw[line].colour = colour;
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
	unsigned		line, offt, length;

	if (handle == NULL)
		return;

	line = results_add_line(handle, TRUE);
	if (line == RESULTS_NULL)
		return;

	offt = textdump_store(handle->text, message);

	if (offt == TEXTDUMP_NULL)
		return;

	handle->redraw[line].type = RESULTS_LINE_TEXT;
	handle->redraw[line].text = offt;
	handle->redraw[line].sprite = FILEICON_ERROR;
	handle->redraw[line].colour = wimp_COLOUR_RED;

	length = strlen(message) + 1;
	if (length > handle->longest_line)
		handle->longest_line = length;
}


/**
 * Add a file to the end of the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param key			The database key for the file.
 * \return			The results line, or RESULTS_NULL on failure.
 */

unsigned results_add_file(struct results_window *handle, unsigned key)
{
	unsigned		file, info;

	if (handle == NULL)
		return RESULTS_NULL;

	/* Add the core filename line */

	file = results_add_line(handle, TRUE);
	if (file == RESULTS_NULL)
		return RESULTS_NULL;

	handle->redraw[file].type = RESULTS_LINE_FILENAME;
	handle->redraw[file].file = key;
	handle->redraw[file].flags |= RESULTS_FLAG_SELECTABLE;

	/* Add the file info line. */

	info = results_add_line(handle, handle->full_info);
	if (info == RESULTS_NULL)
		return file;

	handle->redraw[info].type = RESULTS_LINE_FILEINFO;
	handle->redraw[info].file = key;
	handle->redraw[info].parent = file;

	return file;
}


/**
 * Add a piece of file content to the end of the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param key			The database key for the file.
 * \param parent		The parent line for the file.
 */

void results_add_contents(struct results_window *handle, unsigned key, unsigned parent, char *text)
{
	unsigned		line, offt, length;

	if (handle == NULL || parent == RESULTS_NULL)
		return;

	line = results_add_line(handle, handle->full_info);
	if (line == RESULTS_NULL)
		return;

	offt = textdump_store(handle->text, text);

	if (offt == TEXTDUMP_NULL)
		return;

	handle->redraw[line].type = RESULTS_LINE_CONTENTS;
	handle->redraw[line].text = offt;
	handle->redraw[line].sprite = FILEICON_ERROR;
	handle->redraw[line].colour = wimp_COLOUR_DARK_BLUE;
	handle->redraw[line].file = key;
	handle->redraw[line].parent = parent;

	length = strlen(text) + 1;
	if (length > handle->longest_line)
		handle->longest_line = length;
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
	size_t			truncate_len;
	char			*text, *truncate;

	if (handle == NULL)
		return;

	truncate_len = objdb_get_name_length(handle->objects, OBJDB_NULL_KEY) + 3;
	if (truncate_len < handle->longest_line + 4)
		truncate_len = handle->longest_line + 4;
	truncate = malloc(truncate_len);

	if (truncate == NULL)
		return;

	strcpy(truncate, "...");

	text = textdump_get_base(handle->text);

	width = handle->format_width - (2 * RESULTS_WINDOW_MARGIN) - RESULTS_ICON_WIDTH;

	for (line = (all) ? 0 : handle->formatted_lines; line < handle->redraw_lines; line++) {
		switch (handle->redraw[line].type) {
		case RESULTS_LINE_FILENAME:
			objdb_get_name(handle->objects, handle->redraw[line].file, truncate + 3, truncate_len - 3);

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

	free(truncate);

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
			handle->redraw[handle->display_lines++].index = line;
			break;

		case RESULTS_LINE_FILEINFO:
		case RESULTS_LINE_CONTENTS:
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

	if (handle->redraw_lines >= handle->redraw_size)
		results_extend(handle, handle->redraw_size + RESULTS_ALLOC_REDRAW);

	if (handle->redraw_lines >= handle->redraw_size)
		return RESULTS_NULL;

	/* Get the new line and initialise it. */

	offset = handle->redraw_lines++;

	handle->redraw[offset].type = RESULTS_LINE_NONE;
	handle->redraw[offset].flags = RESULTS_FLAG_NONE;
	handle->redraw[offset].parent = RESULTS_NULL;
	handle->redraw[offset].text = RESULTS_NULL;
	handle->redraw[offset].file = OBJDB_NULL_KEY;
	handle->redraw[offset].sprite = FILEICON_UNKNOWN;
	handle->redraw[offset].truncate = 0;
	handle->redraw[offset].colour = wimp_COLOUR_BLACK;

	/* If the line is for immediate display, add it to the index. */

	if (show || handle->full_info)
		handle->redraw[handle->display_lines++].index = offset;

	return offset;
}


/**
 * Extend the memory allocaton for a results window by the given number of
 * entries.
 *
 * \param *handle		The window to extend.
 * \param lines			The required number of lines in the window.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool results_extend(struct results_window *handle, unsigned lines)
{
	if (handle == NULL || handle->redraw == NULL || handle->redraw_size > lines)
		return FALSE;

	if (flex_extend((flex_ptr) &(handle->redraw), lines * sizeof(struct results_line)) != 1)
		return FALSE;

	handle->redraw_size = lines;

	return TRUE;
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
		return RESULTS_ROW_NONE;

	y = pos->y - state->visible.y1 + state->yscroll;

	row = (unsigned) ROW(y);
	row_y_pos = ROW_Y_POS(y);

	if (row >= handle->display_lines || ROW_ABOVE(row_y_pos) || ROW_BELOW(row_y_pos))
		row = RESULTS_ROW_NONE;

	return row;
}


/**
 * Process drags in a results window. Depending on whether the drag began on a
 * selectable row or not, the drag will either start a file transfer or start
 * a selection dragbox.
 *
 * \param *handle		The handle of the results window.
 * \param row			The row under the click, or RESULTS_ROW_NONE.
 * \param *pointer		Pointer to the current mouse info.
 * \param *state		Pointer to the current window state.
 * \param ctrl_pressed		TRUE if a Ctrl key is down; else FALSE.
 */

static void results_drag_select(struct results_window *handle, unsigned row, wimp_pointer *pointer, wimp_window_state *state, osbool ctrl_pressed)
{
	int			x, y;
	os_box			extent;
	unsigned		filetype;
	struct fileicon_info	icon;
	wimp_drag		drag;
	wimp_auto_scroll_info	scroll;
	char			*sprite = NULL;

	if (handle == NULL || pointer == NULL || state == NULL)
		return;

	x = pointer->pos.x - state->visible.x0 + state->xscroll;
	y = pointer->pos.y - state->visible.y1 + state->yscroll;

	if ((row != RESULTS_ROW_NONE) && (row < handle->display_lines) && (pointer->buttons == wimp_DRAG_SELECT) &&
			(handle->redraw[handle->redraw[row].index].flags & RESULTS_FLAG_SELECTABLE) && !ctrl_pressed) {
		extent.x0 = state->xscroll + RESULTS_WINDOW_MARGIN;
		extent.x1 = state->xscroll + (state->visible.x1 - state->visible.x0) - RESULTS_WINDOW_MARGIN;
		extent.y0 = LINE_Y0(row);
		extent.y1 = LINE_Y1(row);

		if (handle->selection_count == 1 && handle->selection_row == row) {
			filetype = objdb_get_filetype(handle->objects, handle->redraw[handle->redraw[row].index].file);
			fileicon_get_type_icon(filetype, "", &icon);

			if (icon.large != TEXTDUMP_NULL)
				sprite = fileicon_get_base() + icon.large;
			else
				sprite = "file_xxx";
		} else {
			sprite = "package";
		}

		dataxfer_work_area_drag(handle->window, pointer, &extent, sprite, results_xfer_drag_end_handler, handle);
	} else {
		results_select_drag_handle = handle;
		results_select_drag_row = ROW(y);
		results_select_drag_pos = ROW_Y_POS(y);
		results_select_drag_adjust = (pointer->buttons == wimp_DRAG_ADJUST) ? TRUE : FALSE;

		drag.w = handle->window;
		drag.type = wimp_DRAG_USER_RUBBER;

		drag.initial.x0 = pointer->pos.x;
		drag.initial.y0 = pointer->pos.y;
		drag.initial.x1 = pointer->pos.x;
		drag.initial.y1 = pointer->pos.y;

		drag.bbox.x0 = state->visible.x0;
		drag.bbox.y0 = state->visible.y0 + RESULTS_STATUS_HEIGHT;
		drag.bbox.x1 = state->visible.x1;
		drag.bbox.y1 = state->visible.y1 - RESULTS_TOOLBAR_HEIGHT;

		scroll.w = handle->window;

		scroll.pause_zone_sizes.x0 = RESULTS_AUTOSCROLL_BORDER;
		scroll.pause_zone_sizes.y0 = RESULTS_AUTOSCROLL_BORDER + RESULTS_STATUS_HEIGHT;
		scroll.pause_zone_sizes.x1 = RESULTS_AUTOSCROLL_BORDER;
		scroll.pause_zone_sizes.y1 = RESULTS_AUTOSCROLL_BORDER + RESULTS_TOOLBAR_HEIGHT;

		scroll.pause_duration = 0;
		scroll.state_change = wimp_AUTO_SCROLL_DEFAULT_HANDLER;

		wimp_drag_box_with_flags(&drag, wimp_DRAG_BOX_KEEP_IN_LINE | wimp_DRAG_BOX_CLIP);
		wimp_auto_scroll(wimp_AUTO_SCROLL_ENABLE_VERTICAL, &scroll);

		event_set_drag_handler(results_select_drag_end_handler, NULL, handle);
	}
}


/**
 * Process the termination of transfer drags from a results window by sending
 * Message_DataLoad for each selected file to the potential recipient.
 *
 * \param *pointer		The pointer location at the end of the drag.
 * \param *data			The results_window data for the drag.
 */

static void results_xfer_drag_end_handler(wimp_pointer *pointer, void *data)
{
	struct results_window	*handle = (struct results_window *) data;
	osgbpb_info		*info;
	struct objdb_info	object;
	unsigned		row;
	size_t			pathname_len;
	char			*pathname;


	if (handle == NULL)
		return;

	info = malloc(objdb_get_info(handle->objects, OBJDB_NULL_KEY, NULL, NULL));
	if (info == NULL)
		return;

	pathname_len = objdb_get_name_length(handle->objects, OBJDB_NULL_KEY);
	pathname = malloc(pathname_len);
	if (pathname == NULL) {
		free(info);
		return;
	}

	for (row = 0; row < handle->display_lines; row++) {
		if (handle->redraw[handle->redraw[row].index].type != RESULTS_LINE_FILENAME ||
				!(handle->redraw[handle->redraw[row].index].flags & RESULTS_FLAG_SELECTED))
			continue;

		objdb_get_name(handle->objects, handle->redraw[handle->redraw[row].index].file, pathname, pathname_len);
		objdb_get_info(handle->objects, handle->redraw[handle->redraw[row].index].file, info, &object);

		dataxfer_start_load(pointer, pathname, info->size, object.filetype, 0);
	}

	free(pathname);
	free(info);
}


/**
 * Process the termination of selection drags from a results window.
 *
 * \param  *drag		The Wimp poll block from termination.
 * \param  *data		NULL (unused).
 */

static void results_select_drag_end_handler(wimp_dragged *drag, void *data)
{
	int			y, row_y_pos;
	unsigned		row, start, end;
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

	state.w = results_select_drag_handle->window;
	error = xwimp_get_window_state(&state);
	if (error != NULL)
		return;

	/* Calculate the row stats for the drag end. */

	y = pointer.pos.y - state.visible.y1 + state.yscroll;

	row = ROW(y);
	row_y_pos = ROW_Y_POS(y);

	/* Work out the range of rows to be affected. */

	if (row > results_select_drag_row) {
		start = results_select_drag_row;
		if (ROW_BELOW(results_select_drag_pos))
			start++;

		end = row;
		if (ROW_ABOVE(row_y_pos))
			end --;
	} else if (row < results_select_drag_row) {
		start = row;
		if (ROW_BELOW(row_y_pos))
			start++;

		end = results_select_drag_row;
		if (ROW_ABOVE(results_select_drag_pos))
			end --;
	} else if (!((ROW_ABOVE(row_y_pos) && ROW_ABOVE(results_select_drag_pos)) || (ROW_BELOW(row_y_pos) && ROW_BELOW(results_select_drag_pos)))) {
		start = row;
		end = row;
	} else {
		start = RESULTS_ROW_NONE;
		end = RESULTS_ROW_NONE;
	}

	/* Get out if the range isn't valid. The (end < start) state implies
	 * adjacent ABOVE/BELOW bands, which means no icons have been included.
	 */

	if (start == RESULTS_ROW_NONE || end == RESULTS_ROW_NONE || end < start)
		return;

	if (!results_select_drag_adjust)
		results_select_none(results_select_drag_handle);

	for (row = start; row <= end && row < results_select_drag_handle->display_lines; row++) {
		if (!(results_select_drag_handle->redraw[results_select_drag_handle->redraw[row].index].flags & RESULTS_FLAG_SELECTABLE))
			continue;

		if (results_select_drag_handle->redraw[results_select_drag_handle->redraw[row].index].flags & RESULTS_FLAG_SELECTED) {
			results_select_drag_handle->redraw[results_select_drag_handle->redraw[row].index].flags &= ~RESULTS_FLAG_SELECTED;
			results_select_drag_handle->selection_count--;
		} else {
			results_select_drag_handle->redraw[results_select_drag_handle->redraw[row].index].flags |= RESULTS_FLAG_SELECTED;
			results_select_drag_handle->selection_count++;
		}

		wimp_force_redraw(state.w, state.xscroll, LINE_BASE(row),
			state.xscroll + (state.visible.x1 - state.visible.x0), LINE_Y1(row));
	}

	if (results_select_drag_handle->selection_count == 1) {
		for (row = 0; row < results_select_drag_handle->display_lines; row++) {
			if (results_select_drag_handle->redraw[results_select_drag_handle->redraw[row].index].flags & RESULTS_FLAG_SELECTED) {
				results_select_drag_handle->selection_row = row;
				break;
			}
		}
	}
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
	char			*buffer, *filename, *command = "Filer_Run ";
	size_t			buffer_length;
	enum objdb_status	status;

	if (handle == NULL || row >= handle->display_lines)
		return;

	row = handle->redraw[row].index;

	if (row >= handle->redraw_lines || handle->redraw[row].type != RESULTS_LINE_FILENAME || handle->redraw[row].file == OBJDB_NULL_KEY)
		return;

	status = objdb_validate_file(handle->objects, handle->redraw[row].file, TRUE);

	if (status != OBJDB_STATUS_UNCHANGED && status != OBJDB_STATUS_CHANGED) {
		error_msgs_report_info("NotThere");
		return;
	}

	buffer_length = objdb_get_name_length(handle->objects, handle->redraw[row].file);
	buffer = malloc(buffer_length + strlen(command));
	if (buffer == NULL)
		return;

	strcpy(buffer, command);
	filename = buffer + strlen(command);

	if (!objdb_get_name(handle->objects, handle->redraw[row].file, filename, buffer_length))
		return;

	xos_cli(buffer);

	free(buffer);
}


/**
 * Open the directory containing an object in the results window.
 *
 * \param *handle		The handle of the results window to use.
 * \param row			The row of the window to use.
 */

static void results_open_parent(struct results_window *handle, unsigned row)
{
	char			*buffer, *filename, *command = "Filer_OpenDir ";
	size_t			buffer_length;
	unsigned		key;
	enum objdb_status	status;

	if (handle == NULL || row >= handle->display_lines)
		return;

	row = handle->redraw[row].index;

	if (row >= handle->redraw_lines || handle->redraw[row].type != RESULTS_LINE_FILENAME)
		return;

	key = objdb_get_parent(handle->objects, handle->redraw[row].file);

	if (key == OBJDB_NULL_KEY)
		return;

	status = objdb_validate_file(handle->objects, key, TRUE);

	if (status != OBJDB_STATUS_UNCHANGED && status != OBJDB_STATUS_CHANGED) {
		error_msgs_report_info("NotThere");
		return;
	}

	buffer_length = objdb_get_name_length(handle->objects, key);
	buffer = malloc(buffer_length + strlen(command));
	if (buffer == NULL)
		return;

	strcpy(buffer, command);
	filename = buffer + strlen(command);

	if (!objdb_get_name(handle->objects, key, filename, buffer_length))
		return;

	xos_cli(buffer);

	free(buffer);
}


/**
 * Prepare the contents of the object info dialogue.
 *
 * \param *handle		The handle of the results window to prepare for.
 */

static void results_object_info_prepare(struct results_window *handle)
{
	char			*base, *end;
	unsigned		row;
	osgbpb_info		*file;
	struct fileicon_info	info;
	struct objdb_info	object;


	if (handle == NULL || handle->selection_count != 1 || handle->selection_row >= handle->display_lines)
		return;

	row = handle->redraw[handle->selection_row].index;

	if (row >= handle->redraw_lines || handle->redraw[row].type != RESULTS_LINE_FILENAME)
		return;

	/* Get the data. */

	file = malloc(objdb_get_info(handle->objects, handle->redraw[row].file, NULL, NULL));
	if (file == NULL)
		return;

	objdb_get_info(handle->objects, handle->redraw[row].file, file, &object);
	fileicon_get_type_icon(object.filetype, "", &info);

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

	if (info.name != TEXTDUMP_NULL) {
		if (object.filetype <= 0xfff)
			icons_printf(results_object_window, RESULTS_OBJECT_ICON_TYPE, "%-8s (%03X)", base + info.name, object.filetype);
		else
			icons_printf(results_object_window, RESULTS_OBJECT_ICON_TYPE, "%s", base + info.name);
	}

	if (info.large != TEXTDUMP_NULL)
		icons_printf(results_object_window, RESULTS_OBJECT_ICON_ICON, "%s", base + info.large);

	results_create_address_string(file->load_addr, file->exec_addr,
				icons_get_indirected_text_addr(results_object_window, RESULTS_OBJECT_ICON_DATE),
				icons_get_indirected_text_length(results_object_window, RESULTS_OBJECT_ICON_DATE));

	free(file);
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


/**
 * Handle a callback from the dataxfer system and call the file module to
 * save a full set of results and associated data to disc.
 *
 * \param *filename		The filename to save to.
 * \param selection		TRUE if Selection is ticked; else FALSE.
 * \param *data			The handle of the results window to save from.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool results_save_result_data(char *filename, osbool selection, void *data)
{
	struct results_window		*handle = (struct results_window *) data;

	if (handle == NULL)
		return FALSE;

	return file_full_save(handle->file, filename);
}


/**
 * Handle a callback from the dataxfer system and call the file module to
 * save the linked dialogue data to disc.
 *
 * \param *filename		The filename to save to.
 * \param selection		TRUE if Selection is ticked; else FALSE.
 * \param *data			The handle of the results window to save from.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool results_save_dialogue_data(char *filename, osbool selection, void *data)
{
	struct results_window		*handle = (struct results_window *) data;

	if (handle == NULL)
		return FALSE;

	return file_dialogue_save(handle->file, filename);
}


/**
 * Handle a callback from the dataxfer system and save a set of filenames to
 * disc.
 *
 * \param *filename		The filename to save to.
 * \param selection		TRUE if Selection is ticked; else FALSE.
 * \param *data			The handle of the results window to save from.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool results_save_filenames(char *filename, osbool selection, void *data)
{
	struct results_window	*handle = (struct results_window *) data;
	int			i;
	size_t			pathname_len;
	char			*pathname;
	FILE			*out;

	if (handle == NULL)
		return FALSE;


	pathname_len = objdb_get_name_length(handle->objects, OBJDB_NULL_KEY);
	pathname = malloc(pathname_len);
	if (pathname == NULL)
		return FALSE;

	out = fopen(filename, "w");

	if (out == NULL)
		return FALSE;

	for (i = 0; i < handle->redraw_lines; i++) {
		if (handle->redraw[i].type == RESULTS_LINE_FILENAME && (!selection || (handle->redraw[i].flags & RESULTS_FLAG_SELECTED))) {
			objdb_get_name(handle->objects, handle->redraw[i].file, pathname, pathname_len);
			fprintf(out, "%s\n", pathname);
		}
	}

	fclose(out);

	osfile_set_type(filename, osfile_TYPE_TEXT);

	free(pathname);

	return TRUE;
}


/**
 * Copy the names of the current selection on to the clipboard.
 *
 * \param *handle		The handle of the results window to copy from.
 */

static void results_clipboard_copy_filenames(struct results_window *handle)
{
	int		i;
	char		*pathname;
	size_t		pathname_len;

	if (handle == NULL || handle->selection_count == 0)
		return;

	pathname_len = objdb_get_name_length(handle->objects, OBJDB_NULL_KEY);
	pathname = malloc(pathname_len);
	if (pathname == NULL)
		return;

	textdump_clear(results_clipboard);

	for (i = 0; i < handle->redraw_lines; i++) {
		if (handle->redraw[i].type == RESULTS_LINE_FILENAME && (handle->redraw[i].flags & RESULTS_FLAG_SELECTED)) {
			objdb_get_name(handle->objects, handle->redraw[i].file, pathname, pathname_len);
			textdump_store(results_clipboard, pathname);
		}
	}

	clipboard_claim(results_clipboard_find, results_clipboard_size, results_clipboard_release, (void *) handle);

	free(pathname);
}


/**
 * Callback to let the clipboard handler know the location of the clipboard.
 *
 * \param *data			The handle of the results window owning the clipboard.
 * \return			The address of the clipboard contents.
 */

static void *results_clipboard_find(void *data)
{
	return textdump_get_base(results_clipboard);
}


/**
 * Callback to let the clipboard handler know the size of the clipboard.
 *
 * \param *data			The handle of the results window owning the clipboard.
 * \return			The size of the clipboard contents.
 */

static size_t results_clipboard_size(void *data)
{
	return textdump_get_size(results_clipboard);
}


/**
 * Callback to allow the clipboard contents to be released.
 *
 * \param *data			The handle of the results window owning the clipboard.
 */

static void results_clipboard_release(void *data)
{
	textdump_clear(results_clipboard);
}

