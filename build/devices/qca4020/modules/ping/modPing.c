/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#include "xsqca4020.h"
#include "mc.xs.h"

#include "qapi.h"
#include "qapi_net_status.h"
#include "qapi_ns_gen_v4.h"
#include "qapi_ns_utils.h"

#include "qurt_error.h"
#include "qurt_thread.h"
#include "qurt_signal.h"

//#define PING_THREAD_PRIORITY	9
#define PING_THREAD_PRIORITY	19
#define PING_THREAD_STACK_SIZE	2048

enum {
	THREAD_CREATE_SIG_MASK = (1L << 0),
	PING_SIG_MASK = (1L << 1),
	CLOSE_SIG_MASK = (1L << 2),
};

typedef struct xsPingRecord xsPingRecord;
typedef xsPingRecord *xsPing;

struct xsPingRecord {
	xsSlot			obj;
	xsMachine		*the;

	qurt_signal_t	signal;
	qurt_thread_t	thread;
	uint32_t		address;
	uint32_t		size;
	qapi_Status_t	result;
	
	uint8_t			done;
};

static void ping_task(void *pvParameter);
static void pinged(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

void xs_ping_configure(xsMachine *the)
{
	xsPing xsp;
	qurt_thread_attr_t attr;
	int result;
	uint32_t cur_signals;
		
	xsp = c_calloc(sizeof(xsPingRecord), 1);
	if (!xsp)
		xsUnknownError("no memory");
	
	xsmcSetHostData(xsThis, xsp);

	xsp->obj = xsThis;
	xsp->the = the;
	
	inet_pton(AF_INET, xsmcToString(xsArg(0)), &xsp->address);
	xsp->size = xsmcToInteger(xsArg(1));		
	xsRemember(xsp->obj);
	
	qurt_signal_create(&xsp->signal);

	qurt_thread_attr_init(&attr);
	qurt_thread_attr_set_name(&attr, "ping");
	qurt_thread_attr_set_priority(&attr, PING_THREAD_PRIORITY);
	qurt_thread_attr_set_stack_size(&attr, PING_THREAD_STACK_SIZE);

	result = qurt_thread_create(&xsp->thread, &attr, ping_task, xsp);
	if (QURT_EOK == result)
		result = qurt_signal_wait_timed(&xsp->signal, THREAD_CREATE_SIG_MASK, QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK, &cur_signals, 1000); 	
	if (QURT_EOK != result)
		xsUnknownError("create ping thread failed");
}

void xs_ping(xsMachine *the)
{
	xsPing xsp = xsmcGetHostData(xsThis);
	
	qurt_signal_set(&xsp->signal, PING_SIG_MASK);
}

void xs_ping_destructor(void *data)
{
	xsPing xsp = data;
	if (xsp)
		c_free(xsp);
}

void xs_ping_close(xsMachine *the)
{
	xsPing xsp = xsmcGetHostData(xsThis);

	if (NULL == xsp)
		xsUnknownError("close on closed ping");

	xsp->done = true;
	qurt_signal_set(&xsp->signal, CLOSE_SIG_MASK);

	while (0 != xsp->thread)
		modDelayMilliseconds(100);

	qurt_signal_delete(&xsp->signal);
	
	xsForget(xsp->obj);
	xsmcSetHostData(xsThis, NULL);
	xs_ping_destructor(xsp);
}

void ping_task(void *pvParameter)
{
	xsPing xsp = (xsPing)pvParameter;
	qapi_Status_t result;
				
	qurt_signal_set(&xsp->signal, THREAD_CREATE_SIG_MASK);
	
	do {
		qurt_signal_wait(&xsp->signal, PING_SIG_MASK | CLOSE_SIG_MASK, QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK);
		if (xsp->done)
			break;
		
		result = qapi_Net_Ping(xsp->address, xsp->size);
		if (QAPI_OK == result || QAPI_NET_ERR_INVALID_IPADDR == result)
			;
		else
			result = QAPI_NET_ERR_SOCKET_CMD_TIME_OUT;
	
		xsp->result = result;
		modMessagePostToMachine(xsp->the, NULL, 0, pinged, xsp);
	} while (!xsp->done);

	xsp->thread = 0;
	qurt_thread_stop();
}

void pinged(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsPing xsp = refcon;
	uint32_t msg, value;

	if (QAPI_OK == xsp->result) {
		msg = 1;
		value = xsp->size;
	}
	else if (QAPI_NET_ERR_SOCKET_CMD_TIME_OUT == xsp->result) {
		msg = 2;
		value = 0;
	}
	else {
		msg = -1;
		value = xsp->result;
	}
	
	xsBeginHost(xsp->the);
	
	xsmcVars(2);	
	xsmcSetInteger(xsVar(0), msg);
	xsmcSetInteger(xsVar(1), value);
	xsCall2(xsp->obj, xsID_callback, xsVar(0), xsVar(1));

	xsEndHost(xsp->the);
}

