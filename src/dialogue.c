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
 * \file: dialogue.c
 *
 * Search dialogue implementation.
 */

/* ANSI C Header files. */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* OSLib Header files. */

#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/osword.h"
#include "oslib/osfscontrol.h"
#include "oslib/territory.h"
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

#include "dialogue.h"

#include "dataxfer.h"
#include "datetime.h"
#include "discfile.h"
#include "flexutils.h"
#include "ihelp.h"
#include "saveas.h"
#include "search.h"
#include "settime.h"
#include "templates.h"
#include "typemenu.h"


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
#define DIALOGUE_ICON_STORE_ALL 16
#define DIALOGUE_ICON_IMAGE_FS 17
#define DIALOGUE_ICON_SUPPRESS_ERRORS 18
#define DIALOGUE_ICON_FULL_INFO 21
#define DIALOGUE_ICON_NAME_MODE_MENU 22
#define DIALOGUE_ICON_NAME_MODE 23

/* Size Pane Icons */

#define DIALOGUE_SIZE_ICON_MODE_MENU 1
#define DIALOGUE_SIZE_ICON_MODE 2
#define DIALOGUE_SIZE_ICON_MIN 3
#define DIALOGUE_SIZE_ICON_MIN_UNIT_MENU 4
#define DIALOGUE_SIZE_ICON_MIN_UNIT 5
#define DIALOGUE_SIZE_ICON_AND 6
#define DIALOGUE_SIZE_ICON_MAX 7
#define DIALOGUE_SIZE_ICON_MAX_UNIT_MENU 8
#define DIALOGUE_SIZE_ICON_MAX_UNIT 9

/* Date Pane Icons */

#define DIALOGUE_DATE_ICON_DATE 0
#define DIALOGUE_DATE_ICON_AGE 1
#define DIALOGUE_DATE_ICON_DATE_LABEL 2
#define DIALOGUE_DATE_ICON_DATE_MODE 3
#define DIALOGUE_DATE_ICON_DATE_MODE_MENU 4
#define DIALOGUE_DATE_ICON_DATE_FROM 5
#define DIALOGUE_DATE_ICON_DATE_FROM_SET 6
#define DIALOGUE_DATE_ICON_DATE_AND 7
#define DIALOGUE_DATE_ICON_DATE_TO 8
#define DIALOGUE_DATE_ICON_DATE_TO_SET 9
#define DIALOGUE_DATE_ICON_AGE_LABEL 10
#define DIALOGUE_DATE_ICON_AGE_MODE 11
#define DIALOGUE_DATE_ICON_AGE_MODE_MENU 12
#define DIALOGUE_DATE_ICON_AGE_MIN 13
#define DIALOGUE_DATE_ICON_AGE_MIN_UNIT 14
#define DIALOGUE_DATE_ICON_AGE_MIN_UNIT_MENU 15
#define DIALOGUE_DATE_ICON_AGE_MIN_UNIT_OLD 16
#define DIALOGUE_DATE_ICON_AGE_AND 17
#define DIALOGUE_DATE_ICON_AGE_MAX 18
#define DIALOGUE_DATE_ICON_AGE_MAX_UNIT 19
#define DIALOGUE_DATE_ICON_AGE_MAX_UNIT_MENU 20
#define DIALOGUE_DATE_ICON_AGE_MAX_UNIT_OLD 21


/* Type Pane Icons */

#define DIALOGUE_TYPE_ICON_FILE 0
#define DIALOGUE_TYPE_ICON_DIRECTORY 1
#define DIALOGUE_TYPE_ICON_APPLICATION 2
#define DIALOGUE_TYPE_ICON_MODE 3
#define DIALOGUE_TYPE_ICON_MODE_MENU 4
#define DIALOGUE_TYPE_ICON_TYPE 5
#define DIALOGUE_TYPE_ICON_TYPE_MENU 6

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

/* Contents Pane Icons */

#define DIALOGUE_CONTENTS_ICON_MODE 1
#define DIALOGUE_CONTENTS_ICON_MODE_MENU 2
#define DIALOGUE_CONTENTS_ICON_TEXT 3
#define DIALOGUE_CONTENTS_ICON_IGNORE_CASE 4
#define DIALOGUE_CONTENTS_ICON_CTRL_CHARS 5

/* Dialogue Menu Entries */

#define DIALOGUE_MENU_SAVE_SEARCH 0

#define DIALOGUE_MAX_FILE_LINE 1024

#define MAX_BUFFER(current, size) (((size) > (current)) ? (size) : (current))

enum dialogue_name {
	DIALOGUE_NAME_NOT_IMPORTANT = 0,
	DIALOGUE_NAME_EQUAL_TO,
	DIALOGUE_NAME_NOT_EQUAL_TO,
	DIALOGUE_NAME_CONTAINS,
	DIALOGUE_NAME_DOES_NOT_CONTAIN
};

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

enum dialogue_age_unit {
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

	enum dialogue_name		name_mode;				/**< The filename comparison mode.			*/
	char				*filename;				/**< The filename string to be matched.			*/
	osbool				ignore_case;				/**< Whether the search is case sensitive or not.	*/

	/* The File Size. */

	enum dialogue_size		size_mode;				/**< The size comparison mode.				*/
	unsigned			size_min;				/**< The minimum size.					*/
	enum dialogue_size_unit		size_min_unit;				/**< The unit of the minimum size.			*/
	unsigned			size_max;				/**< The maximum size.					*/
	enum dialogue_size_unit		size_max_unit;				/**< The unit of the maximum size.			*/

	/* The File Date/Age. */

	osbool				use_age;				/**< TRUE to set by age; FALSE to set by date.		*/

	enum dialogue_date		date_mode;				/**< The date comparison mode.				*/
	os_date_and_time		date_min;				/**< The minimum date.					*/
	enum datetime_date_status	date_min_status;			/**< The status of the minimum date value.		*/
	os_date_and_time		date_max;				/**< The maximum date.					*/
	enum datetime_date_status	date_max_status;			/**< The status of the maximum date value.		*/

	enum dialogue_age		age_mode;				/**< The age comparison mode.				*/
	unsigned			age_min;				/**< The minimum age.					*/
	enum dialogue_age_unit		age_min_unit;				/**< The unit of the minimum age.			*/
	unsigned			age_max;				/**< The maximum age.					*/
	enum dialogue_age_unit		age_max_unit;				/**< The unit of the maximum age.			*/

	/* The File Type. */

	osbool				type_files;				/**< TRUE to match applications; FALSE to not.		*/
	osbool				type_directories;			/**< TRUE to match directories; FALSE to not.		*/
	osbool				type_applications;			/**< TRUE to match files; FALSE to not.			*/
	enum dialogue_type		type_mode;				/**< The file type comparison mode.			*/
	unsigned			*type_types;				/**< 0xffffffffu terminated list of file types.		*/

	/* The File Attributes. */

	osbool				attributes_locked;			/**< TRUE to match Locked state; FALSE to not.		*/
	osbool				attributes_locked_yes;			/**< TRUE to match Locked files; FALSE for unlocked.	*/
	osbool				attributes_owner_read;			/**< TRUE to match Owner Read state; FALSE to not.	*/
	osbool				attributes_owner_read_yes;		/**< TRUE to match Yes; FALSE to match No.		*/
	osbool				attributes_owner_write;			/**< TRUE to match Owner Write state; FALSE to not.	*/
	osbool				attributes_owner_write_yes;		/**< TRUE to match Yes; FALSE to match No.		*/
	osbool				attributes_public_read;			/**< TRUE to match Public Read state; FALSE to not.	*/
	osbool				attributes_public_read_yes;		/**< TRUE to match Yes; FALSE to match No.		*/
	osbool				attributes_public_write;		/**< TRUE to match Public Write state; FALSE to not.	*/
	osbool				attributes_public_write_yes;		/**< TRUE to match Yes; FALSE to match No.		*/

	/* The File Contents. */

	enum dialogue_contents		contents_mode;				/**< The file contents comparison mode.			*/
	char				*contents_text;				/**< The text to match in a file.			*/
	osbool				contents_ignore_case;			/**< TRUE to ignore case when matching; FALSE to not.	*/
	osbool				contents_ctrl_chars;			/**< TRUE to allow control characters; FALSE to not.	*/

	/* The Search Options. */

	osbool				store_all;				/**< Store all file details.				*/
	osbool				ignore_imagefs;				/**< Ignore the contents of image filing systems.	*/
	osbool				suppress_errors;			/**< Suppress errors during the search.			*/
	osbool				full_info;				/**< Use a full-info display by default.		*/
};


/* Global variables */

struct dialogue_block		*dialogue_data = NULL;				/**< The data block relating to the open dialogue.	*/

unsigned			dialogue_pane = 0;				/**< The currently displayed searh dialogue pane.	*/

static wimp_w			dialogue_window = NULL;				/**< The handle of the main search dialogue window.	*/
static wimp_w			dialogue_panes[DIALOGUE_PANES];			/**< The handles of the search dialogue panes.		*/

static wimp_menu		*dialogue_menu = NULL;				/**< The main search window menu.			*/
static wimp_menu		*dialogue_name_mode_menu = NULL;		/**< The Name Mode popup menu.				*/
static wimp_menu		*dialogue_size_mode_menu = NULL;		/**< The Size Mode popup menu.				*/
static wimp_menu		*dialogue_size_unit_menu = NULL;		/**< The Size Unit popup menu.				*/
static wimp_menu		*dialogue_date_mode_menu = NULL;		/**< The Date Mode popup menu.				*/
static wimp_menu		*dialogue_age_mode_menu = NULL;			/**< The Age Mode popup menu.				*/
static wimp_menu		*dialogue_age_unit_menu = NULL;			/**< The Age Unit popup menu.				*/
static wimp_menu		*dialogue_type_mode_menu = NULL;		/**< The Type Mode popup menu.				*/
static wimp_menu		*dialogue_type_list_menu = NULL;		/**< The Filetype List popup menu.			*/
static wimp_menu		*dialogue_contents_mode_menu = NULL;		/**< The Contents Mode popup menu.			*/

static struct saveas_block	*dialogue_save_search = NULL;			/**< The Save Search savebox data handle.		*/


