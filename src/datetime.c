/* Copyright 2012-2015, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of Locate:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: datetime.c
 *
 * Work with OS date and time values.
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


#define DATETIME_DATE_FORMAT_DAY "%DY/%MN/%CE%YR"
#define DATETIME_DATE_FORMAT_TIME "%DY/%MN/%CE%YR.%24:%MI"


static osbool	datetime_test_numeric_value(char *text);
static int	datetime_days_in_month(int month, int year);
static int	datetime_adjust_two_digit_year(int year);

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
	int			years, days;
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

	/* Correct the number of days in the month. */

	days = datetime_days_in_month(ordinals.month, ordinals.year);

	if (ordinals.date > days)
		ordinals.date = days;

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
	char				*copy, *day, *month, *year, *hour, *minute;

	copy = strdup(text);
	if (copy == NULL)
		return DATETIME_DATE_INVALID;

	day = strtok(copy, "/");
	month = strtok(NULL, "/");
	year = strtok(NULL, ".");
	hour = strtok(NULL, ".:");
	minute = strtok(NULL, "");

	if (day == NULL || month == NULL || year == NULL)
		return DATETIME_DATE_INVALID; 

	result = datetime_assemble_date(atoi(month), day, year, hour, minute, date);

	free(copy);

	return result;
}


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

enum datetime_date_status datetime_assemble_date(int month, char *day, char *year, char *hour, char *minute, os_date_and_time date)
{
	territory_ordinals		ordinals;
	enum datetime_date_status	result = DATETIME_DATE_INVALID;

	/* Process the date; if we can't get this, then exit. */

	if (month < 1 || month > 12 || !datetime_test_numeric_value(day) || !datetime_test_numeric_value(year))
		return DATETIME_DATE_INVALID;

	ordinals.date = atoi(day);
	ordinals.month = month;
	ordinals.year = datetime_adjust_two_digit_year(atoi(year));

	/* Process the time; if we can't get this, then settle for a date. */

	if (!datetime_test_numeric_value(hour) || !datetime_test_numeric_value(minute)) {
		ordinals.hour = 0;
		ordinals.minute = 0;
		result = DATETIME_DATE_DAY;
	} else {
		ordinals.hour = atoi(hour);
		ordinals.minute = atoi(minute);
		result = DATETIME_DATE_TIME;
	}

	/* Times can't be set to the second or centisecond. */

	ordinals.centisecond = 0;
	ordinals.second = 0;

	/* Validate some of the data. */

	if (ordinals.year < 1900 || ordinals.year > 2248)
		return DATETIME_DATE_INVALID;

	if (ordinals.date < 1 || ordinals.date > datetime_days_in_month(ordinals.month, ordinals.year))
		return DATETIME_DATE_INVALID;

	if (ordinals.hour < 0 || ordinals.hour > 23)
		return DATETIME_DATE_INVALID;

	if (ordinals.minute < 0 || ordinals.minute > 59)
		return DATETIME_DATE_INVALID;

	if (xterritory_convert_ordinals_to_time(territory_CURRENT, (os_date_and_time *) date, &ordinals) != NULL)
		result = DATETIME_DATE_INVALID;

	return result;
}


/**
 * Write a date into a text buffer, formatting it according to its status.
 *
 * \param date			The date to be written.
 * \param status		The status of the date.
 * \param *buffer		A pointer to the buffer to take the date.
 * \param length		The length of the buffer, in bytes.
 */

void datetime_write_date(os_date_and_time date, enum datetime_date_status status, char *buffer, size_t length)
{
	if (date == NULL || buffer == NULL)
		return;

	switch (status) {
	case DATETIME_DATE_INVALID:
		if (length >= 1)
			*buffer = '\0';
		break;
	case DATETIME_DATE_DAY:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) date,
				buffer, length, DATETIME_DATE_FORMAT_DAY);
		break;
	case DATETIME_DATE_TIME:
		territory_convert_date_and_time(territory_CURRENT, (const os_date_and_time *) date,
				buffer, length, DATETIME_DATE_FORMAT_TIME);
		break;
	}
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


/**
 * Return the number of days in a given month in a given year.
 *
 * \param month			The month number (1 - 12).
 * \param year			The year to test in.
 * \return			The number of days in the month.
 */

static int datetime_days_in_month(int month, int year)
{
	int	days;

	switch (month) {
	case 4:
	case 6:
	case 9:
	case 11:
		days = 30;
		break;

	case 2:
		days = (((year % 4) == 0) && ((year % 100) != 0)) ? 29 : 28;
		break;

	default:
		days = 31;
		break;
	}

	return days;
}


/**
 * Adjust two-digit years into a sensible range.
 *
 * Years 01 -> 80 => 2001 -> 2080
 * Years 81 -> 99 => 1981 -> 1999
 * Other years remain unchanged.
 *
 * \param year			The year to be adjusted.
 * \return			The adjusted year.
 */

static int datetime_adjust_two_digit_year(int year)
{
	if (year >= 1 && year <= 80)
		year += 2000;
	else if (year >= 81 && year <= 99)
		year += 1900;

	return year;
}

