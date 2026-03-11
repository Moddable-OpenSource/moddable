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
#include "mc.xs.h"      // for xsID_ values
#include "moddableAppState.h"

#include "builtinCommon.h"

#include "applib/battery_state_service.h"
#include "system/passert.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				*onSample;

	BatteryChargeState	sample;
	uint8_t				haveSample;
} PebbleBatteryRecord, *PebbleBattery;

static void batteryData(BatteryChargeState charge);

void xs_battery_destructor(void *data)
{
	PebbleBattery pb = data;
	if (!pb) return;

	if (pb->onSample)
		battery_state_service_unsubscribe();

	setModdableAppState(battery, C_NULL);

	c_free(pb);
}

void xs_battery_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PebbleBattery pb = it;

	if (pb->onSample)
		(*markRoot)(the, pb->onSample);
}

static const xsHostHooks xsBatteryHooks = {
	xs_battery_destructor,
	xs_battery_mark,
	NULL
};

void xs_battery(xsMachine *the)
{
	PebbleBattery pb;
	
	if (getModdableAppState(battery))
		xsUnknownError("only one");
	
	xsSlot *onSample = builtinGetCallback(the, xsID_onSample);

	builtinInitializeTarget(the);

	pb = c_calloc(1, sizeof(PebbleBatteryRecord));
	if (!pb)
		xsRangeError("no memory");

	pb->obj = xsThis;
	xsRemember(pb->obj);
	pb->the = the;
	pb->onSample = onSample;
	setModdableAppState(battery, pb);

	xsmcSetHostData(xsThis, pb);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsBatteryHooks);

	if (onSample) {
		pb->haveSample = true;
		pb->sample = battery_state_service_peek();
		battery_state_service_subscribe(batteryData);
	}
}

void xs_battery_close(xsMachine *the)
{
	PebbleBattery pb = xsmcGetHostData(xsThis);

	if (pb && xsmcGetHostDataValidate(xsThis, (void *)&xsBatteryHooks)) {
		xsForget(pb->obj);
		xs_battery_destructor(pb);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_battery_sample(xsMachine *the)
{
	PebbleBattery pb = xsmcGetHostDataValidate(xsThis, (void *)&xsBatteryHooks);
	BatteryChargeState data;

	if (pb->onSample) {
		if (!pb->haveSample)
			return;
		data = pb->sample;
		pb->haveSample = false;
	}
	else
		data = battery_state_service_peek();

	xsSlot tmp;
	xsmcSetNewObject(xsResult);
	xsmcSetInteger(tmp, data.charge_percent);
	xsmcSet(xsResult, xsID_percent, tmp);
	xsmcSetBoolean(tmp, data.is_charging);
	xsmcSet(xsResult, xsID_charging, tmp);
	xsmcSetBoolean(tmp, data.is_plugged);
	xsmcSet(xsResult, xsID_plugged, tmp);
}

void batteryData(BatteryChargeState charge)
{
	PebbleBattery pb = getModdableAppState(battery);
	if (C_NULL == pb)
		return;

	pb->sample = charge;
	pb->haveSample = true;

	xsMachine *the = pb->the;
	xsBeginHost(the);
		xsCallFunction0(xsReference(pb->onSample), pb->obj);
	xsEndHost(the);
}
