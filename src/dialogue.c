/* Locate - dialogue.c
 * (c) Stephen Fryatt, 2012
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

#include "flexutils.h"
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

#define DIALOGUE_DATE_FORMAT_DAY "%DY/%MN/%CE%YR"
#define DIALOGUE_DATE_FORMAT_TIME "%DY/%MN/%CE%YR.%24:%MI"

#define DIALOGUE_MAX_FILE_LINE 1024

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

enum dialogue_date_status {
	DIALOGUE_DATE_INVALID = 0,
	DIALOGUE_DATE_DAY,
	DIALOGUE_DATE_TIME
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
	enum dialogue_date_status	date_min_status;			/**< The status of the minimum date value.		*/
	os_date_and_time		date_max;				/**< The maximum date.					*/
	enum dialogue_date_status	date_max_status;			/**< The status of the maximum date value.		*/

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

static wimp_menu		*dialogue_menu = NULL;				/**< The main search window menu.			*/
static wimp_menu		*dialogue_size_mode_menu = NULL;		/**< The Size Mode popup menu.				*/
static wimp_menu		*dialogue_size_unit_menu = NULL;		/**< The Size Unit popup menu.				*/
static wimp_menu		*dialogue_date_mode_menu = NULL;		/**< The Date Mode popup menu.				*/
static wimp_menu		*dialogue_age_mode_menu = NULL;			/**< The Age Mode popup menu.				*/
static wimp_menu		*dialogue_age_unit_menu = NULL;			/**< The Age Unit popup menu.				*/
static wimp_menu		*dialogue_type_mode_menu = NULL;		/**< The Type Mode popup menu.				*/
static wimp_menu		*dialogue_contents_mode_menu = NULL;		/**< The Contents Mode popup menu.			*/


