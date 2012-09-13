/* Locate - file.h
 * (c) Stephen Fryatt, 2012
 *
 * Search file record creation, maipulation and deletion.
 */

#ifndef LOCATE_FILE
#define LOCATE_FILE

struct file_block;

/**
 * Create a new file with no data associated to it.
 *
 * \return			Pointer to an empty file block, or NULL.
 */

struct file_block *file_create(void);


/**
 * Create a new file block by opening a search window.
 *
 * \param *pointer		The pointer position to open the dialogue at.
 * \param *path			A path to use, or NULL for the default.
 * \param *template		A template to use, or NULL for the default.
 */

void file_create_dialogue(wimp_pointer *pointer, char *path, struct file_block *template);


/**
 * Create a new search and results window for the file.
 *
 * \param *file			The file to create the objects for.
 * \param *paths		The path(s) to search.
 * \return			The new search block, or NULL on failure.
 */

struct search_block *file_create_search(struct file_block *file, char *paths);


/**
 * Create a new file block by loading in pre-saved results.
 */

void file_create_results(void);


/**
 * Destroy a file, freeing its data and closing any windows.
 *
 * \param *block		The handle of the file to destroy.
 */

void file_destroy(struct file_block *block);


/**
 * Destroy all of the open file blocks, freeing data in the process.
 */

void file_destroy_all(void);


/**
 * Identify whether a file has a search active.
 *
 * \param *file			The file to be tested.
 * \return			TRUE if it has an active search; else FALSE.
 */

osbool file_search_active(struct file_block *file);


/**
 * Stop any active search associated with a file.
 *
 * \param *file			The file to be stopped.
 */

void file_stop_search(struct file_block *file);

#endif

