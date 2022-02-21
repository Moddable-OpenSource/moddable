/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
#include "xsHosts.h"

#include "mc.xs.h"			// for xsID_ values

#ifdef mxInstrument
	#include "modInstrumentation.h"

	static void workerSampleInstrumentation(modTimer timer, void *refcon, int refconSize);
#endif

#if ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"

	static void workerLoop(void *pvParameter);
#elif qca4020
	#include "FreeRTOS.h"
	#include "task.h"
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
	uint32_t				keyCount;
	xsBooleanValue			closing;
	xsBooleanValue			shared;
#ifdef INC_FREERTOS_H
	TaskHandle_t			task;
#endif
	char					module[1];
};

typedef struct modWorkerRecord modWorkerRecord;
typedef modWorkerRecord *modWorker;

static void xs_worker_postfromworker(xsMachine *the);
static void xs_worker_close(xsMachine *the);

static void xs_worker_postfromsharedinstantiator(xsMachine *the);
static void xs_worker_postfrominstantiator(xsMachine *the);

static void workerDeliverMarshall(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);

static void workerDeliverConnect(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);

static int workerStart(modWorker worker);
static void workerTerminate(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength);

static modWorker gWorkers;

static void xs_emptyworker_destructor(void *data) {
}

void xs_worker_destructor(void *data)
{
	modCriticalSectionDeclare;
	modWorker worker = data;

	if (worker) {
		modWorker walker, prev;
		int useCount = 1;

		if (worker->shared) {
			useCount = 0;
			modCriticalSectionBegin();
				for (walker = gWorkers; NULL != walker; walker = walker->next) {
					if (walker->the == worker->the)
						useCount += 1;
				}
			modCriticalSectionEnd();
		}
		
		if (1 == useCount) {
#ifdef INC_FREERTOS_H
			if (worker->task) {
				vTaskDelete(worker->task);
				worker->task = NULL;
				vTaskDelay(1);	// necessary to allow idle task to run so task memory is freed. perhaps there's a better solution?
			}
#endif
			if (worker->the)
				xsDeleteMachine(worker->the);
		}

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
	modCriticalSectionDeclare;
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
		xsVar(0) = xsNewHostObject(xs_emptyworker_destructor);
		xsmcSetHostData(xsVar(0), worker);
		worker->ownerPort = xsVar(0);
		xsmcSet(xsThis, xsID_port, xsVar(0));

		xsVar(1) = xsNewHostFunction(xs_worker_postfromsharedinstantiator, 1);
		xsmcSet(xsVar(0), xsID_postMessage, xsVar(1));
	}

	xsmcSetHostData(xsThis, worker);

	if (shared) {
		modWorker walker, target = NULL;

		modCriticalSectionBegin();
		for (walker = gWorkers; NULL != walker; walker = walker->next) {
			if (!walker->shared || (0 != c_strcmp(walker->module, worker->module)))
				continue;
			target = walker;
			break;
		}

		modCriticalSectionEnd();

		if (target) {
			worker->the = target->the;
#ifdef INC_FREERTOS_H
			worker->task = target->task;
#endif
			modMessagePostToMachine(worker->the, NULL, 0, (modMessageDeliver)workerDeliverConnect, worker);
			goto done;
		}
	}

	if (xsmcArgc > 1) {
		xsmcGet(xsVar(0), xsArg(1), xsID_allocation);
		worker->allocation = xsmcToInteger(xsVar(0));

		xsmcGet(xsVar(0), xsArg(1), xsID_stackCount);
		worker->stackCount = xsmcToInteger(xsVar(0));

		xsmcGet(xsVar(0), xsArg(1), xsID_slotCount);
		worker->slotCount = xsmcToInteger(xsVar(0));

		xsmcGet(xsVar(0), xsArg(1), xsID_keyCount);
		worker->keyCount = xsmcToInteger(xsVar(0));
	}

#ifdef INC_FREERTOS_H
#if ESP32
	#if 0 == CONFIG_LOG_DEFAULT_LEVEL
		#define kStack (((5 * 1024) + XT_STACK_EXTRA_CLIB) / sizeof(StackType_t))
	#else
		#define kStack (((6 * 1024) + XT_STACK_EXTRA_CLIB) / sizeof(StackType_t))
	#endif

	xTaskCreate(workerLoop, worker->module, kStack, worker, 8, &worker->task);
#elif qca4020
	#if 0 == CONFIG_LOG_DEFAULT_LEVEL
		#define kStack ((9 * 1024) / sizeof(StackType_t))
	#else
		#define kStack ((10 * 1024) / sizeof(StackType_t))
	#endif

	xTaskCreate(workerLoop, worker->module, kStack, worker, 10, &worker->task);
#endif

	modMachineTaskWait(the);

#else // !INC_FREERTOS_H
	workerStart(worker);
#endif

	if (NULL == worker->the) {
#ifdef INC_FREERTOS_H
		if (worker->task) {
			vTaskDelete(worker->task);
			worker->task = NULL;
		}
#endif
		xsUnknownError("unable to instantiate worker");
	}

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
	if (NULL == xsmcGetHostData(xsThis))
		return;

	workerTerminate(the, xsmcGetHostDataValidate(xsThis, (void *)xs_worker_destructor), NULL, 0);
}

void xs_worker_postfromsharedinstantiator(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)xs_emptyworker_destructor);
	xs_worker_postfrominstantiator(the);
}

