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

#endif

