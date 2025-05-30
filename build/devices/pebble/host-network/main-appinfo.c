#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values
#include "process_management/app_manager.h"
#include "process_management/pebble_process_md.h"

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
