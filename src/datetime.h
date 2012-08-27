/* Locate - datetime.h
 * (c) Stephen Fryatt, 2012
 *
 * Work with OS Date and Time values.
 */

#ifndef LOCATE_DATETIME
#define LOCATE_DATETIME

#include "oslib/types.h"
#include "flex.h"


/**
 * Add two os_date_and_time values together, storing the result in the first.
 *
 * \param date		The base date value, to which the result is written.
 * \param add		The date value to be added to the base.
 * \return		TRUE if successful; else FALSE if an overflow occurred.
 */

osbool datetime_add_date(os_date_and_time date, os_date_and_time add);


/**
 * Subtract one os_date_and_time value from another, storing the result in the first.
 *
 * \param date		The base date value, to which the result is written.
 * \param subtract	The date value to be subtracted from the base.
 * \return		TRUE if successful; else FALSE if an overflow occurred.
 */

osbool datetime_subtract_date(os_date_and_time date, os_date_and_time subtract);


/**
 * Convert an os_date_and_time object into two unsigned integers.
 *
 * \param date			The five-byte date to be converted.
 * \param *high			The variable to take the high byte of the date.
 * \param *low			The variable to take the low four bytes of the date.
 */

void datetime_get_date(os_date_and_time date, unsigned *high, unsigned *low);


/**
 * Create an os_date_and_time object from two unsigned integers.
 *
 * \param date			The date to be set up.
 * \param high			The high byte of the five-byte date.
 * \param low			The low four bytes of the five-byte date.
 */

void datetime_set_date(os_date_and_time date, unsigned high, unsigned low);


/**
 * Copy one five-byte date into another.
 *
 * \param out			The date to receive the copy.
 * \param in			The date to be copied.
 */

void datetime_copy_date(os_date_and_time out, os_date_and_time in);

#endif

