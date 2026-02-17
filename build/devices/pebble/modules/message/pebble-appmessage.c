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

#include "builtinCommon.h"
#include "moddableAppState.h"

#include "applib/app_message/app_message.h"
#include "applib/event_service_client.h"
#include "services/common/evented_timer.h"
#include "syscall/syscall.h"
#include "util/dict.h"

#define kPKJSReadyMessage (15025)

typedef struct  {
	struct PebbleMessageRecord		*firstInstance;
	EventServiceInfo					commSessionEvent;
	xsSlot								*map;			// most recent received, unread message
	EventedTimerID						invokeUpdateActivate;
	uint16_t								inbound;
	uint16_t								outbound;
	uint8_t								writable;			// an instance could send a message
} PebbleMessageStateRecord, *PebbleMessageState;

typedef struct PebbleMessageRecord PebbleMessageRecord;
typedef PebbleMessageRecord *PebbleMessage;

struct PebbleMessageRecord {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				*onReadable;
	xsSlot				*onWritable;
	xsSlot				*onSuspend;
	xsSlot				*keys;		// map from keys as strings to keys as integers
	EventedTimerID		initial;
	EventedTimerID		readable;
	uint8_t				active;
	uint8_t				writable;
	PebbleMessage		next;
	PebbleMessageState	state;
};

void xs_appmessage_destructor(void *data)
{
	PebbleMessage pm = data;
	if (!pm) return;

	PebbleMessageState state = pm->state;
	if (!state) return;

	if (state->firstInstance == pm)
		state->firstInstance = pm->next;
	else if (state->firstInstance) {
		PebbleMessage walker = state->firstInstance;
		while (walker && walker->next != pm)
			walker = walker->next;
		if (walker)
			walker->next = pm->next;
	}

	evented_timer_cancel(pm->initial);
	evented_timer_cancel(pm->readable);
	
	if (C_NULL == state->firstInstance) {
		event_service_client_unsubscribe(&state->commSessionEvent);
		app_message_close();
		c_free(state);
		setModdableAppState(appMessage, C_NULL);
	}

	c_free(pm);
}

void xs_appmessage_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PebbleMessage pm = it;

	if (pm->onReadable)
		(*markRoot)(the, pm->onReadable);
	if (pm->onWritable)
		(*markRoot)(the, pm->onWritable);
	if (pm->onSuspend)
		(*markRoot)(the, pm->onSuspend);
	if (pm->state->map)
		(*markRoot)(the, pm->state->map);
	if (pm->keys)
		(*markRoot)(the, pm->keys);
}

static const xsHostHooks xsAppMessageHooks = {
	xs_appmessage_destructor,
	xs_appmessage_mark,
	NULL
};

static void suspendWritable(PebbleMessage writer);
static void invokeOnReadable(void *context);
static void invokeOnWritable(void *context);
static void invokeUpdateActivate(void *context);
static void messageReceived(DictionaryIterator *iterator, void *context);
static void messageSent(DictionaryIterator *iterator, void *context);
static void messageSendFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void updateActive(uint8_t active);
static void commSessionEvent(PebbleEvent *e, void *unused);

