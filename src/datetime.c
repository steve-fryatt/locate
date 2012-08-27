/* Locate - datetime.c
 * (c) Stephen Fryatt, 2012
 *
 * Work with OS Date and Time values.
 */

/* ANSI C Header files. */

/* Acorn C Header files. */

/* SFLib Header files. */

/* OSLib Header files. */

#include "oslib/os.h"
#include "oslib/types.h"

/* Application header files. */

#include "datetime.h"


/**
 * Add two os_date_and_time values together, storing the result in the first.
 *
 * \param date		The base date value, to which the result is written.
 * \param add		The date value to be added to the base.
 * \return		TRUE if successful; else FALSE if an overflow occurred.
 */

osbool datetime_add_date(os_date_and_time date, os_date_and_time add)
{
	unsigned	date_lo, date_hi, add_lo, add_hi, result_lo, result_hi;

	datetime_get_date(date, &date_hi, &date_lo);
	datetime_get_date(add, &add_hi, &add_lo);

	result_hi = date_hi + add_hi;
	result_lo = date_lo + add_lo;

	if (result_lo < date_lo)
		result_hi++;

	datetime_set_date(date, result_hi, result_lo);

	return ((result_hi & 0xffffff00u) != 0x0u) ? FALSE : TRUE;
}


/**
 * Subtract one os_date_and_time value from another, storing the result in the first.
 *
 * \param date		The base date value, to which the result is written.
 * \param subtract	The date value to be subtracted from the base.
 * \return		TRUE if successful; else FALSE if an overflow occurred.
 */

osbool datetime_subtract_date(os_date_and_time date, os_date_and_time subtract)
{
	unsigned	date_lo, date_hi, subtract_lo, subtract_hi, result_lo, result_hi;

	datetime_get_date(date, &date_hi, &date_lo);
	datetime_get_date(subtract, &subtract_hi, &subtract_lo);

	result_hi = date_hi - subtract_hi;
	result_lo = date_lo - subtract_lo;

	if (result_lo > date_lo)
		result_hi--;

	datetime_set_date(date, result_hi, result_lo);

	return ((result_hi & 0xffffff00u) != 0x0u) ? FALSE : TRUE;
}


/**
 * Convert an os_date_and_time object into two unsigned integers.
 *
 * \param date			The five-byte date to be converted.
 * \param *high			The variable to take the high byte of the date.
 * \param *low			The variable to take the low four bytes of the date.
 */

void datetime_get_date(os_date_and_time date, unsigned *high, unsigned *low)
{
	if (low != NULL)
		*low = date[0] | (date[1] << 8) | (date[2] << 16) | (date[3] << 24);

	if (high != NULL)
		*high = date[4];
}


/**
 * Create an os_date_and_time object from two unsigned integers.
 *
 * \param date			The date to be set up.
 * \param high			The high byte of the five-byte date.
 * \param low			The low four bytes of the five-byte date.
 */

void datetime_set_date(os_date_and_time date, unsigned high, unsigned low)
{
	date[0] = (low & 0xffu);
	date[1] = (low & 0xff00u) >> 8;
	date[2] = (low & 0xff0000u) >> 16;
	date[3] = (low & 0xff000000u) >> 24;
	date[4] = (high & 0xffu);
}


/**
 * Copy one five-byte date into another.
 *
 * \param out			The date to receive the copy.
 * \param in			The date to be copied.
 */

void datetime_copy_date(os_date_and_time out, os_date_and_time in)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
	out[4] = in[4];
}

