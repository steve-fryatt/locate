/* Copyright 2012, Stephen Fryatt
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
 * \file: settime.c
 *
 * Date entry dialogue implementation.
 */

/* ANSI C Header files. */

/* Acorn C Header files. */

/* SFLib Header files. */

#include "sflib/event.h"
#include "sflib/icons.h"
#include "sflib/windows.h"

/* OSLib Header files. */

#include "oslib/osword.h"
#include "oslib/territory.h"
#include "oslib/wimp.h"

/* Application header files. */

#include "settime.h"

#include "datetime.h"
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
#define SETTIME_ICON_TIME_FIELD 11
#define SETTIME_ICON_TIME_COLON 12
#define SETTIME_ICON_HOUR 13
#define SETTIME_ICON_MINUTE 14
#define SETTIME_ICON_HOUR_DOWN 15
#define SETTIME_ICON_HOUR_UP 16
#define SETTIME_ICON_MINUTE_DOWN 17
#define SETTIME_ICON_MINUTE_UP 18


static wimp_menu			*settime_month_menu = NULL;		/**< The popup menu of months.					*/

static wimp_w				settime_window = NULL;			/**< The handle of the dialogue box.				*/

static wimp_w				settime_parent_window = NULL;		/**< The parent window handle.					*/
static wimp_i				settime_parent_icon;			/**< The parent icon handle.					*/

static os_date_and_time			settime_initial_date;			/**< The date set when the dialogue opened.			*/
static enum datetime_date_status	settime_initial_status;			/**< The date status when the dialogue opened.			*/


static void	settime_set_window(enum datetime_date_status, os_date_and_time date);
static void	settime_redraw_window(void);
static void	settime_click_handler(wimp_pointer *pointer);
static osbool	settime_keypress_handler(wimp_key *key);


/**
 * Initialise the Set Time dialogue.
 */

void settime_initialise(void)
{
	settime_month_menu = templates_get_menu(TEMPLATES_MENU_MONTH);

	settime_window = templates_create_window("SetTime");
	ihelp_add_window(settime_window, "SetTime", NULL);
	event_add_window_mouse_event(settime_window, settime_click_handler);
	event_add_window_key_event(settime_window, settime_keypress_handler);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_DATE, SETTIME_ICON_DATE_UP, SETTIME_ICON_DATE_DOWN, 1, 31, 1);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_YEAR, SETTIME_ICON_YEAR_UP, SETTIME_ICON_YEAR_DOWN, 1901, 2156, 1);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_HOUR, SETTIME_ICON_HOUR_UP, SETTIME_ICON_HOUR_DOWN, 0, 23, 1);
	event_add_window_icon_bump(settime_window, SETTIME_ICON_MINUTE, SETTIME_ICON_MINUTE_UP, SETTIME_ICON_MINUTE_DOWN, 0, 59, 1);
	event_add_window_icon_popup(settime_window, SETTIME_ICON_MONTH_POPUP, settime_month_menu, SETTIME_ICON_MONTH, NULL);
}


/**
 * Open the Set Time dialogue for a text icon, using the date and time given
 * in the field (if recognised) to set the fields up.
 *
 * \param w			The window handle of the parent dialogue.
 * \param i			The icon handle to take the date from.
 * \param *pointer		The current position of the pointer.
 */

void settime_open(wimp_w w, wimp_i i, wimp_pointer *pointer)
{
	oswordreadclock_utc_block	now;


	if (pointer == NULL)
		return;

	if (windows_get_open(settime_window))
		wimp_close_window(settime_window);

	settime_parent_window = w;
	settime_parent_icon = i;

	settime_initial_status = datetime_read_date(icons_get_indirected_text_addr(w, i), settime_initial_date);

	if (settime_initial_status == DATETIME_DATE_INVALID) {
		now.op = oswordreadclock_OP_UTC;
		oswordreadclock_utc(&now);

		datetime_copy_date(settime_initial_date, now.utc);
		settime_initial_status = DATETIME_DATE_DAY;
	}

	settime_set_window(settime_initial_status, settime_initial_date);
	windows_open_centred_at_pointer(settime_window, pointer);
	icons_put_caret_at_end(settime_window, SETTIME_ICON_DATE);
}


/**
 * Notify that a window has been closed.  If it is the Set Time dialogue's
 * current parent, then the dialogue is closed and its contents abandoned.
 *
 * \param w			The handle of the window being closed.
 */

void settime_close(wimp_w w)
{
	if (windows_get_open(settime_window) && (settime_parent_window == w)) {
		wimp_close_window(settime_window);
		settime_parent_window = NULL;
	}
}


/**
 * Fill the Set Time dialogue with date and time values.
 *
 * \param status		The date status to use.
 * \param date			The date values to place in the dialogue.
 */

