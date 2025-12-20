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

#include "applib/accel_service.h"
#include "system/passert.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				*onSample;
	xsSlot				*onTap;
	xsSlot				*onDoubleTap;
	AccelData			sample;
	uint8_t				haveSample;
} PebbleAccelerometerRecord, *PebbleAccelerometer;

static void accelerometerData(AccelData *data, uint32_t num_samples);
static void singleTap(AccelAxisType axis, int32_t direction);
static void doubleTap(AccelAxisType axis, int32_t direction);

void xs_accelerometer_destructor(void *data)
{
	PebbleAccelerometer pa = data;
	if (!pa) return;

	accel_data_service_unsubscribe();
	if (pa->onTap)
		accel_tap_service_unsubscribe();
	if (pa->onDoubleTap)
		accel_double_tap_service_unsubscribe();

	setModdableAppState(accelerometer, C_NULL);

	c_free(pa);
}

void xs_accelerometer_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PebbleAccelerometer pa = it;

	if (pa->onSample)
		(*markRoot)(the, pa->onSample);
	if (pa->onTap)
		(*markRoot)(the, pa->onTap);
	if (pa->onDoubleTap)
		(*markRoot)(the, pa->onDoubleTap);
}

static const xsHostHooks xsAccelerometerHooks = {
	xs_accelerometer_destructor,
	xs_accelerometer_mark,
	NULL
};

void xs_accelerometer(xsMachine *the)
{
	PebbleAccelerometer pa;
	
	if (getModdableAppState(accelerometer))
		xsUnknownError("only one");
	
	xsSlot *onSample = builtinGetCallback(the, xsID_onSample);
	xsSlot *onTap = builtinGetCallback(the, xsID_onTap);
	xsSlot *onDoubleTap = builtinGetCallback(the, xsID_onDoubleTap);

	builtinInitializeTarget(the);

	pa = c_calloc(1, sizeof(PebbleAccelerometerRecord));
	if (!pa)
		xsRangeError("no memory");

	pa->obj = xsThis;
	xsRemember(pa->obj);
	pa->the = the;
	pa->onSample = onSample;
	pa->onTap = onTap;
	pa->onDoubleTap = onDoubleTap;
	setModdableAppState(accelerometer, pa);

	xsmcSetHostData(xsThis, pa);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAccelerometerHooks);

	if (onSample)
		accel_data_service_subscribe(1, accelerometerData);
	else
		accel_data_service_subscribe(0, C_NULL);		// activates accelerometer but not callback or buffer - see accelerometer_peek_test.c

	if (pa->onTap)
		accel_tap_service_subscribe(singleTap);

	if (pa->onDoubleTap)
		accel_double_tap_service_subscribe(doubleTap);
}

void xs_accelerometer_close(xsMachine *the)
{
	PebbleAccelerometer pa = xsmcGetHostData(xsThis);

	if (pa && xsmcGetHostDataValidate(xsThis, (void *)&xsAccelerometerHooks)) {
		xsForget(pa->obj);
		xs_accelerometer_destructor(pa);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_accelerometer_configure(xsMachine *the)
{
	/* PebbleAccelerometer pa = */ xsmcGetHostDataValidate(xsThis, (void *)&xsAccelerometerHooks);
	xsSlot tmp;

	if (xsmcHas(xsArg(0), xsID_hz)) {
		xsmcGet(tmp, xsArg(0), xsID_hz);
		int hz = xsmcToInteger(tmp);
		AccelSamplingRate rate;
		if (hz <= 15)
			rate = ACCEL_SAMPLING_10HZ;
		else if (hz <= 35)
			rate = ACCEL_SAMPLING_25HZ;
		else if (hz <= 75)
			rate = ACCEL_SAMPLING_50HZ;
		else
			rate = ACCEL_SAMPLING_100HZ;

		accel_service_set_sampling_rate(rate);
	}
}

void xs_accelerometer_sample(xsMachine *the)
{
	PebbleAccelerometer pa = xsmcGetHostDataValidate(xsThis, (void *)&xsAccelerometerHooks);
	AccelData data;

	if (pa->onSample) {
		if (!pa->haveSample)
			return;
		data = pa->sample;
		pa->haveSample = false;
	}
	else {
		if (accel_service_peek(&data) < 0)
			return;
	}

	//@@ convert to M^2 per second
	xsSlot tmp;
	xsmcSetNewObject(xsResult);
	xsmcSetNumber(tmp, data.x);
	xsmcSet(xsResult, xsID_x, tmp);
	xsmcSetNumber(tmp, data.y);
	xsmcSet(xsResult, xsID_y, tmp);
	xsmcSetNumber(tmp, data.z);
	xsmcSet(xsResult, xsID_z, tmp);
}

void accelerometerData(AccelData *data, uint32_t num_samples)
{
	PebbleAccelerometer pa = getModdableAppState(accelerometer); 
	pa->sample = *data;
	pa->haveSample = true;

	if (pa->onSample) {
		xsMachine *the = pa->the;
		xsBeginHost(the);
			xsCallFunction0(xsReference(pa->onSample), pa->obj);
		xsEndHost(the);
	}
}

static void doTap(PebbleAccelerometer pa, AccelAxisType axis, int32_t direction, xsSlot *func)
{
	if (!func)
		return;

	char msg[3];
	xsMachine *the = pa->the;
	msg[0] = (ACCEL_AXIS_X == axis) ? 'x' : ((ACCEL_AXIS_Y == axis) ? 'y' : 'z');
	msg[1] = (direction >= 0) ? '+' : '-';
	msg[2] = 0;

	xsBeginHost(the);
		xsmcSetString(xsResult, msg);
		xsCallFunction1(xsReference(func), pa->obj, xsResult);
	xsEndHost(the);
}

void singleTap(AccelAxisType axis, int32_t direction)
{
	PebbleAccelerometer pa = getModdableAppState(accelerometer); 
	doTap(pa, axis, direction, pa->onTap);
}

void doubleTap(AccelAxisType axis, int32_t direction)
{
	PebbleAccelerometer pa = getModdableAppState(accelerometer); 
	doTap(pa, axis, direction, pa->onDoubleTap);
}
