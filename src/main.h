/* Locate - locate.h
 *
 * (c) Stephen Fryatt, 2012
 */

#ifndef LOCATE_MAIN
#define LOCATE_MAIN


/**
 * Application-wide global variables.
 */

extern wimp_t			main_task_handle;
extern osbool			main_quit_flag;
extern osspriteop_area		*main_wimp_sprites;

/**
 * Main code entry point.
 */

int main(int argc, char *argv[]);

#endif

