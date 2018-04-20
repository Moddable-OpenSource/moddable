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

#include "xsesp.h"
#include "mc.xs.h"			// for xsID_ values

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"

	static void workerLoop(void *pvParameter);
#endif

struct modWorkerRecord {
	struct modWorkerRecord	*next;

	xsMachine				*the;
	xsMachine				*parent;
	xsSlot					owner;
	xsSlot					ownerPort;
	xsSlot					workerPort;
	uint32_t				allocation;
	uint32_t				stackCount;
	uint32_t				slotCount;
	xsBooleanValue			closing;
	xsBooleanValue			shared;
#if ESP32
	TaskHandle_t			task;
#endif
	char						module[1];
};

typedef struct modWorkerRecord modWorkerRecord;
typedef modWorkerRecord *modWorker;

static void xs_worker_postfromworker(xsMachine *the);
static void xs_worker_close(xsMachine *the);

#define USE_MARSHALL 1
#if USE_MARSHALL
	static void workerDeliverMarshall(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);
#else
	static void workerDeliverArrayBuffer(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);
	static void workerDeliverJSON(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);
#endif

static void workerDeliverConnect(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);

static int workerStart(modWorker worker);
static void workerTerminate(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);

static modWorker gWorkers;

void xs_worker_destructor(void *data)
{
	modWorker worker = data;

	if (worker) {
		modWorker walker, prev;

#if ESP32
		if (worker->task) {
			vTaskDelete(worker->task);
			worker->task = NULL;
			vTaskDelay(1);	// necessary to allow idle task to run so task memory is freed. perhaps there's a better solution?
		}
#endif
		if (worker->the)
			xsDeleteMachine(worker->the);

		modCriticalSectionBegin();
			for (walker = gWorkers, prev = NULL; NULL != walker; walker = walker->next) {
				if (walker != worker) {
					prev = walker;
					continue;
				}

				if (prev)
					prev->next = walker->next;
				else
					gWorkers = walker->next;
				break;
			}
		modCriticalSectionEnd();

		c_free(worker);
	}
}

static void workerConstructor(xsMachine *the, xsBooleanValue shared)
{
	modWorker worker;
	char *module = xsmcToString(xsArg(0));

	xsmcVars(2);

	worker = c_calloc(sizeof(modWorkerRecord) + c_strlen(module), 1);
	if (!worker)
		xsUnknownError("no memory");

	worker->parent = the;
	worker->owner = xsThis;
	c_strcpy(worker->module, module);
	worker->shared = shared;

	if (!shared)
		worker->ownerPort = xsThis;
	else {
		xsVar(0) = xsNewHostObject(NULL);
		xsmcSetHostData(xsVar(0), worker);
		worker->ownerPort = xsVar(0);
		xsmcSet(xsThis, xsID_port, xsVar(0));

		xsVar(1) = xsNewHostFunction(xs_worker_postfrominstantiator, 1);
		xsmcSet(xsVar(0), xsID_postMessage, xsVar(1));
	}

	xsmcSetHostData(xsThis, worker);

	if (shared) {
		modWorker walker, target = NULL;

		modCriticalSectionBegin();
		for (walker = gWorkers; NULL != walker; walker = walker->next) {
			if (0 != c_strcmp(walker->module, worker->module))
				continue;
			target = walker;
			break;
		}

		modCriticalSectionEnd();

		if (target) {
			worker->the = target->the;
			modMessagePostToMachine(worker->the, NULL, 0, (modMessageDeliver)workerDeliverConnect, worker);
			goto done;
		}
	}

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
	#if 0 == CONFIG_LOG_DEFAULT_LEVEL
		#define kStack ((4 * 1024) / sizeof(StackType_t))
	#else
		#define kStack ((6 * 1024) / sizeof(StackType_t))
	#endif

	//	xTaskCreatePinnedToCore(workerLoop, worker->module, 4096, worker, 5, &worker->task, xTaskGetAffinity(xTaskGetCurrentTaskHandle()) ? 0 : 1);
	xTaskCreate(workerLoop, worker->module, kStack, worker, 8, &worker->task);

	modMachineTaskWait(the);
#else
	workerStart(worker);
#endif

	if (NULL == worker->the)
		xsUnknownError("unable to instantiate worker");

done:
	xsRemember(worker->owner);

	modCriticalSectionBegin();
		worker->next = gWorkers;
		gWorkers = worker;
	modCriticalSectionEnd();
}

void xs_worker(xsMachine *the)
{
	workerConstructor(the, false);
}

void xs_sharedworker(xsMachine *the)
{
	workerConstructor(the, true);
}

void xs_worker_terminate(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);

	if (NULL == worker)
		return;

	workerTerminate(the, worker, NULL, 0);
}

void xs_worker_postfrominstantiator(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);
	char *message;
	uint32_t messageLength;
	uint8_t kind;

	if (NULL == worker->the)
		xsUnknownError("worker terminated");

	if (worker->closing)
		xsUnknownError("worker closing");

