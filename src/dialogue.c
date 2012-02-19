/* Locate - dialogue.c
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
#define DIALOGUE_ICON_FILENAME 4
#define DIALOGUE_ICON_PANE 5
#define DIALOGUE_ICON_SIZE 6
#define DIALOGUE_ICON_DATE 7
#define DIALOGUE_ICON_TYPE 8
#define DIALOGUE_ICON_ATTRIBUTES 9
#define DIALOGUE_ICON_CONTENTS 10
#define DIALOGUE_ICON_DRAG 11
#define DIALOGUE_ICON_IGNORE_CASE 12
#define DIALOGUE_ICON_SHOW_OPTS 13
#define DIALOGUE_ICON_SEARCH_PATH 20
#define DIALOGUE_ICON_BACKGROUND_SEARCH 16
#define DIALOGUE_ICON_IMAGE_FS 17
#define DIALOGUE_ICON_SUPPRESS_ERRORS 18
#define DIALOGUE_ICON_FULL_INFO 21

/* Size Pane Icons */

#define DIALOGUE_SIZE_MODE_MENU 1
#define DIALOGUE_SIZE_MODE 2
#define DIALOGUE_SIZE_MIN 3
#define DIALOGUE_SIZE_MIN_UNIT_MENU 4
#define DIALOGUE_SIZE_MIN_UNIT 5
#define DISLOGUE_SIZE_MAX 7
#define DIALOGUE_SIZE_MAX_UNIT_MENU 8
#define DIALOGUE_SIZE_MAX_UNIT 9

/* Date Pane Icons */

#define DIALOGUE_DATE_ICON_DATE 0
#define DIALOGUE_DATE_ICON_AGE 1

/* Attributes Pane Icons */

#define DIALOGUE_ATTRIBUTES_ICON_LOCKED 0
#define DIALOGUE_ATTRIBUTES_ICON_LOCKED_YES 1
#define DIALOGUE_ATTRIBUTES_ICON_LOCKED_NO 2
#define DIALOGUE_ATTRIBUTES_ICON_OWN_READ 3
#define DIALOGUE_ATTRIBUTES_ICON_OWN_READ_YES 4
#define DIALOGUE_ATTRIBUTES_ICON_OWN_READ_NO 5
#define DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE 6
#define DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_YES 7
#define DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_NO 8
#define DIALOGUE_ATTRIBUTES_ICON_PUB_READ 9
#define DIALOGUE_ATTRIBUTES_ICON_PUB_READ_YES 10
#define DIALOGUE_ATTRIBUTES_ICON_PUB_READ_NO 11
#define DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE 12
#define DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_YES 13
#define DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_NO 14

enum dialogue_size {
	DIALOGUE_SIZE_NOT_IMPORTANT = 0,
	DIALOGUE_SIZE_EQUAL_TO,
	DIALOGUE_SIZE_NOT_EQUAL_TO,
	DIALOGUE_SIZE_GREATER_THAN,
	DIALOGUE_SIZE_LESS_THAN,
	DIALOGUE_SIZE_BETWEEN,
	DIALOGUE_SIZE_NOT_BETWEEN
};

enum dialogue_size_unit {
	DIALOGUE_SIZE_BYTES,
	DIALOGUE_SIZE_KBYTES,
	DIALOGUE_SIZE_MBYTES
};

enum dialogue_date {
	DIALOGUE_DATE_AT_ANY_TIME = 0,
	DIALOGUE_DATE_AT,
	DIALOGUE_DATE_AT_ANY_TIME_BUT,
	DIALOGUE_DATE_AFTER,
	DIALOGUE_DATE_BEFORE,
	DIALOGUE_DATE_BETWEEN,
	DIALOGUE_DATE_NOT_BETWEEN
};

enum dialogue_age {
	DIALOGUE_AGE_ANY_AGE = 0,
	DIALOGUE_AGE_EXACTLY,
	DIALOGUE_AGE_ANY_AGE_BUT,
	DIALOGUE_AGE_LESS_THAN,
	DIALOGUE_AGE_MORE_THAN,
	DIALOGUE_AGE_BETWEEN,
	DIALOGUE_AGE_NOT_BETWEEN
};

