/* Locate - dataxfer.h
 *
 * (c) Stephen Fryatt, 2012
 */

#ifndef LOCATE_DATAXFER
#define LOCATE_DATAXFER

/* ==================================================================================================================
 * Static constants
 */


#define TEXT_FILE_TYPE 0xfff
#define LOCATE_FILE_TYPE 0x1a1;

/**
 * A savebox data handle.
 */

struct dataxfer_savebox;

/**
 * Initialise the data transfer system.
 */

void dataxfer_initialise(void);


/**
 * Create a new save dialogue definition.
 *
 * \param: selection		TRUE if the dialogue has a Selection switch; FALSE if not.
 * \param: *sprite		Pointer to the sprite name for the dialogue.
 * \param: *save_callback	The callback function for saving data.
 */

struct dataxfer_savebox *dataxfer_new_savebox(osbool selection, char *sprite, osbool (*save_callback)(char *filename, osbool selection));


/**
 * Initialise a save dialogue definition with two filenames, and an indication of
 * the selection icon status.  This is called when opening a menu or creating a
 * dialogue from a toolbar.
 *
 * \param *handle		The handle of the save dialogue to be initialised.
 * \param *fullname		Pointer to the filename for a full save.
 * \param *selectname		Pointer to the filename for a selection save.
 * \param selection		TRUE if the Selection option is enabled; else FALSE.
 * \param selected		TRUE if the Selection option is selected; else FALSE.
 */

void dataxfer_savebox_initialise(struct dataxfer_savebox *handle, char *fullname, char *selectname, osbool selection, osbool selected);


/**
 * Prepare the physical save dialogue based on the current state of the given
 * dialogue data.  Any existing data will be saved into the appropriate
 * dialogue definition first.
 *
 * \param *handle		The handle of the save dialogue to prepare.
 */

void dataxfer_savebox_prepare(struct dataxfer_savebox *handle);


/**
 * Start dragging an icon from a dialogue, creating a sprite to drag and starting
 * a drag action.  When the action completes, a callback will be made to the
 * supplied function.
 *
 * \param w		The window where the drag is starting.
 * \param i		The icon to be dragged.
 * \param callback	A callback function
 */

void dataxfer_save_window_drag(wimp_w w, wimp_i i, void (* drag_end_callback)(wimp_pointer *pointer, void *data), void *drag_end_data);


/**
 * Start a data save action by sending a message to another task.  The data
 * transfer protocol will be started, and at an appropriate time a callback
 * will be made to save the data.
 *
 * \param *pointer		The Wimp pointer details of the save target.
 * \param *name			The proposed file leafname.
 * \param size			The estimated file size.
 * \param type			The proposed file type.
 * \param *save_callback	The function to be called with the full pathname
 *				to save the file.
 * \param *data			Data to be passed to the callback function.
 * \return			TRUE on success; FALSE on failure.
 */

osbool dataxfer_start_save(wimp_pointer *pointer, char *name, int size, bits type, osbool (*save_callback)(char *filename, void *data), void *data);

#endif

