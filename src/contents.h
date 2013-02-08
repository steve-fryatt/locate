/* Copyright 2013, Stephen Fryatt
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
 * \file: contents.h
 *
 * File contents search.
 */

#ifndef LOCATE_CONTENTS
#define LOCATE_CONTENTS

#include "objdb.h"
#include "results.h"

//#include <stdlib.h>
//#include "oslib/types.h"

struct contents_block;

/**
 * Initialise the contents search system.
 */

struct contents_block *contents_create(struct objdb_block *objects, struct results_window *results);

void contents_destroy(struct contents_block *handle);

void contents_add_file(struct contents_block *handle, unsigned key);

#endif