void xs_appmessage(xsMachine *the)
{
	PebbleMessage pm;
	xsSlot tmp;

	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);
	xsSlot *onWritable = builtinGetCallback(the, xsID_onWritable);
	xsSlot *onSuspend = builtinGetCallback(the, xsID_onSuspend);

	builtinInitializeTarget(the);

	if (xsmcHas(xsArg(0), xsID_format)) {
		xsmcGet(tmp, xsArg(0), xsID_format);
		if (0 != c_strcmp(xsmcToString(tmp), "map"))
			xsRangeError("only map");
	}

	PebbleMessageState state = getModdableAppState(appMessage);

	uint32_t inbound = state ? state->inbound : 0, outbound = state ? state->outbound : 0;
	if (C_NULL == state) {
		inbound = app_message_inbox_size_maximum();
		outbound = app_message_outbox_size_maximum();
		if (xsmcHas(xsArg(0), xsID_input)) {
			xsmcGet(tmp, xsArg(0), xsID_input);
			uint32_t t = xsmcToInteger(tmp);
			if (t < inbound)
				inbound = t;
		}
		if (xsmcHas(xsArg(0), xsID_output)) {
			xsmcGet(tmp, xsArg(0), xsID_output);
			uint32_t t = xsmcToInteger(tmp);
			if (t < outbound)
				outbound = t;
		}
	}

	pm = c_calloc(1, sizeof(PebbleMessageRecord));
	if (!pm)
		xsRangeError("no memory");

	if (C_NULL == state) {
		state = c_calloc(1, sizeof(PebbleMessageStateRecord));
		if (!state) {
			c_free(pm);
			xsUnknownError("no memory");
		}
		state->invokeUpdateActivate = EVENTED_TIMER_INVALID_ID;

		if (APP_MSG_OK != app_message_open(inbound, outbound)) {
			c_free(pm);
			c_free(state);
			xsUnknownError("app_message_open failed");
		}

		app_message_register_inbox_received(messageReceived);
		app_message_register_outbox_sent(messageSent);
		app_message_register_outbox_failed(messageSendFailed);

		state->commSessionEvent.type = PEBBLE_COMM_SESSION_EVENT;
		state->commSessionEvent.handler = commSessionEvent;
		state->commSessionEvent.context = C_NULL;
		event_service_client_subscribe(&state->commSessionEvent);

		state->inbound = inbound;
		state->outbound = outbound;
	}

	pm->state = state;
	pm->obj = xsThis;
	pm->the = the;
	pm->onReadable = onReadable;
	pm->onWritable = onWritable;
	pm->onSuspend = onSuspend;

	xsmcSetHostData(xsThis, pm);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAppMessageHooks);
	xsRemember(pm->obj);

	if (xsmcHas(xsArg(0), xsID_keys)) {
		xsmcGet(tmp, xsArg(0), xsID_keys);
		pm->keys = xsmcToReference(tmp);
	}

	if (C_NULL == state->firstInstance)
		state->writable = sys_app_pp_get_comm_session() ? 1 : 0;
	pm->initial = EVENTED_TIMER_INVALID_ID;
	pm->readable = EVENTED_TIMER_INVALID_ID;
	if (!getModdableAppState(notFirst)) {
		DictionaryIterator *iter;
		app_message_outbox_begin(&iter);
		dict_write_int8(iter, kPKJSReadyMessage, (int8_t)1);
		app_message_outbox_send();
	}
	else if (state->writable && getModdableAppState(pkjsReady)) {
		pm->writable = pm->active = true;
		if (pm->onWritable)
			pm->initial = evented_timer_register(1, false, invokeOnWritable, pm);
	}

	pm->next = state->firstInstance;
	state->firstInstance = pm;

	setModdableAppState(appMessage, state);
}

void xs_appmessage_close(xsMachine *the)
{
	PebbleMessage pm = xsmcGetHostData(xsThis);
	if (pm && xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks)) {
		xsForget(pm->obj);
		xs_appmessage_destructor(pm);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_appmessage_read(xsMachine *the)
{
	PebbleMessage pm = xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks);

	if (C_NULL == pm->state->map)
		return;

	xsmcSetNewObject(xsResult);
	xsSlot tmp = xsReference(pm->state->map);
	xsmcSet(xsResult, xsID_map, tmp);
	if (pm->keys) {
		tmp = xsReference(pm->keys);
		xsmcSet(xsResult, xsID_keys, tmp);
	}
	pm->state->map = C_NULL;
}

