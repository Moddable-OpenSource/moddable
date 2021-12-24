/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "modTimer.h"

enum {
	kStateIdle = 0,
	kStateActivating,
	kStateActivated,
};

enum {
	kEventKindReading = 1,
	kEventKindActivate = 2,
	kEventKindError = 4,
	kEventKindProperty = 0x80,
	kEventKindMask = 0x7F
};

enum {
	kHandlerKindFunction = 1,
	kHandlerKindObject
};

struct EventHandlerRecord {
	struct EventHandlerRecord	*next;

	xsSlot						*reference;
	uint8_t						eventKind;
	uint8_t						handlerKind;
	uint8_t						service;
	uint8_t						removed;
};
typedef struct EventHandlerRecord EventHandlerRecord;
typedef struct EventHandlerRecord *EventHandler;

struct xsSensorRecord {
	xsMachine			*the;
	xsSlot				obj;

	uint8_t				state;
//	uint8_t				pendingReadingNotification;
	uint8_t				servicing;
	uint8_t				remember;
	int					interval;
//	int					lastEventFiredAt;		// in milliseconds
	uint32_t			timestamp;
	modTimer			timer;

	EventHandler		handlers;
};
typedef struct xsSensorRecord xsSensorRecord;
typedef xsSensorRecord *xsSensor;

static void xs_sensor_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void readSensor(modTimer timer, void *refcon, int refconSize);
static void triggerEvent(xsSensor sensor, uint8_t eventKind, const char *errorMsg);
static void addEventListener(xsMachine *the, uint8_t eventKind, xsSlot *callback);
static void removeEventListener(xsMachine *the, uint8_t eventKind, xsSlot *callback);
static void returnEventListener(xsMachine *the, uint8_t eventKind);
static void updateRemember(xsSensor sensor);
static void triggerErrorEvent(xsSensor sensor, const char *msg);
static void doDeactivate(xsSensor sensor);

const xsHostHooks ICACHE_FLASH_ATTR xsSensorHooks = {
	xs_sensor_destructor,
	xs_sensor_mark,
	NULL
};

void xs_sensor_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	xsSensor sensor = it;
	EventHandler handler;

	for (handler = sensor->handlers; NULL != handler; handler = handler->next)
		(*markRoot)(the, handler->reference);
}

void xs_sensor_destructor(void *data)
{
	xsSensor sensor = data;

	if (!sensor) return;

	if (sensor->timer)
		modTimerRemove(sensor->timer);

	while (sensor->handlers) {
		EventHandler next = sensor->handlers->next;
		c_free(sensor->handlers);
		sensor->handlers = next;
	}

	c_free(sensor);
}

void xs_sensor(xsMachine *the)
{
	xsSensor sensor;
	uint32_t interval = 33;

	if ((xsmcArgc > 0) && xsmcTest(xsArg(0))) {
		if (xsmcHas(xsArg(0), xsID_frequency)) {
			xsmcVars(1);
			xsmcGet(xsVar(0), xsArg(0), xsID_frequency);
			interval = xsmcToNumber(xsVar(0)) / 1000;
		}
	}

	sensor = c_calloc(1, sizeof(xsSensorRecord));
	xsmcSetHostData(xsThis, sensor);
	sensor->obj = xsThis;
	sensor->the = the;

	if (interval <= 0)
		interval = 1;
	sensor->interval = interval;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsSensorHooks);
}

void xs_sensor_start(xsMachine *the)
{
	xsSensor sensor = xsmcGetHostData(xsThis);

	if (kStateIdle != sensor->state)
		return;

	sensor->state = kStateActivating;

	sensor->timer = modTimerAdd(sensor->interval, sensor->interval, readSensor, &sensor, sizeof(sensor));
	if (NULL == sensor->timer)
		xsUnknownError("out of memory");

	updateRemember(sensor);
}

void xs_sensor_stop(xsMachine *the)
{
	xsSensor sensor = xsmcGetHostData(xsThis);

	doDeactivate(sensor);

	updateRemember(sensor);
}

void xs_sensor_get_activated(xsMachine *the)
{
	xsSensor sensor = xsmcGetHostData(xsThis);

	xsmcSetBoolean(xsResult, kStateActivated == sensor->state);
}

void xs_sensor_get_hasReading(xsMachine *the)
{
	xsSensor sensor = xsmcGetHostData(xsThis);

	xsmcSetBoolean(xsResult, 0 != sensor->timestamp);
}

void xs_sensor_get_timestamp(xsMachine *the)
{
	xsSensor sensor = xsmcGetHostData(xsThis);

	if (0 == sensor->timestamp)
		xsResult = xsNull;
	else
		xsmcSetInteger(xsResult, sensor->timestamp);
}

