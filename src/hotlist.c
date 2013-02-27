/* Copyright 2013, Stephen Fryatt
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
 * \file: hotlist.c
 *
 * Hotlist implementation.
 */

/* ANSI C header files */

#include <string.h>

/* Acorn C header files */

/* OSLib header files */

//#include "oslib/os.h"
#include "oslib/wimp.h"

/* SF-Lib header files. */

//#include "sflib/debug.h"
//#include "sflib/errors.h"
//#include "sflib/event.h"
//#include "sflib/icons.h"
//#include "sflib/menus.h"
//#include "sflib/msgs.h"
//#include "sflib/string.h"
//#include "sflib/url.h"
//#include "sflib/windows.h"

/* Application header files */

#include "iconbar.h"

//#include "choices.h"
//#include "dataxfer.h"
#include "dialogue.h"
//#include "discfile.h"
//#include "file.h"
#include "ihelp.h"
//#include "main.h"
#include "templates.h"


/* Iconbar menu */

#define ICONBAR_MENU_INFO 0
#define ICONBAR_MENU_HELP 1
#define ICONBAR_MENU_HISTORY 2
#define ICONBAR_MENU_CHOICES 3
#define ICONBAR_MENU_QUIT 4

/* Program Info Window */

#define ICON_PROGINFO_AUTHOR  4
#define ICON_PROGINFO_VERSION 6
#define ICON_PROGINFO_WEBSITE 8

#if 0
static void	iconbar_click_handler(wimp_pointer *pointer);
static void	iconbar_menu_prepare(wimp_w w, wimp_menu *menu, wimp_pointer *pointer);
static void	iconbar_menu_selection(wimp_w w, wimp_menu *menu, wimp_selection *selection);
static osbool	iconbar_proginfo_web_click(wimp_pointer *pointer);
static osbool	iconbar_icon_drop_handler(wimp_message *message);
static osbool	iconbar_load_locate_file(wimp_w w, wimp_i i, unsigned filetype, char *filename, void *data);
#endif


static wimp_menu		*hotlist_menu = NULL;				/**< The hotlist menu handle.					*/
static wimp_w			hotlist_add_window = NULL;			/**< The add to hotlist window handle.			*/
//static struct dialogue_block	*iconbar_last_search_dialogue = NULL;		/**< The handle of the last search dialogue, or NULL if none.	*/


/**
 * Initialise the iconbar icon and its associated menus and dialogues.
 */

void hotlist_initialise(void)
{
	//char*			date = BUILD_DATE;
	//wimp_icon_create	icon_bar;

	//iconbar_menu = templates_get_menu(TEMPLATES_MENU_ICONBAR);

	hotlist_add_window = templates_create_window("HotlistAdd");
	//templates_link_menu_dialogue("ProgInfo", iconbar_info_window);
	ihelp_add_window(hotlist_add_window, "HotlistAdd", NULL);
}


/**
 * Add a dialogue to the hotlist.
 *
 * \param *dialogue		The handle of the dialogue to be added.
 */

void hotlist_add_dialogue(struct dialogue_block *dialogue)
{

}

