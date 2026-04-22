/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

#include "builtinCommon.h"

#include "applib/touch_service.h"

typedef struct {
	xsMachine *the;
	xsSlot obj;
	xsSlot *onSample;
	TouchEventType type;
	uint16_t x;
	uint16_t y;
	uint8_t haveSample;
} PebbleTouchRecord, *PebbleTouch;

static void prv_touch_event_handler(const TouchEvent *event, void *context);
static void xs_touch_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
void xs_touch_destructor(void *data);

static const xsHostHooks xsTouchHooks = {
	xs_touch_destructor,
	xs_touch_mark,
	NULL
};

void xs_touch_destructor(void *data)
{
	PebbleTouch pt = data;
	if (!pt) return;
	touch_service_unsubscribe();
	c_free(pt);
}

void xs_touch(xsMachine *the)
{
	xsSlot *onSample = builtinGetCallback(the, xsID_onSample);

	builtinInitializeTarget(the);

	PebbleTouch pt = c_calloc(1, sizeof(PebbleTouchRecord));
	if (!pt)
		xsRangeError("no memory");

	pt->obj = xsThis;
	xsRemember(pt->obj);
	pt->the = the;
	pt->onSample = onSample;
	pt->haveSample = 0;

	xsmcSetHostData(xsThis, pt);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsTouchHooks);

	touch_service_subscribe(prv_touch_event_handler, pt);
}

void xs_touch_close(xsMachine *the)
{
	if (!xsmcGetHostData(xsThis)) return;

	PebbleTouch pt = xsmcGetHostDataValidate(xsThis, (void *)&xsTouchHooks);
	xsForget(pt->obj);
	xs_touch_destructor(pt);
	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

void xs_touch_sample(xsMachine *the)
{
	PebbleTouch pt = xsmcGetHostDataValidate(xsThis, (void *)&xsTouchHooks);
	if (!pt->haveSample)
		return;

	xsmcVars(2);
	uint8_t reportPoint = (TouchEvent_Touchdown == pt->type) || (TouchEvent_PositionUpdate == pt->type);
	xsmcSetNewArray(xsResult, reportPoint ? 1 : 0);
	if (reportPoint) {
		xsmcSetNewObject(xsVar(0));
		xsmcSetIndex(xsResult, 0, xsVar(0));

		xsmcSetInteger(xsVar(1), pt->x);
		xsmcSet(xsVar(0), xsID_x, xsVar(1));

		xsmcSetInteger(xsVar(1), pt->y);
		xsmcSet(xsVar(0), xsID_y, xsVar(1));

		xsmcSetInteger(xsVar(1), 0);
		xsmcSet(xsVar(0), xsID_id, xsVar(1));
	}

	pt->haveSample = 0;
}

void prv_touch_event_handler(const TouchEvent *event, void *context)
{
	PebbleTouch pt = context;
	pt->type = event->type;
	pt->x = event->x;
	pt->y = event->y;
	pt->haveSample = 1;
	if (pt->onSample) {
		xsMachine *the = pt->the;
		xsBeginHost(the);
			xsCallFunction0(xsReference(pt->onSample), pt->obj);
		xsEndHost(the);
	}
}

void xs_touch_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PebbleTouch pt = it;
	if (pt->onSample)
		(*markRoot)(the, pt->onSample);
}
