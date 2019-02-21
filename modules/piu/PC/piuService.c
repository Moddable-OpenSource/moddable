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

#include "xsAll.h"
#include "piuAll.h"
#if mxLinux
#include <glib.h>
#elif mxWindows
#include <process.h>
#else
#include "pthread.h"
#endif

extern txPreparation* xsPreparation();

typedef struct ServiceThreadStruct ServiceThreadRecord, *ServiceThread;
typedef struct ServiceEventStruct ServiceEventRecord, *ServiceEvent;
typedef struct ServiceMessageStruct ServiceMessageRecord, *ServiceMessage;
typedef struct ServiceProxyStruct ServiceProxyRecord, *ServiceProxy;

typedef void (*ServiceCallback)(ServiceEvent event);

struct ServiceThreadStruct {
	xsMachine* the;
	ServiceEvent firstEvent;
	ServiceEvent lastEvent;
	ServiceProxy firstProxy;
#if mxLinux
	GMainContext* main_context;
	GMutex mutex;
#elif mxWindows
	HANDLE event;
	CRITICAL_SECTION lock;
	HANDLE handle;
#else	
	pthread_mutex_t mutex;
	CFRunLoopRef runLoop;
	CFRunLoopSourceRef runLoopSource;
#endif
	char name[1];
};

struct ServiceEventStruct {
	ServiceEvent nextEvent;
	ServiceCallback callback;
};

struct ServiceMessageStruct {
	ServiceEventRecord event;
	ServiceProxy proxy;
	void* request;
	void* response;
	xsSlot resolve;
	xsSlot reject;
	char key[1];
};

struct ServiceProxyStruct {
	ServiceProxy nextProxy;
	xsSlot behavior;
	ServiceThread client;
	ServiceThread server;
	ServiceProxy reverse;
	ServiceEventRecord creation;
	ServiceEventRecord deletion;
	uint32_t usage;
	char name[1];
};


static void ServiceThreadCreate(xsMachine* the);
static void ServiceThreadInitialize(void* it);
static void ServiceThreadMark(xsMachine* the, void* it, xsMarkRoot markRoot);
#if mxLinux	
static gboolean ServiceThreadPerform(void* it);
#else
static void ServiceThreadPerform(void* it);
#endif
static void ServiceThreadSignal(ServiceThread thread, ServiceEvent event);

static void ServiceEventCreate(ServiceEvent event);
static void ServiceEventDelete(ServiceEvent event);
static void ServiceEventInvoke(ServiceEvent event);
static void ServiceEventReject(ServiceEvent event);
static void ServiceEventResolve(ServiceEvent event);

static void ServiceMessageReject(xsMachine* the, ServiceMessage message);
static void ServiceMessageResolve(xsMachine* the, ServiceMessage message);
static void ServiceMessageThenReject(xsMachine* the);
static void ServiceMessageThenResolve(xsMachine* the);

static void ServiceProxyInvoke(xsMachine* the);

static xsHostHooks ServiceThreadHooks = {
	NULL,
	ServiceThreadMark,
	NULL
};

static txMachine ServiceRoot;
static ServiceThreadRecord MainThread;

#if mxLinux
static gpointer ServiceThreadLoop(gpointer it)
{
	ServiceThread thread = it;
	GMainContext* main_context = g_main_context_new();
	GMainLoop* main_loop;
	g_main_context_push_thread_default(main_context);
	ServiceThreadInitialize(thread);
	main_loop = g_main_loop_new(main_context, FALSE);
	g_mutex_unlock(&(thread->mutex));
	g_main_loop_run(main_loop);
	g_main_loop_unref(main_loop);
	g_main_context_pop_thread_default(main_context);
	g_main_context_unref(main_context);
	return NULL;
}
#elif mxWindows
static unsigned int __stdcall ServiceThreadLoop(void* it)
{
	ServiceThread thread = it;
	MSG msg;
	ServiceThreadInitialize(thread);
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    return 0;
}
#else
static void ServiceThreadSchedule(void *it, CFRunLoopRef runloop, CFStringRef mode)
{
	ServiceThread thread = it;
    pthread_mutex_unlock(&(thread->mutex));
}
static void* ServiceThreadLoop(void* it)
{
	ServiceThread thread = it;
	ServiceThreadInitialize(thread);
	CFRunLoopRun();
    return NULL;
}
#endif