static void	dialogue_close_window(void);
static void	dialogue_change_pane(unsigned pane);
static void	dialogue_toggle_size(bool expand);
static void	dialogue_set_window(struct dialogue_block *dialogue);
static void	dialogue_shade_size_pane(void);
static void	dialogue_shade_date_pane(void);
static void	dialogue_shade_type_pane(void);
static void	dialogue_shade_attributes_pane(void);
static void	dialogue_shade_contents_pane(void);
static void	dialogue_write_filetype_list(char *buffer, size_t length, unsigned types[]);
static osbool	dialogue_read_window(struct dialogue_block *dialogue);
static osbool	dialogue_read_filetype_list(flex_ptr list, char *buffer);
static enum	dialogue_date_status dialogue_read_date(char *text, os_date_and_time *date);
static osbool	dialogue_test_numeric_value(char *text);
static void	dialogue_redraw_window(void);
static void	dialogue_click_handler(wimp_pointer *pointer);
static osbool	dialogue_keypress_handler(wimp_key *key);
static void	dialogue_menu_selection_handler(wimp_w window, wimp_menu *menu, wimp_selection *selection);
static osbool	dialogue_icon_drop_handler(wimp_message *message);
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
	dialogue_size_mode_menu = templates_get_menu(TEMPLATES_MENU_SIZE_MODE);
	dialogue_size_unit_menu = templates_get_menu(TEMPLATES_MENU_SIZE_UNIT);
	dialogue_date_mode_menu = templates_get_menu(TEMPLATES_MENU_DATE_MODE);
	dialogue_age_mode_menu = templates_get_menu(TEMPLATES_MENU_AGE_MODE);
	dialogue_age_unit_menu = templates_get_menu(TEMPLATES_MENU_AGE_UNIT);
	dialogue_type_mode_menu = templates_get_menu(TEMPLATES_MENU_TYPE_MODE);
	dialogue_contents_mode_menu = templates_get_menu(TEMPLATES_MENU_CONTENTS_MODE);

	/* Initialise the main window. */

	def->icons[DIALOGUE_ICON_SEARCH_PATH].data.indirected_text.text = heap_alloc(buf_size);
	def->icons[DIALOGUE_ICON_SEARCH_PATH].data.indirected_text.size = buf_size;
	dialogue_window = wimp_create_window(def);
	icons_printf(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, "");
	free(def);
	ihelp_add_window(dialogue_window, "Search", NULL);
	event_add_window_mouse_event(dialogue_window, dialogue_click_handler);
	event_add_window_key_event(dialogue_window, dialogue_keypress_handler);
	event_add_window_menu_selection(dialogue_window, dialogue_menu_selection_handler);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_SIZE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_DATE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_TYPE, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_ATTRIBUTES, FALSE);
	event_add_window_icon_radio(dialogue_window, DIALOGUE_ICON_CONTENTS, FALSE);

	/* Initialise the size pane. */

	dialogue_panes[DIALOGUE_PANE_SIZE] = templates_create_window("SizePane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_SIZE], "Search.Size", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_keypress_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_SIZE], dialogue_menu_selection_handler);
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MODE_MENU,
			dialogue_size_mode_menu, DIALOGUE_SIZE_ICON_MODE, "SizeMode");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MIN_UNIT_MENU,
			dialogue_size_unit_menu, DIALOGUE_SIZE_ICON_MIN_UNIT, "SizeUnit");
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_SIZE], DIALOGUE_SIZE_ICON_MAX_UNIT_MENU,
			dialogue_size_unit_menu, DIALOGUE_SIZE_ICON_MAX_UNIT, "SizeUnit");

	/* Initialise the date pane. */

	dialogue_panes[DIALOGUE_PANE_DATE] = templates_create_window("DatePane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_DATE], "Search.Date", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_DATE], dialogue_keypress_handler);
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
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_TYPE], "Search.Type", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_keypress_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_TYPE], dialogue_menu_selection_handler);
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_TYPE], DIALOGUE_TYPE_ICON_MODE_MENU,
			dialogue_type_mode_menu, DIALOGUE_TYPE_ICON_MODE, "TypeMode");

	/* Initialise the attributes pane. */

	dialogue_panes[DIALOGUE_PANE_ATTRIBUTES] = templates_create_window("AttribPane");
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], "Search.Attributes", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_ATTRIBUTES], dialogue_keypress_handler);
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
	ihelp_add_window (dialogue_panes[DIALOGUE_PANE_CONTENTS], "Search.Contents", NULL);
	event_add_window_mouse_event(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_click_handler);
	event_add_window_key_event(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_keypress_handler);
	event_add_window_menu_selection(dialogue_panes[DIALOGUE_PANE_CONTENTS], dialogue_menu_selection_handler);
	event_add_window_icon_popup(dialogue_panes[DIALOGUE_PANE_CONTENTS], DIALOGUE_CONTENTS_ICON_MODE_MENU,
			dialogue_contents_mode_menu, DIALOGUE_CONTENTS_ICON_MODE, "ContentsMode");

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
	int			i;

	if (file == NULL)
		return NULL;

	/* Allocate all of the memory that we require. */

	new = heap_alloc(sizeof(struct dialogue_block));
	if (new == NULL)
		mem_ok = FALSE;

	if (mem_ok) {
		new->path = NULL;
		new->filename = NULL;
		new->type_types = NULL;
		new->contents_text = NULL;

		if (flex_alloc((flex_ptr) &(new->path), strlen(config_str_read("SearchPath")) + 1) == 0)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->filename), strlen("") + 1) == 0)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->type_types), sizeof(unsigned)) == 0)
			mem_ok = FALSE;

		if (flex_alloc((flex_ptr) &(new->contents_text), strlen("") + 1) == 0)
			mem_ok = FALSE;

	}

	if (!mem_ok) {
		dialogue_destroy(new);
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
	new->size_min_unit = DIALOGUE_SIZE_KBYTES;
	new->size_max = 0;
	new->size_max_unit = DIALOGUE_SIZE_KBYTES;

	/* Date and Age Details. */

	new->use_age = FALSE;

	new->date_mode = DIALOGUE_DATE_AT_ANY_TIME;
	for (i = 0; i < 5; i++) {
		new->date_min[i] = 0;
		new->date_max[i] = 0;
	}
	new->date_min_status = DIALOGUE_DATE_INVALID;
	new->date_max_status = DIALOGUE_DATE_INVALID;

	new->age_mode = DIALOGUE_AGE_ANY_AGE;
	new->age_min = 0;
	new->age_min_unit = DIALOGUE_AGE_DAYS;
	new->age_max = 0;
	new->age_max_unit = DIALOGUE_AGE_DAYS;


	/* Type Details. */

	new->type_files = TRUE;
	new->type_directories = TRUE;
	new->type_applications = TRUE;
	new->type_mode = DIALOGUE_TYPE_OF_ANY;
	new->type_types[0] = 0xffffffffu;

	/* Attribute Details. */

	new->attributes_locked = FALSE;
	new->attributes_locked_yes = FALSE;
	new->attributes_owner_read = FALSE;
	new->attributes_owner_read_yes = TRUE;
	new->attributes_owner_write = FALSE;
	new->attributes_owner_write_yes = TRUE;
	new->attributes_public_read = FALSE;
	new->attributes_public_read_yes = TRUE;
	new->attributes_public_write = FALSE;
	new->attributes_public_write_yes = TRUE;

	/* Contents Details. */

	new->contents_mode = DIALOGUE_CONTENTS_ARE_NOT_IMPORTANT;
	strcpy(new->contents_text, "");
	new->contents_ignore_case = TRUE;
	new->contents_ctrl_chars = FALSE;

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
 *
 * \param *dialogue		The dialogue data block to read the settings from.
 */

static void dialogue_set_window(struct dialogue_block *dialogue)
{
	icons_printf(dialogue_window, DIALOGUE_ICON_SEARCH_PATH, "%s", dialogue->path);

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
	switch (dialogue->date_min_status) {
	case DIALOGUE_DATE_INVALID:
		icons_printf(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM, "");
		break;
	case DIALOGUE_DATE_DAY:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_min),
				icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM),
				icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM),
				DIALOGUE_DATE_FORMAT_DAY);
		break;
	case DIALOGUE_DATE_TIME:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_min),
				icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM),
				icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM),
				DIALOGUE_DATE_FORMAT_TIME);
		break;
	}
	switch (dialogue->date_max_status) {
	case DIALOGUE_DATE_INVALID:
		icons_printf(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO, "");
		break;
	case DIALOGUE_DATE_DAY:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_max),
				icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO),
				icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO),
				DIALOGUE_DATE_FORMAT_DAY);
		break;
	case DIALOGUE_DATE_TIME:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_max),
				icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO),
				icons_get_indirected_text_length(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO),
				DIALOGUE_DATE_FORMAT_TIME);
		break;
	}

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

	icons_set_selected(dialogue_window, DIALOGUE_ICON_BACKGROUND_SEARCH, dialogue->background);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_IMAGE_FS, dialogue->ignore_imagefs);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_SUPPRESS_ERRORS, dialogue->suppress_errors);
	icons_set_selected(dialogue_window, DIALOGUE_ICON_FULL_INFO, dialogue->full_info);

	/* Update icon shading for the panes. */

	dialogue_shade_size_pane();
	dialogue_shade_date_pane();
	dialogue_shade_type_pane();
	dialogue_shade_attributes_pane();
	dialogue_shade_contents_pane();
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
		if (xosfscontrol_read_file_type(types[i], (bits *) &name[0], (bits *) &name[4]) != NULL)
			continue;

		if (insert != buffer && insert < end)
			*insert++ = ',';

		for (j = 0; j < 8 && !isspace(name[j]); j++)
			*insert++ = name[j];
	}

	*insert = '\0';
}


