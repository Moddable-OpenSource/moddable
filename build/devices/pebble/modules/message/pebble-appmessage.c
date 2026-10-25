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
#include "services/evented_timer.h"
#include "syscall/syscall.h"
#include "applib/dict.h"

#define kPKJSReadyMessage (15025)

typedef struct  {
	struct PebbleMessageRecord		*firstInstance;
	EventServiceInfo				commSessionEvent;
	EventedTimerID					invokeUpdateActivate;
	EventedTimerID					readable;
	DictionaryIterator				*outbox;			// begun but not sent, because a write failed
	uint16_t						inbound;
	uint16_t						outbound;
	uint16_t						useCount;
	uint8_t							connected;
	uint8_t							writable;
	uint8_t							sending;
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
	xsSlot				*queue;
	EventedTimerID		initial;
	uint8_t				active;
	uint8_t				writable;
	uint8_t				closed;
	PebbleMessage		next;
	PebbleMessageState	state;
};

static int openInstances(PebbleMessageState state)
{
	int count = 0;
	for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
		if (!pm->closed)
			count++;
	}
	return count;
}

static void disconnect(PebbleMessageState state)
{
	if (!state->connected)
		return;
	state->connected = false;

	evented_timer_cancel(state->invokeUpdateActivate);
	evented_timer_cancel(state->readable);
	state->invokeUpdateActivate = state->readable = EVENTED_TIMER_INVALID_ID;
	event_service_client_unsubscribe(&state->commSessionEvent);
	app_message_close();
	state->outbox = C_NULL;
	setModdableAppState(appMessage, C_NULL);
}

// work around inability to cancel a message after app_message_outbox_begin
static AppMessageResult outboxBegin(PebbleMessageState state, DictionaryIterator **iter)
{
	if (state->outbox) {
		*iter = state->outbox;
		dict_write_begin(*iter, (uint8_t *)(*iter)->dictionary, state->outbound);
		return APP_MSG_OK;
	}

	AppMessageResult result = app_message_outbox_begin(iter);
	if (APP_MSG_OK == result)
		state->outbox = *iter;
	return result;
}

static AppMessageResult outboxSend(PebbleMessageState state)
{
	AppMessageResult result = app_message_outbox_send();
	if (APP_MSG_OK == result) {
		state->outbox = C_NULL;
		state->sending = 1;
	}
	return result;
}

static void releaseMessageState(PebbleMessageState state)
{
	// Closed records stay linked while a dispatch may be walking the list.
	if (--state->useCount > openInstances(state))
		return;

	PebbleMessage *link = &state->firstInstance;
	while (*link) {
		PebbleMessage pm = *link;
		if (pm->closed) {
			*link = pm->next;
			c_free(pm);
		}
		else
			link = &pm->next;
	}

	if (!state->useCount)
		c_free(state);
}

static void closeInstance(PebbleMessage pm)
{
	PebbleMessageState state = pm->state;

	pm->closed = true;
	pm->queue = C_NULL;		// no longer marked, so unread messages are collected
	evented_timer_cancel(pm->initial);
	pm->initial = EVENTED_TIMER_INVALID_ID;
	if (!openInstances(state))
		disconnect(state);
	releaseMessageState(state);
}

// ignoring useCount because destructor only called on VM teardown
void xs_appmessage_destructor(void *data)
{
	PebbleMessage pm = data;
	if (!pm)
		return;

	PebbleMessageState state = pm->state;
	if (state) {
		disconnect(state);
		for (PebbleMessage walker = state->firstInstance, next; C_NULL != walker; walker = next) {
			next = walker->next;
			evented_timer_cancel(walker->initial);
			walker->state = C_NULL;
			if (walker->closed)
				c_free(walker);		// open records are freed by their own destructor calls
		}
		c_free(state);
	}
	c_free(pm);
}