void ServiceThreadCreate(xsMachine* the)
{
	xsStringValue name = xsToString(xsArg(0));
	xsIntegerValue length = c_strlen(name);
	ServiceThread thread = c_malloc(sizeof(ServiceThreadRecord) + length);
	//fprintf(stderr, "ServiceThreadCreate thread %p\n", thread);
	if (thread == NULL)
		xsUnknownError("not enough memory");
	c_memset(thread, 0, sizeof(ServiceThreadRecord));
	c_memcpy(&(thread->name[0]), name, length + 1);
	xsResult = xsNewHostObject(NULL);
	xsSetHostData(xsResult, thread);
#if mxLinux
	g_mutex_init(&(thread->mutex));
	g_mutex_lock(&(thread->mutex));
	g_thread_new(name, ServiceThreadLoop, thread);
#elif mxWindows
	thread->event = CreateEvent(0, FALSE, FALSE, NULL);
	thread->handle = (HANDLE)_beginthreadex(NULL, 0, ServiceThreadLoop, thread, 0, NULL);
	WaitForSingleObject(thread->event, INFINITE);
#else
	pthread_attr_t	attr;
	pthread_t pthread;
    pthread_mutex_init(&(thread->mutex), NULL);
    pthread_mutex_lock(&(thread->mutex));
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pthread, &attr, &ServiceThreadLoop, thread);
    pthread_attr_destroy(&attr);
#endif
}


void ServiceThreadInitialize(void* it)
{
	ServiceThread thread = it;
	//fprintf(stderr, "ServiceThreadInitialize thread %p\n", thread);
	txMachine* root = &ServiceRoot;
	txPreparation* preparation = xsPreparation();
	thread->the = fxCloneMachine(&preparation->creation, root, thread->name[0] ? thread->name : "main", NULL);
	xsBeginHost(thread->the);
	{
		xsVars(3);
		xsVar(0) = xsNewHostObject(NULL);
		xsVar(1) = xsNewHostConstructor(ServiceThreadCreate, 1, xsVar(0));
		xsVar(2) = xsNewHostObject(NULL);
		xsSetHostData(xsVar(2), thread);
		xsSetHostHooks(xsVar(2), &ServiceThreadHooks);
		xsSet(xsVar(1), xsID_current, xsVar(2));
		xsVar(2) = xsNewHostObject(NULL);
		xsSetHostData(xsVar(2), &MainThread);
		xsSet(xsVar(1), xsID_main, xsVar(2));
		xsSet(xsGlobal, xsID_Thread, xsVar(1));
	}
	xsEndHost(thread->the);
#if mxLinux
	thread->main_context = g_main_context_get_thread_default();
#elif mxWindows
	thread->the->thread = thread;
	thread->the->threadCallback = ServiceThreadPerform;
	InitializeCriticalSection(&thread->lock);
	if (thread->event)
		SetEvent(thread->event);
#else
    CFRunLoopSourceContext runLoopSourceContext = {0, thread, NULL, NULL, NULL, NULL, NULL, ServiceThreadSchedule, NULL, ServiceThreadPerform};
	thread->runLoop = CFRunLoopGetCurrent();
    thread->runLoopSource = CFRunLoopSourceCreate(NULL, 0, &runLoopSourceContext);
	CFRunLoopAddSource(thread->runLoop, thread->runLoopSource, kCFRunLoopDefaultMode);
#endif
}

txMachine* ServiceThreadMain()
{
	txMachine* root = &ServiceRoot;
	ServiceThread thread = &MainThread;
	txPreparation* preparation = xsPreparation();
	
	//fprintf(stderr, "ServiceThreadMain thread %p\n", thread);
	c_memset(root, 0, sizeof(txMachine));
	root->archive = preparation;
	root->keyArray = preparation->keys;
	root->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.keyCount;
	root->keyIndex = (txID)preparation->keyCount;
	root->nameModulo = preparation->nameModulo;
	root->nameTable = preparation->names;
	root->symbolModulo = preparation->symbolModulo;
	root->symbolTable = preparation->symbols;
	
	root->stack = &preparation->stack[0];
	root->stackBottom = &preparation->stack[0];
	root->stackTop = &preparation->stack[preparation->stackCount];
	
	root->firstHeap = &preparation->heap[0];
	root->freeHeap = &preparation->heap[preparation->heapCount - 1];
	root->aliasCount = (txID)preparation->aliasCount;
	
	c_memset(thread, 0, sizeof(ServiceThreadRecord));
	ServiceThreadInitialize(thread);
	
	return thread->the;
}

void ServiceThreadMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	ServiceThread thread = it;
	ServiceProxy proxy = thread->firstProxy;
	while (proxy) {
		(*markRoot)(the, &proxy->behavior);
		proxy = proxy->nextProxy;
	}
}

#if mxLinux	
gboolean ServiceThreadPerform(void* it)
#else
void ServiceThreadPerform(void* it)
#endif
{
	ServiceThread thread = it;
	ServiceEvent event;
	//fprintf(stderr, "ServiceThreadPerform thread %p\n", thread);
#if mxLinux	
	g_mutex_lock(&(thread->mutex));
#elif mxWindows
	EnterCriticalSection(&(thread->lock));
#else
    pthread_mutex_lock(&(thread->mutex));
#endif
	event = thread->firstEvent;
	thread->firstEvent = NULL;
	thread->lastEvent = NULL;
#if mxLinux
	g_mutex_unlock(&(thread->mutex));
#elif mxWindows
	LeaveCriticalSection(&(thread->lock));
#else
    pthread_mutex_unlock(&(thread->mutex));
#endif
	while (event) {
		ServiceEvent nextEvent = event->nextEvent;
		ServiceCallback callback = event->callback;
		event->nextEvent = NULL;
		event->callback = NULL;
		(*callback)(event);
		event = nextEvent;
	}	
#if mxLinux
	return G_SOURCE_REMOVE;
#endif
}

void ServiceThreadSignal(ServiceThread thread, ServiceEvent event)
{
	//fprintf(stderr, "ServiceThreadSignal thread %p event %p\n", thread, event);
#if mxLinux
	g_mutex_lock(&(thread->mutex));
#elif mxWindows
	EnterCriticalSection(&(thread->lock));
#else
    pthread_mutex_lock(&(thread->mutex));
#endif
	if (thread->lastEvent)
		thread->lastEvent->nextEvent = event;
	else
		thread->firstEvent = event;
	thread->lastEvent = event;
#if mxLinux
	g_mutex_unlock(&(thread->mutex));
// 	g_main_context_invoke(thread->main_context, ServiceThreadPerform, thread);
	GSource* idle_source = g_idle_source_new();
	g_source_set_callback(idle_source, ServiceThreadPerform, thread, NULL);
	g_source_set_priority(idle_source, G_PRIORITY_DEFAULT);
	g_source_attach(idle_source, thread->main_context);
	g_source_unref(idle_source);
#elif mxWindows
	LeaveCriticalSection(&(thread->lock));
	PostMessage(thread->the->window, WM_SERVICE, 0, 0);
#else
    pthread_mutex_unlock(&(thread->mutex));
    CFRunLoopSourceSignal(thread->runLoopSource);
    CFRunLoopWakeUp(thread->runLoop);
#endif
}

void ServiceEventCreate(ServiceEvent event)
{
	ServiceProxy proxy = (ServiceProxy)(((uint8_t*)event) - offsetof(ServiceProxyRecord, creation));
	ServiceThread thread = proxy->server;
	//fprintf(stderr, "ServiceEventCreate thread %p proxy %p\n", thread, proxy);
	xsBeginHost(thread->the);
	{
		xsVars(4);
		xsTry {
			xsVar(0) = xsGet(xsGlobal, xsID_require);
			xsVar(0) = xsCall1(xsVar(0), xsID_weak, xsString(proxy->name));
			if (proxy->reverse) {
				xsVar(1) = xsNewHostFunction(ServiceProxyInvoke, 4);
				xsVar(2) = xsGet(xsGlobal, xsID_Service);
				xsVar(2) = xsGet(xsVar(2), xsID_prototype);
				xsVar(3) = xsGet(xsGlobal, xsID_Object);
				xsVar(3) = xsCall1(xsVar(3), xsID_create, xsVar(2));
				proxy->reverse->usage++;
				xsSetHostData(xsVar(3), proxy->reverse);
				proxy->reverse = NULL;
				xsVar(2) = xsNew2(xsGlobal, xsID_Proxy, xsVar(1), xsVar(3));
				fxPush(xsVar(2));
				fxPushCount(the, 1);
			}
			else
				fxPushCount(the, 0);
			fxPush(xsVar(0));
			fxNew(the);
			proxy->behavior = fxPop();
		}
		xsCatch {
		}
		proxy->nextProxy = thread->firstProxy;
		thread->firstProxy = proxy;
	}
	xsEndHost(thread->the);
}