/**
 * Update the search settings from the values in the Search Dialogue window.
 *
 * \param *dialogue		The dialogue data block to write the settings to.
 * \return			TRUE if successful; else FALSE.
 */

static osbool dialogue_read_window(struct dialogue_block *dialogue)
{
	osbool		success = TRUE;

	if (!flexutils_store_string((flex_ptr) &(dialogue->path), icons_get_indirected_text_addr(dialogue_window, DIALOGUE_ICON_SEARCH_PATH))) {
		if (success)
			error_msgs_report_error("NoMemStoreParams");

		success = FALSE;
	}

	if (!flexutils_store_string((flex_ptr) &(dialogue->filename), icons_get_indirected_text_addr(dialogue_window, DIALOGUE_ICON_FILENAME))) {
		if (success)
			error_msgs_report_error("NoMemStoreParams");

		success = FALSE;
	}

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
	dialogue->date_min_status = dialogue_read_date(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_FROM),
			&(dialogue->date_min));
	dialogue->date_max_status = dialogue_read_date(icons_get_indirected_text_addr(dialogue_panes[DIALOGUE_PANE_DATE], DIALOGUE_DATE_ICON_DATE_TO),
			&(dialogue->date_max));

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

	dialogue->background = icons_get_selected(dialogue_window, DIALOGUE_ICON_BACKGROUND_SEARCH);
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
 * Parse a textual date into five-byte OS date values.
 *
 * \param *text			The text to be parsed.
 * \param *date			The location to store the resulting date.
 * \return			The status of the resulting date.
 */

static enum dialogue_date_status dialogue_read_date(char *text, os_date_and_time *date)
{
	enum dialogue_date_status	result;
	territory_ordinals		ordinals;
	char				*copy, *day, *month, *year, *hour, *minute;

	copy = strdup(text);
	if (copy == NULL)
		return DIALOGUE_DATE_INVALID;

	day = strtok(copy, "/");
	month = strtok(NULL, "/");
	year = strtok(NULL, ".");
	hour = strtok(NULL, ".:");
	minute = strtok(NULL, "");

	/* If day, month or year are invalid then it's not a date. */

	if (!dialogue_test_numeric_value(day) || !dialogue_test_numeric_value(month) || !dialogue_test_numeric_value(year)) {
		free(copy);
		return DIALOGUE_DATE_INVALID;
	}

	ordinals.date = atoi(day);
	ordinals.month = atoi(month);
	ordinals.year = atoi(year);

	/* 01 -> 80 == 2001 -> 2080; 81 -> 99 == 1981 -> 1999 */