void xs_appmessage_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PebbleMessage pm = it;
	if (!pm || pm->closed || !pm->state) return;

	if (pm->onReadable)
		(*markRoot)(the, pm->onReadable);
	if (pm->onWritable)
		(*markRoot)(the, pm->onWritable);
	if (pm->onSuspend)
		(*markRoot)(the, pm->onSuspend);
	if (pm->queue)
		(*markRoot)(the, pm->queue);
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
static void updateActive(PebbleMessageState state, uint8_t active);
static void commSessionEvent(PebbleEvent *e, void *context);

void xs_appmessage(xsMachine *the)
{
	PebbleMessage pm;
	xsSlot tmp;

	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);
	xsSlot *onWritable = builtinGetCallback(the, xsID_onWritable);
	xsSlot *onSuspend = builtinGetCallback(the, xsID_onSuspend);

	builtinInitializeTarget(the);

	if (xsmcGet(tmp, xsArg(0), xsID_format)) {
		if (0 != c_strcmp(xsmcToString(tmp), "map"))
			xsRangeError("only map");
	}

	PebbleMessageState state = getModdableAppState(appMessage);

	uint32_t inbound = state ? state->inbound : 0, outbound = state ? state->outbound : 0;
	if (C_NULL == state) {
		inbound = app_message_inbox_size_maximum();
		outbound = app_message_outbox_size_maximum();
		if (xsmcGet(tmp, xsArg(0), xsID_input)) {
			uint32_t t = xsmcToUnsigned(tmp);
			if (t < inbound)
				inbound = t;
		}
		if (xsmcGet(tmp, xsArg(0), xsID_output)) {
			uint32_t t = xsmcToUnsigned(tmp);
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
		state->readable = EVENTED_TIMER_INVALID_ID;

		if (APP_MSG_OK != app_message_open(inbound, outbound)) {
			c_free(pm);
			c_free(state);
			xsUnknownError("app_message_open failed");
		}

		app_message_set_context(state);
		app_message_register_inbox_received(messageReceived);
		app_message_register_outbox_sent(messageSent);
		app_message_register_outbox_failed(messageSendFailed);

		state->commSessionEvent.type = PEBBLE_COMM_SESSION_EVENT;
		state->commSessionEvent.handler = commSessionEvent;
		state->commSessionEvent.context = state;
		event_service_client_subscribe(&state->commSessionEvent);

		state->inbound = inbound;
		state->outbound = outbound;
		state->connected = true;
	}

	state->useCount++;		// held until this instance closes
	pm->state = state;
	pm->obj = xsThis;
	pm->the = the;
	pm->onReadable = onReadable;
	pm->onWritable = onWritable;
	pm->onSuspend = onSuspend;

	xsmcSetHostData(xsThis, pm);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAppMessageHooks);
	xsRemember(pm->obj);

	if (xsmcGet(tmp, xsArg(0), xsID_keys))
		pm->keys = xsmcToReference(tmp);

	if (C_NULL == state->firstInstance)
		state->writable = sys_app_pp_get_comm_session() ? 1 : 0;
	pm->initial = EVENTED_TIMER_INVALID_ID;
	if (!getModdableAppState(notFirst)) {
		DictionaryIterator *iter;
		if (APP_MSG_OK == outboxBegin(state, &iter)) {
			dict_write_int8(iter, kPKJSReadyMessage, (int8_t)1);
			outboxSend(state);
		}
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
		closeInstance(pm);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_appmessage_read(xsMachine *the)
{
	PebbleMessage pm = xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks);

	if (C_NULL == pm->queue)
		return;

	xsmcVars(2);
	xsVar(0) = xsReference(pm->queue);
	xsVar(1) = xsCall0(xsVar(0), xsID_shift);
	if (xsUndefinedType == xsmcTypeOf(xsVar(1)))
		return;

	xsmcSetNewObject(xsResult);
	xsmcSet(xsResult, xsID_map, xsVar(1));
	if (pm->keys) {
		xsVar(0) = xsReference(pm->keys);
		xsmcSet(xsResult, xsID_keys, xsVar(0));
	}
}