static struct	dialogue_block *dialogue_load_legacy_file(struct file_block *file, struct discfile_block *load);
static void	dialogue_close_window(void);
static void	dialogue_change_pane(unsigned pane);
static void	dialogue_toggle_size(bool expand);
static void	dialogue_set_window(struct dialogue_block *dialogue);
static void	dialogue_shade_window(void);
static void	dialogue_shade_size_pane(void);
static void	dialogue_shade_date_pane(void);
static void	dialogue_shade_type_pane(void);
static void	dialogue_shade_attributes_pane(void);
static void	dialogue_shade_contents_pane(void);
static void	dialogue_write_filetype_list(char *buffer, size_t length, unsigned types[]);
static osbool	dialogue_read_window(struct dialogue_block *dialogue);
static osbool	dialogue_read_filetype_list(flex_ptr list, char *buffer);
static void	dialogue_redraw_window(void);
static void	dialogue_click_handler(wimp_pointer *pointer);
static osbool	dialogue_keypress_handler(wimp_key *key);
static void	dialogue_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void	dialogue_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning);
static void	dialogue_menu_selection_handler(wimp_w window, wimp_menu *menu, wimp_selection *selection);
static void	dialogue_menu_close_handler(wimp_w w, wimp_menu *menu);
static void	dialogue_drag_end_handler(wimp_pointer *pointer, void *data);
static osbool	dialogue_xfer_save_handler(char *filename, void *data);
static osbool	dialogue_icon_drop_handler(wimp_message *message);
static void	dialogue_start_search(struct dialogue_block *dialogue);
static int	dialogue_scale_size(unsigned base, enum dialogue_size_unit unit, osbool top);
static void	dialogue_scale_age(os_date_and_time date, unsigned base, enum dialogue_age_unit unit, int round);
static osbool	dialogue_save_settings(char *filename, osbool selection, void *data);
static void	dialogue_dump_settings(struct dialogue_block *dialogue);


/**
 * Initialise the Search Dialogue module.
 */

void dialogue_initialise(void)
{
	wimp_window	*def = templates_load_window("Search");
	int		buf_size = config_int_read("PathBufSize");

	if (def == NULL)
		error_msgs_report_fatal("BadTemplate");

	/* Initialise the menus used in the window. */

	dialogue_menu = templates_get_menu(TEMPLATES_MENU_SEARCH);
	dialogue_name_mode_menu = templates_get_menu(TEMPLATES_MENU_NAME_MODE);
	dialogue_size_mode_menu = templates_get_menu(TEMPLATES_MENU_SIZE_MODE);
	dialogue_size_unit_menu = templates_get_menu(TEMPLATES_MENU_SIZE_UNIT);
	dialogue_date_mode_menu = templates_get_menu(TEMPLATES_MENU_DATE_MODE);
	dialogue_age_mode_menu = templates_get_menu(TEMPLATES_MENU_AGE_MODE);
	dialogue_age_unit_menu = templates_get_menu(TEMPLATES_MENU_AGE_UNIT);
	dialogue_type_mode_menu = templates_get_menu(TEMPLATES_MENU_TYPE_MODE);
	dialogue_contents_mode_menu = templates_get_menu(TEMPLATES_MENU_CONTENTS_MODE);

	dialogue_save_search = saveas_create_dialogue(FALSE, "file_1a1", dialogue_save_settings);

	/* Initialise the main window. */

	def->icons[DIALOGUE_ICON_SEARCH_PATH].data.indirected_text.text = heap_alloc(buf_size);
	def->icons[DIALOGUE_ICON_SEARCH_PATH].data.indirected_text.size = buf_size;
	dialogue_window = wimp_create_window(def);
	icons_printf(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, "");
	free(def);
	ihelp_add_window(dialogue_window, "Search", NULL);
	event_add_window_menu(dialogue_window, dialogue_menu);
	event_add_window_mouse_event(dialogue_window, dialogue_click_handler);
	event_add_window_key_event(dialogue_window, dialogue_keypress_handler);
	event_add_window_menu_prepare(dialogue_window, dialogue_menu_prepare_handler);
	event_add_window_menu_warning(dialogue_window, dialogue_menu_warning_handler);
	event_add_window_menu_selection(dialogue_window, dialogue_menu_selection_handler);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_SIZE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_DATE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_TYPE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_ATTRIBUTES, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_CONTENTS, FALSE);
	event_add_window_icon_popup(dialogue_window, DIALOGUE_ICON_NAME_MODE_MENU,
			dialogue_name_mode_menu, DIALOGUE_ICON_NAME_MODE, "NameMode");

	/* Initialise the size pane. */

	dialogue_panes[DIALOGUE_PANE_SIZE] = templates_create_window("SizePane");
	ihelp_add_window(dialogue_panes[DIALOGUE_PANE_SIZE], "Size", NULL);
	event_add_window_menu(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_menu);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_keypress_handler);
	event_add_window_menu_prepare(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_menu_prepare_handler);
	event_add_window_menu_warning(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_menu_warning_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_menu_selection_handler);
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MODE_MENU,
			dialogue_size_mode_menu, DIALOGUE_SIZE_ICON_MODE, "SizeMode");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MIN_UNIT_MENU,
			dialogue_size_unit_menu, DIALOGUE_SIZE_ICON_MIN_UNIT, "SizeUnit");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MAX_UNIT_MENU,
			dialogue_size_unit_menu, DIALOGUE_SIZE_ICON_MAX_UNIT, "SizeUnit");

	/* Initialise the date pane. */

	dialogue_panes[DIALOGUE_PANE_DATE] = templates_create_window("DatePane");
	ihelp_add_window(dialogue_panes[DIALOGUE_PANE_DATE], "Date", NULL);
	event_add_window_menu(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_menu);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_keypress_handler);
	event_add_window_menu_prepare(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_menu_prepare_handler);
	event_add_window_menu_warning(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_menu_warning_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_menu_selection_handler);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE, FALSE);
	event_add_window_icon_radio(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE, FALSE);
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_MODE_MENU,
			dialogue_date_mode_menu, DIALOGUE_DATE_ICON_DATE_MODE, "DateMode");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MODE_MENU,
			dialogue_age_mode_menu, DIALOGUE_DATE_ICON_AGE_MODE, "AgeMode");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MIN_UNIT_MENU,
			dialogue_age_unit_menu, DIALOGUE_DATE_ICON_AGE_MIN_UNIT, "AgeUnit");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MAX_UNIT_MENU,
			dialogue_age_unit_menu, DIALOGUE_DATE_ICON_AGE_MAX_UNIT, "AgeUnit");

	/* Initialise the type pane. */

	dialogue_panes[DIALOGUE_PANE_TYPE] = templates_create_window("TypePane");
	ihelp_add_window(dialogue_panes[DIALOGUE_PANE_TYPE], "Type", NULL);
	event_add_window_menu(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_menu);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_keypress_handler);
	event_add_window_menu_prepare(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_menu_prepare_handler);
	event_add_window_menu_warning(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_menu_warning_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_menu_selection_handler);
	event_add_window_menu_close(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_menu_close_handler);
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_MODE_MENU,
			dialogue_type_mode_menu, DIALOGUE_TYPE_ICON_MODE, "TypeMode");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE_MENU,
			dialogue_type_mode_menu, -1, NULL);

	/* Initialise the attributes pane. */

	dialogue_panes[DIALOGUE_PANE_ATTRIBUTES] = templates_create_window("AttribPane");
	ihelp_add_window(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], "Attributes", NULL);
	event_add_window_menu(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_menu);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_keypress_handler);
	event_add_window_menu_prepare(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_menu_prepare_handler);
	event_add_window_menu_warning(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_menu_warning_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_menu_selection_handler);
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

	/* Initialise the content pane. */

	dialogue_panes[DIALOGUE_PANE_CONTENTS] = templates_create_window("ContentPane");
	ihelp_add_window(dialogue_panes[DIALOGUE_PANE_CONTENTS], "Contents", NULL);
	event_add_window_menu(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_menu);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_keypress_handler);
	event_add_window_menu_prepare(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_menu_prepare_handler);
	event_add_window_menu_warning(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_menu_warning_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_menu_selection_handler);
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_MODE_MENU,
			dialogue_contents_mode_menu, DIALOGUE_CONTENTS_ICON_MODE, "ContentsMode");

	event_add_message_handler(message_DATA_LOAD, EVENT_MESSAGE_INCOMING, dialogue_icon_drop_handler);
}


/**
 * Create a new set of dialogue data with the default values.
 *
 * \param *file			The file to which the dialogue belongs.
 * \param *path			The search path to use, or NULL for default.
 * \param *template		A dialogue to copy the settings from, or NULL for
 *				default values.
 * \return			Pointer to the new block, or NULL on failure.
 */

struct dialogue_block *dialogue_create(struct file_block *file, char *path, struct dialogue_block *template)
{
	struct dialogue_block	*new;
	osbool			mem_ok = TRUE;
	int			i;

	if (file == NULL)
		return NULL;

	if (path == NULL)
		path = config_str_read("SearchPath");

	/* Count the number of filetypes that are in the block, then
	 * allocate all of the memory that we require.
	 */

	i = 1;

	if (template != NULL)
		while (template->type_types[i - 1] != 0xffffffffu)
			i++;

	new = heap_alloc(sizeof(struct dialogue_block));
	if (new == NULL)
		mem_ok = FALSE;

