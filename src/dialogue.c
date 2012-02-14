/* Locate - dialogue.c
 * (c) Stephen Fryatt, 2012
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */


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

#include "dialogue.h"

#include "ihelp.h"
#include "templates.h"


/* Search Dialogue panes. */

#define DIALOGUE_PANES 5							/**< The number of search dialogue panes.		*/

#define DIALOGUE_PANE_SIZE 0
#define DIALOGUE_PANE_DATE 1
#define DIALOGUE_PANE_TYPE 2
#define DIALOGUE_PANE_ATTRIBUTES 3
#define DIALOGUE_PANE_CONTENTS 4

/* Search Dialogue Icons. */

#define DIALOGUE_ICON_SEARCH 1
#define DIALOGUE_ICON_CANCEL 0
#define DIALOGUE_ICON_PANE 5
#define DIALOGUE_ICON_SIZE 6
#define DIALOGUE_ICON_DATE 7
#define DIALOGUE_ICON_TYPE 8
#define DIALOGUE_ICON_ATTRIBUTES 9
#define DIALOGUE_ICON_CONTENTS 10
#define DIALOGUE_ICON_SEARCH_PATH 20
//#define CHOICE_ICON_BACKGROUND_SEARCH 7
//#define CHOICE_ICON_IMAGE_FS 7
//#define CHOICE_ICON_SUPPRESS_ERRORS 8
//#define CHOICE_ICON_FULL_INFO 10
//#define CHOICE_ICON_HISTORY_SIZE 14
//#define CHOICE_ICON_CONFIRM_HISTORY 16
//#define CHOICE_ICON_PLUGIN_QUIT 19
//#define CHOICE_ICON_PLUGIN_WINDOW 20
//#define CHOICE_ICON_AUTOSCROLL 21
//#define CHOICE_ICON_MENU_ICONS 22



/* Global variables */

unsigned			dialogue_pane = 0;				/**< The currently displayed searh dialogue pane.	*/

static wimp_w			dialogue_window = NULL;				/**< The handle of the main search dialogue window.	*/
static wimp_w			dialogue_panes[DIALOGUE_PANES];			/**< The handles of the search dialogue panes.		*/


static void	dialogue_close_window(void);
static void	dialogue_change_pane(unsigned pane);
static void	dialogue_set_window(void);
static void	dialogue_read_window(void);
static void	dialogue_redraw_window(void);
static void	dialogue_click_handler(wimp_pointer *pointer);
static osbool	dialogue_keypress_handler(wimp_key *key);

//static osbool	handle_choices_icon_drop(wimp_message *message);


/**
 * Initialise the Search Dialogue module.
 */