static void settime_set_window(enum datetime_date_status status, os_date_and_time date)
{
	territory_ordinals		ordinals;


	territory_convert_time_to_ordinals(territory_CURRENT, (const os_date_and_time *) date, &ordinals);

	icons_printf(settime_window, SETTIME_ICON_DATE, "%d", ordinals.date);
	icons_printf(settime_window, SETTIME_ICON_YEAR, "%d", ordinals.year);
	event_set_window_icon_popup_selection(settime_window, SETTIME_ICON_MONTH_POPUP, ordinals.month - 1);

	icons_set_selected(settime_window, SETTIME_ICON_SET_TIME, status == DATETIME_DATE_TIME);

	icons_printf(settime_window, SETTIME_ICON_HOUR, "%02d", ordinals.hour);
	icons_printf(settime_window, SETTIME_ICON_MINUTE, "%02d", ordinals.minute);

	icons_set_group_shaded_when_off(settime_window, SETTIME_ICON_SET_TIME, 8,
			SETTIME_ICON_TIME_FIELD, SETTIME_ICON_TIME_COLON,
			SETTIME_ICON_HOUR, SETTIME_ICON_HOUR_DOWN, SETTIME_ICON_HOUR_UP,
			SETTIME_ICON_MINUTE, SETTIME_ICON_MINUTE_DOWN, SETTIME_ICON_MINUTE_UP);

}


/**
 * Force a refresh of all the updatable fields in the dialogue.
 */

static void settime_redraw_window(void)
{
	wimp_set_icon_state(settime_window, SETTIME_ICON_DATE, 0, 0);
	wimp_set_icon_state(settime_window, SETTIME_ICON_MONTH, 0, 0);
	wimp_set_icon_state(settime_window, SETTIME_ICON_YEAR, 0, 0);
	wimp_set_icon_state(settime_window, SETTIME_ICON_HOUR, 0, 0);
	wimp_set_icon_state(settime_window, SETTIME_ICON_MINUTE, 0, 0);
}


/**
 * Write the date and time from the dialogue back to the parent icon.
 *
 * \return			TRUE if successful; FALSE if not.
 */

static osbool settime_write_back_time(void)
{
	int				month;
	enum datetime_date_status	result;
	os_date_and_time		date;


	month = event_get_window_icon_popup_selection(settime_window, SETTIME_ICON_MONTH_POPUP) + 1;

	result = datetime_assemble_date(month,
			icons_get_indirected_text_addr(settime_window, SETTIME_ICON_DATE),
			icons_get_indirected_text_addr(settime_window, SETTIME_ICON_YEAR),
			icons_get_indirected_text_addr(settime_window, SETTIME_ICON_HOUR),
			icons_get_indirected_text_addr(settime_window, SETTIME_ICON_MINUTE),
			date);

	if (result == DATETIME_DATE_INVALID)
		return FALSE;

	if (!icons_get_selected(settime_window, SETTIME_ICON_SET_TIME))
		result = DATETIME_DATE_DAY;

	datetime_copy_date(settime_initial_date, date);
	settime_initial_status = result;

	datetime_write_date(date, result,
			icons_get_indirected_text_addr(settime_parent_window, settime_parent_icon),
			icons_get_indirected_text_length(settime_parent_window, settime_parent_icon));
	wimp_set_icon_state(settime_parent_window, settime_parent_icon, 0, 0);

	return TRUE;
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
			if (pointer->buttons == wimp_CLICK_SELECT || pointer->buttons == wimp_CLICK_ADJUST) {
				if (settime_write_back_time() && pointer->buttons == wimp_CLICK_SELECT)
					settime_close(settime_parent_window);
			}
			break;

		case SETTIME_ICON_CANCEL:
			if (pointer->buttons == wimp_CLICK_SELECT) {
				settime_close(settime_parent_window);
			} else if (pointer->buttons == wimp_CLICK_ADJUST) {
				settime_set_window(settime_initial_status, settime_initial_date);
				settime_redraw_window();
				icons_replace_caret_in_window(settime_window);
			}
			break;

		case SETTIME_ICON_SET_TIME:
			icons_set_group_shaded_when_off(settime_window, SETTIME_ICON_SET_TIME, 8,
					SETTIME_ICON_TIME_FIELD, SETTIME_ICON_TIME_COLON,
					SETTIME_ICON_HOUR, SETTIME_ICON_HOUR_DOWN, SETTIME_ICON_HOUR_UP,
					SETTIME_ICON_MINUTE, SETTIME_ICON_MINUTE_DOWN, SETTIME_ICON_MINUTE_UP);
			icons_replace_caret_in_window(settime_window);
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