	if (mem_ok) {
		new->path = NULL;
		new->filename = NULL;
		new->type_types = NULL;
		new->contents_text = NULL;

		if (flex_alloc((flex_ptr) &(new->path), strlen((template != NULL) ? template->path : path) + 1) == 0)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->filename), strlen((template != NULL) ? template->filename : "") + 1) == 0)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->type_types), i * sizeof(unsigned)) == 0)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->contents_text), strlen((template != NULL) ? template->contents_text : "") + 1) == 0)
			mem_ok = FALSE;

	}

	if (!mem_ok) {
		dialogue_destroy(new);
		return NULL;
	}

	/* If the memory allocation succeeded, initialise the data in the block. */

	new->file = file;

	new->pane = (template != NULL) ? template->pane : DIALOGUE_PANE_SIZE;

	strcpy(new->path, (template != NULL) ? template->path : path);

	/* Filename Details */

	new->name_mode = (template != NULL) ? template->name_mode : DIALOGUE_NAME_EQUAL_TO;
	strcpy(new->filename, (template != NULL) ? template->filename : "");
	new->ignore_case = (template != NULL) ? template->ignore_case : TRUE;

	/* Size Details */

	new->size_mode = (template != NULL) ? template->size_mode : DIALOGUE_SIZE_NOT_IMPORTANT;
	new->size_min = (template != NULL) ? template->size_min : 0;
	new->size_min_unit = (template != NULL) ? template->size_min_unit : DIALOGUE_SIZE_KBYTES;
	new->size_max = (template != NULL) ? template->size_max : 0;
	new->size_max_unit = (template != NULL) ? template->size_max_unit : DIALOGUE_SIZE_KBYTES;

	/* Date and Age Details. */

	new->use_age = (template != NULL) ? template->use_age : FALSE;

	new->date_mode = (template != NULL) ? template->date_mode : DIALOGUE_DATE_AT_ANY_TIME;
	for (i = 0; i < 5; i++) {
		new->date_min[i] = (template != NULL) ? template->date_min[i] : 0;
		new->date_max[i] = (template != NULL) ? template->date_max[i] : 0;
	}
	new->date_min_status = (template != NULL) ? template->date_min_status : DATETIME_DATE_INVALID;
	new->date_max_status = (template != NULL) ? template->date_max_status : DATETIME_DATE_INVALID;

	new->age_mode = (template != NULL) ? template->age_mode : DIALOGUE_AGE_ANY_AGE;
	new->age_min = (template != NULL) ? template->age_min : 0;
	new->age_min_unit = (template != NULL) ? template->age_min_unit : DIALOGUE_AGE_DAYS;
	new->age_max = (template != NULL) ? template->age_max : 0;
	new->age_max_unit = (template != NULL) ? template->age_max_unit : DIALOGUE_AGE_DAYS;

	/* Type Details. */

	new->type_files = (template != NULL) ? template->type_files : TRUE;
	new->type_directories = (template != NULL) ? template->type_directories : TRUE;
	new->type_applications = (template != NULL) ? template->type_applications : TRUE;
	new->type_mode = (template != NULL) ? template->type_mode : DIALOGUE_TYPE_OF_ANY;

	for (i = 0; template != NULL && template->type_types[i] != 0xffffffffu; i++)
		new->type_types[i] = template->type_types[i];

	new->type_types[i] = 0xffffffffu;

	/* Attribute Details. */

	new->attributes_locked = (template != NULL) ? template->attributes_locked : FALSE;
	new->attributes_locked_yes = (template != NULL) ? template->attributes_locked_yes : FALSE;
	new->attributes_owner_read = (template != NULL) ? template->attributes_owner_read : FALSE;
	new->attributes_owner_read_yes = (template != NULL) ? template->attributes_owner_read_yes : TRUE;
	new->attributes_owner_write = (template != NULL) ? template->attributes_owner_write : FALSE;
	new->attributes_owner_write_yes = (template != NULL) ? template->attributes_owner_write_yes : TRUE;
	new->attributes_public_read = (template != NULL) ? template->attributes_public_read : FALSE;
	new->attributes_public_read_yes = (template != NULL) ? template->attributes_public_read_yes : TRUE;
	new->attributes_public_write = (template != NULL) ? template->attributes_public_write : FALSE;
	new->attributes_public_write_yes = (template != NULL) ? template->attributes_public_write_yes : TRUE;

	/* Contents Details. */

	new->contents_mode = (template != NULL) ? template->contents_mode : DIALOGUE_CONTENTS_ARE_NOT_IMPORTANT;
	strcpy(new->contents_text, (template != NULL) ? template->contents_text : "");
	new->contents_ignore_case = (template != NULL) ? template->contents_ignore_case : TRUE;
	new->contents_ctrl_chars = (template != NULL) ? template->contents_ctrl_chars : FALSE;

	/* Search Options */

	new->store_all = (template != NULL) ? template->store_all : config_opt_read("StoreAll");
	new->ignore_imagefs = (template != NULL) ? template->ignore_imagefs : config_opt_read("ImageFS");
	new->suppress_errors = (template != NULL) ? template->suppress_errors : config_opt_read("SuppressErrors");
	new->full_info = (template != NULL) ? template->full_info : config_opt_read("FullInfoDisplay");

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

	if (dialogue->path != NULL)
		flex_free((flex_ptr) &(dialogue->path));
	if (dialogue->filename != NULL)
		flex_free((flex_ptr) &(dialogue->filename));
	if (dialogue->type_types != NULL)
		flex_free((flex_ptr) &(dialogue->type_types));
	if (dialogue->contents_text != NULL)
		flex_free((flex_ptr) &(dialogue->contents_text));

	heap_free(dialogue);
}


/**
 * Save a dialogue's settings to an open disc file.
 *
 * \param *dialogue		The dialogue to be saved.
 * \param *out			The handle of the file to save to.
 */

void dialogue_save_file(struct dialogue_block *dialogue, struct discfile_block *out)
{
	if (dialogue == NULL || out == NULL)
		return;

	discfile_start_section(out, DISCFILE_SECTION_DIALOGUE);

	/* Write out the dialogue options. */

	discfile_start_chunk(out, DISCFILE_CHUNK_OPTIONS);
	discfile_write_option_unsigned(out, "PAN", dialogue->pane);

	/* The Search Path. */

	discfile_write_option_string(out, "PAT", dialogue->path);

	/* The Filename. */

	discfile_write_option_unsigned(out, "FMD", dialogue->name_mode);
	discfile_write_option_string(out, "FNM", dialogue->filename);
	discfile_write_option_boolean(out, "FIC", dialogue->ignore_case);

	/* The File Size. */

	discfile_write_option_unsigned(out, "SMD", dialogue->size_mode);
	discfile_write_option_unsigned(out, "SMN", dialogue->size_min);
	discfile_write_option_unsigned(out, "SUM", dialogue->size_min_unit);
	discfile_write_option_unsigned(out, "SMX", dialogue->size_max);
	discfile_write_option_unsigned(out, "SUX", dialogue->size_max_unit);

	/* The File Date/Age. */

	discfile_write_option_boolean(out, "AGE", dialogue->use_age);

	discfile_write_option_unsigned(out, "DMD", dialogue->date_mode);
	discfile_write_option_date(out, "DMN", dialogue->date_min);
	discfile_write_option_unsigned(out, "DSM", dialogue->date_min_status);
	discfile_write_option_date(out, "DMX", dialogue->date_max);
	discfile_write_option_unsigned(out, "DSX", dialogue->date_max_status);

	discfile_write_option_unsigned(out, "AMD", dialogue->age_mode);
	discfile_write_option_unsigned(out, "AMN", dialogue->age_min);
	discfile_write_option_unsigned(out, "AUM", dialogue->age_min_unit);
	discfile_write_option_unsigned(out, "AMX", dialogue->age_max);
	discfile_write_option_unsigned(out, "AUX", dialogue->age_max_unit);

	/* The File Type. */

	discfile_write_option_boolean(out, "TFI", dialogue->type_files);
	discfile_write_option_boolean(out, "TDR", dialogue->type_directories);
	discfile_write_option_boolean(out, "TAP", dialogue->type_applications);
	discfile_write_option_unsigned(out, "TMD", dialogue->type_mode);
	discfile_write_option_unsigned_array(out, "TTL", dialogue->type_types, 0xffffffffu);

	/* The File Attributes. */

	discfile_write_option_boolean(out, "PLK", dialogue->attributes_locked);
	discfile_write_option_boolean(out, "PLY", dialogue->attributes_locked_yes);
	discfile_write_option_boolean(out, "Prd", dialogue->attributes_owner_read);
	discfile_write_option_boolean(out, "PrY", dialogue->attributes_owner_read_yes);
	discfile_write_option_boolean(out, "Pwr", dialogue->attributes_owner_write);
	discfile_write_option_boolean(out, "PwY", dialogue->attributes_owner_write_yes);
	discfile_write_option_boolean(out, "PRD", dialogue->attributes_public_read);
	discfile_write_option_boolean(out, "PRY", dialogue->attributes_public_read_yes);
	discfile_write_option_boolean(out, "PWR", dialogue->attributes_public_write);
	discfile_write_option_boolean(out, "PRY", dialogue->attributes_public_write_yes);

	/* The File Contents. */

	discfile_write_option_unsigned(out, "CMD", dialogue->contents_mode);
	discfile_write_option_string(out, "CTX", dialogue->contents_text);
	discfile_write_option_boolean(out, "CIC", dialogue->contents_ignore_case);
	discfile_write_option_boolean(out, "CCC", dialogue->contents_ctrl_chars);

	/* The Search Options. */

	discfile_write_option_boolean(out, "ALL", dialogue->store_all);
	discfile_write_option_boolean(out, "IMG", dialogue->ignore_imagefs);
	discfile_write_option_boolean(out, "ERR", dialogue->suppress_errors);
	discfile_write_option_boolean(out, "FUL", dialogue->full_info);

	discfile_end_chunk(out);
	discfile_end_section(out);
}


/**
 * Load a dialogue's settings from an open disc file and create a new
 * dialogue structure from them.
 *
 * \param *file			The file to which the dialogue will belong.
 * \param *load			The handle of the file to load from.
 * \return			The handle of the new dialogue, or NULL.
 */

struct dialogue_block *dialogue_load_file(struct file_block *file, struct discfile_block *load)
{
	struct dialogue_block	*dialogue;

	if (file == NULL || load == NULL)
		return NULL;

	if (discfile_read_format(load) != DISCFILE_LOCATE2)
		return dialogue_load_legacy_file(file, load);

	dialogue = dialogue_create(file, NULL, NULL);
	if (dialogue == NULL)
		return NULL;

	if (!discfile_open_section(load, DISCFILE_SECTION_DIALOGUE) || !discfile_open_chunk(load, DISCFILE_CHUNK_OPTIONS)) {
		dialogue_destroy(dialogue);
		return NULL;
	}

	/* Read in the dialogue options. */

	discfile_read_option_unsigned(load, "PAN", &dialogue->pane);

	/* The Search Path. */

	discfile_read_option_flex_string(load, "PAT", (flex_ptr) &dialogue->path);

	/* The Filename. */

	discfile_read_option_unsigned(load, "FMD", &dialogue->name_mode);
	discfile_read_option_flex_string(load, "FNM", (flex_ptr) &dialogue->filename);
	discfile_read_option_boolean(load, "FIC", &dialogue->ignore_case);

	/* The File Size. */

	discfile_read_option_unsigned(load, "SMD", &dialogue->size_mode);
	discfile_read_option_unsigned(load, "SMN", &dialogue->size_min);
	discfile_read_option_unsigned(load, "SUM", &dialogue->size_min_unit);
	discfile_read_option_unsigned(load, "SMX", &dialogue->size_max);
	discfile_read_option_unsigned(load, "SUX", &dialogue->size_max_unit);

	/* The File Date/Age. */

	discfile_read_option_boolean(load, "AGE", &dialogue->use_age);

	discfile_read_option_unsigned(load, "DMD", &dialogue->date_mode);
	discfile_read_option_date(load, "DMN", dialogue->date_min);
	discfile_read_option_unsigned(load, "DSM", &dialogue->date_min_status);
	discfile_read_option_date(load, "DMX", dialogue->date_max);
	discfile_read_option_unsigned(load, "DSX", &dialogue->date_max_status);

	discfile_read_option_unsigned(load, "AMD", &dialogue->age_mode);
	discfile_read_option_unsigned(load, "AMN", &dialogue->age_min);
	discfile_read_option_unsigned(load, "AUM", &dialogue->age_min_unit);
	discfile_read_option_unsigned(load, "AMX", &dialogue->age_max);
	discfile_read_option_unsigned(load, "AUX", &dialogue->age_max_unit);

	/* The File Type. */

	discfile_read_option_boolean(load, "TFI", &dialogue->type_files);
	discfile_read_option_boolean(load, "TDR", &dialogue->type_directories);
	discfile_read_option_boolean(load, "TAP", &dialogue->type_applications);
	discfile_read_option_unsigned(load, "TMD", &dialogue->type_mode);
	discfile_read_option_unsigned_array(load, "TTL", (flex_ptr) &dialogue->type_types, 0xffffffffu);

	/* The File Attributes. */

