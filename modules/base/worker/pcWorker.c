#include "mc.xs.h"
#include "screen.h"

#ifdef mxInstrument	
extern void fxDescribeInstrumentation(xsMachine* the, xsIntegerValue count, xsStringValue* names, xsStringValue* units);
extern void fxSampleInstrumentation(xsMachine* the, xsIntegerValue count, xsIntegerValue* values);
#endif

typedef struct sxWorker txWorker;
typedef struct sxWorkerEvent txWorkerEvent;
typedef void (*txWorkerCallback)(txWorker* worker, txWorkerEvent* event);

struct sxWorker {
	xsMachine* machine;
	txWorkerEvent* queue;
	mxWorkerPlatform;
#if mxLinux
	GMainLoop* main_loop;
#endif
	xsSlot reference;
	xsMachine* ownerMachine;
	xsSlot ownerReference;
	char name[1];
};

struct sxWorkerEvent {
	txWorkerEvent* next;
	txWorkerCallback callback;
	xsSlot* reference;
	void* argument;
};

static void fxWorkerInitialize(void* it, void* context);
static void fxWorkerMessage(txWorker* worker, txWorkerEvent* event);
#if mxLinux	
static gboolean fxWorkerPerform(void* it);
#else
static void fxWorkerPerform(void* it);
#endif
extern void fxWorkerPlatform(void* it);
static void fxWorkerSignal(txWorker* worker, txWorkerEvent* event);
static void fxWorkerTerminate(txWorker* worker, txWorkerEvent* event);

static void xs_worker_postfromworker(xsMachine *the);

#if mxLinux
static gpointer fxWorkerLoop(gpointer it)
{
	txWorker* worker = it;
	GMainContext* main_context = g_main_context_new();
	g_main_context_push_thread_default(main_context);
	fxWorkerInitialize(worker, NULL);
	worker->main_loop = g_main_loop_new(main_context, FALSE);
	g_mutex_unlock(&(worker->mutex));
	g_main_loop_run(worker->main_loop);
	g_main_loop_unref(worker->main_loop);
	g_main_context_pop_thread_default(main_context);
	g_main_context_unref(main_context);
	return NULL;
}
#elif mxWindows
static unsigned int __stdcall fxWorkerLoop(void* it)
{
	txWorker* worker = it;
	MSG msg;
	fxWorkerInitialize(worker, NULL);
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    return 0;
}
#else
static void WorkerThreadSchedule(void *it, CFRunLoopRef runloop, CFStringRef mode)
{
	txWorker* worker = it;
    pthread_mutex_unlock(&(worker->mutex));
}
static void* fxWorkerLoop(void* it)
{
	txWorker* worker = it;
	fxWorkerInitialize(worker, NULL);
	CFRunLoopRun();
    return NULL;
}
#endif

void fxWorkerInitialize(void* it, void* context)
{
	txWorker* worker = it;
	void* preparation = xsPreparation();
	worker->machine = fxPrepareMachine(NULL, preparation, worker->name, worker, NULL);
	xsBeginHost(worker->machine);
	{
		xsVars(3);
		xsVar(0) = xsNewHostObject(NULL);
		xsSetHostData(xsVar(0), worker);
		xsVar(1) = xsNewHostFunction(xs_worker_postfromworker, 1);
		xsSet(xsVar(0), xsID_postMessage, xsVar(1));
// 		xsVar(1) = xsNewHostFunction(xs_worker_close, 0);
// 		xsSet(xsVar(0), xsID_close, xsVar(1));
		xsSet(xsGlobal, xsID_self, xsVar(0));
		worker->reference = xsVar(0);
		xsRemember(worker->reference);
		
		xsVar(0) = xsAwaitImport(worker->name, XS_IMPORT_DEFAULT);
		if (xsTest(xsVar(0)) && xsIsInstanceOf(xsVar(0), xsFunctionPrototype))
			xsCallFunction0(xsVar(0), xsGlobal);
	}
	xsEndHost(worker->machine);
#if mxInstrument
	fxDescribeInstrumentation(worker->machine, 0, NULL, NULL);
#endif
	fxWorkerPlatform(it);
}

void fxWorkerMessage(txWorker* worker, txWorkerEvent* event)
{
	xsBeginHost(worker->machine);
	{
		xsVars(1);
		xsTry {
			xsCollectGarbage();
			xsResult = xsDemarshall(event->argument);
			xsVar(0) = xsAccess(*(event->reference));
			xsCall1(xsVar(0), xsID_onmessage, xsResult);
		}
		xsCatch {
		}
	}
	xsEndHost(worker->machine);
#if mxInstrument
	fxSampleInstrumentation(worker->machine, 0, NULL);
#endif
	c_free(event->argument);
	c_free(event);
}

void fxWorkerPlatform(void* it)
{
	txWorker* worker = it;
#if mxLinux
	worker->main_context = g_main_context_get_thread_default();
#elif mxWindows
	worker->machine->thread = worker;
	worker->machine->threadCallback = fxWorkerPerform;
	InitializeCriticalSection(&worker->lock);
	if (worker->event)
		SetEvent(worker->event);
#else
    CFRunLoopSourceContext runLoopSourceContext = {0, worker, NULL, NULL, NULL, NULL, NULL, WorkerThreadSchedule, NULL, fxWorkerPerform};
	worker->runLoop = CFRunLoopGetCurrent();
    worker->runLoopSource = CFRunLoopSourceCreate(NULL, 0, &runLoopSourceContext);
	CFRunLoopAddSource(worker->runLoop, worker->runLoopSource, kCFRunLoopDefaultMode);
#endif
}

