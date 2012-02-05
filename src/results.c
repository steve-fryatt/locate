/* Locate - results.c
 * (c) Stephen Fryatt, 2012
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


/* Results window icons. */



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

//static void	choices_click_handler(wimp_pointer *pointer);
//static osbool	choices_keypress_handler(wimp_key *key);


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
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_create(struct file_block *file)
{
	struct results_window	*new;
	int			status_height;

	new = heap_alloc(sizeof(struct results_window));
	if (new == NULL) {
		error_msgs_report_error("NoMemResultsCreate");
		return NULL;
	}

	new->file = file;

	/* Create the window and open it on screen. */

	status_height = results_status_def->visible.y1 - results_status_def->visible.y0;

	windows_place_as_footer(results_window_def, results_status_def, status_height);

	new->window = wimp_create_window(results_window_def);
	new->status = wimp_create_window(results_status_def);

	ihelp_add_window(new->window, "Results", NULL);
	ihelp_add_window(new->status, "ResultsStatus", NULL);

	event_add_window_user_data(new->window, new);
	event_add_window_user_data(new->status, new);

	event_add_window_close_event(new->window, results_close_handler);

//	event_add_window_mouse_event(choices_window, choices_click_handler);
//	event_add_window_key_event(choices_window, choices_keypress_handler);

//	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, handle_choices_icon_drop);

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
	if (handle == NULL)
		return;

	ihelp_remove_window(handle->window);
	event_delete_window(handle->window);
	wimp_delete_window(handle->window);

	ihelp_remove_window(handle->window);
	event_delete_window(handle->status);
	wimp_delete_window(handle->status);

	heap_free(handle);
}

