/* Copyright 2012-2016, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: main.c
 *
 * Core program code and resource loading.
 */

/* ANSI C header files */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Acorn C header files */

#include "flex.h"

/* OSLib header files */

#include "oslib/wimp.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osspriteop.h"
#include "oslib/uri.h"
#include "oslib/hourglass.h"
#include "oslib/pdriver.h"
#include "oslib/help.h"

/* SF-Lib header files. */

#include "sflib/colpick.h"
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
#include "sflib/resources.h"
#include "sflib/saveas.h"
#include "sflib/string.h"
#include "sflib/templates.h"
#include "sflib/url.h"
#include "sflib/windows.h"

/* Application header files */

#include "main.h"

#include "choices.h"
#include "clipboard.h"
#include "dialogue.h"
#include "file.h"
#include "fileicon.h"
#include "hotlist.h"
#include "iconbar.h"
#include "objdb.h"
#include "plugin.h"
#include "results.h"
#include "search.h"
#include "settime.h"

/* ------------------------------------------------------------------------------------------------------------------ */

static void	main_poll_loop(void);
static void	main_initialise(void);
static void	main_post_initialise(void);
static void	main_parse_command_line(int argc, char *argv[]);
static osbool	main_message_quit(wimp_message *message);


/*
 * Cross file global variables
 */

wimp_t			main_task_handle;
osbool			main_quit_flag = FALSE;
osspriteop_area		*main_wimp_sprites;


/**
 * Main code entry point.
 */

int main(int argc, char *argv[])
{
	main_initialise();

	main_parse_command_line(argc, argv);

	main_post_initialise();

	main_poll_loop();

	file_destroy_all();

	hotlist_terminate();
	fileicon_terminate();
	msgs_terminate();
	wimp_close_down(main_task_handle);

	return 0;
}


/**
 * Wimp Poll loop.
 */

static void main_poll_loop(void)
{
	wimp_event_no		reason;
	wimp_block		blk;
	wimp_poll_flags		mask;

	while (!main_quit_flag) {
		mask = ((search_poll_required()) ? 0 : wimp_MASK_NULL);

		reason = wimp_poll(mask, &blk, 0);

		/* Events are passed to Event Lib first; only if this fails
		 * to handle them do they get passed on to the internal
		 * inline handlers shown here.
		 *
		 * \TODO -- search_poll_all() might need to be pulled outside
		 * the if() statement, if other Null Poll claimants interfere
		 * via Event Lib.
		 */

		if (!event_process_event(reason, &blk, 0)) {
			switch (reason) {
			case wimp_NULL_REASON_CODE:
				search_poll_all();
				break;

			case wimp_OPEN_WINDOW_REQUEST:
				wimp_open_window(&(blk.open));
				break;

			case wimp_CLOSE_WINDOW_REQUEST:
				wimp_close_window(blk.close.w);
				break;
			}
		}
	}
}


/**
 * Application initialisation.
 */