#if USE_MARSHALL
	message = xsMarshall(xsArg(0));
	if (modMessagePostToMachine(worker->the, (uint8_t *)&message, sizeof(message), (modMessageDeliver)workerDeliverMarshall, worker))
		xsUnknownError("post from instantiator failed");
#else
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
#endif
}

void xs_worker_postfromworker(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);
	char *message;
	uint32_t messageLength;
	uint8_t kind;

	if (worker->closing)
		xsUnknownError("worker closing");

	xsmcVars(2);

#if USE_MARSHALL
	message = xsMarshall(xsArg(0));
	if (modMessagePostToMachine(worker->parent, (uint8_t *)&message, sizeof(message), (modMessageDeliver)workerDeliverMarshall, worker))
		xsUnknownError("post from worker failed");
#else
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
#endif
}

void xs_worker_close(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);
	modMessagePostToMachine(worker->parent, NULL, 0, (modMessageDeliver)workerTerminate, worker);
}

#if USE_MARSHALL

void workerDeliverMarshall(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	if (worker->closing)
		return;

	xsBeginHost(the);

	xsmcVars(1);

	xsVar(0) = xsDemarshall(*(char **)message);
	c_free(*(char **)message);

	if (the == worker->parent)
		xsCall1(worker->ownerPort, xsID_onmessage, xsVar(0));
	else
		xsCall1(worker->workerPort, xsID_onmessage, xsVar(0));

	xsEndHost(the);
}

#else
void workerDeliverArrayBuffer(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	if (worker->closing)
		return;

	xsBeginHost(the);

	xsmcVars(2);

	xsVar(0) = xsArrayBuffer(message, messageLength);

	if (the == worker->parent)
		xsCall1(worker->ownerPort, xsID_onmessage, xsVar(0));
	else
		xsCall1(worker->workerPort, xsID_onmessage, xsVar(0));

	xsEndHost(the);
}

void workerDeliverJSON(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	if (worker->closing)
		return;

	xsBeginHost(the);

	xsmcVars(3);

	xsVar(0) = xsString(message);
	xsmcGet(xsVar(1), xsGlobal, xsID_JSON);
	xsVar(2) = xsCall1(xsVar(1), xsID_parse, xsVar(0));

	if (the == worker->parent)
		xsCall1(worker->ownerPort, xsID_onmessage, xsVar(2));
	else
		xsCall1(worker->workerPort, xsID_onmessage, xsVar(2));

	xsEndHost(the);
}
#endif

void workerDeliverConnect(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(the);

	xsmcVars(4);

	xsVar(0) = xsmcNewArray(0);									// ports = []
	xsVar(1) = xsNewHostObject(NULL);							// port = {}
	xsmcSetHostData(xsVar(1), worker);
	worker->workerPort = xsVar(1);
	xsRemember(worker->workerPort);
	xsCall1(xsVar(0), xsID_push, xsVar(1));

	xsVar(2) = xsmcNewObject();									// e = {}
	xsmcSet(xsVar(2), xsID_ports, xsVar(0));						// e.ports = ports

	xsVar(3) = xsNewHostFunction(xs_worker_postfromworker, 1);	// postMessage
	xsmcSet(xsVar(1), xsID_postMessage, xsVar(3));				// port.postMessage = postMessage
	xsCall1(xsGlobal, xsID_onconnect, xsVar(2));					// onconnect(e)

	//@@ this eats any exception in onconnect... should be propagated
	xsEndHost(the);
}

int workerStart(modWorker worker)
{
	xsMachine *the;
	int result = 0;

	the = ESP_cloneMachine(worker->allocation, worker->stackCount, worker->slotCount, worker->module);
	if (!the)
		return -1;

	xsBeginHost(the);

	xsmcVars(2);

	xsTry {
		if (!worker->shared) {
			xsVar(0) = xsNewHostObject(NULL);
			xsmcSetHostData(xsVar(0), worker);
			xsmcSet(xsGlobal, xsID_self, xsVar(0));
			worker->workerPort = xsVar(0);
			xsRemember(worker->workerPort);

			xsVar(1) = xsNewHostFunction(xs_worker_postfromworker, 1);
			xsmcSet(xsVar(0), xsID_postMessage, xsVar(1));

			xsVar(1) = xsNewHostFunction(xs_worker_close, 0);
			xsmcSet(xsVar(0), xsID_close, xsVar(1));
		}

		xsmcGet(xsVar(0), xsGlobal, xsID_require);
		xsVar(0) = xsCall1(xsVar(0), xsID_weak, xsString(worker->module));
		if (xsmcTest(xsVar(0)) && xsmcIsInstanceOf(xsVar(0), xsFunctionPrototype))
			xsCallFunction0(xsVar(0), xsGlobal);

		if (worker->shared)
			workerDeliverConnect(the, worker, NULL, 0);
	}
	xsCatch {
		result = -1;
	}
	xsEndHost(the);

	if (result) {
		xsDeleteMachine(the);
		the = NULL;
	}

	worker->the = the;

	return result;
}

void workerTerminate(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	worker->closing = true;

	xsForget(worker->owner);
	xsmcSetHostData(worker->owner, NULL);

	xs_worker_destructor(worker);
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

