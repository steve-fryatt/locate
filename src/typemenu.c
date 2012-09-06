/* Locate - typemenu.c
 * (c) Stephen Fryatt, 2012
 *
 * Implement a menu of filetypes.
 */

/* ANSI C Header files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/debug.h"
#include "sflib/heap.h"
#include "sflib/msgs.h"
#include "sflib/string.h"

/* OSLib Header files. */

#include "oslib/os.h"
#include "oslib/types.h"

/* Application header files. */

#include "typemenu.h"

#include "fileicon.h"

#define TYPEMENU_NAME_LENGTH 9							/**< The space allocated to a filetype name.			*/
#define TYPEMENU_VALIDATION_LENGTH 11						/**< The space allocated to a validation string.		*/
#define TYPEMENU_ALLOCATE_BLOCK 50						/**< The number of menu entries that are allocated at a time.	*/


struct typemenu_data {
	char		name[TYPEMENU_NAME_LENGTH];				/**< The name of the filetype.					*/
	char		validation[TYPEMENU_VALIDATION_LENGTH];			/**< The validation string for the menu entry.			*/
	osbool		small;							/**< True if a small iconsprite is available.			*/
	unsigned	type;							/**< The filetype number (or 0x1000 for untyped).		*/
};


static int				typemenu_entries = 0;			/**< The number of entries in the menu.				*/
static size_t				typemenu_data_alloc = 0;		/**< The amount of space allocated to the menu data.		*/
static struct typemenu_data		*typemenu_types = NULL;			/**< The data belonging to the menu entries.			*/
static wimp_menu			*typemenu_menu = NULL;			/**< The typemenu itself.					*/


static int	typemenu_qsort_compare(const void *a, const void *b);

/**
 * Build a typemenu.
 *
 * \return			A pointer to the menu block.
 */

