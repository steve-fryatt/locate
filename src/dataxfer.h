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

/**
 * Initialise the data transfer system.
 */

void dataxfer_initialise(void);


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
 * \return			TRUE on success; FALSE on failure.
 */

osbool dataxfer_start_save(wimp_pointer *pointer, char *name, int size, bits type, osbool (*save_callback)(char *filename));

#endif