	if (ordinals.year >= 1 && ordinals.year <= 80)
		ordinals.year += 2000;
	else if (ordinals.year >= 81 && ordinals.year <= 99)
		ordinals.year += 1900;

	if (dialogue_test_numeric_value(hour) && dialogue_test_numeric_value(minute)) {
		ordinals.hour = atoi(hour);
		ordinals.minute = atoi(minute);
		result = DIALOGUE_DATE_TIME;
	} else {
		ordinals.hour = 0;
		ordinals.minute = 0;
		result = DIALOGUE_DATE_DAY;
	}

	ordinals.centisecond = 0;
	ordinals.second = 0;

	free(copy);

	if (xterritory_convert_ordinals_to_time(territory_CURRENT, date, &ordinals) != NULL)
		result = DIALOGUE_DATE_INVALID;

	return result;
}


/**
 * Test a string to make sure that it only contains decimal digits.
 *
 * \param *text			The string to test.
 * \return			TRUE if strink OK; else FALSE.
 */

static osbool dialogue_test_numeric_value(char *text)
{
	int	i;
	osbool	result = TRUE;

	if (text == NULL)
		return FALSE;

	for (i = 0; i < strlen(text); i++)
		if (!isdigit(text[i])) {
			result = FALSE;
			break;
		}

	return result;
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
				dialogue_read_window(dialogue_data);

				dialogue_dump_settings(dialogue_data);

				if (pointer->buttons == wimp_CLICK_SELECT)
					dialogue_close_window();
			}
			break;

		case DIALOGUE_ICON_CANCEL:
			if (pointer->buttons == wimp_CLICK_SELECT) {
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
		}
	} else if (pointer->w == dialogue_panes[DIALOGUE_PANE_DATE])
		dialogue_shade_date_pane();
	else if (pointer->w == dialogue_panes[DIALOGUE_PANE_TYPE])
		dialogue_shade_type_pane();
	else if (pointer->w == dialogue_panes[DIALOGUE_PANE_ATTRIBUTES])
		dialogue_shade_attributes_pane();
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
		dialogue_read_window(dialogue_data);
		//config_save();
		dialogue_dump_settings(dialogue_data);
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
 * Process menu selections in the Search window.
 *
 * \param window	The window to handle an event for.
 * \param *menu		The menu from which the selection was made.
 * \param *selection	The selection made.
 */

static void dialogue_menu_selection_handler(wimp_w window, wimp_menu *menu, wimp_selection *selection)
{
	if (menu == dialogue_size_mode_menu)
		dialogue_shade_size_pane();
	else if (menu == dialogue_date_mode_menu || menu == dialogue_age_mode_menu)
		dialogue_shade_date_pane();
	else if (menu == dialogue_type_mode_menu)
		dialogue_shade_type_pane();
	else if (menu == dialogue_contents_mode_menu)
		dialogue_shade_contents_pane();
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


/**
 * Dump the contents of the a search parameter block for debugging.
 *
 * \param *dialogue		The dialogue data block to dump the settings from.
 */

static void dialogue_dump_settings(struct dialogue_block *dialogue)
{
	char	line[DIALOGUE_MAX_FILE_LINE];
	int	i, index;

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
	switch (dialogue->date_min_status) {
	case DIALOGUE_DATE_INVALID:
		*line = '\0';
		break;
	case DIALOGUE_DATE_DAY:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_min),
				line, DIALOGUE_MAX_FILE_LINE, DIALOGUE_DATE_FORMAT_DAY);
		break;
	case DIALOGUE_DATE_TIME:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_min),
				line, DIALOGUE_MAX_FILE_LINE, DIALOGUE_DATE_FORMAT_TIME);
		break;
	}
	debug_printf("Min Date: '%s'", line);
	debug_printf("Max Date Status: %d", dialogue->date_max_status);
	switch (dialogue->date_max_status) {
	case DIALOGUE_DATE_INVALID:
		*line = '\0';
		break;
	case DIALOGUE_DATE_DAY:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_max),
				line, DIALOGUE_MAX_FILE_LINE, DIALOGUE_DATE_FORMAT_DAY);
		break;
	case DIALOGUE_DATE_TIME:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) &(dialogue->date_max),
				line, DIALOGUE_MAX_FILE_LINE, DIALOGUE_DATE_FORMAT_TIME);
		break;
	}
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

	debug_printf("Search In Background: %s", config_return_opt_string(dialogue->background));
	debug_printf("Ignore ImageFS Contents: %s", config_return_opt_string(dialogue->ignore_imagefs));
	debug_printf("Suppress Errors: %s", config_return_opt_string(dialogue->suppress_errors));
	debug_printf("Display Full Info: %s", config_return_opt_string(dialogue->full_info));
}


