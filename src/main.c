/* Locate - main.c
 *
 * (c) Stephen Fryatt, 2012
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

#include "sflib/config.h"
#include "sflib/resources.h"
#include "sflib/heap.h"
#include "sflib/windows.h"
#include "sflib/icons.h"
#include "sflib/menus.h"
#include "sflib/transfer.h"
#include "sflib/url.h"
#include "sflib/msgs.h"
#include "sflib/debug.h"
#include "sflib/config.h"
#include "sflib/errors.h"
#include "sflib/string.h"
#include "sflib/colpick.h"
#include "sflib/event.h"

/* Application header files */

#include "main.h"

#include "choices.h"
#include "dataxfer.h"
#include "dialogue.h"
#include "file.h"
#include "iconbar.h"
#include "ihelp.h"
#include "results.h"
#include "templates.h"

/* ------------------------------------------------------------------------------------------------------------------ */

static void	main_poll_loop(void);
static void	main_initialise(void);
static osbool	main_message_quit(wimp_message *message);
static osbool	main_message_prequit(wimp_message *message);


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

	main_poll_loop();

	file_destroy_all();

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

	while (!main_quit_flag) {
		reason = wimp_poll(wimp_MASK_NULL, &blk, 0);

		/* Events are passed to Event Lib first; only if this fails
		 * to handle them do they get passed on to the internal
		 * inline handlers shown here.
		 */

		if (!event_process_event(reason, &blk, 0)) {
			switch (reason) {
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
	static char		task_name[255];
	char			resources[255], res_temp[255];
	osspriteop_area		*sprites;

	wimp_MESSAGE_LIST(13)	message_list;
	wimp_version_no		wimp_version;


	hourglass_on();

	strcpy(resources, "<Locate$Dir>.Resources");
	resources_find_path(resources, sizeof(resources));

	/* Load the messages file. */

	snprintf(res_temp, sizeof(res_temp), "%s.Messages", resources);
	msgs_initialise(res_temp);

	/* Initialise the error message system. */

	error_initialise("TaskName", "TaskSpr", NULL);

	/* Initialise with the Wimp. */

	message_list.messages[0]=message_URI_RETURN_RESULT;
	message_list.messages[1]=message_ANT_OPEN_URL;
	message_list.messages[2]=message_DATA_SAVE;
	message_list.messages[3]=message_DATA_SAVE_ACK;
	message_list.messages[4]=message_DATA_LOAD;
	message_list.messages[5]=message_MENU_WARNING;
	message_list.messages[6]=message_MENUS_DELETED;
	message_list.messages[7]=message_HELP_REQUEST;
	message_list.messages[8]=message_TASK_INITIALISE;
	message_list.messages[9]=message_TASK_CLOSE_DOWN;
	message_list.messages[10]=message_PRE_QUIT;
	message_list.messages[11]=message_QUIT;

	msgs_lookup("TaskName", task_name, sizeof (task_name));
	main_task_handle = wimp_initialise(wimp_VERSION_RO3, task_name, (wimp_message_list *) &message_list, &wimp_version);

	event_add_message_handler(message_QUIT, EVENT_MESSAGE_INCOMING, main_message_quit);
	event_add_message_handler(message_PRE_QUIT, EVENT_MESSAGE_INCOMING, main_message_prequit);

	/* Initialise the flex heap. */

	flex_init(task_name, 0, 0);
	heap_initialise();

	/* Initialise the configuration. */

	config_initialise(task_name, "Locate", "<Locate$Dir>");

	config_str_init("SearchPath", "ADFS::4.$");
	config_int_init("PathBufSize", 4095);
	config_opt_init("Multitask", TRUE);
	config_opt_init("ImageFS", FALSE);
	config_opt_init("SuppressErrors", TRUE);
	config_int_init("HistorySize", 10);
	config_opt_init("ConfirmHistoryAdd", TRUE);
	config_opt_init("FileMenuSprites", FALSE);
	config_opt_init("ScrollResults", TRUE);
	config_int_init("OSGBPBReadSize", 1000);
	config_opt_init("QuitAsPlugin", FALSE);
	config_opt_init("SearchWindAsPlugin", FALSE);
	config_opt_init("FullInfoDisplay", FALSE);

	config_load();

	/* Load the menu structure. */

	snprintf(res_temp, sizeof(res_temp), "%s.Menus", resources);
	templates_load_menus(res_temp);

	/* Load the window templates. */

	sprites = resources_load_user_sprite_area("<Locate$Dir>.Sprites");

	main_wimp_sprites = sprites;

	snprintf(res_temp, sizeof(res_temp), "%s.Templates", resources);
	templates_open(res_temp);

	/* Initialise the individual modules. */

	ihelp_initialise();
	dataxfer_initialise();
	choices_initialise();
	dialogue_initialise();
	results_initialise();
	iconbar_initialise();
	url_initialise();

	templates_close();

	hourglass_off();
}


/**
 * Handle incoming Message_Quit.
 */

static osbool main_message_quit(wimp_message *message)
{
	main_quit_flag = TRUE;

	return TRUE;
}


/**
 * Handle incoming Message_PreQuit.
 */

static osbool main_message_prequit(wimp_message *message)
{
	return TRUE;

	message->your_ref = message->my_ref;
	wimp_send_message(wimp_USER_MESSAGE_ACKNOWLEDGE, message, message->sender);

	return TRUE;
}