void dialogue_initialise(void)
{
	wimp_window	*def = templates_load_window("Search");
	int		buf_size = config_int_read("PathBufSize");

	if (def == NULL)
		error_msgs_report_fatal("BadTemplate");

	def->icons[DIALOGUE_ICON_SEARCH_PATH].data.indirected_text.text = heap_alloc(buf_size);
	def->icons[DIALOGUE_ICON_SEARCH_PATH].data.indirected_text.size = buf_size;
	dialogue_window = wimp_create_window(def);
	icons_printf(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, "");
	free(def);
	ihelp_add_window(dialogue_window, "Search", NULL);
	event_add_window_mouse_event(dialogue_window, dialogue_click_handler);
	event_add_window_key_event(dialogue_window, dialogue_keypress_handler);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_SIZE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_DATE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_TYPE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_ATTRIBUTES, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_CONTENTS, FALSE);

	dialogue_panes[DIALOGUE_PANE_SIZE] = templates_create_window("SizePane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_SIZE], "Search.Size", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_keypress_handler);

	dialogue_panes[DIALOGUE_PANE_DATE] = templates_create_window("DatePane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_DATE], "Search.Date", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_keypress_handler);

	dialogue_panes[DIALOGUE_PANE_TYPE] = templates_create_window("TypePane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_TYPE], "Search.Type", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_keypress_handler);

	dialogue_panes[DIALOGUE_PANE_ATTRIBUTES] = templates_create_window("AttribPane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], "Search.Attributes", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_keypress_handler);

	dialogue_panes[DIALOGUE_PANE_CONTENTS] = templates_create_window("ContentPane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], "Search.Contents", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_keypress_handler);


//	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, handle_choices_icon_drop);
}


/**
 * Open the Search Dialogue window at the mouse pointer.
 *
 * \param *pointer		The details of the pointer to open the window at.
 */

void dialogue_open_window(wimp_pointer *pointer)
{
	if (windows_get_open(dialogue_window))
		return;

	dialogue_pane = DIALOGUE_PANE_SIZE;

	icons_set_radio_group_selected(dialogue_window, dialogue_pane, DIALOGUE_PANES,
			DIALOGUE_ICON_SIZE, DIALOGUE_ICON_DATE, DIALOGUE_ICON_TYPE,
			DIALOGUE_ICON_ATTRIBUTES, DIALOGUE_ICON_CONTENTS);

	dialogue_set_window();

	windows_open_with_pane_centred_at_pointer(dialogue_window, dialogue_panes[dialogue_pane],
			DIALOGUE_ICON_PANE, 0, pointer);

	icons_put_caret_at_end(dialogue_window, DIALOGUE_ICON_SEARCH_PATH);
}


/**
 * Close the Search Dialogue window.
 */

static void dialogue_close_window(void)
{
	wimp_close_window(dialogue_window);
}


/**
 * Switch the visible pane in the Search Dialogue.
 *
 * \param pane		The new pane to select.
 */

static void dialogue_change_pane(unsigned pane)
{
	unsigned	old_pane;
	wimp_caret	caret;

	if (pane >= DIALOGUE_PANES || !windows_get_open(dialogue_window) || pane == dialogue_pane)
		return;

	wimp_get_caret_position(&caret);

	old_pane = dialogue_pane;
	dialogue_pane = pane;

	icons_set_radio_group_selected(dialogue_window, dialogue_pane, DIALOGUE_PANES,
			DIALOGUE_ICON_SIZE, DIALOGUE_ICON_DATE, DIALOGUE_ICON_TYPE,
			DIALOGUE_ICON_ATTRIBUTES, DIALOGUE_ICON_CONTENTS);

	windows_open_pane_centred_in_icon(dialogue_window, dialogue_panes[pane], DIALOGUE_ICON_PANE, 0,
			dialogue_panes[old_pane]);

	wimp_close_window(dialogue_panes[old_pane]);
/*
	if (caret.w == dialogue_panes[old_pane]) {
		switch (pane) {
		case CHOICE_PANE_GENERAL:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_GENERAL], 2,
					CHOICE_ICON_DATEIN, CHOICE_ICON_DATEOUT);
			break;

		case CHOICE_PANE_CURRENCY:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_CURRENCY], 2,
					CHOICE_ICON_DECIMALPLACE, CHOICE_ICON_DECIMALPOINT);
			break;

		case CHOICE_PANE_SORDER:
			place_dialogue_caret(choices_panes[CHOICE_PANE_SORDER], wimp_ICON_WINDOW);
			break;

		case CHOICE_PANE_REPORT:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_REPORT], 2,
					CHOICE_ICON_FONTSIZE, CHOICE_ICON_FONTSPACE);
			break;

		case CHOICE_PANE_PRINT:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_PRINT], 4,
					CHOICE_ICON_MTOP, CHOICE_ICON_MLEFT,
					CHOICE_ICON_MRIGHT, CHOICE_ICON_MBOTTOM);
			break;

		case CHOICE_PANE_TRANSACT:
			place_dialogue_caret_fallback(choices_panes[CHOICE_PANE_TRANSACT], 1,
					CHOICE_ICON_AUTOCOMP);
			break;

		case CHOICE_PANE_ACCOUNT:
			place_dialogue_caret(choices_panes[CHOICE_PANE_ACCOUNT], wimp_ICON_WINDOW);
			break;
		}
	} */
}


/**
 * Set the contents of the Search Dialogue window to reflect the current settings.
 */

static void dialogue_set_window(void)
{
	/*
	icons_printf(choices_window, CHOICE_ICON_SEARCH_PATH, "%s", config_str_read("SearchPath"));

	icons_set_selected(choices_window, CHOICE_ICON_BACKGROUND_SEARCH, config_opt_read("Multitask"));
	icons_set_selected(choices_window, CHOICE_ICON_IMAGE_FS, config_opt_read("ImageFS"));
	icons_set_selected(choices_window, CHOICE_ICON_SUPPRESS_ERRORS, config_opt_read("SuppressErrors"));
	icons_set_selected(choices_window, CHOICE_ICON_FULL_INFO, config_opt_read("FullInfoDisplay"));
	icons_set_selected(choices_window, CHOICE_ICON_CONFIRM_HISTORY, config_opt_read("ConfirmHistoryAdd"));
	icons_set_selected(choices_window, CHOICE_ICON_PLUGIN_QUIT, config_opt_read("QuitAsPlugin"));
	icons_set_selected(choices_window, CHOICE_ICON_PLUGIN_WINDOW, config_opt_read("SearchWindAsPlugin"));
	icons_set_selected(choices_window, CHOICE_ICON_AUTOSCROLL, config_opt_read("ScrollResults"));
	icons_set_selected(choices_window, CHOICE_ICON_MENU_ICONS, config_opt_read("FileMenuSprites"));

	icons_printf(choices_window, CHOICE_ICON_HISTORY_SIZE, "%d", config_int_read("HistorySize"));
	*/
}


/**
 * Update the search settings from the values in the Search Dialogue window.
 */

static void dialogue_read_window(void)
{
	/* Read the main window. */

	/*
	config_str_set("SearchPath", icons_get_indirected_text_addr(choices_window, CHOICE_ICON_SEARCH_PATH));

	config_opt_set("Multitask", icons_get_selected(choices_window, CHOICE_ICON_BACKGROUND_SEARCH));
	config_opt_set("ImageFS", icons_get_selected(choices_window, CHOICE_ICON_IMAGE_FS));
	config_opt_set("SuppressErrors", icons_get_selected(choices_window, CHOICE_ICON_SUPPRESS_ERRORS));
	config_opt_set("FullInfoDisplay", icons_get_selected(choices_window, CHOICE_ICON_FULL_INFO));
	config_opt_set("ConfirmHistoryAdd", icons_get_selected(choices_window, CHOICE_ICON_CONFIRM_HISTORY));
	config_opt_set("QuitAsPlugin", icons_get_selected(choices_window, CHOICE_ICON_PLUGIN_QUIT));
	config_opt_set("SearchWindAsPlugin", icons_get_selected(choices_window, CHOICE_ICON_PLUGIN_WINDOW));
	config_opt_set("ScrollResults", icons_get_selected(choices_window, CHOICE_ICON_AUTOSCROLL));
	config_opt_set("FileMenuSprites", icons_get_selected(choices_window, CHOICE_ICON_MENU_ICONS));

	config_int_set("HistorySize", atoi(icons_get_indirected_text_addr(choices_window, CHOICE_ICON_HISTORY_SIZE)));
	*/
}


/**
 * Refresh the Search dialogue, to reflech changed icon states.
 */

static void dialogue_redraw_window(void)
{
	/*
	wimp_set_icon_state(choices_window, CHOICE_ICON_SEARCH_PATH, 0, 0);
	wimp_set_icon_state(choices_window, CHOICE_ICON_HISTORY_SIZE, 0, 0);

	icons_replace_caret_in_window(choices_window);
	*/
}


/**
 * Process mouse clicks in the Search dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void dialogue_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	switch ((int) pointer->i) {
	case DIALOGUE_ICON_SEARCH:
		if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
			dialogue_read_window();

			if (pointer->buttons == wimp_CLICK_SELECT)
				dialogue_close_window();
		}
		break;

	case DIALOGUE_ICON_CANCEL:
		if (pointer->buttons == wimp_CLICK_SELECT) {
			dialogue_close_window();
		} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			dialogue_set_window();
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
	}
}


/**
 * Process keypresses in the Search window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool dialogue_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		dialogue_read_window();
		//config_save();
		dialogue_close_window();
		break;

	case wimp_KEY_ESCAPE:
		dialogue_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

#if 0
/**
 * Check incoming Message_DataSave to see if it's a file being dropped into the
 * the PDF filename icon.
 *
 * \param *message		The incoming message block.
 * \return			TRUE if we claim the message as intended for us; else FALSE.
 */

static osbool handle_choices_icon_drop(wimp_message *message)
{
	wimp_full_message_data_xfer	*datasave = (wimp_full_message_data_xfer *) message;

	char				*insert, *end, path[256], *p;

	/* If it isn't our window, don't claim the message as someone else
	 * might want it.
	 */

	if (datasave == NULL || datasave->w != choices_window)
		return FALSE;

	/* If it is our window, but not the icon we care about, claim
	 * the message.
	 */

	if (datasave->i != CHOICE_ICON_SEARCH_PATH)
		return TRUE;

	/* It's our window and the correct icon, so start by copying the filename. */

	strcpy(path, datasave->file_name);

	/* If it's a folder, take just the pathname. */

	if (datasave->file_type <= 0xfff)
		string_find_pathname(path);

	insert = icons_get_indirected_text_addr(datasave->w, datasave->i);
	end = insert + icons_get_indirected_text_length(datasave->w, datasave->i) - 1;

	/* Unless Shift is pressed, find the end of the current buffer. */

	if (osbyte1(osbyte_IN_KEY, 0xfc, 0xff) == 0 && osbyte1(osbyte_IN_KEY, 0xf9, 0xff) == 0) {
		while (*insert != '\0' && insert < end)
			insert++;

		if (insert < end)
			*insert++ = ',';

		*insert = '\0';
	}

	/* Copy the new path across. */

	p = path;

	while (*p != '\0' && insert < end)
		*insert++ = *p++;

	*insert = '\0';

	if (*p != '\0')
		error_msgs_report_error("PathBufFull");

	icons_replace_caret_in_window(datasave->w);
	wimp_set_icon_state(datasave->w, datasave->i, 0, 0);

	return TRUE;
}
#endif

