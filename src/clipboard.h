/* Copyright 2012, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: clipboard.h
 *
 * Global Clipboard support.
 */

#ifndef LOCATE_CLIPBOARD
#define LOCATE_CLIPBOARD

#include <stdlib.h>
#include "oslib/types.h"

/**
 * Initialise the global clipboard system.
 */

void clipboard_initialise(void);


/**
 * Claim the cliboard contents, supplying a set of callback functions which will
 * provide details in the event that they are required.
 *
 * \param *find			Callback function to supply address of data.
 * \param *size			Callback function to supply size of data.
 * \param *release		Callback function to notify of loss of clipboard.
 * \param *data			Data to be passed to the callback functions.
 */

osbool clipboard_claim(void *(*find)(void *), size_t (*size)(void *), void (*release)(void *), void *data);

#endif