	discfile_read_option_boolean(load, "PLK", &dialogue->attributes_locked);
	discfile_read_option_boolean(load, "PLY", &dialogue->attributes_locked_yes);
	discfile_read_option_boolean(load, "Prd", &dialogue->attributes_owner_read);
	discfile_read_option_boolean(load, "PrY", &dialogue->attributes_owner_read_yes);
	discfile_read_option_boolean(load, "Pwr", &dialogue->attributes_owner_write);
	discfile_read_option_boolean(load, "PwY", &dialogue->attributes_owner_write_yes);
	discfile_read_option_boolean(load, "PRD", &dialogue->attributes_public_read);
	discfile_read_option_boolean(load, "PRY", &dialogue->attributes_public_read_yes);
	discfile_read_option_boolean(load, "PWR", &dialogue->attributes_public_write);
	discfile_read_option_boolean(load, "PRY", &dialogue->attributes_public_write_yes);

	/* The File Contents. */

	discfile_read_option_unsigned(load, "CMD", &dialogue->contents_mode);
	discfile_read_option_flex_string(load, "CTX", (flex_ptr) &dialogue->contents_text);
	discfile_read_option_boolean(load, "CIC", &dialogue->contents_ignore_case);
	discfile_read_option_boolean(load, "CCC", &dialogue->contents_ctrl_chars);

	/* The Search Options. */

	discfile_read_option_boolean(load, "ALL", &dialogue->store_all);
	discfile_read_option_boolean(load, "IMG", &dialogue->ignore_imagefs);
	discfile_read_option_boolean(load, "ERR", &dialogue->suppress_errors);
	discfile_read_option_boolean(load, "FUL", &dialogue->full_info);

	discfile_close_chunk(load);
	discfile_close_section(load);

	return dialogue;
}


/**
 * Load a dialogue's settings from an open legacy disc file and create a new
 * dialogue structure from them.
 *
 * \param *file			The file to which the dialogue will belong.
 * \param *load			The handle of the file to load from.
 * \return			The handle of the new dialogue, or NULL.
 */

static struct dialogue_block *dialogue_load_legacy_file(struct file_block *file, struct discfile_block *load)
{
	struct dialogue_block	*dialogue;
	unsigned		flags;
	char			buffer[4095];

	if (file == NULL || load == NULL)
		return NULL;

	if (discfile_read_format(load) != DISCFILE_LOCATE0 && discfile_read_format(load) != DISCFILE_LOCATE1)
		return NULL;

	dialogue = dialogue_create(file, NULL, NULL);
	if (dialogue == NULL)
		return NULL;

	if (!discfile_legacy_open_section(load, DISCFILE_LEGACY_SECTION_DIALOGUE)) {
		dialogue_destroy(dialogue);
		return NULL;
	}

	/* Discard two unused words */

	discfile_legacy_read_word(load); /* Section size */
	discfile_legacy_read_word(load); /* Flag word */

	/* Selected pane, search path and filename options. */

	dialogue->pane = discfile_legacy_read_word(load);
	discfile_legacy_read_flex_string(load, (flex_ptr) &dialogue->path);
	discfile_legacy_read_flex_string(load, (flex_ptr) &dialogue->filename);
	dialogue->ignore_case = discfile_legacy_read_word(load);

	/* Search options flag word. */

	flags = discfile_legacy_read_word(load);

	dialogue->ignore_imagefs = (flags & 0x2u) ? TRUE : FALSE;
	dialogue->suppress_errors = (flags & 0x4u) ? TRUE : FALSE;
	dialogue->full_info = (flags & 0x8u) ? TRUE : FALSE;

	/* Two unused words. */

	discfile_legacy_read_word(load);
	discfile_legacy_read_word(load);

	/* Sizes. */

	dialogue->size_mode = discfile_legacy_read_word(load);
	dialogue->size_min = atoi(discfile_legacy_read_string(load, buffer, sizeof(buffer)));
	dialogue->size_min_unit = discfile_legacy_read_word(load);
	dialogue->size_max = atoi(discfile_legacy_read_string(load, buffer, sizeof(buffer)));
	dialogue->size_max_unit = discfile_legacy_read_word(load);

	/* Date or Age mode */

	dialogue->use_age = (discfile_legacy_read_word(load) == 0) ? FALSE : TRUE;

	/* Dates */

	dialogue->date_mode = discfile_legacy_read_word(load);
	dialogue->date_min_status = datetime_read_date(discfile_legacy_read_string(load, buffer, sizeof(buffer)), dialogue->date_min);
	dialogue->date_max_status = datetime_read_date(discfile_legacy_read_string(load, buffer, sizeof(buffer)), dialogue->date_max);

	/* Ages */

	dialogue->age_mode = discfile_legacy_read_word(load);
	dialogue->age_min = atoi(discfile_legacy_read_string(load, buffer, sizeof(buffer)));
	dialogue->age_min_unit = discfile_legacy_read_word(load);
	dialogue->age_max = atoi(discfile_legacy_read_string(load, buffer, sizeof(buffer)));
	dialogue->age_max_unit = discfile_legacy_read_word(load);

	/* Types */

	flags = discfile_legacy_read_word(load);

	dialogue->type_files = (flags & 0x1u) ? TRUE : FALSE;
	dialogue->type_directories = (flags & 0x2u) ? TRUE : FALSE;
	dialogue->type_applications = (flags & 0x4u) ? TRUE : FALSE;
	dialogue->type_mode = discfile_legacy_read_word(load);
	dialogue_read_filetype_list((flex_ptr) &(dialogue->type_types), discfile_legacy_read_string(load, buffer, sizeof(buffer)));

	/* Attributes */

	flags = discfile_legacy_read_word(load);

	dialogue->attributes_locked = (flags & 0x01u) ? TRUE : FALSE;
	dialogue->attributes_owner_read = (flags & 0x02u) ? TRUE : FALSE;
	dialogue->attributes_owner_write = (flags & 0x04u) ? TRUE : FALSE;
	dialogue->attributes_public_read = (flags & 0x08u) ? TRUE : FALSE;
	dialogue->attributes_public_write = (flags & 0x10u) ? TRUE : FALSE;

	flags = discfile_legacy_read_word(load);

	dialogue->attributes_locked_yes = (flags & 0x01u) ? TRUE : FALSE;
	dialogue->attributes_owner_read_yes = (flags & 0x02u) ? TRUE : FALSE;
	dialogue->attributes_owner_write_yes = (flags & 0x04u) ? TRUE : FALSE;
	dialogue->attributes_public_read_yes = (flags & 0x08u) ? TRUE : FALSE;
	dialogue->attributes_public_write_yes = (flags & 0x10u) ? TRUE : FALSE;

	/* Contents */

	dialogue->contents_mode = discfile_legacy_read_word(load);
	discfile_legacy_read_flex_string(load, (flex_ptr) &dialogue->contents_text);

	flags = discfile_legacy_read_word(load);

	dialogue->contents_ignore_case = (flags & 0x1u) ? TRUE : FALSE;
	dialogue->contents_ctrl_chars = (flags & 0x2u) ? TRUE : FALSE;

	discfile_legacy_close_section(load);

	return dialogue;
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

	dialogue_set_window(dialogue_data);

	windows_open_with_pane_centred_at_pointer(dialogue_window, dialogue_panes[dialogue_pane],
			DIALOGUE_ICON_PANE, 0, pointer);
	dialogue_toggle_size(FALSE);

	icons_put_caret_at_end(dialogue_window, DIALOGUE_ICON_SEARCH_PATH);
}


/**
 * Identify whether the Search Dialogue window is currently open.
 *
 * \return		TRUE if the window is open; else FALSE.
 */

osbool dialogue_window_is_open(void)
{
	return windows_get_open(dialogue_window);
}


/**
 * Close the Search Dialogue window.
 */

static void dialogue_close_window(void)
{
	wimp_close_window(dialogue_window);

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

	if (caret.w == dialogue_panes[old_pane]) {
		switch (pane) {
		case DIALOGUE_PANE_SIZE:
			icons_put_caret_in_group(dialogue_panes[DIALOGUE_PANE_SIZE], 2,
					DIALOGUE_SIZE_ICON_MIN, DIALOGUE_SIZE_ICON_MAX);
			break;

		case DIALOGUE_PANE_DATE:
			icons_put_caret_in_group(dialogue_panes[DIALOGUE_PANE_DATE], 4,
					DIALOGUE_DATE_ICON_DATE_FROM, DIALOGUE_DATE_ICON_DATE_TO,
					DIALOGUE_DATE_ICON_AGE_MIN, DIALOGUE_DATE_ICON_AGE_MAX);
			break;

		case DIALOGUE_PANE_TYPE:
			icons_put_caret_in_group(dialogue_panes[DIALOGUE_PANE_TYPE], 1, DIALOGUE_TYPE_ICON_TYPE);
			break;

		case DIALOGUE_PANE_ATTRIBUTES:
			icons_put_caret_at_end(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], wimp_ICON_WINDOW);
			break;

		case DIALOGUE_PANE_CONTENTS:
			icons_put_caret_in_group(dialogue_panes[DIALOGUE_PANE_CONTENTS], 1, DIALOGUE_CONTENTS_ICON_TEXT);
			break;
		}

		wimp_get_caret_position(&caret);

		if (caret.i == wimp_ICON_WINDOW)
			icons_put_caret_at_end(dialogue_window, DIALOGUE_ICON_FILENAME);
	}
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
 *
 * \param *dialogue		The dialogue data block to read the settings from.
 */

