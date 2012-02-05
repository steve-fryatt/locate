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


struct results_window {
	wimp_w		window;
	wimp_w		status;
};



/* Global variables */

static wimp_window	*results_window_def = NULL;
static wimp_window	*results_status_def = NULL;

//static void	choices_click_handler(wimp_pointer *pointer);
//static osbool	choices_keypress_handler(wimp_key *key);


/**
 * Initialise the Results module.
 */

void results_initialise(void)
{
	wimp_window	*results_window_def = templates_load_window("Results");
	wimp_window	*results_status_def = templates_load_window("ResultsPane");

	if (results_window_def == NULL || results_pane_def == NULL)
		error_msgs_report_fatal("BadTemplate");
}


/**
 * Create and open a new results window.
 *
 * \return			The results window handle, or NULL on failure.
 */

struct results_window *results_create(void)
{
	struct results_window *new;

	new = heap_alloc(sizeof(struct results_window));
	if (new == NULL)
		return NULL;

	new->window = wimp_create_window(results_window_def);
	new->status = wimp_create_window(results_status_def);

//	ihelp_add_window(choices_window, "Choices", NULL);

//	event_add_window_mouse_event(choices_window, choices_click_handler);
//	event_add_window_key_event(choices_window, choices_keypress_handler);

//	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, handle_choices_icon_drop);

	return new;
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

	wimp_delete_window(handle->window);
	wimp_delete_window(handle->status);

	heap_free(handle);
}
