/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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
	#include "freertos/semphr.h"
	//#include <esp_log.h>
	//static const char *TAG = "modWorker";
	static void workerLoop(void *pvParameter);
#elif qca4020 || nrf52
	#include "FreeRTOS.h"
	#include "task.h"
	#include "semphr.h"
	static void workerLoop(void *pvParameter);
#elif __ZEPHYR__
	#include <zephyr/kernel.h>
	#include <zephyr/drivers/gpio.h>

	static void workerLoop(void *a, void *b, void *c);
#endif

struct modWorkerRecord {
	struct modWorkerRecord	*next;

	xsMachine				*the;
	xsMachine				*parent;
	xsSlot					owner;
	xsSlot					ownerPort;
	xsSlot					workerPort;
	xsCreation				creation;
	xsBooleanValue			closing;
	xsBooleanValue			shared;
#ifdef INC_FREERTOS_H
	TaskHandle_t			task;
#elif __ZEPHYR__
	struct k_thread		thread;
	k_thread_stack_t		*stack;
	k_tid_t					threadID;
	struct k_sem			semaphore;
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

static void doModMessagePostToMachine(xsMachine *the, xsMachine *targetThe, uint8_t *message, modMessageDeliver callback, void *refcon);

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
#elif __ZEPHYR__
			if (worker->threadID)
				k_thread_abort(worker->threadID);
			if (worker->stack)
				k_thread_stack_free(worker->stack);
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

static void getIntegerProperty(xsMachine *the, xsSlot *slot, xsIdentifier id, xsIntegerValue *value)
{
	if (!xsmcHas(*slot, id))
		return;
	
	xsSlot tmp;
	xsmcGet(tmp, *slot, id);
	*value = xsmcToInteger(tmp);
}

static void workerConstructor(xsMachine *the, xsBooleanValue shared)
{
	modCriticalSectionDeclare;
	modWorker worker;
	char *module = xsmcToString(xsArg(0));
#ifdef INC_FREERTOS_H
	xsIntegerValue priority = 0, core = -1;   // core tskNO_AFFINITY
#endif

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
#elif __ZEPHYR__
			worker->thread = target->thread;		//@@
			worker->stack = target->stack;
			worker->threadID = target->threadID;
#endif
			doModMessagePostToMachine(the, worker->the, NULL, (modMessageDeliver)workerDeliverConnect, worker);
			goto done;
		}
	}

	xsCreation *creationP;
	(void *)xsPreparationAndCreation(&creationP);
	worker->creation = *creationP;
	if (xsmcArgc > 1) {
		if (xsmcHas(xsArg(1), xsID_allocation) ||
			 xsmcHas(xsArg(1), xsID_stackCount) ||
			 xsmcHas(xsArg(1), xsID_slotCount) ||
		 	 xsmcHas(xsArg(1), xsID_keyCount)) {
#ifdef mxDebug
			xsTrace("deprecated worker creation parameters detected. update recommended.\n");
#endif
			xsIntegerValue allocation, stackCount, slotCount, keyCount;
			xsmcGet(xsVar(0), xsArg(1), xsID_allocation);
			allocation = xsmcToInteger(xsVar(0));

			xsmcGet(xsVar(0), xsArg(1), xsID_stackCount);
			stackCount = xsmcToInteger(xsVar(0));

			xsmcGet(xsVar(0), xsArg(1), xsID_slotCount);
			slotCount = xsmcToInteger(xsVar(0));

			xsmcGet(xsVar(0), xsArg(1), xsID_keyCount);
			keyCount = xsmcToInteger(xsVar(0));

			if (stackCount)
				worker->creation.stackCount = stackCount;

			if (slotCount)
				worker->creation.initialHeapCount = slotCount;
			
			if (keyCount)
				worker->creation.initialKeyCount = keyCount;

			if (allocation) {
				worker->creation.staticSize = allocation;
				int available = worker->creation.staticSize - (worker->creation.stackCount * sizeof(xsSlot)) - 1024; 
				if (worker->creation.initialChunkSize > (available / 2)) {
					worker->creation.initialChunkSize = 512;
					worker->creation.incrementalChunkSize = 512;
				}
				if (worker->creation.initialHeapCount > (available / (2 * sizeof(xsSlot))))
					worker->creation.initialHeapCount = available / (2 * sizeof(xsSlot));
				if (worker->creation.incrementalHeapCount > (worker->creation.initialHeapCount / 8) || !worker->creation.incrementalHeapCount)
					worker->creation.incrementalHeapCount = 8;
			}
		}
		else {
			getIntegerProperty(the, &xsArg(1), xsID_static, &worker->creation.staticSize);
			xsmcGet(xsVar(0), xsArg(1), xsID_chunk);
			if (xsmcTest(xsVar(0))) {
				getIntegerProperty(the, &xsVar(0), xsID_initial, &worker->creation.initialChunkSize);
				getIntegerProperty(the, &xsVar(0), xsID_incremental, &worker->creation.incrementalChunkSize);
			}
			xsmcGet(xsVar(0), xsArg(1), xsID_heap);
			if (xsmcTest(xsVar(0))) {
				getIntegerProperty(the, &xsVar(0), xsID_initial, &worker->creation.initialHeapCount);
				getIntegerProperty(the, &xsVar(0), xsID_incremental, &worker->creation.incrementalHeapCount);
			}
			getIntegerProperty(the, &xsArg(1), xsID_stack, &worker->creation.stackCount);
			xsmcGet(xsVar(0), xsArg(1), xsID_keys);
			if (xsmcTest(xsVar(0))) {
				getIntegerProperty(the, &xsVar(0), xsID_initial, &worker->creation.initialKeyCount);
				getIntegerProperty(the, &xsVar(0), xsID_incremental, &worker->creation.incrementalKeyCount);
				getIntegerProperty(the, &xsVar(0), xsID_name, &worker->creation.nameModulo);
				getIntegerProperty(the, &xsVar(0), xsID_symbol, &worker->creation.symbolModulo);
			}
			getIntegerProperty(the, &xsArg(1), xsID_nativeStack, &worker->creation.nativeStackSize);
#ifdef INC_FREERTOS_H
#if ESP32	// Multicore only supported on ESP32
			getIntegerProperty(the, &xsArg(1), xsID_core, &core);
#endif
#define kWorkerTaskPriority		(tskIDLE_PRIORITY + 1) // Default priority is tskIDLE_PRIORITY + 1
			getIntegerProperty(the, &xsArg(1), xsID_priority, &priority);
			if (priority == 0)
				priority = kWorkerTaskPriority;
#endif
		}
	}