enum dialogue_age_units {
	DIALOGUE_AGE_MINUTES,
	DIALOGUE_AGE_HOURS,
	DIALOGUE_AGE_DAYS,
	DIALOGUE_AGE_WEEKS,
	DIALOGUE_AGE_MONTHS,
	DIALOGUE_AGE_YEARS
};

enum dialogue_type {
	DIALOGUE_TYPE_OF_ANY = 0,
	DIALOGUE_TYPE_OF_TYPE,
	DIALOGUE_TYPE_NOT_OF_TYPE
};

enum dialogue_contents {
	DIALOGUE_CONTENTS_ARE_NOT_IMPORTANT = 0,
	DIALOGUE_CONTENTS_INCLUDE,
	DIALOGUE_CONTENTS_DO_NOT_INCLUDE
};

/* Settings block for a search dialogue window. */

struct dialogue_block {
	struct file_block		*file;					/**< The file to which the dialogue data belongs.	*/

	unsigned			pane;					/**< The pane which is visible.				*/

	/* The Search Path. */

	char				*path;					/**< The paths to be searched.				*/

	/* The Filename. */

	char				*filename;				/**< The filename string to be matched.			*/
	osbool				ignore_case;				/**< Whether the search is case sensitive or not.	*/

	/* The File Size. */

	enum dialogue_size		size_mode;				/**< The size comparison mode.				*/
	unsigned			size_min;				/**< The minimum size.					*/
	enum dialogue_size_unit		size_min_unit;				/**< The unit of the minimum size.			*/
	unsigned			size_max;				/**< The maximum size.					*/
	enum dialogue_size_unit		size_max_unit;				/**< The unit of the maximum size.			*/

	/* Date/Age. */

	osbool				use_age;				/**< TRUE to set by age; FALSE to set by date.		*/

	/* The Search Options. */

	osbool				background;				/**< Search in the background.				*/
	osbool				ignore_imagefs;				/**< Ignore the contents of image filing systems.	*/
	osbool				suppress_errors;			/**< Suppress errors during the search.			*/
	osbool				full_info;				/**< Use a full-info display by default.		*/

};


/* Global variables */

struct dialogue_block		*dialogue_data = NULL;				/**< The data block relating to the open dialogue.	*/

unsigned			dialogue_pane = 0;				/**< The currently displayed searh dialogue pane.	*/

static wimp_w			dialogue_window = NULL;				/**< The handle of the main search dialogue window.	*/
static wimp_w			dialogue_panes[DIALOGUE_PANES];			/**< The handles of the search dialogue panes.		*/