static void dialogue_set_window(struct dialogue_block *dialogue)
{
	if (dialogue == NULL)
		return;

	icons_printf(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, "%s", dialogue->path);

	event_set_window_icon_popup_selection(dialogue_window, DIALOGUE_ICON_NAME_MODE_MENU, dialogue->name_mode);
	icons_printf(dialogue_window, DIALOGUE_ICON_FILENAME, "%s", dialogue->filename);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_IGNORE_CASE, dialogue->ignore_case);

	/* Set the Size pane */

	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MODE_MENU, dialogue->size_mode);
	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MIN_UNIT_MENU, dialogue->size_min_unit);
	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MAX_UNIT_MENU, dialogue->size_max_unit);
	icons_printf(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MIN, "%d", dialogue->size_min);
	icons_printf(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MAX, "%d", dialogue->size_max);

	/* Set the Date / Age pane. */

	icons_set_selected(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE, !dialogue->use_age);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE, dialogue->use_age);

	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_MODE_MENU, dialogue->date_mode);
	datetime_write_date(dialogue->date_min, dialogue->date_min_status,
			icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM),
			icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM));
	datetime_write_date(dialogue->date_max, dialogue->date_max_status,
			icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO),
			icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO));

	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MODE_MENU, dialogue->age_mode);
	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MIN_UNIT_MENU, dialogue->age_min_unit);
	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MAX_UNIT_MENU, dialogue->age_max_unit);
	icons_printf(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MIN, "%d", dialogue->age_min);
	icons_printf(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MAX, "%d", dialogue->age_max);

	/* Set the Type pane */

	icons_set_selected(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_DIRECTORY, dialogue->type_directories);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_APPLICATION, dialogue->type_applications);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_FILE, dialogue->type_files);
	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_MODE_MENU, dialogue->type_mode);
	dialogue_write_filetype_list(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE),
			icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE),
			dialogue->type_types);

	/* Set the Attributes pane. */

	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED, dialogue->attributes_locked);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ, dialogue->attributes_owner_read);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE, dialogue->attributes_owner_write);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ, dialogue->attributes_public_read);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE, dialogue->attributes_public_write);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED_YES, dialogue->attributes_locked_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ_YES, dialogue->attributes_owner_read_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_YES, dialogue->attributes_owner_write_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ_YES, dialogue->attributes_public_read_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_YES, dialogue->attributes_public_write_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED_NO, !dialogue->attributes_locked_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ_NO, !dialogue->attributes_owner_read_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_NO, !dialogue->attributes_owner_write_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ_NO, !dialogue->attributes_public_read_yes);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_NO, !dialogue->attributes_public_write_yes);

	/* Set the Contents pane. */

	event_set_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_MODE_MENU, dialogue->contents_mode);
	icons_printf(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_TEXT, "%s", dialogue->contents_text);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_IGNORE_CASE, dialogue->contents_ignore_case);
	icons_set_selected(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_CTRL_CHARS, dialogue->contents_ctrl_chars);

	/* Set the search options. */

	icons_set_selected(dialogue_window, DIALOGUE_ICON_STORE_ALL, dialogue->store_all);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_IMAGE_FS, dialogue->ignore_imagefs);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_SUPPRESS_ERRORS, dialogue->suppress_errors);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_FULL_INFO, dialogue->full_info);

	/* Update icon shading for the panes. */

	dialogue_shade_window();
	dialogue_shade_size_pane();
	dialogue_shade_date_pane();
	dialogue_shade_type_pane();
	dialogue_shade_attributes_pane();
	dialogue_shade_contents_pane();
}


/**
 * Update the icon shading in the main window.
 */

static void dialogue_shade_window(void)
{
	enum dialogue_name	mode = event_get_window_icon_popup_selection(dialogue_window, DIALOGUE_ICON_NAME_MODE_MENU);

	icons_set_group_shaded(dialogue_window, mode == DIALOGUE_NAME_NOT_IMPORTANT, 2,
			DIALOGUE_ICON_FILENAME, DIALOGUE_ICON_IGNORE_CASE);

	icons_replace_caret_in_window(dialogue_window);
}


/**
 * Update the icon shading in the size pane.
 */

static void dialogue_shade_size_pane(void)
{
	enum dialogue_size	mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MODE_MENU);

	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_SIZE], mode == DIALOGUE_SIZE_NOT_IMPORTANT, 3,
			DIALOGUE_SIZE_ICON_MIN, DIALOGUE_SIZE_ICON_MIN_UNIT, DIALOGUE_SIZE_ICON_MIN_UNIT_MENU);
	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_SIZE], mode != DIALOGUE_SIZE_BETWEEN && mode != DIALOGUE_SIZE_NOT_BETWEEN, 4,
			DIALOGUE_SIZE_ICON_MAX, DIALOGUE_SIZE_ICON_MAX_UNIT, DIALOGUE_SIZE_ICON_MAX_UNIT_MENU, DIALOGUE_SIZE_ICON_AND);

	icons_replace_caret_in_window(dialogue_panes[DIALOGUE_PANE_SIZE]);
}


/**
 * Update the icon shading in the date pane.
 */

static void dialogue_shade_date_pane(void)
{
	enum dialogue_date	date_mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_MODE_MENU);
	enum dialogue_age	age_mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MODE_MENU);


	icons_set_group_deleted_when_off(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE, 8,
			DIALOGUE_DATE_ICON_DATE_LABEL, DIALOGUE_DATE_ICON_DATE_MODE, DIALOGUE_DATE_ICON_DATE_MODE_MENU,
			DIALOGUE_DATE_ICON_DATE_FROM, DIALOGUE_DATE_ICON_DATE_FROM_SET, DIALOGUE_DATE_ICON_DATE_AND,
			DIALOGUE_DATE_ICON_DATE_TO, DIALOGUE_DATE_ICON_DATE_TO_SET);

	icons_set_group_deleted_when_off(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE, 12,
			DIALOGUE_DATE_ICON_AGE_LABEL, DIALOGUE_DATE_ICON_AGE_MODE, DIALOGUE_DATE_ICON_AGE_MODE_MENU,
			DIALOGUE_DATE_ICON_AGE_MIN, DIALOGUE_DATE_ICON_AGE_MIN_UNIT, DIALOGUE_DATE_ICON_AGE_MIN_UNIT_MENU,
			DIALOGUE_DATE_ICON_AGE_MIN_UNIT_OLD, DIALOGUE_DATE_ICON_AGE_AND, DIALOGUE_DATE_ICON_AGE_MAX,
			DIALOGUE_DATE_ICON_AGE_MAX_UNIT, DIALOGUE_DATE_ICON_AGE_MAX_UNIT_MENU, DIALOGUE_DATE_ICON_AGE_MAX_UNIT_OLD);

	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_DATE], date_mode == DIALOGUE_DATE_AT_ANY_TIME, 2,
			DIALOGUE_DATE_ICON_DATE_FROM, DIALOGUE_DATE_ICON_DATE_FROM_SET);
	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_DATE], date_mode != DIALOGUE_DATE_BETWEEN && date_mode != DIALOGUE_DATE_NOT_BETWEEN, 3,
			DIALOGUE_DATE_ICON_DATE_AND, DIALOGUE_DATE_ICON_DATE_TO, DIALOGUE_DATE_ICON_DATE_TO_SET);

	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_DATE], age_mode == DIALOGUE_AGE_ANY_AGE, 4,
			DIALOGUE_DATE_ICON_AGE_MIN, DIALOGUE_DATE_ICON_AGE_MIN_UNIT,
			DIALOGUE_DATE_ICON_AGE_MIN_UNIT_MENU, DIALOGUE_DATE_ICON_AGE_MIN_UNIT_OLD);
	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_DATE], age_mode != DIALOGUE_AGE_BETWEEN && age_mode != DIALOGUE_AGE_NOT_BETWEEN, 5,
			DIALOGUE_DATE_ICON_AGE_MAX, DIALOGUE_DATE_ICON_AGE_MAX_UNIT,
			DIALOGUE_DATE_ICON_AGE_MAX_UNIT_MENU, DIALOGUE_DATE_ICON_AGE_MAX_UNIT_OLD, DIALOGUE_DATE_ICON_AGE_AND);

	icons_replace_caret_in_window(dialogue_panes[DIALOGUE_PANE_DATE]);

	windows_redraw(dialogue_panes[DIALOGUE_PANE_DATE]);
}


/**
 * Update the icon shading in the type pane.
 */

static void dialogue_shade_type_pane(void)
{
	enum dialogue_type	mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_MODE_MENU);
	osbool			files = icons_get_selected(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_FILE);

	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_TYPE], !files, 2,
			DIALOGUE_TYPE_ICON_MODE, DIALOGUE_TYPE_ICON_MODE_MENU);
	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_TYPE], !files || mode == DIALOGUE_TYPE_OF_ANY, 2,
			DIALOGUE_TYPE_ICON_TYPE, DIALOGUE_TYPE_ICON_TYPE_MENU);

	icons_replace_caret_in_window(dialogue_panes[DIALOGUE_PANE_TYPE]);
}


/**
 * Update the icon shading in the attributes pane.
 */

static void dialogue_shade_attributes_pane(void)
{
	icons_set_group_shaded_when_off(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED, 2,
			DIALOGUE_ATTRIBUTES_ICON_LOCKED_YES, DIALOGUE_ATTRIBUTES_ICON_LOCKED_NO);
	icons_set_group_shaded_when_off(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ, 2,
			DIALOGUE_ATTRIBUTES_ICON_OWN_READ_YES, DIALOGUE_ATTRIBUTES_ICON_OWN_READ_NO);
	icons_set_group_shaded_when_off(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE, 2,
			DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_YES, DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_NO);
	icons_set_group_shaded_when_off(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ, 2,
			DIALOGUE_ATTRIBUTES_ICON_PUB_READ_YES, DIALOGUE_ATTRIBUTES_ICON_PUB_READ_NO);
	icons_set_group_shaded_when_off(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE, 2,
			DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_YES, DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_NO);
}


/**
 * Update the icon shading in the contents pane.
 */

static void dialogue_shade_contents_pane(void)
{
	enum dialogue_contents	mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_MODE_MENU);

	icons_set_group_shaded(dialogue_panes[DIALOGUE_PANE_CONTENTS], mode == DIALOGUE_CONTENTS_ARE_NOT_IMPORTANT, 3,
			DIALOGUE_CONTENTS_ICON_TEXT, DIALOGUE_CONTENTS_ICON_IGNORE_CASE, DIALOGUE_CONTENTS_ICON_CTRL_CHARS);

	icons_replace_caret_in_window(dialogue_panes[DIALOGUE_PANE_CONTENTS]);
}


/**
 * Create a comma separated list from the supplied array of filetype values,
 * and insert it into the supplied buffer.
 *
 * \param *buffer		The buffer to take the list of names.
 * \param length		The length of the buffer.
 * \param types[]		The array of types to convert, 0xffffffffu terminated.
 */

static void dialogue_write_filetype_list(char *buffer, size_t length, unsigned types[])
{
	char	*insert, *end, name[9];
	int	i, j;

	insert = buffer;
	end = insert + length - 1;
	name[8] = '\0';

	for (i = 0; types[i] != 0xffffffffu; i++) {
		if (types[i] == 0x1000u)
			strncpy(name, "Untyped", sizeof(name));
		else if (xosfscontrol_read_file_type(types[i], (bits *) &name[0], (bits *) &name[4]) != NULL)
			continue;

		if (insert != buffer && insert < end)
			*insert++ = ',';

		for (j = 0; j < 8 && !isspace(name[j]); j++)
			*insert++ = name[j];
	}

	*insert = '\0';
}


/**
 * Update the search settings from the values in the Search Dialogue window. If
 * an error occurs (either in parsing data, or in allocating memory), it will be
 * reported to the user before the call returns.
 *
 * \param *dialogue		The dialogue data block to write the settings to.
 * \return			TRUE if successful; else FALSE if a parsing error occurred.
 */