#ifdef INC_FREERTOS_H
#if ESP32
	#if 0 == CONFIG_LOG_DEFAULT_LEVEL
		#define kStack ((5 * 1024) + XT_STACK_EXTRA_CLIB)
	#else
		#define kStack ((6 * 1024) + XT_STACK_EXTRA_CLIB)
	#endif
#elif qca4020
	#define kStack ((9 * 1024)
#elif nrf52
	#define kStack (10 * 1024)
#endif
	// FreeRTOS Task Creation. If we are specifying a core, use that, otherwise use the FreeRTOS xTaskCreate.
	if (core != -1) {	// Use specified core, only for ESP32 for now
		//ESP_LOGI(TAG, "Creating worker task on core %d with priority %d\n", core, worker->creation.priority);
		xTaskCreatePinnedToCore(workerLoop, worker->module, (worker->creation.nativeStackSize ? worker->creation.nativeStackSize : kStack) / sizeof(StackType_t),
							worker, priority, &worker->task, core);
	}
	else {	// Use default core
		//ESP_LOGI(TAG, "Creating worker task with default core and priority %d\n", worker->creation.priority);
		xTaskCreate(workerLoop, worker->module, (worker->creation.nativeStackSize ? worker->creation.nativeStackSize : kStack) / sizeof(StackType_t),
							worker, priority, &worker->task);
	}

	modMachineTaskWait(the);
#elif __ZEPHYR__
	k_sem_init(&worker->semaphore, 0, 1);

	size_t nativeStackSize = worker->creation.nativeStackSize ? worker->creation.nativeStackSize : 4096;
	worker->stack = k_thread_stack_alloc(nativeStackSize, 0);
    if (C_NULL == worker->stack)
		xsUnknownError("stack alloc failed");

    worker->threadID = k_thread_create(&worker->thread, worker->stack, nativeStackSize,
								workerLoop, worker, C_NULL, C_NULL, 5, 0, K_NO_WAIT);
	k_thread_name_set(worker->threadID, worker->module);
	k_sem_take(&worker->semaphore, K_FOREVER);
#else // !INC_FREERTOS_H && !__ZEPHYR__
	workerStart(worker);
