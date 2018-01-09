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

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"

	static void workerLoop(void *pvParameter);
#endif

struct modWorkerRecord {
	xsMachine		*the;
	xsMachine		*parent;
	xsSlot			owner;
	uint32_t		allocation;
	uint32_t		stackCount;
	uint32_t		slotCount;
#if ESP32
	TaskHandle_t	task;
#endif
	char			module[1];
};

typedef struct modWorkerRecord modWorkerRecord;
typedef modWorkerRecord *modWorker;

static void xs_worker_postfromworker(xsMachine *the);
static void xs_worker_close(xsMachine *the);

static void workerDeliverArrayBuffer(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);
static void workerDeliverJSON(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);

static int workerStart(modWorker worker);

void xs_worker_destructor(void *data)
{
	modWorker worker = data;

	if (worker) {
#if ESP32
		if (worker->task) {
			vTaskDelete(worker->task);
			worker->task = NULL;
		}
#endif
		if (worker->the)
			xsDeleteMachine(worker->the);

		c_free(worker);
	}
}

void xs_worker(xsMachine *the)
{
	modWorker worker;
	char *module = xsmcToString(xsArg(0));

	xsmcVars(1);

	worker = c_calloc(sizeof(modWorkerRecord) + c_strlen(module), 1);
	if (!worker)
		xsUnknownError("no memory");

	worker->parent = the;
	worker->owner = xsThis;
	c_strcpy(worker->module, module);

	xsmcSetHostData(xsThis, worker);

	if (xsmcArgc > 1) {
		if (xsmcHas(xsArg(1), xsID_allocation)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_allocation);
			worker->allocation = xsmcToInteger(xsVar(0));
		}
		if (xsmcHas(xsArg(1), xsID_stackCount)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_stackCount);
			worker->stackCount = xsmcToInteger(xsVar(0));
		}
		if (xsmcHas(xsArg(1), xsID_slotCount)) {
			xsmcGet(xsVar(0), xsArg(1), xsID_slotCount);
			worker->slotCount = xsmcToInteger(xsVar(0));
		}
	}
#if ESP32
	//	xTaskCreatePinnedToCore(workerLoop, "workertask", 4096, worker, 5, &worker->task, xTaskGetAffinity(xTaskGetCurrentTaskHandle()) ? 0 : 1);
	xTaskCreate(workerLoop, "workertask", 4096 , worker, 5, &worker->task);

	modMachineTaskWait(the);
#else
	workerStart(worker);
#endif

	if (NULL == worker->the)
		xsUnknownError("unable to instantiate worker");

	xsRemember(worker->owner);
}

void xs_worker_terminate(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);

	if (NULL == worker->the)
		return;

#if ESP32
	vTaskDelete(worker->task);
	worker->task = NULL;
#endif

	xsDeleteMachine(worker->the);
	worker->the = NULL;

	xsForget(worker->owner);

	c_free(worker);
	xsmcSetHostData(xsThis, NULL);
}

void xs_worker_postfrominstantiator(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);
	char *message;
	uint32_t messageLength;
	uint8_t kind;

	if (NULL == worker->the)
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
		messageLength = c_strlen(message) + 1;
		kind = 0;
	}

	if (modMessagePostToMachine(worker->the, message, messageLength, (modMessageDeliver)(kind ? workerDeliverArrayBuffer : workerDeliverJSON), worker))
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
		messageLength = c_strlen(message) + 1;
		kind = 0;
	}

	if (modMessagePostToMachine(worker->parent, message, messageLength, (modMessageDeliver)(kind ? workerDeliverArrayBuffer : workerDeliverJSON), worker))
		xsUnknownError("post from worker failed");
}

void xs_worker_close(xsMachine *the)
{
	xsUnknownError("worker close umimplemented");		//@@
}

void workerDeliverArrayBuffer(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(the);

	xsmcVars(0);

	xsVar(0) = xsArrayBuffer(message, messageLength);

	if (the == worker->parent)
		xsCall1(worker->owner, xsID_onmessage, xsVar(0));	// calling instantiator - through instance
	else {
		xsmcGet(xsVar(0), xsGlobal, xsID_self);
		xsCall1(xsVar(0), xsID_onmessage, xsVar(0));	// calling worker - through self
	}

	xsEndHost(the);
}

void workerDeliverJSON(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(the);

	xsmcVars(3);

	xsVar(0) = xsString(message);
	xsmcGet(xsVar(1), xsGlobal, xsID_JSON);
	xsVar(2) = xsCall1(xsVar(1), xsID_parse, xsVar(0));

	if (the == worker->parent)
		xsCall1(worker->owner, xsID_onmessage, xsVar(2));	// calling instantiator - through instance
	else {
		xsmcGet(xsVar(0), xsGlobal, xsID_self);
		xsCall1(xsVar(0), xsID_onmessage, xsVar(2));		// calling worker - through self
	}

	xsEndHost(the);
}

int workerStart(modWorker worker)
{
	xsMachine *the;

	the = ESP_cloneMachine(worker->allocation, worker->stackCount, worker->slotCount, worker->module);
	if (!the)
		return -1;

	the->context = worker;

	xsBeginHost(the);

	xsmcVars(2);

	xsVar(0) = xsmcNewObject();
	xsmcSet(xsGlobal, xsID_self, xsVar(0));

	xsVar(1) = xsNewHostFunction(xs_worker_postfromworker, 1);
	xsmcSet(xsVar(0), xsID_postMessage, xsVar(1));

	xsVar(1) = xsNewHostFunction(xs_worker_close, 0);
	xsmcSet(xsVar(0), xsID_close, xsVar(1));

	xsTry {
		xsmcGet(xsVar(0), xsGlobal, xsID_require);
		xsVar(0) = xsCall1(xsVar(0), xsID_weak, xsString(worker->module));
		if (xsmcTest(xsVar(0)) && xsmcIsInstanceOf(xsVar(0), xsFunctionPrototype)) {
			xsCallFunction0(xsVar(0), xsGlobal);
		}
	}
	xsCatch {
		return -1;
	}
	xsEndHost(the);

	worker->the = the;

	return 0;
}

#if ESP32

void workerLoop(void *pvParameter)
{
	modWorker worker = (modWorker)pvParameter;

	if (workerStart(worker)) {
		modMachineTaskWake(worker->parent);
		return;
	}
	modMachineTaskWake(worker->parent);

	while (true) {
		modTimersExecute();
		modMessageService(worker->the, modTimersNext());
	}
}
#endif