static osbool dialogue_read_window(struct dialogue_block *dialogue)
{
	char		error[128];
	osbool		success = TRUE;

	if (dialogue == NULL)
		return FALSE;

	dialogue->pane = icons_get_radio_group_selected(dialogue_window, DIALOGUE_PANES,
			DIALOGUE_ICON_SIZE, DIALOGUE_ICON_DATE, DIALOGUE_ICON_TYPE,
			DIALOGUE_ICON_ATTRIBUTES, DIALOGUE_ICON_CONTENTS);

	if (!flexutils_store_string((flex_ptr) &(dialogue->path), icons_get_indirected_text_addr(dialogue_window, DIALOGUE_ICON_SEARCH_PATH))) {
		if (success)
			error_msgs_report_error("NoMemStoreParams");

		success = FALSE;
	}

	if (success && !search_validate_paths(dialogue->path)) {
		success = FALSE;
	}

	if (!flexutils_store_string((flex_ptr) &(dialogue->filename), icons_get_indirected_text_addr(dialogue_window, DIALOGUE_ICON_FILENAME))) {
		if (success)
			error_msgs_report_error("NoMemStoreParams");

		success = FALSE;
	}

	dialogue->name_mode = event_get_window_icon_popup_selection(dialogue_window, DIALOGUE_ICON_NAME_MODE_MENU);
	dialogue->ignore_case = icons_get_selected(dialogue_window, DIALOGUE_ICON_IGNORE_CASE);

	/* Set the Size pane */

	dialogue->size_mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MODE_MENU);
	dialogue->size_min_unit = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MIN_UNIT_MENU);
	dialogue->size_max_unit = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MAX_UNIT_MENU);
	dialogue->size_min = atoi(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MIN));
	dialogue->size_max = atoi(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MAX));

	/* Set the Date / Age pane. */

	dialogue->use_age = icons_get_selected(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE);

	dialogue->date_mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_MODE_MENU);
	dialogue->date_min_status = datetime_read_date(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM),
			dialogue->date_min);
	dialogue->date_max_status = datetime_read_date(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO),
			dialogue->date_max);

	if (success && dialogue->date_min_status == DATETIME_DATE_INVALID) {
		msgs_param_lookup("BadDate", error, sizeof(error), icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM), NULL, NULL, NULL);
		error_report_info(error);
		success = FALSE;
	}

	if (success && dialogue->date_max_status == DATETIME_DATE_INVALID) {
		msgs_param_lookup("BadDate", error, sizeof(error), icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO), NULL, NULL, NULL);
		error_report_info(error);
		success = FALSE;
	}

	dialogue->age_mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MODE_MENU);
	dialogue->age_min_unit = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MIN_UNIT_MENU);
	dialogue->age_max_unit = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MAX_UNIT_MENU);
	dialogue->age_min = atoi(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MIN));
	dialogue->age_max = atoi(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MAX));

	/* Set the Type pane */

	dialogue->type_directories = icons_get_selected(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_DIRECTORY);
	dialogue->type_applications = icons_get_selected(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_APPLICATION);
	dialogue->type_files = icons_get_selected(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_FILE);
	dialogue->type_mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_MODE_MENU);
	if (!dialogue_read_filetype_list((flex_ptr) &(dialogue->type_types), icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE)))
		success = FALSE;

	/* Set the Attributes pane. */

	dialogue->attributes_locked = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED);
	dialogue->attributes_owner_read = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ);
	dialogue->attributes_owner_write = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE);
	dialogue->attributes_public_read = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ);
	dialogue->attributes_public_write = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE);
	dialogue->attributes_locked_yes = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_LOCKED_YES);
	dialogue->attributes_owner_read_yes = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_READ_YES);
	dialogue->attributes_owner_write_yes = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_OWN_WRITE_YES);
	dialogue->attributes_public_read_yes = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_READ_YES);
	dialogue->attributes_public_write_yes = icons_get_selected(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], DIALOGUE_ATTRIBUTES_ICON_PUB_WRITE_YES);

	/* Set the Contents pane. */

	dialogue->contents_mode = event_get_window_icon_popup_selection(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_MODE_MENU);
	if (!flexutils_store_string((flex_ptr) &(dialogue->contents_text), icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_TEXT))) {
		if (success)
			error_msgs_report_error("NoMemStoreParams");

		success = FALSE;
	}
	dialogue->contents_ignore_case = icons_get_selected(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_IGNORE_CASE);
	dialogue->contents_ctrl_chars = icons_get_selected(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_CTRL_CHARS);

	/* Set the search options. */

	dialogue->store_all = icons_get_selected(dialogue_window, DIALOGUE_ICON_STORE_ALL);
	dialogue->ignore_imagefs = icons_get_selected(dialogue_window, DIALOGUE_ICON_IMAGE_FS);
	dialogue->suppress_errors = icons_get_selected(dialogue_window, DIALOGUE_ICON_SUPPRESS_ERRORS);
	dialogue->full_info = icons_get_selected(dialogue_window, DIALOGUE_ICON_FULL_INFO);

	return success;
}


/**
 * Convert a comma-separated list of filetypes into a filetype list.  The list
 * will be terminated with 0xffffffffu.
 *
 * \param ptr			Pointer to a flex block to take the new list.
 * \param *buffer		The buffer to be converted.
 * \return			TRUE if successful; else FALSE.
 */

static osbool dialogue_read_filetype_list(flex_ptr ptr, char *buffer)
{
	char		*c, *copy, *name, error[128];
	int		types = 0, i = 0;
	bits		type;
	unsigned	*list;
	osbool		success = TRUE;

	/* Find out how many filetypes are listed in the buffer. */

	if (*buffer != '\0')
		types++;

	for (c = buffer; *c != '\0'; c++)
		if (*c == ',')
			types++;

	/* Allocate enough memory to store all the types, plus a terminator. */

	if (flex_extend(ptr, sizeof(unsigned) * (types + 1)) == 0)
		return FALSE;

	list = (unsigned *) *ptr;

	list[0] = 0xffffffffu;

	/* Run through the buffer converting type names into values. */

	copy = strdup(buffer);
	if (copy == NULL)
		return FALSE;

	name = strtok(copy, ",");
	while (name != NULL && i <= types) {
		if (xosfscontrol_file_type_from_string(name, &type) == NULL) {
			list[i++] = type;
		} else if (string_nocase_strcmp(name, "Untyped") == 0) {
			list[i++] = 0x1000u;
		} else {
			msgs_param_lookup("BadFiletype", error, sizeof(error), name, NULL, NULL, NULL);
			error_report_info(error);
			success = FALSE;
		}

		name = strtok(NULL, ",");
	}

	free(copy);

	list[i] = 0xffffffffu;

	return success;
}


/**
 * Refresh the Search dialogue, to reflech changed icon states.
 */

static void dialogue_redraw_window(void)
{
	wimp_caret caret;

	wimp_set_icon_state(dialogue_window, DIALOGUE_ICON_FILENAME, 0, 0);
	wimp_set_icon_state(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, 0, 0);

	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MIN, 0, 0);
	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MAX, 0, 0);

	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM, 0, 0);
	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO, 0, 0);
	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MIN, 0, 0);
	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_AGE_MAX, 0, 0);

	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE, 0, 0);

	wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_TEXT, 0, 0);

	/* Update the caret position. */

	icons_replace_caret_in_window(dialogue_panes[dialogue_pane]);

	wimp_get_caret_position(&caret);
	if (caret.w == dialogue_panes[dialogue_pane] && caret.i == wimp_ICON_WINDOW)
		icons_put_caret_at_end(dialogue_window, DIALOGUE_ICON_FILENAME);

	icons_replace_caret_in_window(dialogue_window);

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
				if (!dialogue_read_window(dialogue_data))
					break;

				dialogue_start_search(dialogue_data);

				if (pointer->buttons == wimp_CLICK_SELECT) {
					settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
					dialogue_close_window();
				}
			}
			break;

		case DIALOGUE_ICON_CANCEL:
			if (pointer->buttons == wimp_CLICK_SELECT) {
				if (dialogue_data != NULL)
					file_destroy(dialogue_data->file);
				settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
				dialogue_close_window();
			} else if (pointer->buttons == wimp_CLICK_ADJUST) {
				dialogue_set_window(dialogue_data);
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

		case DIALOGUE_ICON_DRAG:
			if (pointer->buttons == wimp_DRAG_SELECT)
				dataxfer_save_window_drag(dialogue_window, DIALOGUE_ICON_DRAG, dialogue_drag_end_handler, NULL);
			break;
		}
	} else if (pointer->w == dialogue_panes[DIALOGUE_PANE_DATE]) {
		dialogue_shade_date_pane();
		if (pointer->i == DIALOGUE_DATE_ICON_DATE_FROM_SET || pointer->i == DIALOGUE_DATE_ICON_DATE_TO_SET)
			settime_open(pointer->w, (pointer->i == DIALOGUE_DATE_ICON_DATE_FROM_SET) ? DIALOGUE_DATE_ICON_DATE_FROM : DIALOGUE_DATE_ICON_DATE_TO, pointer);
	} else if (pointer->w == dialogue_panes[DIALOGUE_PANE_TYPE])
		dialogue_shade_type_pane();
	else if (pointer->w == dialogue_panes[DIALOGUE_PANE_ATTRIBUTES])
		dialogue_shade_attributes_pane();
}


/**
 * Process keypresses in the Search window.
 *
 * \param *key			The keypress event block to handle.
 * \return			TRUE if the event was handled; else FALSE.
 */

static osbool dialogue_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
		if (!dialogue_read_window(dialogue_data))
			break;
		dialogue_start_search(dialogue_data);
		dialogue_close_window();
		break;

	case wimp_KEY_ESCAPE:
		settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
		if (dialogue_data != NULL)
			file_destroy(dialogue_data->file);
		dialogue_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}


/**
 * Process menu prepare events in the Search window.
 *
 * \param w			The handle of the owning window.
 * \param *menu			The menu handle.
 * \param *pointer		The pointer position, or NULL for a re-open.
 */

static void dialogue_menu_prepare_handler(wimp_w w, wimp_menu *menu, wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	if (menu == dialogue_menu) {
		saveas_initialise_dialogue(dialogue_save_search, "SrchName", NULL, FALSE, FALSE, dialogue_data);
		return;
	}

	if (pointer->w == dialogue_panes[DIALOGUE_PANE_TYPE] && pointer->i == DIALOGUE_TYPE_ICON_TYPE_MENU) {
		dialogue_type_list_menu = typemenu_build();
		event_set_menu_block(dialogue_type_list_menu);
		templates_set_menu(TEMPLATES_MENU_TYPES, dialogue_type_list_menu);
		templates_set_menu_token("FileTypeMenu");
		return;
	}
}


/**
 * Process submenu warning events in the Search window.
 *
 * \param w		The handle of the owning window.
 * \param *menu		The menu handle.
 * \param *warning	The submenu warning message data.
 */

static void dialogue_menu_warning_handler(wimp_w w, wimp_menu *menu, wimp_message_menu_warning *warning)
{
	if (menu != dialogue_menu)
		return;

	switch (warning->selection.items[0]) {
	case DIALOGUE_MENU_SAVE_SEARCH:
		saveas_prepare_dialogue(dialogue_save_search);
		wimp_create_sub_menu(warning->sub_menu, warning->pos.x, warning->pos.y);
		break;
	}
}


/**
 * Process menu selections in the Search window.
 *
 * \param window		The window to handle an event for.
 * \param *menu			The menu from which the selection was made.
 * \param *selection		The selection made.
 */