#endif

	if (NULL == worker->the) {
#ifdef INC_FREERTOS_H
		if (worker->task) {
			vTaskDelete(worker->task);
			worker->task = NULL;
		}
#elif __ZEPHYR__
		k_thread_abort(worker->threadID);
		k_thread_stack_free(worker->stack);
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
	doModMessagePostToMachine(the, worker->the, message, (modMessageDeliver)workerDeliverMarshall, worker);
}

void xs_worker_postfromworker(xsMachine *the)
{
	modWorker worker = xsmcGetHostDataValidate(xsThis, (void *)xs_emptyworker_destructor);
	char *message;

	if (worker->closing)
		xsUnknownError("worker closing");

	message = xsMarshall(xsArg(0));
	doModMessagePostToMachine(the, worker->parent, message, (modMessageDeliver)workerDeliverMarshall, worker);
}

void xs_worker_close(xsMachine *the)
{
	modWorker worker = xsmcGetHostDataValidate(xsThis, (void *)xs_emptyworker_destructor);
	doModMessagePostToMachine(the, worker->parent, NULL, (modMessageDeliver)workerTerminate, worker);
}

void workerDeliverMarshall(xsMachine *the, modWorker worker, uint8_t *message, uint16_t messageLength)
{
	if (worker->closing) {
		c_free(*(char **)message);
		return;
	}

	xsBeginHost(the);

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
	xsmcSet(xsVar(2), xsID_ports, xsVar(0));					// e.ports = ports

	xsVar(3) = xsNewHostFunction(xs_worker_postfromworker, 1);	// postMessage
	xsmcSet(xsVar(1), xsID_postMessage, xsVar(3));				// port.postMessage = postMessage
	xsCall1(xsGlobal, xsID_onconnect, xsVar(2));				// onconnect(e)

	//@@ this eats any exception in onconnect... should be propagated?
	xsEndHost(the);
}

int workerStart(modWorker worker)
{
	xsMachine *the;
	int result = 0;

	the = modCloneMachine(&worker->creation, worker->module);
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

void doModMessagePostToMachine(xsMachine *the, xsMachine *targetThe, uint8_t *message, modMessageDeliver callback, void *refcon)
{
	char *error;
	int result;

	if (message) {
		result = modMessagePostToMachine(targetThe, (uint8_t *)&message, sizeof(message), callback, refcon);
		if (!result)
			return;
		c_free(message);
	}
	else {
		result = modMessagePostToMachine(targetThe, NULL, 0, callback, refcon);
		if (!result)
			return;
	}

	if (-1 == result)
		error = "no memory";
	else if (-2 == result)
		error = "timeout";
	else
		error = "unknown";

	xsUnknownError(error);
}

#ifdef INC_FREERTOS_H

void workerLoop(void *pvParameter)
{
	modWorker worker = (modWorker)pvParameter;

#if CONFIG_ESP_TASK_WDT_EN
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
#elif nrf52
		modWatchDogReset();
#endif
	}
}
#elif __ZEPHYR__

void workerLoop(void *a, void *b, void *c)
{
	modWorker worker = (modWorker)a;

	if (workerStart(worker)) {
		k_sem_give(&worker->semaphore);
		while (true)		// wait: caller deletes task
			k_msleep(1000);
	}
	k_sem_give(&worker->semaphore);

	while (true) {
		modTimersExecute();
		modMessageService(worker->the, modTimersNext());
		modInstrumentationAdjust(Turns, +1);
	}
}

#endif

#ifdef mxInstrument
extern void fxSampleInstrumentation(xsMachine * the, xsIntegerValue count, xsIntegerValue* values);

void workerSampleInstrumentation(modTimer timer, void *refcon, int refconSize)
{
	xsMachine *the = *(xsMachine **)refcon;
#ifdef INC_FREERTOS_H
	extern SemaphoreHandle_t gInstrumentMutex;
	xSemaphoreTake(gInstrumentMutex, portMAX_DELAY);
#elif __ZEPHYR__
	extern struct k_mutex gInstrumentMutex;
	k_mutex_lock(&gInstrumentMutex, K_FOREVER);
#endif

	fxSampleInstrumentation(the, 0, NULL);
	modInstrumentMachineReset(the);

#ifdef INC_FREERTOS_H
	xSemaphoreGive(gInstrumentMutex);
#elif __ZEPHYR__
	k_mutex_unlock(&gInstrumentMutex);
#endif
}
#endif