wimp_menu *typemenu_build(void)
{
	int		length, context = 0, line, width = 0;
	char		buffer[TYPEMENU_NAME_LENGTH], *sprite_base, *var_name;
	os_var_type	type;
	os_error	*error;
	osbool		small;

	/* Collect a full set of types from the system. */

	typemenu_entries = 0;

	if (typemenu_types == NULL) {
		typemenu_data_alloc = TYPEMENU_ALLOCATE_BLOCK;
		typemenu_types = heap_alloc(sizeof(struct typemenu_data) * typemenu_data_alloc);

		if (typemenu_types == NULL)
			return NULL;
	}

	sprite_base = fileicon_get_base();

	/* Set up the special "undefined" type. */

	msgs_lookup("Untyped", typemenu_types[typemenu_entries].name, TYPEMENU_NAME_LENGTH);
	typemenu_types[typemenu_entries].name[TYPEMENU_NAME_LENGTH - 1] = '\0';

	typemenu_types[typemenu_entries].validation[0] = 'S';
	strncpy(typemenu_types[typemenu_entries].validation + 1, sprite_base + fileicon_get_special_icon(FILEICON_UNTYPED, &small), TYPEMENU_VALIDATION_LENGTH - 1);
	typemenu_types[typemenu_entries].validation[TYPEMENU_VALIDATION_LENGTH - 1] = '\0';

	typemenu_types[typemenu_entries].type = 0x1000u;

	typemenu_entries++;

	do {
		error = xos_read_var_val("File$Type_*", buffer, sizeof(buffer), context, os_VARTYPE_STRING, &length, &context, &type);

		if (error == NULL && length > 0 && type == os_VARTYPE_STRING) {
			buffer[(length < sizeof(buffer)) ? length : (sizeof(buffer) - 1)] = '\0';

			/* Allocate some more buffer space if we need it. */

			if (typemenu_entries >= typemenu_data_alloc) {
				typemenu_data_alloc += TYPEMENU_ALLOCATE_BLOCK;
				typemenu_types = heap_extend(typemenu_types, sizeof(struct typemenu_data) * typemenu_data_alloc);
				sprite_base = fileicon_get_base();
			}

			if (typemenu_types == NULL)
				break;

			/* Get the variable name and extract the filetype from it. */

			var_name = (char *) context;
			typemenu_types[typemenu_entries].type = strtoul(var_name + 10, NULL, 16);

			/* Get the type name. */

			strncpy(typemenu_types[typemenu_entries].name, buffer, TYPEMENU_NAME_LENGTH);
			typemenu_types[typemenu_entries].name[TYPEMENU_NAME_LENGTH - 1] = '\0';

			/* Get the iconsprite name. */

			typemenu_types[typemenu_entries].validation[0] = 'S';
			strncpy(typemenu_types[typemenu_entries].validation + 1,
					sprite_base + fileicon_get_type_icon(typemenu_types[typemenu_entries].type, "",
					&(typemenu_types[typemenu_entries].small)), TYPEMENU_VALIDATION_LENGTH - 1);
			typemenu_types[typemenu_entries].validation[TYPEMENU_VALIDATION_LENGTH - 1] = '\0';

			typemenu_entries++;
		}
	} while (error == NULL && length > 0);

	if (typemenu_types == NULL)
		return NULL;

	/* Sort the entries into alphabetical order, leaving "Untyped" at the top. */

	qsort(typemenu_types + 1, typemenu_entries - 1, sizeof (struct typemenu_data), typemenu_qsort_compare);

	/* Allocate space for the Wimp menu block. */

	if (typemenu_menu != NULL)
		heap_free(typemenu_menu);

	typemenu_menu = heap_alloc(28 + 24 * typemenu_entries);

	if (typemenu_menu == NULL)
		return NULL;

	/* Build the menu from the collected types. */

	for (line = 0; line < typemenu_entries; line++) {
		if (strlen(typemenu_types[line].name) > width)
			width = strlen(typemenu_types[line].name);

		/* Set the menu and icon flags up. */

		typemenu_menu->entries[line].menu_flags = 0;

		typemenu_menu->entries[line].sub_menu = (wimp_menu *) -1;
		typemenu_menu->entries[line].icon_flags = wimp_ICON_TEXT | wimp_ICON_SPRITE |
				wimp_ICON_VCENTRED | wimp_ICON_FILLED | wimp_ICON_INDIRECTED |
				((typemenu_types[line].small) ? 0 : wimp_ICON_HALF_SIZE) |
				wimp_COLOUR_BLACK << wimp_ICON_FG_COLOUR_SHIFT |
				wimp_COLOUR_WHITE << wimp_ICON_BG_COLOUR_SHIFT;

		/* Set the menu icon contents up. */

		typemenu_menu->entries[line].data.indirected_text.text = typemenu_types[line].name;
		typemenu_menu->entries[line].data.indirected_text.validation = typemenu_types[line].validation;
		typemenu_menu->entries[line].data.indirected_text.size = TYPEMENU_NAME_LENGTH;
	}

	if (typemenu_entries > 1)
		typemenu_menu->entries[0].menu_flags |= wimp_MENU_SEPARATE;

	typemenu_menu->entries[line - 1].menu_flags |= wimp_MENU_LAST;

	msgs_lookup ("FileMenu", typemenu_menu->title_data.text, 12);
	typemenu_menu->title_fg = wimp_COLOUR_BLACK;
	typemenu_menu->title_bg = wimp_COLOUR_LIGHT_GREY;
	typemenu_menu->work_fg = wimp_COLOUR_BLACK;
	typemenu_menu->work_bg = wimp_COLOUR_WHITE;

	typemenu_menu->width = 40 + (width + 1) * 16;
	typemenu_menu->height = 44;
	typemenu_menu->gap = 0;

	return typemenu_menu;
}


/**
 * Sort comparison for sorting filetype data into alphabetical order.
 *
 * \param *a			The first data block for comparison.
 * \param *b			The second data block for comparison.
 * \return			The result of the comparison.
 */

static int typemenu_qsort_compare(const void* a, const void *b)
{
	struct typemenu_data *first = a, *second = b;

	return string_nocase_strcmp(first->name, second->name);
}


/**
 * Process a selection from the type menu, adding the selected type into
 * a list of filetypes if it isn't already there.
 *
 * \param selection		The menu selection.
 * \param type_list		A flex block containing a type list.
 */

void typemenu_process_selection(int selection, flex_ptr type_list)
{
	unsigned	*types;
	int		entries = 0;
	osbool		found = FALSE;

	if (type_list == NULL || *type_list == NULL)
		return;

	if (selection < 0 || selection >= typemenu_entries)
		return;

	types = (unsigned *) *type_list;

	/* Scan the list of types to see if the new one is already there. */

	while (types[entries] != 0xffffffffu) {
		if (types[entries] == typemenu_types[selection].type) {
			found = TRUE;
			break;
		}

		entries++;
	}

	if (found)
		return;

	/* If the type isn't already in the list, extend it by one and
	 * add the type to the end.
	 */

	if (flex_extend(type_list, sizeof(unsigned) * (entries + 1)) == 0)
		return;

	types[entries++] = typemenu_types[selection].type;
	types[entries] = 0xffffffffu;
}