static void	dialogue_close_window(void);
static void	dialogue_change_pane(unsigned pane);
static void	dialogue_toggle_size(bool expand);
static void	dialogue_set_window(void);
static void	dialogue_read_window(void);
static void	dialogue_redraw_window(void);
static void	dialogue_click_handler(wimp_pointer *pointer);
static osbool	dialogue_keypress_handler(wimp_key *key);
static osbool	dialogue_icon_drop_handler(wimp_message *message);


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
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_MODE_MENU,
			templates_get_menu(TEMPLATES_MENU_SIZE_MODE), DIALOGUE_SIZE_MODE, "SizeMode");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_MIN_UNIT_MENU,
			templates_get_menu(TEMPLATES_MENU_SIZE_UNIT), DIALOGUE_SIZE_MIN_UNIT, "SizeUnit");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_MAX_UNIT_MENU,
			templates_get_menu(TEMPLATES_MENU_SIZE_UNIT), DIALOGUE_SIZE_MAX_UNIT, "SizeUnit");

	dialogue_panes[DIALOGUE_PANE_DATE] = templates_create_window("DatePane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_DATE], "Search.Date", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_keypress_handler);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE, TRUE);

	dialogue_panes[DIALOGUE_PANE_TYPE] = templates_create_window("TypePane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_TYPE], "Search.Type", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_keypress_handler);

	dialogue_panes[DIALOGUE_PANE_ATTRIBUTES] = templates_create_window("AttribPane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], "Search.Attributes", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_keypress_handler);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED_YES, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED_NO, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ_YES, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ_NO, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_YES, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_NO, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ_YES, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ_NO, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_YES, TRUE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_NO, TRUE);

	dialogue_panes[DIALOGUE_PANE_CONTENTS] = templates_create_window("ContentPane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], "Search.Contents", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_keypress_handler);

	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, dialogue_icon_drop_handler);
}


/**
 * Create a new set of dialogue data with the default values.
 *
 * \param *file			The file to which the dialogue belongs.
 * \return			Pointer to the new block, or NULL on failure.
 */

struct dialogue_block *dialogue_create(struct file_block *file)
{
	struct dialogue_block	*new;
	osbool			mem_ok = TRUE;

	if (file == NULL)
		return NULL;

	/* Allocate all of the memory that we require. */

	new = heap_alloc(sizeof(struct dialogue_block));
	if (new == NULL)
		mem_ok = FALSE;

	if (mem_ok) {
		new->path = NULL;
		new->filename = NULL;

		if (flex_alloc((flex_ptr) &(new->path), strlen(config_str_read("SearchPath")) + 1) == 0)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->filename), strlen("") + 1) == 0)
			mem_ok = FALSE;
	}

	if (!mem_ok) {
		if (new->path != NULL)
			flex_free((flex_ptr) &(new->path));
		if (new->filename != NULL)
			flex_free((flex_ptr) &(new->filename));

		if (new != NULL)
			heap_free(new);

		return NULL;
	}

	/* If the memory allocation succeeded, initialise the data in the block. */

	new->file = file;

	new->pane = DIALOGUE_PANE_SIZE;

	strcpy(new->path, config_str_read("SearchPath"));

	/* Filename Details */

	strcpy(new->filename, "");
	new->ignore_case = TRUE;

	/* Size Details */

	new->size_mode = DIALOGUE_SIZE_NOT_IMPORTANT;
	new->size_min = 0;
	new->size_min_unit = DIALOGUE_SIZE_BYTES;
	new->size_max = 0;
	new->size_max_unit = DIALOGUE_SIZE_BYTES;

	/* Date and Age. */

	new->use_age = FALSE;

	/* Search Options */

	new->background = config_opt_read("Multitask");
	new->ignore_imagefs = config_opt_read("ImageFS");
	new->suppress_errors = config_opt_read("SuppressErrors");
	new->full_info = config_opt_read("FullInfoDisplay");

	return new;
}


/**
 * Destroy a dialogue and its data.
 *
 * \param *dialogue		The dialogue to be destroyed.
 */

void dialogue_destroy(struct dialogue_block *dialogue)
{
	if (dialogue == NULL)
		return;

	flex_free((flex_ptr) &(dialogue->path));
	flex_free((flex_ptr) &(dialogue->filename));

	heap_free(dialogue);
}


/**
 * Open the Search Dialogue window at the mouse pointer.
 *
 * \param *dialogue		The dialogue details to use to open the window.
 * \param *pointer		The details of the pointer to open the window at.
 */

void dialogue_open_window(struct dialogue_block *dialogue, wimp_pointer *pointer)
{
	if (dialogue == NULL || windows_get_open(dialogue_window))
		return;

	dialogue_data = dialogue;

	dialogue_pane = dialogue->pane;

	icons_set_radio_group_selected(dialogue_window, dialogue_pane, DIALOGUE_PANES,
			DIALOGUE_ICON_SIZE, DIALOGUE_ICON_DATE, DIALOGUE_ICON_TYPE,
			DIALOGUE_ICON_ATTRIBUTES, DIALOGUE_ICON_CONTENTS);

	icons_set_selected(dialogue_window, DIALOGUE_ICON_SHOW_OPTS, FALSE);

	dialogue_set_window();

	windows_open_with_pane_centred_at_pointer(dialogue_window, dialogue_panes[dialogue_pane],
			DIALOGUE_ICON_PANE, 0, pointer);
	dialogue_toggle_size(FALSE);

	icons_put_caret_at_end(dialogue_window, DIALOGUE_ICON_SEARCH_PATH);
}


