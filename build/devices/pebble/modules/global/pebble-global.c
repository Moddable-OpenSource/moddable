/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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
#include "mc.xs.h"      // for xsID_ values
#include "moddableAppState.h"
#include "services/clock.h"
#include "applib/app_launch_reason.h"
#include "applib/app_light.h"
#include "applib/app_watch_info.h"
#include "applib/connection_service.h"
#include "process_state/app_state/app_state.h"

static void connectionChanged(bool connected)
{
	xsMachine *the = getModdableAppState(the);
	
	xsBeginHost(the);
		xsmcVars(1);
		xsmcGet(xsResult, xsGlobal, xsID_watch);
		xsmcSetStringX(xsVar(0), "connected");
		xsCall1(xsResult, xsID_do, xsVar(0));
	xsEndHost(the);
}

void xs_global_connected(xsMachine *the)
{
	if (0 == xsmcArgc) {
		xsmcSetNewObject(xsResult);
		xsSlot tmp;
		xsmcSetBoolean(tmp, connection_service_peek_pebble_app_connection());
		xsmcSet(xsResult, xsID_app, tmp);
		xsmcSetBoolean(tmp, connection_service_peek_pebblekit_connection());
		xsmcSet(xsResult, xsID_pebblekit, tmp);
	}
	else if (xsmcTest(xsArg(0))) {
		ConnectionHandlers css = {
			.pebble_app_connection_handler = connectionChanged,
			.pebblekit_connection_handler = connectionChanged,
		};
		connection_service_subscribe(css);
	}
	else
		connection_service_unsubscribe();
}

void xs_global_get_hour12(xsMachine *the)
{
	xsmcSetBoolean(xsResult, !clock_is_24h_style());
}

static void prv_unobstructed_change(AnimationProgress progress, void *context)
{
	xsMachine *the = context;

	xsBeginHost(the);
		xsmcVars(1);
		xsmcGet(xsResult, xsGlobal, xsID_watch);
		xsmcSetStringX(xsVar(0), "resize");
		xsCall1(xsResult, xsID_do, xsVar(0));
	xsEndHost(the);
}

static const UnobstructedAreaHandlers gUnobstructedHandlers = {
  .change = prv_unobstructed_change,
};

void xs_global_obstructed(xsMachine *the)
{
	if (xsmcTest(xsArg(0))) {
		if (app_manager_is_watchface_running()) {
			app_unobstructed_area_service_subscribe(gUnobstructedHandlers, the);
			xsmcSetTrue(xsResult);
		}
	}
	else
		app_unobstructed_area_service_unsubscribe();
}

static void willFocus(bool in_focus)
{
	xsMachine *the = getModdableAppState(the);
	
	xsBeginHost(the);
		xsmcVars(2);
		xsmcGet(xsResult, xsGlobal, xsID_watch);
		xsmcSetStringX(xsVar(0), "willFocus");
		xsmcSetBoolean(xsVar(1), in_focus);
		xsCall2(xsResult, xsID_do, xsVar(0), xsVar(1));
	xsEndHost(the);
}

static void didFocus(bool in_focus)
{
	xsMachine *the = getModdableAppState(the);
	
	xsBeginHost(the);
		xsmcVars(2);
		xsmcGet(xsResult, xsGlobal, xsID_watch);
		xsmcSetStringX(xsVar(0), "didFocus");
		xsmcSetBoolean(xsVar(1), in_focus);
		xsCall2(xsResult, xsID_do, xsVar(0), xsVar(1));
	xsEndHost(the);
}

void xs_global_focus(xsMachine *the)
{
	int action = xsmcToInteger(xsArg(0));
	if (1 == action)
		setModdableAppState(willFocus, 1);
	else if (2 == action)
		setModdableAppState(didFocus, 1);
	else if (-1 == action)
		setModdableAppState(willFocus, 0);
	else if (-2 == action)
		setModdableAppState(didFocus, 0);

	if (!getModdableAppState(willFocus) && !getModdableAppState(didFocus))
		app_focus_service_unsubscribe();
	else {
		AppFocusHandlers handlers = {0};
		if (getModdableAppState(willFocus))
			handlers.will_focus = willFocus;
		if (getModdableAppState(didFocus))
			handlers.did_focus = didFocus;
		app_focus_service_subscribe_handlers(handlers);
	}
}

static void wakeup(WakeupId id, int32_t cookie)
{
	xsMachine *the = getModdableAppState(the);
	
	xsBeginHost(the);
		xsmcVars(2);
		xsmcGet(xsResult, xsGlobal, xsID_watch);
		xsmcSetStringX(xsVar(0), "wakeup");

		xsSlot tmp;
		xsmcSetNewObject(xsVar(1));
		xsmcSetInteger(tmp, id);
		xsmcSet(xsVar(1), xsID_id, tmp);
		xsmcSetInteger(tmp, cookie);
		xsmcSet(xsVar(1), xsID_cookie, tmp);

		xsCall2(xsResult, xsID_do, xsVar(0), xsVar(1));
	xsEndHost(the);
}

void xs_global_wakeup(xsMachine *the)
{
	if (xsmcTest(xsArg(0)))
		app_wakeup_service_subscribe(wakeup);
	else
		app_wakeup_service_subscribe(C_NULL);
}

void xs_global_firmwareVersion_get(xsMachine *the)
{
	WatchInfoVersion version = watch_info_get_firmware_version();
	xsmcSetNewObject(xsResult);
	xsSlot tmp;
	xsmcSetInteger(tmp, version.major);
	xsmcSet(xsResult, xsID_major, tmp);
	xsmcSetInteger(tmp, version.minor);
	xsmcSet(xsResult, xsID_minor, tmp);
	xsmcSetInteger(tmp, version.patch);
	xsmcSet(xsResult, xsID_patch, tmp);
}

void xs_global_model_get(xsMachine *the)
{
	xsmcSetInteger(xsResult, (int)watch_info_get_model());
}

void xs_global_launch_get(xsMachine *the)
{
	xsSlot tmp;
	xsmcSetNewObject(xsResult);
	xsmcSetInteger(tmp, app_launch_reason());
	xsmcSet(xsResult, xsID_reason, tmp);
	xsmcSetUnsigned(tmp, app_launch_get_args());
	xsmcSet(xsResult, xsID_arguments, tmp);
}

void xs_global_wake_get(xsMachine *the)
{
	WakeupId id;
	int32_t cookie;

	if (!app_wakeup_get_launch_event(&id, &cookie))
		return;

	xsSlot tmp;
	xsmcSetNewObject(xsResult);
	xsmcSetInteger(tmp, id);
	xsmcSet(xsResult, xsID_id, tmp);
	xsmcSetInteger(tmp, cookie);
	xsmcSet(xsResult, xsID_cookie, tmp);
}

void xs_global_light(xsMachine *the)
{
	if (0 == xsmcArgc)
		app_light_enable_interaction();
	else
		app_light_enable(xsmcTest(xsArg(0)));
}