void xs_sensor_set_onreading(xsMachine *the)
{
	addEventListener(the, kEventKindReading | kEventKindProperty, &xsArg(0));
}

void xs_sensor_get_onreading(xsMachine *the)
{
	returnEventListener(the, kEventKindReading | kEventKindProperty);
}

void xs_sensor_set_onactivate(xsMachine *the)
{
	addEventListener(the, kEventKindActivate | kEventKindProperty, &xsArg(0));
}

void xs_sensor_get_onactivate(xsMachine *the)
{
	returnEventListener(the, kEventKindActivate | kEventKindProperty);
}

void xs_sensor_set_onerror(xsMachine *the)
{
	addEventListener(the, kEventKindError | kEventKindProperty, &xsArg(0));
}

void xs_sensor_get_onerror(xsMachine *the)
{
	returnEventListener(the, kEventKindError | kEventKindProperty);
}

void xs_sensor_addEventListener(xsMachine *the)
{
	uint8_t kind;
	char *kindStr = xsmcToString(xsArg(0));
	if (0 == c_strcmp(kindStr, "reading"))
		kind = kEventKindReading;
	else if (0 == c_strcmp(kindStr, "activate"))
		kind = kEventKindActivate;
	else if (0 == c_strcmp(kindStr, "error"))
		kind = kEventKindError;
	else
		xsUnknownError("unsupported");
	addEventListener(the, kind, &xsArg(1));
}

void xs_sensor_removeEventListener(xsMachine *the)
{
	uint8_t kind;
	char *kindStr = xsmcToString(xsArg(0));
	if (0 == c_strcmp(kindStr, "reading"))
		kind = kEventKindReading;
	else if (0 == c_strcmp(kindStr, "activate"))
		kind = kEventKindActivate;
	else if (0 == c_strcmp(kindStr, "error"))
		kind = kEventKindError;
	else
		xsUnknownError("unsupported");
	removeEventListener(the, kind, &xsArg(1));
}

void addEventListener(xsMachine *the, uint8_t eventKind, xsSlot *callback)
{
	xsSensor sensor = xsmcGetHostData(xsThis);
	EventHandler handler, toAdd, prev = NULL;
	xsSlot *reference = xsToReference(*callback);
	if (NULL == reference)
		xsUnknownError("invalid EventHandler");

	xsmcVars(3);

	// see if already present
	for (handler = sensor->handlers; NULL != handler; prev = handler, handler = handler->next) {
		if (eventKind != handler->eventKind)
			continue;

		if (kEventKindProperty & eventKind) {
			handler->reference = reference;
			return;
		}

		xsmcGet(xsVar(0), xsGlobal, xsID_Object);
		xsVar(2) = xsReference(handler->reference);
		xsVar(1) = xsCall2(xsVar(0), xsID_is, xsVar(2), *callback);
		if (xsmcToBoolean(xsVar(1)))
			return;
	}

	// add it
	toAdd = c_calloc(sizeof(EventHandlerRecord), 1);
	if (NULL == toAdd)
		xsUnknownError("no memory");

	toAdd->eventKind = eventKind;
	toAdd->handlerKind = xsmcIsInstanceOf(*callback, xsFunctionPrototype) ? kHandlerKindFunction : kHandlerKindObject;
	toAdd->reference = reference;

	if (NULL == prev)
		sensor->handlers = toAdd;
	else
		prev->next = toAdd;

	updateRemember(sensor);
}

void removeEventListener(xsMachine *the, uint8_t eventKind, xsSlot *callback)
{
	xsSensor sensor = xsmcGetHostData(xsThis);
	EventHandler handler, prev = NULL;
	uint8_t servicing = sensor->servicing;

	xsmcVars(3);

	for (handler = sensor->handlers; NULL != handler; prev = handler, handler = handler->next) {
		if (eventKind != handler->eventKind)
			continue;

		xsmcGet(xsVar(0), xsGlobal, xsID_Object);
		xsVar(2) = xsReference(handler->reference);
		xsVar(1) = xsCall2(xsVar(0), xsID_is, xsVar(2), *callback);
		if (!xsmcToBoolean(xsVar(1)))
			continue;

		if (servicing) {
			handler->service = false;
			handler->removed = true;
			break;
		}

		if (prev)
			prev->next = handler->next;
		else
			sensor->handlers = handler->next;

		c_free(handler);
		break;
	}

	updateRemember(sensor);
}

void returnEventListener(xsMachine *the, uint8_t eventKind)
{
	xsSensor sensor = xsmcGetHostData(xsThis);
	EventHandler handler;

	for (handler = sensor->handlers; NULL != handler; handler = handler->next) {
		if (eventKind != handler->eventKind)
			continue;

		xsResult = xsReference(handler->reference);
		break;
	}
}