void ServiceEventDelete(ServiceEvent event)
{
	ServiceProxy proxy = (ServiceProxy)(((uint8_t*)event) - offsetof(ServiceProxyRecord, deletion));
	ServiceThread thread = proxy->server;
	
	//fprintf(stderr, "ServiceEventDelete thread %p proxy %p\n", thread, proxy);
	ServiceProxy* address = &(thread->firstProxy);
	while (*address) {
		if (*address == proxy) {
			*address = proxy->nextProxy;
			c_free(proxy);
			break;
		}
		address = &((*address)->nextProxy);
	}
	fxCollectGarbage(thread->the);
}

void ServiceEventInvoke(ServiceEvent event)
{
	ServiceMessage message = (ServiceMessage)event;
	ServiceProxy proxy = message->proxy;
	ServiceThread thread = proxy->server;
	//fprintf(stderr, "ServiceEventInvoke thread %p proxy %p\n", thread, proxy);
	xsBeginHost(thread->the);
	{
		xsVars(7);
		xsTry {
			xsVar(0) = xsAccess(proxy->behavior);
			xsVar(1) = xsGetAt(xsVar(0), xsString(message->key));
			if (xsTest(xsVar(1))) {
				xsVar(2) = xsDemarshall(message->request);
				xsResult = xsCall2(xsVar(1), xsID_apply, xsVar(0), xsVar(2));
				if (xsIsInstanceOf(xsResult, xsPromisePrototype)) {
					xsVar(3) = xsGet(xsGlobal, xsID_Promise);
					xsVar(3) = xsCall1(xsVar(3), xsID_resolve, xsResult);
					xsVar(4) = xsNewHostFunction(ServiceMessageThenResolve, 1);
					xsVar(5) = xsNewHostFunction(ServiceMessageThenReject, 1);
					xsVar(6) = xsNewHostObject(NULL);
					xsSetHostData(xsVar(6), message);
					xsSet(xsVar(4), xsID_message, xsVar(6));
					xsSet(xsVar(5), xsID_message, xsVar(6));
					xsCall2(xsVar(3), xsID_then, xsVar(4), xsVar(5));
				}
				else {
					ServiceMessageResolve(the, message);
				}
			}
			else {
				xsReferenceError("method %s not found", message->key);
			}
		}
		xsCatch {
			xsResult = xsException;
			ServiceMessageReject(the, message);
		}
	}
	xsEndHost(thread->the);
}

void ServiceEventReject(ServiceEvent event)
{
	ServiceMessage message = (ServiceMessage)event;
	ServiceProxy proxy = message->proxy;
	ServiceThread thread = proxy->client;
	//fprintf(stderr, "ServiceEventReject thread %p proxy %p\n", thread, proxy);
	xsBeginHost(thread->the);
	{
		xsVars(2);
		xsVar(0) = xsAccess(message->reject);
		if (message->response) {
			xsTry {
				xsVar(1) = xsDemarshall(message->response);
			}
			xsCatch {
				xsVar(0) = xsAccess(message->reject);
			}
			c_free(message->response);
		}
		xsForget(message->reject);
		xsForget(message->resolve);
		c_free(message);
		ServiceProxyDelete(proxy);
		(void)xsCallFunction1(xsVar(0), xsUndefined, xsVar(1));
	}
	xsEndHost(thread->the);
}

void ServiceEventResolve(ServiceEvent event)
{
	ServiceMessage message = (ServiceMessage)event;
	ServiceProxy proxy = message->proxy;
	ServiceThread thread = proxy->client;
	//fprintf(stderr, "ServiceEventResolve thread %p proxy %p\n", thread, proxy);
	xsBeginHost(thread->the);
	{
		xsVars(2);
		xsVar(0) = xsAccess(message->resolve);
		if (message->response) {
			xsTry {
				xsVar(1) = xsDemarshall(message->response);
			}
			xsCatch {
				xsVar(0) = xsAccess(message->reject);
			}
			c_free(message->response);
		}
		xsForget(message->reject);
		xsForget(message->resolve);
		c_free(message);
		ServiceProxyDelete(proxy);
		(void)xsCallFunction1(xsVar(0), xsUndefined, xsVar(1));
	}
	xsEndHost(thread->the);
}

