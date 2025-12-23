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

#include "builtinCommon.h"

#include "applib/app_message/app_message.h"
#include "applib/event_service_client.h"
#include "services/common/evented_timer.h"
#include "syscall/syscall.h"
#include "util/dict.h"

typedef struct {
	xsMachine			*the;
	xsSlot				obj;
	xsSlot				*onReadable;
	xsSlot				*onWritable;
	xsSlot				*onSuspend;
	xsSlot				*keys;		// map from keys as strings to keys as integers
	xsSlot				*map;			// most recent received, unread message
	EventServiceInfo	commSessionEvent;
	EventedTimerID		initial;
	uint8_t				active;
	uint8_t				inReceiveCallback;		// avoid stumbling over apparent bug in appmessage
} PebbleMessageRecord, *PebbleMessage;

void xs_appmessage_destructor(void *data)
{
	PebbleMessage pm = data;
	if (!pm) return;

	event_service_client_unsubscribe(&pm->commSessionEvent);

	app_message_close();

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
	if (pm->map)
		(*markRoot)(the, pm->map);
	if (pm->keys)
		(*markRoot)(the, pm->keys);
}

static const xsHostHooks xsAppMessageHooks = {
	xs_appmessage_destructor,
	xs_appmessage_mark,
	NULL
};

static void initialOnWritable(void *context);
static void messageReceived(DictionaryIterator *iterator, void *context);
static void messageSent(DictionaryIterator *iterator, void *context);
static void messageSendFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context);
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

	uint32_t inbound = app_message_inbox_size_maximum(), outbound = app_message_outbox_size_maximum();
	if (xsmcHas(xsArg(0), xsID_input)) {
		xsmcGet(tmp, xsArg(0), xsID_input);
		inbound = xsmcToInteger(tmp);
	}
	if (xsmcHas(xsArg(0), xsID_output)) {
		xsmcGet(tmp, xsArg(0), xsID_output);
		outbound = xsmcToInteger(tmp);
	}
	if (APP_MSG_OK != app_message_open(inbound, outbound))
		xsUnknownError("app_message_open failed");

	pm = c_calloc(1, sizeof(PebbleMessageRecord));
	if (!pm)
		xsRangeError("no memory");

	pm->obj = xsThis;
	pm->the = the;
	pm->onReadable = onReadable;
	pm->onWritable = onWritable;
	pm->onSuspend = onSuspend;

	xsmcSetHostData(xsThis, pm);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAppMessageHooks);
	xsRemember(pm->obj);

	app_message_set_context(pm);
	app_message_register_inbox_received(messageReceived);

	if (xsmcHas(xsArg(0), xsID_keys)) {
		xsmcGet(tmp, xsArg(0), xsID_keys);
		pm->keys = xsmcToReference(tmp);
	}

	pm->initial = EVENTED_TIMER_INVALID_ID;
	if (pm->onWritable) {
		if (sys_app_pp_get_comm_session())
			pm->initial = evented_timer_register(0, false, initialOnWritable, pm);
		
		app_message_register_outbox_sent(messageSent);
		app_message_register_outbox_failed(messageSendFailed);
	}

	pm->commSessionEvent.type = PEBBLE_COMM_SESSION_EVENT; 
	pm->commSessionEvent.handler = commSessionEvent; 
	pm->commSessionEvent.context = pm;
	event_service_client_subscribe(&pm->commSessionEvent);
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

	if (C_NULL == pm->map)
		return;

	xsmcSetNewObject(xsResult);
	xsSlot tmp = xsReference(pm->map);
	xsmcSet(xsResult, xsID_map, tmp);
	if (pm->keys) {
		tmp = xsReference(pm->keys);
		xsmcSet(xsResult, xsID_keys, tmp);
	}
	pm->map = C_NULL;
}

void xs_appmessage_write(xsMachine *the)
{
	PebbleMessage pm = xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks);
	xsSlot tmp;
	int length, i;

	if (pm->inReceiveCallback)
		xsUnknownError("cannot safely write from read callback");

	xsmcVars(3);

	xsmcGet(tmp, xsArg(1), xsID_length);
	length = xsmcToInteger(tmp);

	DictionaryIterator *iter;
	if (APP_MSG_OK != app_message_outbox_begin(&iter))
		xsUnknownError("output_begin failed");

	for (i = 0; i < length; i++) {
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
			int32_t valueD = xsmcToNumber(xsVar(1));
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

	app_message_outbox_send();
}

void initialOnWritable(void *context)
{
	PebbleMessage pm = context;

	evented_timer_cancel(pm->initial);
	pm->initial = EVENTED_TIMER_INVALID_ID; 

	xsBeginHost(pm->the);
		xsCallFunction0(xsReference(pm->onWritable), pm->obj);
	xsEndHost(pm-the);
}

void messageReceived(DictionaryIterator *iterator, void *context)
{
	PebbleMessage pm = context;

	pm->inReceiveCallback = 1;

	xsBeginHost(pm->the);

		xsmcVars(3);
		xsVar(0) = xsNew0(xsGlobal, xsID_Map);

		for (Tuple *t = dict_read_first(iterator); NULL != t; t = dict_read_next(iterator)) {
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
			xsmcSetInteger(xsVar(2), t->key);
			xsCall2(xsVar(0), xsID_set, xsVar(2), xsVar(1));
		}

		pm->map = xsmcToReference(xsVar(0));		// overwrites last message, if unread

		if (pm->onReadable) {
			xsmcSetInteger(xsResult, 1);
			xsCallFunction1(xsReference(pm->onReadable), pm->obj, xsResult);
		}

	xsEndHost(pm->the);

	pm->inReceiveCallback = 0;
}

void messageSent(DictionaryIterator *iterator, void *context)
{
	initialOnWritable(context);
}

void messageSendFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
	messageSent(iterator, context);		//@@ notify of error
}

void commSessionEvent(PebbleEvent *e, void *context) {
	PebbleMessage pm = context;
	PebbleCommSessionEvent *pcse = &e->bluetooth.comm_session_event;
	if (!pcse->is_system) // Need pkjs, which runs inside the Pebble app, so need system session.
		return;

	if (pcse->is_open == pm->active)
		return;
	
	pm->active = pcse->is_open;
	if (pm->active) {
		if (pm->onWritable)
			initialOnWritable(pm);
	}
	else if (pm->onSuspend) {
		xsBeginHost(pm->the);
			xsCallFunction0(xsReference(pm->onSuspend), pm->obj);
		xsEndHost(pm-the);
	}
}
