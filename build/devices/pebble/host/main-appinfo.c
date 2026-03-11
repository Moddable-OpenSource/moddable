/*
 * Copyright (c) 2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values
#include "process_management/app_manager.h"
#include "process_management/pebble_process_md.h"

#include "applib/app_watch_info.h"
#include "mfg/mfg_serials.h"
#include "mfg/mfg_info.h"
#include "applib/i18n.h"


void xs_appinfo_get_uuid(xsMachine *the)
{
	char buffer[UUID_STRING_BUFFER_LENGTH];
	uuid_to_string(&app_manager_get_current_app_md()->uuid, buffer);
	xsmcSetString(xsResult, buffer);
}

void xs_appinfo_get_name(xsMachine *the)
{
	const PebbleProcessMd *md = app_manager_get_current_app_md();
	xsmcSetString(xsResult, (char *)process_metadata_get_name(md));
}

void xs_appinfo_get_isWatchface(xsMachine *the)
{
	xsmcSetBoolean(xsResult, app_manager_is_watchface_running());
}

void xs_device_serialNumber_get(xsMachine *the)
{
  char serial[MFG_SERIAL_NUMBER_SIZE + 1];
  mfg_info_get_serialnumber(serial, sizeof(serial));
  if (!c_isEmpty(serial))
	  xsmcSetString(xsResult, serial);
}

void xs_device_language_get(xsMachine *the)
{
	xsmcSetString(xsResult, (char *)app_get_system_locale());
}
