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
#include "services/common/evented_timer.h"
#include "util/dict.h"

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	xsSlot		*onReadable;
	xsSlot		*onWritable;
	xsSlot		*map;
} PebbleMessageRecord, *PebbleMessage;

void xs_appmessage_destructor(void *data)
{
	PebbleMessage pm = data;
	if (!pm) return;

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
	if (pm->map)
		(*markRoot)(the, pm->map);
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

void xs_appmessage(xsMachine *the)
{
	PebbleMessage pm;
	xsSlot tmp;

	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);
	xsSlot *onWritable = builtinGetCallback(the, xsID_onWritable);

	builtinInitializeTarget(the);

//	uint8_t format = builtinInitializeFormat(the, kIOFormatBuffer);
//	if ((kIOFormatNumber != format) && (kIOFormatBuffer != format))
//		xsRangeError("invalid format");

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

	xsmcSetHostData(xsThis, pm);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAppMessageHooks);
	xsRemember(pm->obj);

	pm->obj = xsThis;
	pm->the = the;
	pm->onReadable = onReadable;
	pm->onWritable = onWritable;
	
	app_message_set_context(pm);
	app_message_register_inbox_received(messageReceived);

	if (pm->onWritable) {
		evented_timer_register(0, false, initialOnWritable, pm);		//@@ memory leak -- need to dispose this...
		
		app_message_register_outbox_sent(messageSent);
		app_message_register_outbox_failed(messageSendFailed);
	}
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
	
	if (!pm->map)
		return;

	xsResult = xsReference(pm->map);
	pm->map = C_NULL;
}

void xs_appmessage_write(xsMachine *the)
{
	/* PebbleMessage pm = */ xsmcGetHostDataValidate(xsThis, (void *)&xsAppMessageHooks);
	xsSlot tmp;
	int length, i;

	xsmcGet(tmp, xsArg(1), xsID_length);
	length = xsmcToInteger(tmp);

	DictionaryIterator *iter;
	if (APP_MSG_OK != app_message_outbox_begin(&iter))
		xsUnknownError("output_begin failed");

	for (i = 0; i < length; i++) {
		xsmcGetIndex(tmp, xsArg(1), i);
		int type = xsmcTypeOf(tmp);
		if (!((type == xsIntegerType) || (type == xsNumberType)))
			xsUnknownError("invalid key");
		uint32_t key = (uint32_t)xsmcToInteger(tmp);
		tmp = xsCall1(xsArg(0), xsID_get, tmp);
		type = xsmcTypeOf(tmp);
		if ((xsStringType == type) || (xsStringXType == type)) {
			if (dict_write_cstring(iter, key, xsmcToString(tmp)))
				xsUnknownError("overflow");
		}
		else if ((xsReferenceType == type) && xsmcIsInstanceOf(tmp, xsArrayBufferPrototype)) {
			void *data;
			xsUnsignedValue count;
			xsmcGetBufferReadable(tmp, &data, &count);
			if (dict_write_data(iter, key, data, count))
				xsUnknownError("overflow");
		}
		else
			xsUnknownError("unexpected value");
	}

	app_message_outbox_send();
}

void xs_appmessage_get_format(xsMachine *the)
{
}

void xs_appmessage_set_format(xsMachine *the)
{
}

void initialOnWritable(void *context)
{
	PebbleMessage pm = context;

	xsBeginHost(pm->the);
		xsCallFunction0(xsReference(pm->onWritable), pm->obj);
	xsEndHost(pm-the);
}

void messageReceived(DictionaryIterator *iterator, void *context)
{
	PebbleMessage pm = context;

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
				default:		//@@ fill in integer types
					PBL_LOG(LOG_LEVEL_ALWAYS, " ignore key %d", (int)t->key);
					continue;
			}
			xsmcSetInteger(xsVar(2), t->key);
			xsCall2(xsVar(0), xsID_set, xsVar(2), xsVar(1));
		}

		pm->map = xsmcToReference(xsVar(0));		//@@ overwrites last message, if unread

		if (pm->onReadable) {
			xsmcSetInteger(xsResult, 1);
			xsCallFunction1(xsReference(pm->onReadable), pm->obj, xsResult);
		}

	xsEndHost(pm->the);
}

void messageSent(DictionaryIterator *iterator, void *context)
{
	initialOnWritable(context);
}

void messageSendFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
	messageSent(iterator, context);		//@@ notify of error
}