void xs_appmessage_write(xsMachine *the)
{
	PebbleMessage pm = xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks);
	xsSlot tmp;

	if( !pm->writable || !pm->state->writable)
		xsUnknownError("not writable");

	xsmcVars(3);

	xsmcGet(tmp, xsArg(1), xsID_length);
	int length = xsmcToInteger(tmp);

	DictionaryIterator *iter;
	if (APP_MSG_OK != app_message_outbox_begin(&iter))
		xsUnknownError("output_begin failed");

	for (int i = 0; i < length; i++) {
		xsmcGetIndex(xsVar(0), xsArg(1), i);
		xsVar(1) = xsCall1(xsArg(0), xsID_get, xsVar(0));

		int keyType = xsmcTypeOf(xsVar(0));
		if (!((keyType == xsIntegerType) || (keyType == xsNumberType))) {
			if (!pm->keys)
				xsUnknownError("no key map");

			xsmcToString(xsVar(0));
			xsVar(0) = xsCall1(xsReference(pm->keys), xsID_get, xsVar(0));
			if (xsmcTypeOf(xsVar(0)) == xsUndefinedType)
				xsUnknownError("unmapped key");
		}
		uint32_t key = (uint32_t)xsmcToInteger(xsVar(0));
		int valueType = xsmcTypeOf(xsVar(1));
		if ((xsStringType == valueType) || (xsStringXType == valueType)) {
			if (dict_write_cstring(iter, key, xsmcToString(xsVar(1))))
				xsUnknownError("overflow");
		}
		else if (xsReferenceType == valueType) {		// some kind of buffer
			void *data;
			xsUnsignedValue count;
			xsmcGetBufferReadable(xsVar(1), &data, &count);
			if (dict_write_data(iter, key, data, count))
				xsUnknownError("overflow");
		}
		else if ((xsIntegerType == valueType) || (xsNumberType == valueType)) {
			xsNumberValue valueD = xsmcToNumber(xsVar(1));
			if (valueD >= 2147483648.0)
				dict_write_uint32(iter, key, (uint32_t)valueD);
			else {
				int32_t value = xsmcToInteger(xsVar(1));
				if ((-128 <= value) && (value <= 127))
					dict_write_int8(iter, key, (int8_t)value);
				else if ((-32768 <= value) && (value <= 32767))
					dict_write_int16(iter, key, (int16_t)value);
				else
					dict_write_int32(iter, key, value);
			}
		}
		else if (xsBooleanType == valueType)
			dict_write_int8(iter, key, (int8_t)xsmcToInteger(xsVar(1)));
		else
			xsUnknownError("unexpected value");
	}

	pm->state->writable = 0;
	app_message_outbox_send();

	pm->writable = false;
	suspendWritable(pm);
}

void suspendWritable(PebbleMessage writer)
{
	PebbleMessageState state = getModdableAppState(appMessage);
	for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
		if (pm->writable && pm->active && (pm != writer)) {
			pm->active = false;
			pm->writable = false;
			if (pm->onSuspend) {
				xsBeginHost(pm->the);
					xsCallFunction0(xsReference(pm->onSuspend), pm->obj);
				xsEndHost(pm-the);
			}
		}
	}
}

void xs_appmessage_get_input(xsMachine *the)
{
	PebbleMessage pm = xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks);
	xsmcSetInteger(xsResult, pm->state->inbound);
}

void xs_appmessage_get_output(xsMachine *the)
{
	PebbleMessage pm = xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks);
	xsmcSetInteger(xsResult, pm->state->outbound);
}

void invokeUpdateActivate(void *context)
{
	PebbleMessageState state = getModdableAppState(appMessage);

	evented_timer_cancel(state->invokeUpdateActivate);
	state->invokeUpdateActivate = EVENTED_TIMER_INVALID_ID;

	updateActive(1);
}

void invokeOnReadable(void *context)
{
	PebbleMessage pm = context;

	evented_timer_cancel(pm->readable);
	pm->readable = EVENTED_TIMER_INVALID_ID;

	xsBeginHost(pm->the);
		xsmcSetInteger(xsResult, 1);
		xsCallFunction1(xsReference(pm->onReadable), pm->obj, xsResult);
	xsEndHost(pm->the);
}

void invokeOnWritable(void *context)
{
	PebbleMessage pm = context;

	if (C_NULL == pm->onWritable)
		return;

	evented_timer_cancel(pm->initial);
	pm->initial = EVENTED_TIMER_INVALID_ID; 

	xsBeginHost(pm->the);
		xsCallFunction0(xsReference(pm->onWritable), pm->obj);
	xsEndHost(pm-the);
}

