/* Copyright 2012, Stephen Fryatt
 *
 * This file is part of Locate:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
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
 * \file: settime.h
 *
 * Date entry dialogue implementation.
 */

#ifndef LOCATE_SETTIME
#define LOCATE_SETTIME


/**
 * Initialise the Set Time dialogue.
 */

void settime_initialise(void);


/**
 * Open the Set Time dialogue for a text icon, using the date and time given
 * in the field (if recognised) to set the fields up.
 *
 * \param w			The window handle of the parent dialogue.
 * \param i			The icon handle to take the date from.
 * \param *pointer		The current position of the pointer.
 */

void settime_open(wimp_w w, wimp_i i, wimp_pointer *pointer);


/**
 * Notify that a window has been closed.  If it is the Set Time dialogue's
 * current parent, then the dialogue is closed and its contents abandoned.
 *
 * \param w			The handle of the window being closed.
 */

void settime_close(wimp_w w);

#endif