#if mxLinux	
gboolean fxWorkerPerform(void* it)
#else
void fxWorkerPerform(void* it)
#endif
{
	txWorker* worker = it;
	txWorkerEvent* event;
#if mxLinux
	g_mutex_lock(&(worker->mutex));
#elif mxWindows
	EnterCriticalSection(&(worker->lock));
#else
    pthread_mutex_lock(&(worker->mutex));
#endif
	event = worker->queue;
	worker->queue = NULL;
#if mxLinux
	g_mutex_unlock(&(worker->mutex));
#elif mxWindows
	LeaveCriticalSection(&(worker->lock));
#else
    pthread_mutex_unlock(&(worker->mutex));
#endif
	while (event) {
		txWorkerEvent* next = event->next;
		txWorkerCallback callback = event->callback;
		event->next = NULL;
		event->callback = NULL;
		(*callback)(worker, event);
		event = next;
	}	
#if mxLinux
	return G_SOURCE_REMOVE;
#endif
}

void fxWorkerSignal(txWorker* worker, txWorkerEvent* event)
{
	txWorkerEvent** address;
	txWorkerEvent* former;
#if mxLinux
	g_mutex_lock(&(worker->mutex));
#elif mxWindows
	EnterCriticalSection(&(worker->lock));
#else
    pthread_mutex_lock(&(worker->mutex));
#endif
	address = &(worker->queue);
	while ((former = *address))
		address = &(former->next);
	*address = event;
#if mxLinux
	g_mutex_unlock(&(worker->mutex));
	GSource* idle_source = g_idle_source_new();
	g_source_set_callback(idle_source, fxWorkerPerform, worker, NULL);
	g_source_set_priority(idle_source, G_PRIORITY_DEFAULT);
	g_source_attach(idle_source, worker->main_context);
	g_source_unref(idle_source);
#elif mxWindows
	LeaveCriticalSection(&(worker->lock));
	PostMessage(worker->machine->window, WM_SERVICE, 0, 0);
#else
    pthread_mutex_unlock(&(worker->mutex));
    CFRunLoopSourceSignal(worker->runLoopSource);
    CFRunLoopWakeUp(worker->runLoop);
#endif
}

void fxWorkerTerminate(txWorker* worker, txWorkerEvent* event)
{
#if mxLinux
	g_main_loop_quit(worker->main_loop);
#elif mxWindows
	DeleteCriticalSection(&(worker->lock));
	CloseHandle(worker->event);
#else
	CFRunLoopRemoveSource(worker->runLoop, worker->runLoopSource, kCFRunLoopCommonModes);
	CFRelease(worker->runLoopSource);
#endif
    xsDeleteMachine(worker->machine);
    c_free(event);
    c_free(worker);
#if mxLinux
#elif mxWindows
	_endthreadex(0);
#else
    pthread_exit(NULL);
#endif
}

void xs_worker_destructor(void *data)
{
	if (data) {
		txWorker* worker = data;
		txWorkerEvent* event = c_calloc(sizeof(txWorkerEvent), 1);
		if (event) {
			event->callback = fxWorkerTerminate;
			fxWorkerSignal(worker, event);
		}
	}
}

void xs_worker(xsMachine *the)
{
	char *name = xsToString(xsArg(0));
	txWorker* worker = c_calloc(sizeof(txWorker) + c_strlen(name), 1);
	if (worker == NULL)
		xsUnknownError("not enough memory");
	worker->ownerMachine = the;
	worker->ownerReference = xsThis;
	xsRemember(worker->ownerReference);
	c_strcpy(worker->name, name);
	xsSetHostData(xsThis, worker);
#if mxLinux
	g_mutex_init(&(worker->mutex));
	g_mutex_lock(&(worker->mutex));
	g_thread_new(worker->name, fxWorkerLoop, worker);
#elif mxWindows
	worker->event = CreateEvent(0, FALSE, FALSE, NULL);
	worker->handle = (HANDLE)_beginthreadex(NULL, 0, fxWorkerLoop, worker, 0, NULL);
	WaitForSingleObject(worker->event, INFINITE);
#else
	pthread_attr_t	attr;
	pthread_t pthread;
    pthread_mutex_init(&(worker->mutex), NULL);
    pthread_mutex_lock(&(worker->mutex));
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pthread, &attr, &fxWorkerLoop, worker);
    pthread_attr_destroy(&attr);
#endif
}

void xs_worker_postfrominstantiator(xsMachine *the)
{
	txWorker* worker = xsGetHostData(xsThis);
	txWorkerEvent* event = c_calloc(sizeof(txWorkerEvent), 1);
	if (event == NULL)
		xsUnknownError("not enough memory");
	event->argument = xsMarshall(xsArg(0));
	event->reference = &(worker->reference);
	event->callback = fxWorkerMessage;
	fxWorkerSignal(worker, event);
}

void xs_worker_postfromworker(xsMachine *the)
{
	txWorker* worker = xsGetHostData(xsThis);
	txWorkerEvent* event = c_calloc(sizeof(txWorkerEvent), 1);
	if (event == NULL)
		xsUnknownError("not enough memory");
	event->argument = xsMarshall(xsArg(0));
	event->reference = &(worker->ownerReference);
	event->callback = fxWorkerMessage;
	fxWorkerSignal(worker->ownerMachine->host, event);
}

void xs_worker_terminate(xsMachine *the)
{
	txWorker* worker = xsGetHostData(xsThis);
	xsForget(worker->ownerReference);
	xsSetHostData(xsThis, NULL);
	xs_worker_destructor(worker);
}

void xs_sharedworker(xsMachine *the)
{
	xsDebugger();
}