void messageReceived(DictionaryIterator *iterator, void *context)
{
	PebbleMessageState state = getModdableAppState(appMessage);
	PebbleMessage pm = C_NULL;

	if (!getModdableAppState(pkjsReady)) {
		setModdableAppState(pkjsReady, true);
		setModdableAppState(notFirst, true);
		if (sys_app_pp_get_comm_session())
			state->invokeUpdateActivate = evented_timer_register(1, false, invokeUpdateActivate, C_NULL);		// cannot update active from here because the callback might try to write, which can fail during the receive callback
	}

	xsBeginHost(state->firstInstance->the);

		xsmcVars(3);
		xsVar(0) = xsNew0(xsGlobal, xsID_Map);
		state->map = xsmcToReference(xsVar(0));		// overwrites unread message

		for (Tuple *t = dict_read_first(iterator); C_NULL != t; t = dict_read_next(iterator)) {
			switch (t->type) {
				case TUPLE_CSTRING:
					xsmcSetString(xsVar(1), t->value->cstring);
					break;
				case TUPLE_BYTE_ARRAY:
					xsmcSetArrayBuffer(xsVar(1), t->value->data, t->length);
					break;
				case TUPLE_UINT: {
					uint32_t i;
					if (1 == t->length)
						i = t->value->uint8;
					else if (2 == t->length)
						i = t->value->uint16;
					else
						i = t->value->uint32;
					xsmcSetNumber(xsVar(1), i);	// worst case (could often be integer)
					} break;
				case TUPLE_INT: {
					int32_t i;
					if (1 == t->length)
						i = t->value->int8;
					else if (2 == t->length)
						i = t->value->int16;
					else
						i = t->value->int32;
					xsmcSetInteger(xsVar(1), i);
					} break;
				default:
					PBL_CROAK("unhandled type");
			}

			if (kPKJSReadyMessage == t->key)
				break;		// ignore the proxy's ready message

			xsmcSetInteger(xsVar(2), t->key);
			xsCall2(xsVar(0), xsID_set, xsVar(2), xsVar(1));

			// try to match this dictionary entry to an instance
			for (PebbleMessage walker = state->firstInstance; (C_NULL == pm) && (C_NULL != walker); walker = walker->next) {
				if (!walker->keys)
					continue;
				xsResult = xsCall2(walker->obj, xsID_match, xsVar(2), xsReference(walker->keys));
				if (xsmcTest(xsResult))
					pm = walker;
			}
		}

	xsEndHost(state->firstInstance->the);

	if (pm && pm->onReadable) {
		evented_timer_cancel(pm->readable);
		pm->readable = evented_timer_register(1, false, invokeOnReadable, pm);		// cannot invoke onReadable from here because the callback might try to write, which can fail from this receive callback
	}
}

void messageSent(DictionaryIterator *iterator, void *context)
{
	if (!getModdableAppState(pkjsReady)) return;

	updateActive(1);
}

void messageSendFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
	messageSent(iterator, context);		//@@ notify of error
}

void updateActive(uint8_t active)
{
	PebbleMessageState state = getModdableAppState(appMessage);

	state->writable = active;

	if (active) {
		for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
			if (pm->active && pm->writable)
				continue;

			pm->active = true;
			pm->writable = true;
			invokeOnWritable(pm);
			if (!state->writable) {
				suspendWritable(pm);
				break;		// an instance wrote, so no other instance eligible to write
			}
		}
	}
	else {
		for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
			if (!pm->active)
				continue;

			pm->active = false;
			pm->writable = false;
			if (pm->onSuspend) {
				xsBeginHost(pm->the);
					xsCallFunction0(xsReference(pm->onSuspend), pm->obj);
				xsEndHost(pm->the);
			}
		}
	}
}

void commSessionEvent(PebbleEvent *e, void *context)
{
	PebbleCommSessionEvent *pcse = &e->bluetooth.comm_session_event;
	updateActive(pcse->is_open && pcse->is_system && getModdableAppState(pkjsReady));
}
