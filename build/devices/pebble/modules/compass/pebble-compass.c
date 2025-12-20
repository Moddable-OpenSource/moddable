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

/*
	warning: untested
*/

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values
#include "moddableAppState.h"

#include "builtinCommon.h"

#include "applib/compass_service.h"
#include "system/passert.h"
#include "util/trig.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				*onSample;
	CompassHeadingData	sample;
	uint8_t				haveSample;
} PebbleCompassRecord, *PebbleCompass;

static void compassData(CompassHeadingData data);

void xs_compass_destructor(void *data)
{
	PebbleCompass pc = data;
	if (!pc) return;

	compass_service_unsubscribe();

	setModdableAppState(compass, C_NULL);

	c_free(pc);
}

void xs_compass_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PebbleCompass pc = it;

	if (pc->onSample)
		(*markRoot)(the, pc->onSample);
}

static const xsHostHooks xsCompassHooks = {
	xs_compass_destructor,
	xs_compass_mark,
	NULL
};

void xs_compass(xsMachine *the)
{
	PebbleCompass pc;
	
	if (getModdableAppState(compass))
		xsUnknownError("only one");
	
	xsSlot *onSample = builtinGetCallback(the, xsID_onSample);

	builtinInitializeTarget(the);

	pc = c_calloc(1, sizeof(PebbleCompassRecord));
	if (!pc)
		xsRangeError("no memory");

	pc->obj = xsThis;
	xsRemember(pc->obj);
	pc->the = the;
	pc->onSample = onSample;
	setModdableAppState(compass, pc);

	xsmcSetHostData(xsThis, pc);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsCompassHooks);

	if (onSample)
		compass_service_subscribe(compassData);
	else
		compass_service_subscribe(C_NULL);
}

void xs_compass_close(xsMachine *the)
{
	PebbleCompass pc = xsmcGetHostData(xsThis);

	if (pc && xsmcGetHostDataValidate(xsThis, (void *)&xsCompassHooks)) {
		xsForget(pc->obj);
		xs_compass_destructor(pc);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_compass_configure(xsMachine *the)
{
	/* PebbleCompass pc = */ xsmcGetHostDataValidate(xsThis, (void *)&xsCompassHooks);
	xsSlot tmp;

	if (xsmcHas(xsArg(0), xsID_filter)) {
		xsmcGet(tmp, xsArg(0), xsID_filter);
		int filter = xsmcToNumber(tmp);
		compass_service_set_heading_filter(normalize_angle(DEG_TO_TRIGANGLE(filter)));
	}
}

void xs_compass_sample(xsMachine *the)
{
#if CAPABILITY_HAS_MAGNETOMETER
	PebbleCompass pc = xsmcGetHostDataValidate(xsThis, (void *)&xsCompassHooks);
	CompassHeadingData data;

	if (pc->onSample) {
		if (!pc->haveSample)
			return;
		data = pc->sample;
		pc->haveSample = false;
	}
	else {
		if (compass_service_peek(&data) < 0)
			return;
	}

	//@@ unit conversion?
	xsSlot tmp;
	xsmcSetNewObject(xsResult);
	
	xsmcSetNumber(tmp, TRIGANGLE_TO_DEG(data.magnetic_heading));
	xsmcSet(xsResult, xsID_heading, tmp);
#endif
}

void compassData(CompassHeadingData data)
{
#if CAPABILITY_HAS_MAGNETOMETER
	PebbleCompass pc = getModdableAppState(compass); 
	pc->sample = data;
	pc->haveSample = true;

	xsMachine *the = pc->the;
	xsBeginHost(the);
		xsCallFunction0(xsReference(pc->onSample), pc->obj);
	xsEndHost(the);
#endif
}