static int queuedMessages(PebbleMessage pm)
{
	if (pm->closed || !pm->queue)
		return 0;

	xsMachine *the = pm->the;
	int count;
	xsBeginHost(the);
		xsmcVars(2);
		xsResult = xsReference(pm->queue);
		xsmcGet(xsResult, xsResult, xsID_length);
		count = xsmcToInteger(xsResult);
	xsEndHost(the);
	return count;
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
	AppMessageResult err = outboxBegin(pm->state, &iter);
	if (APP_MSG_OK != err)
		xsUnknownError("output_begin failed %d", (int)err);

	for (int i = 0; i < length; i++) {
		DictionaryResult result = DICT_OK;
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
		uint32_t key = xsmcToUnsigned(xsVar(0));
		int valueType = xsmcTypeOf(xsVar(1));
		if ((xsStringType == valueType) || (xsStringXType == valueType))
			result = dict_write_cstring(iter, key, xsmcToString(xsVar(1)));
		else if (xsReferenceType == valueType) {		// some kind of buffer
			void *data;
			xsUnsignedValue count;
			xsmcGetBufferReadable(xsVar(1), &data, &count);
			result = dict_write_data(iter, key, data, count);
		}
		else if ((xsIntegerType == valueType) || (xsNumberType == valueType)) {
			xsNumberValue valueD = xsmcToNumber(xsVar(1));
			if (valueD >= 2147483648.0)
				result = dict_write_uint32(iter, key, (uint32_t)valueD);
			else {
				int32_t value = xsmcToInteger(xsVar(1));
				if ((-128 <= value) && (value <= 127))
					result = dict_write_int8(iter, key, (int8_t)value);
				else if ((-32768 <= value) && (value <= 32767))
					result = dict_write_int16(iter, key, (int16_t)value);
				else
					result = dict_write_int32(iter, key, value);
			}
		}
		else if (xsBooleanType == valueType)
			result = dict_write_int8(iter, key, (int8_t)xsmcToInteger(xsVar(1)));
		else
			xsUnknownError("unexpected value");

		if (DICT_OK != result) {
			if (DICT_NOT_ENOUGH_STORAGE == result)
				xsUnknownError("overflow");
			xsUnknownError("dict write failed %d", (int)result);
		}
	}

	pm->state->writable = 0;
	outboxSend(pm->state);

	pm->writable = false;
	suspendWritable(pm);
}

void suspendWritable(PebbleMessage writer)
{
	PebbleMessageState state = writer->state;		// the app state's is clear if the writer closed the last instance
	state->useCount++;
	for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
		if (!pm->closed && pm->writable && pm->active && (pm != writer)) {
			pm->active = false;
			pm->writable = false;
			if (pm->onSuspend) {
				xsMachine *the = pm->the;
				xsBeginHost(the);
					xsCallFunction0(xsReference(pm->onSuspend), pm->obj);
				xsEndHost(pm);
			}
		}
	}
	releaseMessageState(state);
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
	PebbleMessageState state = context;

	evented_timer_cancel(state->invokeUpdateActivate);
	state->invokeUpdateActivate = EVENTED_TIMER_INVALID_ID;

	updateActive(state, 1);
}

void invokeOnReadable(void *context)
{
	PebbleMessageState state = context;

	evented_timer_cancel(state->readable);
	state->readable = EVENTED_TIMER_INVALID_ID;

	state->useCount++;
	for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
		int count = queuedMessages(pm);
		while (count && pm->onReadable) {
			xsMachine *the = pm->the;
			xsBeginHost(the);
				xsmcSetInteger(xsResult, count);
				xsCallFunction1(xsReference(pm->onReadable), pm->obj, xsResult);
			xsEndHost(the);

			int remaining = queuedMessages(pm);		// 0 if onReadable closed the instance
			if (remaining >= count)
				break;
			count = remaining;
		}
	}
	releaseMessageState(state);
}

