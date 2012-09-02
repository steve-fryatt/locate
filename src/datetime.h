/* Locate - datetime.h
 * (c) Stephen Fryatt, 2012
 *
 * Work with OS Date and Time values.
 */

#ifndef LOCATE_DATETIME
#define LOCATE_DATETIME

#include "oslib/types.h"
#include "flex.h"

#define DATETIME_HALF_MINUTE 3000u
#define DATETIME_1_MINUTE 6000u
#define DATETIME_HALF_HOUR 180000u
#define DATETIME_1_HOUR 360000u
#define DATETIME_HALF_DAY 4320000u
#define DATETIME_1_DAY 8640000u
#define DATETIME_HALF_WEEK 30240000u
#define DATETIME_1_WEEK 60480000u
#define DATETIME_15_DAYS 129600000u
#define DATETIME_HALF_YEAR 1576800000u

#define DATETIME_DATE_FORMAT_DAY "%DY/%MN/%CE%YR"
#define DATETIME_DATE_FORMAT_TIME "%DY/%MN/%CE%YR.%24:%MI"


enum datetime_date_status {
	DATETIME_DATE_INVALID = 0,
	DATETIME_DATE_DAY,
	DATETIME_DATE_TIME
};

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


/**
 * Add or subtract a given number of months from a date, rounding the result in
 * to fit the resulting number of days.
 *
 * \param date			The date to be modified.
 * \param months		The number of months to add or subtract.
 */

void datetime_add_months(os_date_and_time date, int months);


/**
 * Parse a textual date into five-byte OS date values.
 *
 * \param *text			The text to be parsed.
 * \param *date			The location to store the resulting date.
 * \return			The status of the resulting date.
 */

enum datetime_date_status datetime_read_date(char *text, os_date_and_time date);


/**
 * Create a date from day, month, year, hour and minute components. Month is
 * supplied numerically; the remaining parameters are supplied as text strings
 * which must be validated.
 *
 * \param month			The month value, in numeric form.
 * \param *day			The day of the month, in string form.
 * \param *year			The year, in string form.
 * \param *hour			The hour, in string form, or NULL.
 * \param *minute		The minute, in string form, or NULL.
 * \param date			The location to store the resulting date.
 * \return			The status of the resulting date.
 */

enum datetime_date_status datetime_assemble_date(int month, char *day, char *year, char *hour, char *minute, os_date_and_time date);

#endif