/**
 * Close the Search Dialogue window.
 */

static void dialogue_close_window(void)
{
	wimp_close_window(dialogue_window);

	if (dialogue_data != NULL)
		file_destroy(dialogue_data->file);

	dialogue_data = NULL;
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
 * Toggle the Search Dialogue to show or hide the search options at the
 * bottom.  The expansion and contraction sizes come from the minimum
 * y extent and the maximum y extent as saved in the Templates file.
 *
 * \param expand		TRUE to expand the window; FALSE to contract.
 */

static void dialogue_toggle_size(bool expand)
{
	wimp_window_info	info;
	int			height;

	if (!windows_get_open(dialogue_window))
		return;

	info.w = dialogue_window;
	if (xwimp_get_window_info_header_only(&info) != NULL)
		return;

	height = (expand) ? (info.extent.y1 - info.extent.y0) : info.ymin;

	if (info.visible.y0 == sf_ICONBAR_HEIGHT)
		info.visible.y1 = info.visible.y0 + height;
	else
		info.visible.y0 = info.visible.y1 - height;

	if (info.visible.y0 < sf_ICONBAR_HEIGHT) {
		info.visible.y0 = sf_ICONBAR_HEIGHT;
		info.visible.y1 = info.visible.y0 + height;

	}

	xwimp_open_window((wimp_open *) &info);
}

/**
 * Set the contents of the Search Dialogue window to reflect the current settings.
 */

static void dialogue_set_window(void)
{
	icons_printf(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, "%s", dialogue_data->path);

	icons_printf(dialogue_window, DIALOGUE_ICON_FILENAME, "%s", dialogue_data->filename);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_IGNORE_CASE, dialogue_data->ignore_case);

	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_MODE_MENU, dialogue_data->size_mode);
	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_MIN_UNIT_MENU, dialogue_data->size_min_unit);
	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_MAX_UNIT_MENU, dialogue_data->size_max_unit);

	icons_set_selected(dialogue_window, DIALOGUE_ICON_BACKGROUND_SEARCH, dialogue_data->background);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_IMAGE_FS, dialogue_data->ignore_imagefs);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_SUPPRESS_ERRORS, dialogue_data->suppress_errors);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_FULL_INFO, dialogue_data->full_info);

	/*
	icons_printf(choices_window, CHOICE_ICON_SEARCH_PATH, "%s", config_str_read("SearchPath"));

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

	if (pointer->w == dialogue_window) {
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

		case DIALOGUE_ICON_SHOW_OPTS:
			dialogue_toggle_size(icons_get_selected(dialogue_window, DIALOGUE_ICON_SHOW_OPTS));
			break;
		}
	} else if (pointer->w == dialogue_panes[DIALOGUE_PANE_DATE]) {

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


/**
 * Check incoming Message_DataSave to see if it's a file being dropped into the
 * the search path icon.
 *
 * \param *message		The incoming message block.
 * \return			TRUE if we claim the message as intended for us; else FALSE.
 */

static osbool dialogue_icon_drop_handler(wimp_message *message)
{
	wimp_full_message_data_xfer	*datasave = (wimp_full_message_data_xfer *) message;

	char				*insert, *end, path[256], *p;

	/* If it isn't our window, don't claim the message as someone else
	 * might want it.
	 */

	if (datasave == NULL || datasave->w != dialogue_window)
		return FALSE;

	/* If it is our window, but not the icon we care about, claim
	 * the message.
	 */

	if (datasave->i != DIALOGUE_ICON_SEARCH_PATH)
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

