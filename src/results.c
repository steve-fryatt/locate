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


/* Results window icons. */

#define RESULT_ICON_STATUS 1


/* Data structures. */

struct results_window {
	struct file_block	*file;						/**< The file block to which the window belongs.	*/

	wimp_w			window;						/**< The window handle.					*/
	wimp_w			status;						/**< The status bar handle.				*/
};


/* Global variables. */

static wimp_window	*results_window_def = NULL;				/**< Definition for the main results window.		*/
static wimp_window	*results_status_def = NULL;				/**< Definition for the results status pane.		*/


/* Local function prototypes. */

static void	results_close_handler(wimp_close *close);


/**
 * Initialise the Results module.
 */

void results_initialise(void)
{
	results_window_def = templates_load_window("Results");
	results_status_def = templates_load_window("ResultsPane");

	if (results_window_def == NULL || results_status_def == NULL)
		error_msgs_report_fatal("BadTemplate");

	// \TODO -- bodge around lack of redraw handler.

	results_window_def->flags |= wimp_WINDOW_AUTO_REDRAW;
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
	struct results_window	*new;
	char			*title_block, *status_block;
	int			status_height;

	/* Allocate all the memory that we require. */

	new = heap_alloc(sizeof(struct results_window));
	if (new == NULL) {
		error_msgs_report_error("NoMemResultsCreate");
		return NULL;
	}

	title_block = heap_strdup(title);
	if (title_block == NULL) {
		error_msgs_report_error("NoMemResultsCreate");
		heap_free(new);
		return NULL;
	}

	status_block = heap_alloc(STATUS_LENGTH);
	if (status_block == NULL) {
		error_msgs_report_error("NoMemResultsCreate");
		heap_free(title_block);
		heap_free(new);
		return NULL;
	}

	/* Populate the data in the block. */

	new->file = file;

	/* Create the window and open it on screen. */

	status_height = results_status_def->visible.y1 - results_status_def->visible.y0;

	windows_place_as_footer(results_window_def, results_status_def, status_height);

	results_window_def->title_data.indirected_text.text = title_block;
	results_window_def->title_data.indirected_text.size = strlen(title_block) + 1;

	results_status_def->icons[RESULT_ICON_STATUS].data.indirected_text.text = status_block;
	results_status_def->icons[RESULT_ICON_STATUS].data.indirected_text.size = STATUS_LENGTH;
	*status_block = '\0';

	new->window = wimp_create_window(results_window_def);
	new->status = wimp_create_window(results_status_def);

	ihelp_add_window(new->window, "Results", NULL);
	ihelp_add_window(new->status, "ResultsStatus", NULL);

	event_add_window_user_data(new->window, new);
	event_add_window_user_data(new->status, new);

	event_add_window_close_event(new->window, results_close_handler);

	windows_open(new->window);
	windows_open_nested_as_footer(new->status, new->window, status_height);

	return new;
}


/**
 * Handle a results window closing.  This is terminal, and simply calls the
 * file destructor for the associated file data (which in turn calls
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
	status = icons_get_indirected_text_addr(handle->status, RESULT_ICON_STATUS);

	ihelp_remove_window(handle->window);
	event_delete_window(handle->window);
	wimp_delete_window(handle->window);

	ihelp_remove_window(handle->window);
	event_delete_window(handle->status);
	wimp_delete_window(handle->status);

	heap_free(title);
	heap_free(status);
	heap_free(handle);
}

