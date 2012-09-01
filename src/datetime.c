/* Locate - datetime.c
 * (c) Stephen Fryatt, 2012
 *
 * Work with OS Date and Time values.
 */

/* ANSI C Header files. */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Acorn C Header files. */

/* SFLib Header files. */

/* OSLib Header files. */

#include "oslib/os.h"
#include "oslib/territory.h"
#include "oslib/types.h"

/* Application header files. */

#include "datetime.h"


static osbool	datetime_test_numeric_value(char *text);

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


/**
 * Add or subtract a given number of months from a date, rounding the result in
 * to fit the resulting number of days.
 *
 * \param date			The date to be modified.
 * \param months		The number of months to add or subtract.
 */

void datetime_add_months(os_date_and_time date, int months)
{
	int			years, days_in_month;
	territory_ordinals	ordinals;

	territory_convert_time_to_ordinals(territory_CURRENT, (const os_date_and_time *) date, &ordinals);

	years = months / 12;
	months %= 12;

	ordinals.month += months;

	if (ordinals.month > 12) {
		ordinals.month -= 12;
		years++;
	}

	if (ordinals.month < 1) {
		ordinals.month += 12;
		years--;
	}

	ordinals.year += years;

	switch (ordinals.month) {
	case 4:
	case 6:
	case 9:
	case 11:
		days_in_month = 30;
		break;

	case 2:
		days_in_month = (((ordinals.year % 4) == 0) && ((ordinals.year % 100) != 0)) ? 29 : 28;
		break;

	default:
		days_in_month = 31;
		break;
	}

	if (ordinals.date > days_in_month)
		ordinals.date = days_in_month;

	territory_convert_ordinals_to_time(territory_CURRENT, (os_date_and_time *) date, &ordinals);
}


/**
 * Parse a textual date into five-byte OS date values.
 *
 * \param *text			The text to be parsed.
 * \param *date			The location to store the resulting date.
 * \return			The status of the resulting date.
 */

enum datetime_date_status datetime_read_date(char *text, os_date_and_time date)
{
	enum datetime_date_status	result;
	territory_ordinals		ordinals;
	char				*copy, *day, *month, *year, *hour, *minute;

	copy = strdup(text);
	if (copy == NULL)
		return DATETIME_DATE_INVALID;

	day = strtok(copy, "/");
	month = strtok(NULL, "/");
	year = strtok(NULL, ".");
	hour = strtok(NULL, ".:");
	minute = strtok(NULL, "");

	/* If day, month or year are invalid then it's not a date. */

	if (!datetime_test_numeric_value(day) || !datetime_test_numeric_value(month) || !datetime_test_numeric_value(year)) {
		free(copy);
		return DATETIME_DATE_INVALID;
	}

	ordinals.date = atoi(day);
	ordinals.month = atoi(month);
	ordinals.year = atoi(year);

	/* 01 -> 80 == 2001 -> 2080; 81 -> 99 == 1981 -> 1999 */

	if (ordinals.year >= 1 && ordinals.year <= 80)
		ordinals.year += 2000;
	else if (ordinals.year >= 81 && ordinals.year <= 99)
		ordinals.year += 1900;

	if (datetime_test_numeric_value(hour) && datetime_test_numeric_value(minute)) {
		ordinals.hour = atoi(hour);
		ordinals.minute = atoi(minute);
		result = DATETIME_DATE_TIME;
	} else {
		ordinals.hour = 0;
		ordinals.minute = 0;
		result = DATETIME_DATE_DAY;
	}

	ordinals.centisecond = 0;
	ordinals.second = 0;

	free(copy);

	if (xterritory_convert_ordinals_to_time(territory_CURRENT, (os_date_and_time *) date, &ordinals) != NULL)
		result = DATETIME_DATE_INVALID;

	return result;
}


/**
 * Test a string to make sure that it only contains decimal digits.
 *
 * \param *text			The string to test.
 * \return			TRUE if strink OK; else FALSE.
 */

static osbool datetime_test_numeric_value(char *text)
{
	int	i;
	osbool	result = TRUE;

	if (text == NULL)
		return FALSE;

	for (i = 0; i < strlen(text); i++)
		if (!isdigit(text[i])) {
			result = FALSE;
			break;
		}

	return result;
}

