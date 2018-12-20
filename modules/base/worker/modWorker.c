/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "xsesp.h"
#include "mc.xs.h"			// for xsID_ values

struct modWorkerRecord {
	xsMachine	*worker;
	xsMachine	*parent;
	xsSlot		owner;
};

typedef struct modWorkerRecord modWorkerRecord;
typedef modWorkerRecord *modWorker;

static void xs_worker_postfromworker(xsMachine *the);
static void xs_worker_close(xsMachine *the);

void xs_worker_destructor(void *data)
{
	modWorker worker = data;

	if (worker) {
		if (worker->worker)
			fxDeleteMachine(worker->worker);
		c_free(worker);
	}
}

void xs_worker(xsMachine *the)
{
	modWorker worker;
	xsMachine *vm;
	char *module;
	uint32_t allocation = 0, stackCount = 0, slotCount = 0;
	char buffer[256];

	xsmcVars(1);

	if (xsmcArgc > 1) {
		if (xsmcHas(xsArg(1), xsID_allocation)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_allocation);
			allocation = xsmcToInteger(xsVar(0));
		}
		if (xsmcHas(xsArg(1), xsID_stackCount)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_stackCount);
			stackCount = xsmcToInteger(xsVar(0));
		}
		if (xsmcHas(xsArg(1), xsID_slotCount)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_slotCount);
			slotCount = xsmcToInteger(xsVar(0));
		}
	}

	vm = ESP_cloneMachine(allocation, stackCount, slotCount, 1);	// no debugger
	if (!vm)
		xsUnknownError("no memory to instantiate worker");

	module = xsmcToString(xsArg(0));

	worker = c_malloc(sizeof(modWorkerRecord));
	if (!worker) {
		xsDeleteMachine(vm);
		xsUnknownError("no memory");
	}

	worker->worker = vm;
	worker->parent = the;
	worker->owner = xsThis;

	xsmcSetHostData(xsThis, worker);

	vm->context = worker;

	xsBeginHost(vm);

	xsmcVars(2);

	xsVar(0) = xsmcNewObject();
	xsmcSet(xsGlobal, xsID_self, xsVar(0));

	xsVar(1) = xsNewHostFunction(xs_worker_postfromworker, 1);
	xsmcSet(xsVar(0), xsID_postMessage, xsVar(1));

	xsVar(1) = xsNewHostFunction(xs_worker_close, 0);
	xsmcSet(xsVar(0), xsID_close, xsVar(1));

	buffer[0] = 0;
	xsTry {
		xsmcGet(xsVar(0), xsGlobal, xsID_require);
		xsVar(0) = xsCall1(xsVar(0), xsID_weak, xsString(module));
		if (xsmcTest(xsVar(0)) && xsmcIsInstanceOf(xsVar(0), xsFunctionPrototype)) {
			xsCallFunction0(xsVar(0), xsGlobal);
		}
	}
	xsCatch {
		xsmcToStringBuffer(xsException, buffer, sizeof(buffer));
	}
	xsEndHost(vm);

	if (buffer[0]) {
		xsDeleteMachine(vm);
		xsmcSetHostData(xsThis, NULL);
		c_free(worker);
		xsUnknownError(buffer);
	}

	xsRemember(worker->owner);
}

void xs_worker_terminate(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);

	if (!worker->worker)
		return;

	xsDeleteMachine(worker->worker);
	worker->worker = NULL;

	xsForget(worker->owner);
}

void xs_worker_postfrominstantiator(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);
	char *message;
	uint32_t messageLength;
	uint8_t kind;

	if (NULL == worker->worker)
		xsUnknownError("worker terminated");

	xsmcVars(2);

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		message = xsmcToArrayBuffer(xsArg(0));
		messageLength = xsGetArrayBufferLength(xsArg(0));
		kind = 1;
	}
	else {
		xsmcGet(xsVar(0), xsGlobal, xsID_JSON);
		xsVar(1) = xsCall1(xsVar(0), xsID_stringify, xsArg(0));
		message = xsmcToString(xsVar(1));
		messageLength = c_strlen(message);
		kind = 0;
	}

	if (modMessagePostToMachine(worker->worker, NULL, message, messageLength, kind))
		xsUnknownError("post from instantiator failed");
}

void xs_worker_postfromworker(xsMachine *the)
{
	modWorker worker = the->context;
	char *message;
	uint32_t messageLength;
	uint8_t kind;

	xsmcVars(2);

	if (xsmcIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		message = xsmcToArrayBuffer(xsArg(0));
		messageLength = xsGetArrayBufferLength(xsArg(0));
		kind = 1;
	}
	else {
		xsmcGet(xsVar(0), xsGlobal, xsID_JSON);
		xsVar(1) = xsCall1(xsVar(0), xsID_stringify, xsArg(0));
		message = xsmcToString(xsVar(1));
		messageLength = c_strlen(message);
		kind = 0;
	}

	if (modMessagePostToMachine(worker->parent, &worker->owner, message, messageLength, kind))
		xsUnknownError("post from worker failed");
}

void xs_worker_close(xsMachine *the)
{
	xsUnknownError("worker close umimplemented");		//@@
}