void invokeOnWritable(void *context)
{
	PebbleMessage pm = context;
	if (pm->closed || (C_NULL == pm->onWritable))
		return;

	PebbleMessageState state = pm->state;
	evented_timer_cancel(pm->initial);
	pm->initial = EVENTED_TIMER_INVALID_ID;

	if (!pm->writable || !state->writable) {
		pm->writable = pm->active = false;
		return;
	}

	xsMachine *the = pm->the;
	state->useCount++;
	xsBeginHost(the);
		xsCallFunction0(xsReference(pm->onWritable), pm->obj);
	xsEndHost(the);
	releaseMessageState(state);
}

void messageReceived(DictionaryIterator *iterator, void *context)
{
	PebbleMessageState state = context;
	PebbleMessage pm = C_NULL;
	xsMachine *the = state->firstInstance->the;
	state->useCount++;

	if (!getModdableAppState(pkjsReady)) {
		setModdableAppState(pkjsReady, true);
		setModdableAppState(notFirst, true);
		if (sys_app_pp_get_comm_session())
			state->invokeUpdateActivate = evented_timer_register(1, false, invokeUpdateActivate, state);		// cannot update active from here because the callback might try to write, which can fail during the receive callback
	}

	xsBeginHost(the);
		xsmcVars(3);
		xsVar(0) = xsNew0(xsGlobal, xsID_Map);

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
					MODDABLE_FATAL("unhandled type");
			}

			if (kPKJSReadyMessage == t->key)
				break;		// ignore the proxy's ready message

			xsmcSetInteger(xsVar(2), t->key);
			xsCall2(xsVar(0), xsID_set, xsVar(2), xsVar(1));

			// try to match this dictionary entry to an instance
			for (PebbleMessage walker = state->firstInstance; (C_NULL == pm) && (C_NULL != walker); walker = walker->next) {
				if (walker->closed || !walker->keys)
					continue;
				xsResult = xsCall2(walker->obj, xsID_match, xsVar(2), xsReference(walker->keys));
				if (!walker->closed && xsmcTest(xsResult))
					pm = walker;
			}
		}

		if (pm && !pm->closed) {
			if (C_NULL == pm->queue) {
				xsmcSetNewArray(xsVar(2), 0);
				pm->queue = xsmcToReference(xsVar(2));
			}
			else
				xsVar(2) = xsReference(pm->queue);
			xsCall1(xsVar(2), xsID_push, xsVar(0));
		}
	xsEndHost(the);

	if (pm && !pm->closed && pm->onReadable && state->connected && (EVENTED_TIMER_INVALID_ID == state->readable))
		state->readable = evented_timer_register(1, false, invokeOnReadable, state);		// cannot invoke onReadable from here because the callback might try to write, which can fail from this receive callback
	releaseMessageState(state);
}

void messageSent(DictionaryIterator *iterator, void *context)
{
	PebbleMessageState state = context;
	state->sending = 0;
	if (!getModdableAppState(pkjsReady)) return;

	updateActive(state, 1);
}

void messageSendFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
	messageSent(iterator, context);		//@@ notify of error
}

void updateActive(PebbleMessageState state, uint8_t active)
{
	state->useCount++;
	state->writable = active && !state->sending;

	if (state->writable) {
		for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
			if (pm->closed || (pm->active && pm->writable))
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
	else if (!active) {
		for (PebbleMessage pm = state->firstInstance; C_NULL != pm; pm = pm->next) {
			if (pm->closed || !pm->active)
				continue;

			pm->active = false;
			pm->writable = false;
			if (pm->onSuspend) {
				xsMachine *the = pm->the;
				xsBeginHost(the);
					xsCallFunction0(xsReference(pm->onSuspend), pm->obj);
				xsEndHost(the);
			}
		}
	}
	releaseMessageState(state);
}

void commSessionEvent(PebbleEvent *e, void *context)
{
	PebbleCommSessionEvent *pcse = &e->bluetooth.comm_session_event;
	updateActive(context, pcse->is_open && pcse->is_system && getModdableAppState(pkjsReady));
}
