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
 * \file: contents.c
 *
 * File contents search.
 */

/* ANSI C header files */

//#include <stdlib.h>
//#include <stdio.h>

/* Acorn C header files */

/* OSLib header files */

//#include "oslib/osfile.h"
//#include "oslib/types.h"
//#include "oslib/wimp.h"

/* SF-Lib header files. */

//#include "sflib/errors.h"
//#include "sflib/event.h"

/* Application header files */

#include "contents.h"

//#include "dataxfer.h"
//#include "main.h"



/**
 * Initialise the contents search system.
 */

void contents_initialise(void)
{
	event_add_message_handler(message_CLAIM_ENTITY, EVENT_MESSAGE_INCOMING, clipboard_message_claim_entity);
	event_add_message_handler(message_DATA_REQUEST, EVENT_MESSAGE_INCOMING, clipboard_message_data_request);
}