void xs_worker_postfromworkerinstantiator(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)xs_worker_destructor);
	xs_worker_postfrominstantiator(the);
}

void xs_worker_postfrominstantiator(xsMachine *the)
{
	modWorker worker = xsmcGetHostData(xsThis);		// caller validates
	char *message;

	if (NULL == worker->the)
		xsUnknownError("worker terminated");

	if (worker->closing)
		xsUnknownError("worker closing");

	message = xsMarshall(xsArg(0));
	if (modMessagePostToMachine(worker->the, (uint8_t *)&message, sizeof(message), (modMessageDeliver)workerDeliverMarshall, worker))
		xsUnknownError("post from instantiator failed");
}

void xs_worker_postfromworker(xsMachine *the)
{
	modWorker worker = xsmcGetHostDataValidate(xsThis, (void *)xs_emptyworker_destructor);
	char *message;

	if (worker->closing)
		xsUnknownError("worker closing");

	message = xsMarshall(xsArg(0));
	if (modMessagePostToMachine(worker->parent, (uint8_t *)&message, sizeof(message), (modMessageDeliver)workerDeliverMarshall, worker))
		xsUnknownError("post from worker failed");
}

void xs_worker_close(xsMachine *the)
{
	modWorker worker = xsmcGetHostDataValidate(xsThis, (void *)xs_emptyworker_destructor);
	modMessagePostToMachine(worker->parent, NULL, 0, (modMessageDeliver)workerTerminate, worker);
}

void workerDeliverMarshall(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	if (worker->closing)
		return;

	xsBeginHost(the);

	xsCollectGarbage();
	xsResult = xsDemarshall(*(char **)message);
	c_free(*(char **)message);

	if (xsUndefinedType != xsmcTypeOf(xsResult)) {
		xsSlot *port = (the == worker->parent) ? &worker->ownerPort : &worker->workerPort;
		if (xsmcHas(*port, xsID_onmessage))
			xsCall1(*port, xsID_onmessage, xsResult);
	}

	xsEndHost(the);
}

void workerDeliverConnect(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(the);

	xsmcVars(4);

	xsVar(0) = xsmcNewArray(0);									// ports = []
	xsVar(1) = xsNewHostObject(xs_emptyworker_destructor);		// port = {}
	xsmcSetHostData(xsVar(1), worker);
	worker->workerPort = xsVar(1);
	xsRemember(worker->workerPort);
	xsCall1(xsVar(0), xsID_push, xsVar(1));

	xsVar(2) = xsmcNewObject();									// e = {}
	xsmcSet(xsVar(2), xsID_ports, xsVar(0));						// e.ports = ports

	xsVar(3) = xsNewHostFunction(xs_worker_postfromworker, 1);	// postMessage
	xsmcSet(xsVar(1), xsID_postMessage, xsVar(3));				// port.postMessage = postMessage
	xsCall1(xsGlobal, xsID_onconnect, xsVar(2));					// onconnect(e)

	//@@ this eats any exception in onconnect... should be propagated?
	xsEndHost(the);
}

int workerStart(modWorker worker)
{
	xsMachine *the;
	int result = 0;

	the = modCloneMachine(worker->allocation, worker->stackCount, worker->slotCount, worker->keyCount, worker->module);
	if (!the)
		return -1;

#ifdef mxInstrument
	modInstrumentMachineBegin(the, workerSampleInstrumentation, 0, NULL, NULL);
#endif

	xsBeginHost(the);

	xsmcVars(2);

	xsTry {
		if (!worker->shared) {
			xsVar(0) = xsNewHostObject(xs_emptyworker_destructor);
			xsmcSetHostData(xsVar(0), worker);
			xsmcSet(xsGlobal, xsID_self, xsVar(0));
			worker->workerPort = xsVar(0);
			xsRemember(worker->workerPort);

			xsVar(1) = xsNewHostFunction(xs_worker_postfromworker, 1);
			xsmcSet(xsVar(0), xsID_postMessage, xsVar(1));

			xsVar(1) = xsNewHostFunction(xs_worker_close, 0);
			xsmcSet(xsVar(0), xsID_close, xsVar(1));
		}

		xsVar(0) = xsAwaitImport(worker->module, XS_IMPORT_DEFAULT);
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
	xsmcSetHostDestructor(worker->owner, NULL);

	xs_worker_destructor(worker);
}

#ifdef INC_FREERTOS_H

void workerLoop(void *pvParameter)
{
	modWorker worker = (modWorker)pvParameter;

#if CONFIG_TASK_WDT
	esp_task_wdt_add(NULL);
#endif

	if (workerStart(worker)) {
		modMachineTaskWake(worker->parent);
		while (true)		// wait: caller deletes task
			vTaskDelay(1000);
	}
	modMachineTaskWake(worker->parent);


	while (true) {
		modTimersExecute();
		modMessageService(worker->the, modTimersNext());
#if qca4020
		qca4020_watchdog();
#endif
	}
}
#endif

#ifdef mxInstrument
extern void fxSampleInstrumentation(xsMachine * the, xsIntegerValue count, xsIntegerValue* values);

void workerSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	xsMachine *the = *(xsMachine **)refcon;
	fxSampleInstrumentation(the, 0, NULL);
	modInstrumentMachineReset(the);
}
#endif