void readSensor(modTimer timer, void *refcon, int refconSize)
{
	xsSensor sensor = *(xsSensor *)refcon;
	uint32_t timestamp;

	xsBeginHost(sensor->the);

	if (0 == sensor->timestamp) {
		xsTry {
			xsCall0(sensor->obj, xsID__activate);
		}
		xsCatch {
			triggerErrorEvent(sensor, "NotAllowedError");		//@@ not entirely true
			goto done;
		}

		sensor->state = kStateActivated;
		triggerEvent(sensor, kEventKindActivate, NULL);
	}

#if defined (__ets__)
	timestamp = modMilliseconds();
#else
	//@@
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	timestamp =  ((double)(tv.tv_sec - 1512070000) * 1000.0) + ((double)(tv.tv_usec) / 1000.0);
#endif

	xsTry {
		xsCall0(sensor->obj, xsID__poll);
	}
	xsCatch {
		triggerErrorEvent(sensor, "NotReadableError");
		goto done;
	}

	sensor->timestamp = timestamp;
	triggerEvent(sensor, kEventKindReading, NULL);

done:
	xsEndHost(sensor->the);
}

void triggerEvent(xsSensor sensor, uint8_t eventKind, const char *errorMsg)
{
	xsMachine *the = sensor->the;
	EventHandler handler, prev;

	xsmcVars(2);

	sensor->servicing = true;
	for (handler = sensor->handlers; NULL != handler; handler = handler->next)
		handler->service = true;

again:
	for (handler = sensor->handlers; NULL != handler; handler = handler->next) {
		if ((eventKind != (handler->eventKind & kEventKindMask)) || !handler->service)
			continue;

		handler->service = false;

		xsTry {
			xsmcSetNewObject(xsVar(0));
			if (kEventKindReading == eventKind)
				xsmcSetString(xsVar(1), "reading");
			else if (kEventKindActivate == eventKind)
				xsmcSetString(xsVar(1), "activate");
			if (kEventKindError == eventKind)
				xsmcSetString(xsVar(1), "error");
			xsmcDefine(xsVar(0), xsID_type, xsVar(1), xsDontDelete | xsDontSet);

			xsmcSetNumber(xsVar(1), sensor->timestamp);
			xsmcDefine(xsVar(0), xsID_timeStamp, xsVar(1), xsDontDelete | xsDontSet);

			if (errorMsg) {
				xsmcSetString(xsVar(1), (char *)errorMsg);
				xsVar(1) = xsNew1(xsGlobal, xsID_Error, xsVar(1));
				xsmcSet(xsVar(0), xsID_error, xsVar(1));
			}

			xsResult = xsReference(handler->reference);
			if (kHandlerKindFunction == handler->handlerKind)
				xsCallFunction1(xsResult, sensor->obj, xsVar(0));
			else
				xsCall1(xsResult, xsID_handleEvent, xsVar(0));
		}
		xsCatch {
		}
		goto again;
	}

again2:
	for (handler = sensor->handlers, prev = NULL; NULL != handler; prev = handler, handler = handler->next) {
		handler->service = false;
		if (handler->removed) {
			if (prev)
				prev->next = handler->next;
			else
				sensor->handlers = handler->next;

			c_free(handler);
			goto again2;
		}
	}

	sensor->servicing = false;
}

void updateRemember(xsSensor sensor)
{
	uint8_t remember = 0;
	xsMachine *the = sensor->the;

	if (kStateIdle == sensor->state)
		;
	else {
		EventHandler handler;
		uint8_t events = 0;

		for (handler = sensor->handlers; NULL != handler; handler = handler->next)
			events |= handler->eventKind;

		if (kStateActivating == sensor->state)
			remember = (events & (kEventKindReading | kEventKindActivate | kEventKindError)) ? 1 : 0;
		else if (kStateActivated == sensor->state)
			remember = (events & (kEventKindReading | kEventKindError)) ? 1 : 0;
	}

	if (remember == sensor->remember)
		return;

	sensor->remember = remember;
	if (remember)
		xsRemember(sensor->obj);
	else
		xsForget(sensor->obj);
}

void triggerErrorEvent(xsSensor sensor, const char *errorMsg)
{
	doDeactivate(sensor);

	triggerEvent(sensor, kEventKindError, errorMsg);
}

void doDeactivate(xsSensor sensor)
{
	xsMachine *the = sensor->the;

	if (kStateIdle == sensor->state)
		return;

	sensor->state = kStateIdle;

	if (sensor->timer) {
		modTimerRemove(sensor->timer);
		sensor->timer = NULL;
	}

	xsTry {
		xsCall0(sensor->obj, xsID__deactivate);
	}
	xsCatch {
	}
}
