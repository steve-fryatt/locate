/* Locate - settime.c
 * (c) Stephen Fryatt, 2012
 *
 * Provide a dialogue box for entering dates.
 */

/* ANSI C Header files. */

/* Acorn C Header files. */

/* SFLib Header files. */

#include "sflib/event.h"
#include "sflib/windows.h"

/* OSLib Header files. */

#include "oslib/wimp.h"

/* Application header files. */

#include "settime.h"

#include "ihelp.h"
#include "templates.h"

/* Icon Handles. */

#define SETTIME_ICON_CANCEL 0
#define SETTIME_ICON_SET 1
#define SETTIME_ICON_MONTH_POPUP 2
#define SETTIME_ICON_MONTH 3
#define SETTIME_ICON_DATE 4
#define SETTIME_ICON_DATE_DOWN 5
#define SETTIME_ICON_DATE_UP 6
#define SETTIME_ICON_YEAR 7
#define SETTIME_ICON_YEAR_DOWN 8
#define SETTIME_ICON_YEAR_UP 9
#define SETTIME_ICON_SET_TIME 10
#define SETTIME_ICON_HOUR 13
#define SETTIME_ICON_MINUTE 14
#define SETTIME_ICON_HOUR_DOWN 15
#define SETTIME_ICON_HOUR_UP 16
#define SETTIME_ICON_MINUTE_DOWN 17
#define SETTIME_ICON_MINUTE_UP 18


static wimp_w		settime_window = NULL;					/**< The handle of the dialogue box.				*/

static wimp_w		settime_parent_window = NULL;				/**< The parent window handle.					*/
static wimp_i		settime_parent_icon;					/**< The parent icon handle.					*/


static void	settime_click_handler(wimp_pointer *pointer);
static osbool	settime_keypress_handler(wimp_key *key);


/**
 * Initialise the Set Time dialogue.
 */

void settime_initialise(void)
{
	settime_window = templates_create_window("SetTime");
	ihelp_add_window(settime_window, "SetTime", NULL);
	event_add_window_mouse_event(settime_window, settime_click_handler);
	event_add_window_key_event(settime_window, settime_keypress_handler);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_DATE, SETTIME_ICON_DATE_UP, SETTIME_ICON_DATE_DOWN, 1, 31, 1);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_YEAR, SETTIME_ICON_YEAR_UP, SETTIME_ICON_YEAR_DOWN, 1901, 2156, 1);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_HOUR, SETTIME_ICON_HOUR_UP, SETTIME_ICON_HOUR_DOWN, 0, 23, 1);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_MINUTE, SETTIME_ICON_MINUTE_UP, SETTIME_ICON_MINUTE_DOWN, 0, 59, 1);
}


void settime_open(wimp_w w, wimp_i i, wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	if (windows_get_open(settime_window))
		wimp_close_window(settime_window);

	settime_parent_window = w;
	settime_parent_icon = i;

	windows_open_centred_at_pointer(settime_window, pointer);
}


void settime_close(wimp_w w)
{
	if (windows_get_open(settime_window) && (settime_parent_window == w)) {
		wimp_close_window(settime_window);
		settime_parent_window = NULL;
	}
}



/**
 * Process mouse clicks in the Set Time dialogue.
 *
 * \param *pointer		The mouse event block to handle.
 */

static void settime_click_handler(wimp_pointer *pointer)
{
	if (pointer == NULL)
		return;

	if (pointer->w == settime_window) {
		switch ((int) pointer->i) {
		case SETTIME_ICON_SET:
			//if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
			//	dialogue_read_window(dialogue_data);
			//	dialogue_start_search(dialogue_data);

			//	if (pointer->buttons == wimp_CLICK_SELECT) {
			//		settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
			//		dialogue_close_window();
			//	}
			//}
			break;

		case SETTIME_ICON_CANCEL:
			settime_close(settime_parent_window);
			//if (pointer->buttons == wimp_CLICK_SELECT) {
			//	if (dialogue_data != NULL)
			//		file_destroy(dialogue_data->file);
			//	settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
			//	dialogue_close_window();
			//} else if (pointer->buttons == wimp_CLICK_ADJUST) {
			//	dialogue_set_window(dialogue_data);
			//	dialogue_redraw_window();
			//}
			break;
		}
	}
}


/**
 * Process keypresses in the Set Time window.
 *
 * \param *key		The keypress event block to handle.
 * \return		TRUE if the event was handled; else FALSE.
 */

static osbool settime_keypress_handler(wimp_key *key)
{
	if (key == NULL)
		return FALSE;

	switch (key->c) {
	case wimp_KEY_RETURN:
		//settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
		//dialogue_read_window(dialogue_data);
		//dialogue_start_search(dialogue_data);
		//dialogue_close_window();
		break;

	case wimp_KEY_ESCAPE:
		//settime_close(dialogue_panes[DIALOGUE_PANE_DATE]);
		//if (dialogue_data != NULL)
		//	file_destroy(dialogue_data->file);
		//dialogue_close_window();
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

