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
