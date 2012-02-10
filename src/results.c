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

#include "results.h"

#include "ihelp.h"
#include "templates.h"


#define STATUS_LENGTH 128

#define RESULTS_TOOLBAR_HEIGHT 0
#define RESULTS_LINE_HEIGHT 56
#define RESULTS_WINDOW_MARGIN 4
#define RESULTS_LINE_OFFSET 4
#define RESULTS_ICON_HEIGHT 52
#define RESULTS_STATUS_HEIGHT 60

#define RESULTS_ALLOC_REDRAW 50
#define RESULTS_ALLOC_TEXT 1024

/* Results window icons. */

#define RESULTS_ICON_FILE 0
#define RESULTS_ICON_INFO 1

#define RESULTS_ICON_STATUS 1


/* Data structures. */

enum results_line_type {
	RESULTS_LINE_NONE = 0,							/**< A blank line (nothing plotted).					*/
	RESULTS_LINE_TEXT							/**< A line containing unspecific text.					*/
};

/** A line definition for the results window. */

struct results_line {
	enum results_line_type	type;						/**< The type of line.							*/

	unsigned		parent;						/**< The parent line for a group (points to itself for the parent).	*/

	char			*text;						/**< Text pointer for text-based lines (NULL if not used).		*/
};

/** A data structure defining a results window. */

struct results_window {
	struct file_block	*file;						/**< The file block to which the window belongs.			*/

	struct results_line	*redraw;					/**< The array of redraw data for the window.				*/
	unsigned		redraw_lines;					/**< The number of lines in the window.					*/
	unsigned		redraw_size;					/**< The number of redraw lines claimed.				*/

	wimp_w			window;						/**< The window handle.							*/
	wimp_w			status;						/**< The status bar handle.						*/
};


/* Global variables. */

static wimp_window	*results_window_def = NULL;				/**< Definition for the main results window.				*/
static wimp_window	*results_status_def = NULL;				/**< Definition for the results status pane.				*/


/* Local function prototypes. */

static void	results_redraw_handler(wimp_draw *redraw);
static void	results_close_handler(wimp_close *close);
static osbool	results_expand_redraw_lines(struct results_window *handle);


/* Line position calculations. */

#define LINE_BASE(x) (-((x)+1) * RESULTS_LINE_HEIGHT - RESULTS_TOOLBAR_HEIGHT - RESULTS_WINDOW_MARGIN)
#define LINE_Y0(x) (LINE_BASE(x) + RESULTS_LINE_OFFSET)
#define LINE_Y1(x) (LINE_BASE(x) + RESULTS_LINE_OFFSET + RESULTS_ICON_HEIGHT)


/**
 * Initialise the Results module.
 */

void results_initialise(void)
{
	results_window_def = templates_load_window("Results");
	results_window_def->icon_count = 0;

	results_status_def = templates_load_window("ResultsPane");

	if (results_window_def == NULL || results_status_def == NULL)
		error_msgs_report_fatal("BadTemplate");
}


/**
 * Create and open a new results window.
 *
 * \param *file			The file block to which the window belongs.
 * \param *title		The title to use for the window.
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_create(struct file_block *file, char *title)
{
	struct results_window	*new = NULL;
	char			*title_block = FALSE;
	char			*status_block = FALSE;
	int			status_height;
	osbool			mem_ok = TRUE;

	/* Allocate all the memory that we require. */

	new = heap_alloc(sizeof(struct results_window));
	if (new == NULL)
		mem_ok = FALSE;

	if (mem_ok) {
		title_block = heap_strdup(title);
		if (title_block == NULL)
			mem_ok = FALSE;
	}

	if (mem_ok) {
		status_block = heap_alloc(STATUS_LENGTH);
		if (status_block == NULL)
			mem_ok = FALSE;
	}

	if (mem_ok) {
		new->redraw = NULL;

		if (flex_alloc((flex_ptr) &(new->redraw), RESULTS_ALLOC_REDRAW * sizeof(struct results_line)) == 0)
			mem_ok = FALSE;
	}

	/* If any memory allocations failed, free anything that did get
	 * claimed and exit with an error.
	 */

	if (!mem_ok) {
		if (new != NULL && new->redraw != NULL)
			flex_free((flex_ptr) &(new->redraw));

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

	new->redraw_size = RESULTS_ALLOC_REDRAW;

	new->redraw_lines = 0;

	/* Create the window and open it on screen. */

	status_height = results_status_def->visible.y1 - results_status_def->visible.y0;

	windows_place_as_footer(results_window_def, results_status_def, status_height);

	results_window_def->title_data.indirected_text.text = title_block;
	results_window_def->title_data.indirected_text.size = strlen(title_block) + 1;

	results_status_def->icons[RESULTS_ICON_STATUS].data.indirected_text.text = status_block;
	results_status_def->icons[RESULTS_ICON_STATUS].data.indirected_text.size = STATUS_LENGTH;
	*status_block = '\0';

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
	char			buf[255];

	res = (struct results_window *) event_get_window_user_data(redraw->w);

	if (res == NULL)
		return;

	more = wimp_redraw_window(redraw);

	ox = redraw->box.x0 - redraw->xscroll;
	oy = redraw->box.y1 - redraw->yscroll;

	icon = results_window_def->icons;

	while (more) {
		top = (oy - redraw->clip.y1 - RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (top < 0)
			top = 0;

		bottom = ((RESULTS_LINE_HEIGHT * 1.5) + oy - redraw->clip.y0
				- RESULTS_TOOLBAR_HEIGHT) / RESULTS_LINE_HEIGHT;
		if (bottom > res->redraw_lines)
			bottom = res->redraw_lines;

		for (y = top; y < bottom; y++) {
			line = &(res->redraw[y]);

			switch (line->type) {
			case RESULTS_LINE_TEXT:
				icon[RESULTS_ICON_FILE].extent.y0 = LINE_Y0(y);
				icon[RESULTS_ICON_FILE].extent.y1 = LINE_Y1(y);

				sprintf(buf, "Results line %d", y);

				icon[RESULTS_ICON_FILE].data.indirected_text.text = buf;

				wimp_plot_icon(&(icon[RESULTS_ICON_FILE]));

				break;

			default:
				break;
			}
		}

		more = wimp_get_rectangle(redraw);
	}
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

	heap_free(title);
	heap_free(status);
	heap_free(handle);
}


/**
 * Add a line of unspecific text to the end of a results window.
 *
 * \param *handle		The handle of the results window to update.
 * \param *text			The text to add.
 */

void results_add_text(struct results_window *handle, char *text)
{
	if (handle == NULL)
		return;

	if (handle->redraw_lines >= handle->redraw_size)
		results_expand_redraw_lines(handle);

	if (handle->redraw_lines >= handle->redraw_size)
		return;

	handle->redraw[handle->redraw_lines].type = RESULTS_LINE_TEXT;

	handle->redraw_lines++;
}


/**
 * Expand the redraw block allocation for a results window by the standard
 * allocation block.
 *
 * \param *handle		The handle of the results window to update.
 * \return			TRUE if successful; FALSE on failure.
 */

static osbool results_expand_redraw_lines(struct results_window *handle)
{
	if (handle == NULL)
		return FALSE;

	if (flex_extend((flex_ptr) &(handle->redraw), (handle->redraw_size + RESULTS_ALLOC_REDRAW) * sizeof(struct results_line)) == 0)
		return FALSE;

	handle->redraw_size += RESULTS_ALLOC_REDRAW;

	return TRUE;
}