static void dialogue_menu_selection_handler(wimp_w window, wimp_menu *menu, wimp_selection *selection)
{
	unsigned	*typelist;

	if (menu == dialogue_name_mode_menu)
		dialogue_shade_window();
	else if (menu == dialogue_size_mode_menu)
		dialogue_shade_size_pane();
	else if (menu == dialogue_date_mode_menu || menu == dialogue_age_mode_menu)
		dialogue_shade_date_pane();
	else if (menu == dialogue_type_mode_menu)
		dialogue_shade_type_pane();
	else if (menu == dialogue_contents_mode_menu)
		dialogue_shade_contents_pane();
	else if (menu == dialogue_type_list_menu) {
		if (flex_alloc((flex_ptr) &typelist, sizeof(unsigned)) == 0)
			return;

		if (dialogue_read_filetype_list((flex_ptr) &typelist, icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE))) {
			typemenu_process_selection(selection->items[0], (flex_ptr) &typelist);

			dialogue_write_filetype_list(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE),
					icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE),
					typelist);

			wimp_set_icon_state(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE, 0, 0);
		}

		dialogue_shade_type_pane();

		flex_free((flex_ptr) &typelist);
	}
}


/**
 * Process menu close events in the Search window.
 *
 * \param w			The handle of the owning window.
 * \param *menu			The menu handle.
 */

static void dialogue_menu_close_handler(wimp_w w, wimp_menu *menu)
{
//	fontlist_destroy();
//	report_format_font_menu = NULL;
//	report_format_font_icon = -1;
}


/**
 * Process the termination of icon drags from the Search window.
 *
 * \param *pointer		The pointer location at the end of the drag.
 * \param *data			Data passed to the icon drag routine.
 */

static void dialogue_drag_end_handler(wimp_pointer *pointer, void *data)
{
	dataxfer_start_save(pointer, "NULL", 0, 0xffffffffu, 0, dialogue_xfer_save_handler, NULL);
}


/**
 * Process data transfer results for the directory drag by updating the search
 * path.  Exit FALSE because we don't want the message protocol to be completed.
 *
 * \param *filename		The destination of the dragged folder.
 * \param *data			Context data (unused).
 * \return			FALSE to end the transfer.
 */

static osbool dialogue_xfer_save_handler(char *filename, void *data)
{
	char				*insert, *end, path[256], *p;

	strcpy(path, filename);

	string_find_pathname(path);

	insert = icons_get_indirected_text_addr(dialogue_window, DIALOGUE_ICON_SEARCH_PATH);
	end = insert + icons_get_indirected_text_length(dialogue_window, DIALOGUE_ICON_SEARCH_PATH) - 1;

	/* Copy the new path across. */

	p = path;

	while (*p != '\0' && insert < end)
		*insert++ = *p++;

	*insert = '\0';

	if (*p != '\0')
		error_msgs_report_error("PathBufFull");

	icons_replace_caret_in_window(dialogue_window);
	wimp_set_icon_state(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, 0, 0);

	return FALSE;
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

		if (insert != icons_get_indirected_text_addr(datasave->w, datasave->i) && insert < end)
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


/**
 * Take a set of dialogue settings and create a search from them.  This converts
 * the "human-friendly" details from the dialogue into the details used by the
 * search routines.
 *
 * \param *dialogue		The dialogue settings to use.
 */

static void dialogue_start_search(struct dialogue_block *dialogue)
{
	struct search_block		*search;
	size_t				buffer_size = 0;
	char				*buffer;


	/* Dump the settings to Reporter for debugging. */

	dialogue_dump_settings(dialogue);

	/* Calculate the required fixed buffer size and allocate the buffer. */

	buffer_size = MAX_BUFFER(buffer_size, strlen(dialogue->path) + 1);
	buffer_size = MAX_BUFFER(buffer_size, strlen(dialogue->filename) + 1);
	buffer_size = MAX_BUFFER(buffer_size, strlen(dialogue->contents_text) + 1);

	buffer = heap_alloc(buffer_size);
	if (buffer == NULL)
		return;

	/* Create the search and give up if this fails. */

	strncpy(buffer, dialogue->path, buffer_size);

	search = file_create_search(dialogue->file, buffer);

	if (search == NULL) {
		heap_free(buffer);
		return;
	}

	/* Set the generic search options. */

	search_set_options(search, !dialogue->ignore_imagefs, dialogue->store_all, dialogue->full_info,
			dialogue->type_files, dialogue->type_directories, dialogue->type_applications);

	/* Set the filename search options. */

	if (strcmp(dialogue->filename, "") != 0 && strcmp(dialogue->filename, "*") != 0 && dialogue->name_mode != DIALOGUE_NAME_NOT_IMPORTANT) {
		strncpy(buffer, dialogue->filename, buffer_size);
		search_set_filename(search, buffer, dialogue->ignore_case,
				(dialogue->name_mode == DIALOGUE_NAME_NOT_EQUAL_TO || dialogue->name_mode == DIALOGUE_NAME_DOES_NOT_CONTAIN) ? TRUE : FALSE);
	}

	/* Set the size search options. */

	if (dialogue->size_mode != DIALOGUE_SIZE_NOT_IMPORTANT) {
		switch (dialogue->size_mode) {
		case DIALOGUE_SIZE_EQUAL_TO:
			search_set_size(search, TRUE,
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, FALSE),
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, TRUE));
			break;

		case DIALOGUE_SIZE_NOT_EQUAL_TO:
			search_set_size(search, FALSE,
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, FALSE),
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, TRUE));
			break;

		case DIALOGUE_SIZE_GREATER_THAN:
			search_set_size(search, TRUE,
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, TRUE),
					0x7fffffff);
			break;

		case DIALOGUE_SIZE_LESS_THAN:
			search_set_size(search, TRUE,
					0x0u,
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, FALSE));
			break;

		case DIALOGUE_SIZE_BETWEEN:
			search_set_size(search, TRUE,
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, FALSE),
					dialogue_scale_size(dialogue->size_max, dialogue->size_max_unit, TRUE));
			break;

		case DIALOGUE_SIZE_NOT_BETWEEN:
			search_set_size(search, FALSE,
					dialogue_scale_size(dialogue->size_min, dialogue->size_min_unit, FALSE),
					dialogue_scale_size(dialogue->size_max, dialogue->size_max_unit, TRUE));
			break;

		case DIALOGUE_SIZE_NOT_IMPORTANT:
			break;
		}
	}

	/* Set the datestamp search options. */

	if (!dialogue->use_age && dialogue->date_mode != DIALOGUE_DATE_AT_ANY_TIME && dialogue->date_min_status != DATETIME_DATE_INVALID) {
		os_date_and_time	min_date, max_date;

		switch (dialogue->date_mode) {
		case DIALOGUE_DATE_AT:
			datetime_copy_date(min_date, dialogue->date_min);
			datetime_set_date(max_date, 0u, ((dialogue->date_min_status == DATETIME_DATE_DAY) ? DATETIME_1_DAY : DATETIME_1_MINUTE) - 1u);
			datetime_add_date(max_date, min_date);
			search_set_date(search, TRUE, min_date, max_date, FALSE);
			break;

		case DIALOGUE_DATE_AT_ANY_TIME_BUT:
			datetime_copy_date(min_date, dialogue->date_min);
			datetime_set_date(max_date, 0u, ((dialogue->date_min_status == DATETIME_DATE_DAY) ? DATETIME_1_DAY : DATETIME_1_MINUTE) - 1u);
			datetime_add_date(max_date, min_date);
			search_set_date(search, FALSE, min_date, max_date, FALSE);
			break;

		case DIALOGUE_DATE_AFTER:
			datetime_set_date(min_date, 0u, ((dialogue->date_min_status == DATETIME_DATE_DAY) ? DATETIME_1_DAY : DATETIME_1_MINUTE));
			datetime_add_date(min_date, dialogue->date_min);
			datetime_set_date(max_date, 0xffu, 0xffffffffu);
			search_set_date(search, TRUE, min_date, max_date, FALSE);
			break;

		case DIALOGUE_DATE_BEFORE:
			datetime_set_date(min_date, 0u, 0u);
			datetime_copy_date(max_date, dialogue->date_min);
			search_set_date(search, TRUE, min_date, max_date, FALSE);
			break;

		case DIALOGUE_DATE_BETWEEN:
			if (dialogue->date_max_status != DATETIME_DATE_INVALID) {
				datetime_copy_date(min_date, dialogue->date_min);
				datetime_set_date(max_date, 0u, ((dialogue->date_max_status == DATETIME_DATE_DAY) ? DATETIME_1_DAY : DATETIME_1_MINUTE) - 1u);
				datetime_add_date(max_date, dialogue->date_max);
				search_set_date(search, TRUE, min_date, max_date, FALSE);
			}
			break;

		case DIALOGUE_DATE_NOT_BETWEEN:
			if (dialogue->date_max_status != DATETIME_DATE_INVALID) {
				datetime_copy_date(min_date, dialogue->date_min);
				datetime_set_date(max_date, 0u, ((dialogue->date_max_status == DATETIME_DATE_DAY) ? DATETIME_1_DAY : DATETIME_1_MINUTE) - 1u);
				datetime_add_date(max_date, dialogue->date_max);
				search_set_date(search, FALSE, min_date, max_date, FALSE);
			}
			break;

		case DIALOGUE_DATE_AT_ANY_TIME:
			break;
		}
	}

	if (dialogue->use_age && dialogue->age_mode != DIALOGUE_AGE_ANY_AGE) {
		os_date_and_time		min_date, max_date;
		oswordreadclock_utc_block	now;

		now.op = oswordreadclock_OP_UTC;
		oswordreadclock_utc(&now);

		datetime_copy_date(min_date, now.utc);
		datetime_copy_date(max_date, now.utc);

		switch (dialogue->age_mode) {
		case DIALOGUE_AGE_EXACTLY:
			dialogue_scale_age(min_date, dialogue->age_min, dialogue->age_min_unit, -1);
			dialogue_scale_age(max_date, dialogue->age_min, dialogue->age_min_unit, +1);
			search_set_date(search, TRUE, min_date, max_date, TRUE);
			break;

		case DIALOGUE_AGE_ANY_AGE_BUT:
			dialogue_scale_age(min_date, dialogue->age_min, dialogue->age_min_unit, -1);
			dialogue_scale_age(max_date, dialogue->age_min, dialogue->age_min_unit, +1);
			search_set_date(search, FALSE, min_date, max_date, TRUE);
			break;

		case DIALOGUE_AGE_LESS_THAN:
			dialogue_scale_age(min_date, dialogue->age_min, dialogue->age_min_unit, 0);
			datetime_set_date(max_date, 0xffu, 0xffffffffu);
			search_set_date(search, TRUE, min_date, max_date, TRUE);
			break;

		case DIALOGUE_AGE_MORE_THAN:
			datetime_set_date(min_date, 0x0u, 0x0u);
			dialogue_scale_age(max_date, dialogue->age_min, dialogue->age_min_unit, 0);
			search_set_date(search, TRUE, min_date, max_date, TRUE);
			break;

		case DIALOGUE_AGE_BETWEEN:
			dialogue_scale_age(min_date, dialogue->age_min, dialogue->age_min_unit, 0);
			dialogue_scale_age(max_date, dialogue->age_max, dialogue->age_max_unit, 0);
			search_set_date(search, TRUE, min_date, max_date, TRUE);
			break;

		case DIALOGUE_AGE_NOT_BETWEEN:
			dialogue_scale_age(min_date, dialogue->age_min, dialogue->age_min_unit, 0);
			dialogue_scale_age(max_date, dialogue->age_max, dialogue->age_max_unit, 0);
			search_set_date(search, FALSE, min_date, max_date, TRUE);
			break;

		case DIALOGUE_AGE_ANY_AGE:
			break;
		}
	}

	/* Set the filetype search options. */

	if (dialogue->type_mode != DIALOGUE_TYPE_OF_ANY && dialogue->type_types[0] != 0xffffffffu)
		search_set_types(search, dialogue->type_types, (dialogue->type_mode == DIALOGUE_TYPE_NOT_OF_TYPE) ? TRUE : FALSE);

	/* Set the attributes search options. */

	if (dialogue->attributes_locked)
		search_set_attributes(search, fileswitch_ATTR_OWNER_LOCKED, (dialogue->attributes_locked_yes) ? fileswitch_ATTR_OWNER_LOCKED : 0);

	if (dialogue->attributes_owner_read)
		search_set_attributes(search, fileswitch_ATTR_OWNER_READ, (dialogue->attributes_owner_read_yes) ? fileswitch_ATTR_OWNER_READ : 0);

	if (dialogue->attributes_owner_write)
		search_set_attributes(search, fileswitch_ATTR_OWNER_WRITE, (dialogue->attributes_owner_write_yes) ? fileswitch_ATTR_OWNER_WRITE : 0);

	if (dialogue->attributes_public_read)
		search_set_attributes(search, fileswitch_ATTR_WORLD_READ, (dialogue->attributes_public_read_yes) ? fileswitch_ATTR_WORLD_READ : 0);

	if (dialogue->attributes_public_write)
		search_set_attributes(search, fileswitch_ATTR_WORLD_WRITE, (dialogue->attributes_public_write_yes) ? fileswitch_ATTR_WORLD_WRITE : 0);

	/* Set the contents search options. */

	if (strcmp(dialogue->contents_text, "") != 0 && strcmp(dialogue->contents_text, "*") != 0 && dialogue->contents_mode != DIALOGUE_CONTENTS_ARE_NOT_IMPORTANT) {
		strncpy(buffer, dialogue->contents_text, buffer_size);
		search_set_contents(search, buffer, dialogue->contents_ignore_case, (dialogue->contents_mode == DIALOGUE_CONTENTS_DO_NOT_INCLUDE) ? TRUE : FALSE);
	}

	/* Tidy up and start the search. */

	heap_free(buffer);

	search_start(search);
}


