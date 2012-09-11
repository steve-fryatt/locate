/* Locate - results.c
 * (c) Stephen Fryatt, 2012
 *
 * Search result and status display.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* OSLib Header files. */

#include "oslib/osbyte.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"

/* SF-Lib Header files. */

#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/event.h"
#include "sflib/heap.h"
#include "sflib/icons.h"
#include "sflib/msgs.h"
#include "sflib/windows.h"
#include "sflib/debug.h"
#include "sflib/string.h"

/* Application header files. */

#include "results.h"

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

/* Results window icons. */

#define RESULTS_ICON_FILE 0
#define RESULTS_ICON_INFO 1

#define RESULTS_ICON_STATUS 1


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


/** A file information block. */

struct results_file {
	unsigned		filename;					/**< Text offset to the filename.					*/
};

/** A line definition for the results window. */

struct results_line {
	enum results_line_type	type;						/**< The type of line.							*/

	enum results_line_flags	flags;						/**< Any flags relating to the line.					*/

	unsigned		parent;						/**< The parent line for a group (points to itself for the parent).	*/

	unsigned		text;						/**< Text offset for text-based lines (RESULTS_NULL if not used).	*/
	unsigned		file;						/**< File offset for file-based lines.					*/
	unsigned		sprite;						/**< Text offset for the display icon's sprite name.			*/

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

	/* File data storage. */

	struct results_file	*files;						/**< The array of file data for the window.				*/
	unsigned		files_count;					/**< The number of files stored for the window.				*/
	unsigned		files_size;					/**< The number of file data blocks currently claimed.			*/

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

static osspriteop_area			*results_sprite_area = NULL;		/**< The application sprite area.					*/


/* Local function prototypes. */

static void	results_redraw_handler(wimp_draw *redraw);
static void	results_close_handler(wimp_close *close);
static void	results_update_extent(struct results_window *handle);
static unsigned	results_add_line(struct results_window *handle, osbool show);
static unsigned	results_add_fileblock(struct results_window *handle);


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
	results_window_def = templates_load_window("Results");
	results_window_def->icon_count = 0;

	results_status_def = templates_load_window("ResultsPane");

	if (results_window_def == NULL || results_status_def == NULL)
		error_msgs_report_fatal("BadTemplate");

	results_sprite_area = sprites;
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
		new->files = NULL;
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

		if (flex_alloc((flex_ptr) &(new->files), RESULTS_ALLOC_FILES * sizeof(struct results_file)) == 0)
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
		if (new != NULL && new->files != NULL)
			flex_free((flex_ptr) &(new->files));
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

	new->full_info = TRUE;

	new->files_size = RESULTS_ALLOC_FILES;
	new->files_count = 0;

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

	windows_open(new->window);
	windows_open_nested_as_footer(new->status, new->window, status_height);

	return new;
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
	wimp_icon		*icon;
	char			*text, *fileicon;
	char			validation[255];
	char			truncation[1024]; // \TODO -- Allocate properly.
	os_t			start_time;

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

	start_time = os_read_monotonic_time();

