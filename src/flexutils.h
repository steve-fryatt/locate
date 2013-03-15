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
 * \file: flexutils.h
 *
 * Extensions to the Flex implementation.
 */

#ifndef LOCATE_FLEXUTILS
#define LOCATE_FLEXUTILS

#include "oslib/types.h"
#include "flex.h"


/**
 * Store a string in an already-created flex block.
 *
 * \param ptr		The flex block to use.
 * \param *text		The text to store.
 * \return		TRUE if successful; else FALSE.
 */

osbool flexutils_store_string(flex_ptr ptr, char *text);

#endif