/**
 * Scale size values up by standard dialogue box units and round up or down.
 *
 * \param base			The base value to scale.
 * \param unit			The units to be applied to the base value.
 * \param top			TRUE to round up; FALSE to round down.
 * \return			The scaled and rounded value in bytes.
 */

static int dialogue_scale_size(unsigned base, enum dialogue_size_unit unit, osbool top)
{
	int	result;


	switch (unit) {
	case DIALOGUE_SIZE_MBYTES:
		result = (base * 1048576) + ((top) ? +524288 : -524288);
		break;

	case DIALOGUE_SIZE_KBYTES:
		result = (base * 1024) + ((top) ? +512 : -512);
		break;

	case DIALOGUE_SIZE_BYTES:
	default:
		result = base;
		break;
	}

	return result;
}


/**
 * Scale age values up by standard dialogue box units and round up or down.
 *
 * \param date			The base date to be used and updated.
 * \param base			The base value to scale.
 * \param unit			The units to be applied to the base value.
 * \param round			-1 to round down, 1 to round up, 0 to be exact.
 */

static void dialogue_scale_age(os_date_and_time date, unsigned base, enum dialogue_age_unit unit, int round)
{
	os_date_and_time	factor;

	switch (unit) {
	case DIALOGUE_AGE_MINUTES:
		datetime_set_date(factor, 0u, (DATETIME_1_MINUTE * base) + (DATETIME_HALF_MINUTE * round));
		datetime_subtract_date(date, factor);
		break;

	case DIALOGUE_AGE_HOURS:
		datetime_set_date(factor, 0u, (DATETIME_1_HOUR * base) + (DATETIME_HALF_HOUR * round));
		datetime_subtract_date(date, factor);
		break;

	case DIALOGUE_AGE_DAYS:
		datetime_set_date(factor, 0u, (DATETIME_1_DAY * base) + (DATETIME_HALF_DAY * round));
		datetime_subtract_date(date, factor);
		break;

	case DIALOGUE_AGE_WEEKS:
		datetime_set_date(factor, 0u, (DATETIME_1_WEEK * base) + (DATETIME_HALF_WEEK * round));
		datetime_subtract_date(date, factor);
		break;

	case DIALOGUE_AGE_MONTHS:
		datetime_add_months(date, -base);
		datetime_set_date(factor, 0u, DATETIME_15_DAYS);
		if (round < 0)
			datetime_subtract_date(date, factor);
		else if (round > 0)
			datetime_add_date(date, factor);
		break;

	case DIALOGUE_AGE_YEARS:
		datetime_add_months(date, -12 * base);
		datetime_set_date(factor, 0u, DATETIME_HALF_YEAR);
		if (round < 0)
			datetime_subtract_date(date, factor);
		else if (round > 0)
			datetime_add_date(date, factor);
		break;
	}
}


/**
 * Save the current dialogue settings to file.  Used as a DataXfer callback, so
 * must return TRUE on success or FALSE on failure.
 *
 * \param *filename		The filename to save to.
 * \param selection		TRUE to save just the selection, else FALSE.
 * \param *data			Context data: the handle of the parent dialogue.
 * \return			TRUE on success; FALSE on failure.
 */

static osbool dialogue_save_settings(char *filename, osbool selection, void *data)
{
	struct dialogue_block *dialogue = data;

	if (dialogue == NULL)
		return FALSE;

	return file_dialogue_save(dialogue->file, filename);
}



/**
 * Dump the contents of the a search parameter block for debugging.
 *
 * \param *dialogue		The dialogue data block to dump the settings from.
 */

static void dialogue_dump_settings(struct dialogue_block *dialogue)
{
	char			line[DIALOGUE_MAX_FILE_LINE];
	int			i, index;


	if (dialogue == NULL)
		return;

	debug_printf("Search path: '%s'", dialogue->path);

	debug_printf("Filename: '%s'", dialogue->filename);
	debug_printf("Ignore Case: %s", config_return_opt_string(dialogue->ignore_case));

	/* Set the Size pane */

	debug_printf("Size Mode: %d", dialogue->size_mode);
	debug_printf("Min Size Unit: %d", dialogue->size_min_unit);
	debug_printf("Max Size Unit: %d", dialogue->size_max_unit);
	debug_printf("Min Size: %d", dialogue->size_min);
	debug_printf("Max Size: %d", dialogue->size_max);

	/* Set the Date / Age pane. */

	debug_printf("Use Age Mode: %s", config_return_opt_string(dialogue->use_age));

	debug_printf("Date Mode: %d", dialogue->date_mode);
	debug_printf("Min Date Status: %d", dialogue->date_min_status);
	datetime_write_date(dialogue->date_min, dialogue->date_min_status, line, DIALOGUE_MAX_FILE_LINE);
	debug_printf("Min Date: '%s'", line);
	debug_printf("Max Date Status: %d", dialogue->date_max_status);
	datetime_write_date(dialogue->date_max, dialogue->date_max_status, line, DIALOGUE_MAX_FILE_LINE);
	debug_printf("Max Date: '%s'", line);

	debug_printf("Age Mode: %d", dialogue->age_mode);
	debug_printf("Min Age Unit: %d", dialogue->age_min_unit);
	debug_printf("Max Age Unit: %d", dialogue->age_max_unit);
	debug_printf("Min Age: %d", dialogue->age_min);
	debug_printf("Max Age: %d", dialogue->age_max);

	/* Set the Type pane */

	debug_printf("Match Directories: %s", config_return_opt_string(dialogue->type_directories));
	debug_printf("Match Applications: %s", config_return_opt_string(dialogue->type_applications));
	debug_printf("Match Files: %s", config_return_opt_string(dialogue->type_files));
	debug_printf("Type Mode: %d", dialogue->type_mode);

	index = 0;
	for (i = 0; dialogue->type_types[i] != 0xffffffffu; i++) {
		index += snprintf(line + index, DIALOGUE_MAX_FILE_LINE - (index + 1), "%03x,", dialogue->type_types[i]);
	}
	if (index > 0)
		*(line + index - 1) = '\0';
	else
		*line = '\0';
	debug_printf("Type List: '%s'", line);


	//dialogue_write_filetype_list(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE),
	//		icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_TYPE),
	//		dialogue->type_types);

	/* Set the Attributes pane. */

	debug_printf("Test Locked: %s", config_return_opt_string(dialogue->attributes_locked));
	debug_printf("Test Owner Read: %s", config_return_opt_string(dialogue->attributes_owner_read));
	debug_printf("Test Owner Write: %s", config_return_opt_string(dialogue->attributes_owner_write));
	debug_printf("Test Public Read: %s", config_return_opt_string(dialogue->attributes_public_read));
	debug_printf("Test Public Write: %s", config_return_opt_string(dialogue->attributes_public_write));
	debug_printf("Locked Status: %s", config_return_opt_string(dialogue->attributes_locked_yes));
	debug_printf("Owner Read Status: %s", config_return_opt_string(dialogue->attributes_owner_read_yes));
	debug_printf("Owner Write Status: %s", config_return_opt_string(dialogue->attributes_owner_write_yes));
	debug_printf("Public Read Status: %s", config_return_opt_string(dialogue->attributes_public_read_yes));
	debug_printf("Public Write Status: %s", config_return_opt_string(dialogue->attributes_public_write_yes));

	/* Set the Contents pane. */

	debug_printf("Contents Mode: %d", dialogue->contents_mode);
	debug_printf("File Contents: '%s'", dialogue->contents_text);
	debug_printf("Ignore Case in Contents: %s", config_return_opt_string(dialogue->contents_ignore_case));
	debug_printf("Allow Ctrl Chars in Contents: %s", config_return_opt_string(dialogue->contents_ctrl_chars));

	/* Set the search options. */

	debug_printf("Store All Files: %s", config_return_opt_string(dialogue->store_all));
	debug_printf("Ignore ImageFS Contents: %s", config_return_opt_string(dialogue->ignore_imagefs));
	debug_printf("Suppress Errors: %s", config_return_opt_string(dialogue->suppress_errors));
	debug_printf("Display Full Info: %s", config_return_opt_string(dialogue->full_info));
}