static void main_initialise(void)
{
	static char			task_name[255];
	char				resources[255], res_temp[255];
	osspriteop_area			*sprites;


	hourglass_on();

	strcpy(resources, "<Locate$Dir>.Resources");
	resources_find_path(resources, sizeof(resources));

	/* Load the messages file. */

	snprintf(res_temp, sizeof(res_temp), "%s.Messages", resources);
	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName", "TaskSpr", NULL);

	/* Initialise with the Wimp. */

	msgs_lookup("TaskName", task_name, sizeof (task_name));
	main_task_handle = wimp_initialise(wimp_VERSION_RO3, task_name, NULL, NULL);

	event_add_message_handler(message_QUIT, EVENT_MESSAGE_INCOMING, main_message_quit);

	/* Initialise the flex heap. */

	flex_init(task_name, 0, 0);
	heap_initialise();

	/* Initialise the configuration. */

	config_initialise(task_name, "Locate", "<Locate$Dir>");

	config_str_init("SearchPath", "ADFS::4.$");				/**< The default search path.					*/
	config_int_init("PathBufSize", 4095);					/**< The path buffer size, in bytes.				*/
	config_opt_init("StoreAllFiles", FALSE);				/**< TRUE if all file details will be stored; else FALSE.	*/
	config_opt_init("ImageFS", FALSE);					/**< TRUE to search ImageFS contents; else FALSE.		*/
	config_opt_init("SuppressErrors", TRUE);				/**< TRUE to list errors in search results; FALSE to report.	*/
	config_opt_init("ScrollResults", TRUE);					/**< TRUE to scroll the results window to the last entry.	*/
	config_int_init("OSGBPBReadSize", 1000);				/**< The number of bytes allocated ot OS_GBPB calls.		*/
	config_opt_init("QuitAsPlugin", FALSE);					/**< Quit when complete if running as a FilerAction plugin.	*/
	config_opt_init("SearchWindAsPlugin", FALSE);				/**< TRUE to open a search window when acting as a plugin.	*/
	config_opt_init("FullInfoDisplay", FALSE);				/**< TRUE to display full file info by default.			*/
	config_int_init("MultitaskTimeslot", 10);				/**< The timeslot, in cs, allowed for a search poll.		*/
	config_opt_init("ValidatePaths", TRUE);					/**< TRUE to validate search paths on load; FALSE to ignore.	*/

	config_load();

	/* Load the menu structure. */

	snprintf(res_temp, sizeof(res_temp), "%s.Menus", resources);
	templates_load_menus(res_temp);

	/* Load the window templates. */

	sprites = resources_load_user_sprite_area("<Locate$Sprites>.Sprites");

	main_wimp_sprites = sprites;

	snprintf(res_temp, sizeof(res_temp), "%s.Templates", resources);
	templates_open(res_temp);

	/* Initialise the individual modules. */

	ihelp_initialise();
	dataxfer_initialise(main_task_handle, NULL);
	saveas_initialise("SaveAs", "SaveAsSel");
	clipboard_initialise();
	choices_initialise();
	fileicon_initialise();
	settime_initialise();
	dialogue_initialise();
	results_initialise(sprites);
	hotlist_initialise(sprites);
	iconbar_initialise();
	url_initialise();
	plugin_initialise();

	templates_close();

	hourglass_off();
}


/**
 * Perform any remaining initialisation after we've read the command
 * line and processed the passed parameters.  None of this initialisation
 * is to be varried out if we're operating as an iconbar icon-less
 * FilerAction plugin.
 */

static void main_post_initialise(void)
{
	wimp_error_box_selection	selection;
	wimp_pointer			pointer;


	if (plugin_filer_action_launched && config_opt_read("QuitAsPlugin"))
		return;

	iconbar_create_icon();

	if (config_opt_read("ValidatePaths") && !search_validate_paths(config_str_read("SearchPath"), FALSE)) {
		selection = error_msgs_report_question("BadLoadPaths", "BadLoadPathsB");
		if (selection == 1) {
			wimp_get_pointer_info(&pointer);
			choices_open_window(&pointer);
		}
	}
}


/**
 * Take the command line and parse it for useful arguments.
 *
 * \param argc                  The number of parameters passed.
 * \param *argv[]               Pointer to the parameter array.
 */

static void main_parse_command_line(int argc, char *argv[])
{
	int     i;

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-file") == 0 && (i + 1) < argc)
				file_create_from_saved(argv[i + 1]);
			else if (strcmp(argv[i], "-open") == 0 && (i + 1) < argc)
				file_create_dialogue_at(argv[i + 1]);
			else if (strcmp(argv[i], "-plugin") == 0)
				plugin_filer_launched();
		}
	}
}


/**
 * Handle incoming Message_Quit.
 */

static osbool main_message_quit(wimp_message *message)
{
	main_quit_flag = TRUE;

	return TRUE;
}

