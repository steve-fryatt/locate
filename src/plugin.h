/* Copyright 2013-2015, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file: plugin.h
 *
 * Support FilerAction plugins on RISC OS Select.
 */

#ifndef LOCATE_PLUGIN
#define LOCATE_PLUGIN

osbool plugin_filer_action_launched;

/**
 * Initialise the plugin support
 */

void plugin_initialise(void);


/**
 * Notify the plugin support that we have been laucnched by FilerAction, and
 * therefore are already in the middle of a message exchange.
 */

void plugin_filer_launched(void);

#endif