	while (more) {
		top = (oy - redraw->clip.y1 - RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (top < 0)
			top = 0;

		bottom = ((RESULTS_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0
				- RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (bottom > res->redraw_lines)
			bottom = res->redraw_lines;

		for (y = top; y < bottom; y++) {
			line = &(res->redraw[res->redraw[y].index]);

			switch (line->type) {
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

				wimp_plot_icon(&(icon[RESULTS_ICON_FILE]));
				break;

			default:
				break;
			}
		}

		more = wimp_get_rectangle(redraw);
	}

	debug_printf("Redraw complete in %u centiseconds", os_read_monotonic_time() - start_time);
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
	flex_free((flex_ptr) &(handle->files));

	if (handle->text != NULL)
		textdump_destroy(handle->text);

	heap_free(title);
	heap_free(status);
	heap_free(handle);
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
	unsigned	line, offv, offt;
	osbool		small;

	if (handle == NULL)
		return;

	line = results_add_line(handle, TRUE);
	if (line == RESULTS_NULL)
		return;

	offt = textdump_store(handle->text, message);
	offv = fileicon_get_special_icon(FILEICON_ERROR, &small);

	if (offt == TEXTDUMP_NULL || offv == TEXTDUMP_NULL)
		return;

	handle->redraw[line].type = RESULTS_LINE_TEXT;
	handle->redraw[line].text = offt;
	handle->redraw[line].sprite = offv;
	handle->redraw[line].colour = wimp_COLOUR_RED;
	if (!small)
		handle->redraw[line].flags |= RESULTS_FLAG_HALFSIZE;
}


/**
 * Add a file to the end of the results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param key			The database key for the file.
 */

void results_add_file(struct results_window *handle, unsigned key)
{
	unsigned	file, info, data, fileblock;
	unsigned	line, offv, offt;
	osbool		small;
	char		name[256];
	unsigned	type;

	if (handle == NULL)
		return;

	line = results_add_line(handle, TRUE);
	if (line == RESULTS_NULL)
		return;

	objdb_get_name(handle->objects, key, name, sizeof(name));
	type = objdb_get_filetype(handle->objects, key);

	debug_printf("Returned filetype 0x%x", type);

	offt = textdump_store(handle->text, name);
	offv = fileicon_get_type_icon(type, "", &small);

	if (offt == TEXTDUMP_NULL || offv == TEXTDUMP_NULL)
		return;

	handle->redraw[line].type = RESULTS_LINE_TEXT;
	handle->redraw[line].text = offt;
	handle->redraw[line].sprite = offv;
	if (!small)
		handle->redraw[line].flags |= RESULTS_FLAG_HALFSIZE;

/*
	fileblock = results_add_fileblock(handle);
	if (fileblock == RESULTS_NULL)
		return;

	file = results_add_line(handle, TRUE);
	if (file == RESULTS_NULL)
		return;

	data = textdump_store(handle->text, text);

	handle->redraw[file].type = (data == RESULTS_NULL) ? RESULTS_LINE_NONE : RESULTS_LINE_FILENAME;
	handle->redraw[file].parent = file;
	handle->redraw[file].text = data;
	handle->redraw[file].file = fileblock;

	info = results_add_line(handle, TRUE);
	if (info == RESULTS_NULL)
		return;

	data = textdump_store(handle->text, text);

	handle->redraw[info].type = (data == RESULTS_NULL) ? RESULTS_LINE_NONE : RESULTS_LINE_FILEINFO;
	handle->redraw[info].parent = file;
	handle->redraw[info].text = RESULTS_NULL;
	handle->redraw[info].file = fileblock;
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
		case RESULTS_LINE_TEXT:
			if (wimptextop_string_width(text + handle->redraw[line].text, 0) <= width)
				break;

			strcpy(truncate + 3, text + handle->redraw[line].text);
			length = strlen(truncate) - 3;
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
			0, LINE_Y0(handle->redraw_lines - 1),
			handle->format_width, (all) ? LINE_Y1(0) : LINE_Y1(handle->formatted_lines));

	handle->formatted_lines = handle->redraw_lines;

	results_update_extent(handle);
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
	handle->redraw[offset].file = RESULTS_NULL;
	handle->redraw[offset].sprite = RESULTS_NULL;
	handle->redraw[offset].truncate = 0;
	handle->redraw[offset].colour = wimp_COLOUR_BLACK;

	/* If the line is for immediate display, add it to the index. */

	if (show || handle->full_info)
		handle->redraw[handle->display_lines++].index = offset;

	return offset;
}


/**
 * Claim a new line from the file array, allocating more memory if required,
 * set it up and return its offset.
 *
 * \param *handle		The handle of the results window.
 * \return			The offset of the new file, or RESULTS_NULL.
 */

static unsigned results_add_fileblock(struct results_window *handle)
{
	if (handle == NULL)
		return RESULTS_NULL;

	/* Make sure that there is enough space in the block to take the new
	 * line, allocating more if required.
	 */

	if (handle->files_count >= handle->files_size) {
		if (flex_extend((flex_ptr) &(handle->files), (handle->files_size + RESULTS_ALLOC_FILES) * sizeof(struct results_file)) == 0)
			return RESULTS_NULL;

		handle->files_size += RESULTS_ALLOC_FILES;
	}

	/* Get the new line. */

	return handle->files_count++;
}