void ServiceMessageReject(xsMachine* the, ServiceMessage message)
{
	ServiceCallback callback = ServiceEventReject;
	xsTry {
		message->response = xsMarshall(xsResult);
	}
	xsCatch {
		callback = ServiceEventReject;
	}
	message->event.callback = callback;
	ServiceThreadSignal(message->proxy->client, &(message->event));
}

void ServiceMessageResolve(xsMachine* the, ServiceMessage message)
{
	ServiceCallback callback = ServiceEventResolve;
	xsTry {
		message->response = xsMarshall(xsResult);
	}
	xsCatch {
		callback = ServiceEventReject;
	}
	message->event.callback = callback;
	ServiceThreadSignal(message->proxy->client, &(message->event));
}

void ServiceMessageThenReject(xsMachine* the)
{
	ServiceMessage message;
	xsResult = xsGet(xsFunction, xsID_message);
	message = xsGetHostData(xsResult);
	xsResult = xsArg(0);
	ServiceMessageReject(the, message);
}

void ServiceMessageThenResolve(xsMachine* the)
{
	ServiceMessage message;
	xsResult = xsGet(xsFunction, xsID_message);
	message = xsGetHostData(xsResult);
	xsResult = xsArg(0);
	ServiceMessageResolve(the, message);
}

void ServiceProxyCreate(xsMachine* the)
{
	ServiceProxy proxy = NULL;
	xsVars(1);
	xsTry {
		xsStringValue name = xsToString(xsArg(1));
		xsIntegerValue length = c_strlen(name);
		
		proxy = c_malloc(sizeof(ServiceProxyRecord) + length);
		if (proxy == NULL)
			xsUnknownError("not enough memory");
		c_memset(proxy, 0, sizeof(ServiceProxyRecord));
		c_memcpy(&(proxy->name[0]), name, length + 1);

		xsResult = xsGet(xsGlobal, xsID_Thread);
		xsResult = xsGet(xsResult, xsID_current);
		proxy->client = xsGetHostData(xsResult);
		proxy->server = xsGetHostData(xsArg(0));
		
		if ((xsToInteger(xsArgc) > 2) && xsTest(xsArg(2))) {
			ServiceProxy reverse = c_malloc(sizeof(ServiceProxyRecord));
			if (reverse == NULL)
				xsUnknownError("not enough memory");
			c_memset(reverse, 0, sizeof(ServiceProxyRecord));
			reverse->behavior = xsArg(2);
			reverse->client = proxy->server;
			reverse->server = proxy->client;
			proxy->reverse = reverse;
			
			reverse->nextProxy = proxy->client->firstProxy;
			proxy->client->firstProxy = reverse;
		}
	
		proxy->usage++;
		xsSetHostData(xsThis, proxy);
		
		xsVar(0) = xsNewHostFunction(ServiceProxyInvoke, 4);
		xsResult = xsNew2(xsGlobal, xsID_Proxy, xsVar(0), xsThis);
		proxy->creation.callback = ServiceEventCreate;
		ServiceThreadSignal(proxy->server, &(proxy->creation));
	}
	xsCatch {
		if (proxy) {
			c_free(proxy);
		}
		xsThrow(xsException);
	}
}

void ServiceProxyDelete(void* data)
{
	ServiceProxy proxy = data;
	proxy->usage--;
	if (proxy->usage == 0) {
		proxy->deletion.callback = ServiceEventDelete;
		ServiceThreadSignal(proxy->server, &(proxy->deletion));
	}
}

void ServiceProxyInvoke(xsMachine* the) 
{
	ServiceProxy proxy = xsGetHostData(xsThis);
	ServiceMessage message = NULL;
	xsTry {
		xsStringValue key = xsToString(xsArg(0));
		xsIntegerValue length = c_strlen(key);
		message = c_malloc(sizeof(ServiceMessageRecord) + length);
		if (message == NULL)
			xsUnknownError("not enough memory");
		c_memset(message, 0, sizeof(ServiceMessageRecord));
		c_memcpy(&(message->key[0]), key, length + 1);

		message->proxy = proxy;
		message->request = xsMarshall(xsArg(1));
		message->resolve = xsArg(2);
		message->reject = xsArg(3);
		xsRemember(message->resolve);
		xsRemember(message->reject);
		proxy->usage++;
		
		message->event.callback = ServiceEventInvoke;
		ServiceThreadSignal(proxy->server, &(message->event));
	}
	xsCatch {
		if (message) {
			if (message->request)
				c_free(message->request);
			c_free(message);
		}
		xsThrow(xsException);
	}
}


