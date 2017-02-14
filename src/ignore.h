/* Copyright 2016, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * Wildcard search based on code by Alessandro Cantatore:
 * http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
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
 * \file: ignore.h
 *
 * File and folder ignore list.
 */

#ifndef LOCATE_IGNORE
#define LOCATE_IGNORE

struct ignore_block;

void ignore_destroy(struct ignore_block *handle);

osbool ignore_match_object(struct ignore_block *handle, char *name);

osbool ignore_search_content(struct ignore_block *handle, char *name);

#endif
