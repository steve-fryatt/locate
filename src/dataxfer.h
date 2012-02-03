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

#define DRAG_SAVE_PDF    1
#define DRAG_SAVE_SAVEAS 2

enum dataxfer_dragtype {
	DRAGTYPE_NONE = 0,							/**< No drag is in action.			*/
	DRAGTYPE_CONVERT = 1							/**< Dragging a file to start a conversion.	*/
};

/**
 * Initialise the data transfer system.
 */

void dataxfer_initialise(void);


/**
 * Start dragging the icon from the save dialogue.  Called in response to an attempt to drag the icon.
 *
 * \param type		The drag type to start.
 * \param w		The window where the drag is starting.
 * \param i		The icon to be dragged.
 * \param *filename	The filename to be used as a starting point.
 */

void start_save_window_drag(enum dataxfer_dragtype type, wimp_w w, wimp_i i, char *filename);

#endif

