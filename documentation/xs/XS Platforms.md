# XS Platforms
Copyright 2016-2020 Moddable Tech, Inc.<BR>
Revised: September 2, 2020

## History

A platform is a combination of hardware and system software. For each platform, XS requires an interface file, `xsPlatform.h`, and an implementation file, `xsPlatform.c`

Historically, XS used one interface file, `xsPlatform.h` splitting the implementation into two files: `xsPlatform.c` and `xsHost.c`. Many platforms shared the same interface and implementation files, based either on the KinomaJS platform abstraction, or on an adhoc platform abstraction for command line tools.

Further, an XS machine had many ways to find and load modules and programs: from JS files, from stand alone compiled XSB files with or without companion DLL or SO files, and from a linked XSA file with a companion DLL or SO file... The XS platform was in charge of providing such options. 

When we started working on microcontrollers, the main inspiration for XS platforms was the adhoc platform abstraction for command line tools, which was the most complex version.

Today the XS runtime has been significantly streamlined, especially on microcontrollers. XS machines are always cloned from a read-only machine prepared by the XS linker. There are only modules, byte coded by the XS compiler. Modules are either preloaded or prepared to be loaded and unloaded at runtime. 

Consequently, it is now much simpler to build an XS platform. This document describes the necessary interface and implementation files.

## xsPlatform.h

### Basic types

XS uses a few basic types that the interface file has to define.

	#include <stdint.h>
	typedef int8_t txS1;
	typedef uint8_t txU1;
	typedef int16_t txS2;
	typedef uint16_t txU2;
	typedef int32_t txS4;
	typedef uint32_t txU4;

### C defines and includes

XS mostly relies on constants and functions from the C stantard library, accessed thru macros with `C_` or `c_` prefixes:

	#include <math.h>
	#define C_NAN NAN
	//...

	#include <stdlib.h>
	#define c_free free
	#define c_malloc malloc
	//...
	
Such definitions, and the corresponding includes, are the most significant part of the interface file. The macros allows a platform to provide its own constants and functions. See any of the provided `xsPlatform.h` for the list of macros to define.

### ESP macros

The Xtensa instruction set and architecture, used most notably in microcontrollers by Espressif, requires special macros to locate certain constant data in ROM and to read that data. On other platforms these macros are trivially defined:

	#define c_read8(POINTER) *((txU1 *)(POINTER))
	#define c_read16(POINTER) *((txU2 *)(POINTER))
	#define c_read32(POINTER) *((txU4 *)(POINTER))
	
	#define ICACHE_FLASH_ATTR
	#define ICACHE_RODATA_ATTR
	#define ICACHE_XS6RO_ATTR
	#define ICACHE_XS6RO2_ATTR
	#define ICACHE_XS6STRING_ATTR
	#define mxGetKeySlotID(SLOT) (SLOT)->ID
	#define mxGetKeySlotKind(SLOT) (SLOT)->kind


###  `mxMachinePlatform`

The platform can add fields to the machine record by defining the `mxMachinePlatform` macro. Since the machine is passed to all functions that XS calls (as the ubiquitous `the`), it is a convenient way for platforms to have their own context besides the application context.

For instance, on Mac, the `mxMachinePlatform` macro adds references to a socket and a run loop source for the communication with **xsbug**, and another run loop source for promises.

	#include <CoreServices/CoreServices.h>

	#define mxMachinePlatfom \
		CFSocketRef connection; \
		CFRunLoopSourceRef connectionSource; \
		CFRunLoopSourceRef promiseSource;

On Windows, the `mxMachinePlatform` macro adds the socket and message window handles that are used for the same purposes.

	#include <winsock2.h>

	#define mxMachinePlatfom \
		SOCKET connection; \
		HWND window;

## xsPlatform.c

The implementation file first includes `xsAll.h`, which contains the definitions of all XS macros and types, and the declarations of all XS extern functions. Then the platform has to implement the functions described here under.

XS machines do not support multiple threads, though platforms can support multiple threads, each with their own XS machines. All calls and callbacks described here must happen in the thread that created or cloned the machine. 

The functions are grouped into meaningful sections. The xsPlatform.c file can also provide POSIX functions that the platform is missing.

--

- `void fxCreateMachinePlatform(txMachine* the)`

`fxCreateMachinePlatform` is called when creating and cloning an XS machine. The platform initializes the fields defined by its `mxMachinePlatform` macro. By default all fields are zero.

--

- `void fxDeleteMachinePlatform(txMachine* the)`

`fxDeleteMachinePlatform` is called when deleting an XS machine. The platform must dispose or free here appropriate fields defined by its `mxMachinePlatform` macro.

--

### Debug

The functions in this section are only necessary for the debug version of XS. They can be condtionally defined within:

	#ifdef mxDebug
	// debug functions
	#endif
	
If the platform does not support the communication with **xsbug**, functions in this section can be empty, except  `fxIsConnected` and `fxIsReadable`, which must return `0`.

Communication between **xsbug** and the XS machine can be done over either a TCP/IP or serial connection. In the case of a TCP/IP connection, **xsbug** is the server and XS machines are clients. When using a serial connection, **xsbug** continues to communication over TCP/IP and a bridge running on the computer relays data between the serial and TCP connections. In the case of the ESP8266, this relay is performed by **serial2xsbug**.

Platforms must implement `fxIsReadable` to allow XS machines to receive messages from **xsbug** while executing byte codes, i.e. when platforms are inside the `fxRun` function. Most of the time, platforms are outside the `fxRun` function. So they use a system event and `fxDebugCommand` to tell XS about messages from **xsbug**.

For instance on Mac the platform uses `CFSocketCreate` with a `kCFSocketReadCallBack`:

	void fxReadableCallback(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
	{
		txMachine* the = context;
		if (fxIsReadable(the))
			fxDebugCommand(the);
	}

On Windows the platform uses `WSAAsyncSelect` with the `WM_XSBUG` message:

	LRESULT CALLBACK fxMessageWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)	{
	#ifdef mxDebug
		case WM_XSBUG: {
			txMachine* the = (txMachine*)GetWindowLongPtr(window, 0);
			if (fxIsReadable(the))
				fxDebugCommand(the);
		} break;
	#endif
		default:
			return DefWindowProc(window, message, wParam, lParam);
		}
		return 0;
	}

--

- `void fxConnect(txMachine* the)`

XS calls `fxConnect` to connect `the` machine to **xsbug**. 

For TCP/IP connections, platforms create a socket and connect it to **xsbug**. On Mac and Windows the address of **xsbug** is usually `localhost`, on other platforms it is usually defined by an environment variable. The port of **xsbug** defaults to `5002` by convention.

Machines are connect to **xsbug** after being created, i.e. `fxConnect` happens after `fxCreateMachinePlatform`.

--

- `void fxDisconnect(txMachine* the)`

XS calls `fxDisconnect` to disconnect `the` machine from **xsbug**. 

For TCP/IP connections, platforms close the socket.

Machines are disconnected before being deleted, i.e. `fxDisconnect` happens before `fxDeleteMachinePlatform`.

--

- `txBoolean fxIsConnected(txMachine* the)`

XS calls `fxIsConnected` to know if `the` machine is connected to **xsbug**.

--

- `txBoolean fxIsReadable(txMachine* the)`

XS calls `fxIsReadable` to know if `the` machine received a message from **xsbug**. Platforms must return 1 or 0 depending on the availability of bytes to read. 

The performance of the implementation of `fxIsReadable` is important since XS calls `fxIsReadable` at every `LINE` byte code (e.g. for each line of JavaScript source code executed).

--

- `void fxReceive(txMachine* the)`

XS calls `fxReceive` to receive a message from **xsbug**. The implementation reads bytes into `the->debugBuffer` and sets `the->debugOffset` to the number of bytes received. 

XS calls `fxReceive` repeatedly until the entire message is received. The maximum number of bytes that can be read by `fxReceive` is `sizeof(the->debugBuffer) - 1`.

--

- `void fxSend(txMachine* the, txBoolean more)`

XS calls `fxSend` to send a message to **xsbug**. The implementation gets the number of bytes to send from `the->echoOffset` and write bytes from `the->echoBuffer`. 

XS calls `fxSend ` repeatedly until the entire message is sent, `more` equals `1` while the message is incomplete, `0` when the message is complete. 

--

### Eval

The standard `eval` function, `Function` constructor and `Generator` constructor must transform source code into byte codes and keys. 

XS lets the platform decides is such feature is worth the memory it takes.

--

- `txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)`

XS calls `fxParseScript` to transform source code into XS byte codes and keys. The `stream` and `getter` arguments allow the parser to access the source code. The `flags` argument tells the parser the kind of source code: `mxModuleCode`, `mxProgramCode` or `mxEvalCode`.

If the platform supports such feature, it must include `xsScript.h` and implements `fxParseScript` like:

	#include "xsScript.h"

	txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
	{
		txParser _parser;
		txParser* parser = &_parser;
		txParserJump jump;
		txScript* script = NULL;
		fxInitializeParser(parser, the, 32*1024, 1993);
		parser->firstJump = &jump;
		if (c_setjmp(jump.jmp_buf) == 0) {
			fxParserTree(parser, stream, getter, flags, NULL);
			fxParserHoist(parser);
			fxParserBind(parser);
			script = fxParserCode(parser);
		}
		fxTerminateParser(parser);
		return script;
	}

The platform must also compile and link `xsScript.c`, `xsLexical.c`, `xsSyntaxical.c`, `xsTree.c`, `xsSourceMap.c`, `xsScope.c` and `xsCode.c`.

If the platform does not support such feature, `fxParseScript` must return `NULL` and the C files here above do not have to be compiled and linked.

--

### Keys

Keys are the names and symbols that XS uses to identify properties.

--

- `void fxBuildKeys(txMachine* the)`

`fxBuildKeys` is called only when creating an XS machine, in order to initialize the default keys used by the standard ECMAScript host functions.

On most platforms today, XS machines are cloned. The default keys are available and ready to be used in the read-only machine. So `fxBuildKeys` is never called and can be empty. 

If the platform supports the creation of XS machines from scratch, `fxBuildKeys` must be implemented as:

	void fxBuildKeys(txMachine* the)
	{
		int i;
		for (i = 0; i < XS_SYMBOL_ID_COUNT; i++) {
			txID id = the->keyIndex;
			txSlot* description = fxNewSlot(the);
			fxCopyStringC(the, description, gxIDStrings[i]);
			the->keyArray[id] = description;
			the->keyIndex++;
		}
		for (; i < XS_ID_COUNT; i++) {
			fxID(the, gxIDStrings[i]);
		}
	}

--

### Memory

XS machines use two heaps: the chunks heap and the slots heap. 

Chunks are blocks of variable size that the garbage collector can move to compact memory. XS stores strings, buffers, arrays, etc into chunks. On microcontrollers without a dedicated memory management unit, chunks are also useful to store any kind of data. For instance Piu uses chunks to store its containment hierarchy.

Slots are blocks of fixed size (four times the size of a pointer) that never move. XS maintains a list of free slots, slots are allocated from the list and freed into the list by the garbage collector.

--

- `void* fxAllocateChunks(txMachine* the, txSize size)`

XS calls `fxAllocateChunks` to get a system memory block for chunks. Usually implemented as:

	return c_malloc(size);

XS throws an exception if `fxAllocateChunks` returns NULL.

XS checks if the result of `fxAllocateChunks` is contiguous to `the->firstBlock` so microcontrollers can grow the chunks heap without fragmenting system memory.

--

- `txSlot* fxAllocateSlots(txMachine* the, txSize count)`

XS calls `fxAllocateSlots` to get a system memory block for `count` slots. Usually implemented as:

	return (txSlot*)c_malloc(count * sizeof(txSlot));

XS throws an exception if `fxAllocateSlots ` returns NULL.

--

- `void fxFreeChunks(txMachine* the, void* chunks)`

XS calls `fxFreeChunks` to free the `chunks` system memory block. Usually implemented as:

	c_free(chunks);

--

- `void fxFreeSlots(txMachine* the, void* slots)`

XS calls `fxFreeSlots` to free the `slots` system memory block. Usually implemented as:

	c_free(slots);

--

### Modules

On platforms that support several ways to get modules, the implementation of `fxFindModule` and `fxLoadModule` can be complex. On microcontrollers, where all modules are prepared or preloaded, the implementation of `fxFindModule`and `fxLoadModule` can be simple enough, as demonstrated by the code snippets here under.

--

- `txID fxFindModule(txMachine* the, txID moduleID, txSlot* slot)`

XS calls `fxFindModule` to find an imported or required module. 

The `moduleID` argument is the importing or requiring module identifier. It is `XS_NO_ID` when the machine itself requires a module.

The `slot` argument is the imported or required module name. It is the module specifier of the `import` syntactical construct or the module parameter of the `require` host function. 

If the module is found, `fxFindModule` returns the module identifier, otherwise `fxFindModule` returns `XS_NO_ID`.

A module identifier is a unique `txID`, but the platform defines the format of its corresponding key: it can be a path, a URL, a URI... 

The platform defines also how the importing or requiring module identifier and the imported or required module name are merged. The usual convention is based on absolute (`/*`), relative (`./*`, `../*`) or search (*) paths.

Finding modules can involve looking for various kinds of files, using a set of preferred locations, etc.  But on microcontrollers, all modules modules are prepared and ready to be found:

	txID fxFindModule(txMachine* the, txID moduleID, txSlot* slot)
	{
		txPreparation* preparation = the->archive;
		char name[PATH_MAX];
		char path[PATH_MAX];
		txBoolean absolute = 0, relative = 0, search = 0;
		txInteger dot = 0;
		txSlot *key;
		txString slash;
		txID id;
		
		fxToStringBuffer(the, slot, name, sizeof(name));
		if (!c_strncmp(name, "/", 1)) {
			absolute = 1;
		}	
		else if (!c_strncmp(name, "./", 2)) {
			dot = 1;
			relative = 1;
		}	
		else if (!c_strncmp(name, "../", 3)) {
			dot = 2;
			relative = 1;
		}
		else {
			relative = 1;
			search = 1;
		}
		if (absolute) {
			c_strcpy(path, preparation->base);
			c_strcat(path, name + 1);
			if (fxFindScript(the, path, &id))
				return id;
		}
		if (relative && (moduleID != XS_NO_ID)) {
			key = fxGetKey(the, moduleID);
			c_strcpy(path, key->value.key.string);
			slash = c_strrchr(path, '/');
			if (!slash)
				return XS_NO_ID;
			if (dot == 0)
				slash++;
			else if (dot == 2) {
				*slash = 0;
				slash = c_strrchr(path, '/');
				if (!slash)
					return XS_NO_ID;
			}
			if (!c_strncmp(path, preparation->base, preparation->baseLength)) {
				*slash = 0;
				c_strcat(path, name + dot);
				if (fxFindScript(the, path, &id))
					return id;
			}
		}
		if (search) {
			c_strcpy(path, preparation->base);
			c_strcat(path, name);
			if (fxFindScript(the, path, &id))
				return id;
		}
		return XS_NO_ID;
	}
	
	txBoolean fxFindScript(txMachine* the, txString path, txID* id)
	{
		txPreparation* preparation = the->archive;
		txInteger c = preparation->scriptCount;
		txScript* script = preparation->scripts;
		path += preparation->baseLength;
		c_strcat(path, ".xsb");
		while (c > 0) {
			if (!c_strcmp(path, script->path)) {
				path -= preparation->baseLength;
				*id = fxNewNameC(the, path);
				return 1;
			}
			c--;
			script++;
		}
		*id = XS_NO_ID;
		return 0;
	}

--

- `void fxLoadModule(txMachine* the, txID moduleID)`

XS calls `fxLoadModule` to tell the platform to prepare the byte codes, keys and host functions of a module. When ready, the platform  must call `fxResolveModule` with a `txScript` structure that references the byte codes, keys and host functions of the module.

Preparing modules can involve reading and mapping files, parsing, scoping and byte coding scripts, loading dynamic libraries, etc. But on microcontrollers, all `txScript` structures are available and ready to be used:

	void fxLoadModule(txMachine* the, txID moduleID)
	{
		txString path = fxGetKeyName(the, moduleID);
		txScript* script = fxLoadScript(the, path);
		fxResolveModule(the, moduleID, script, C_NULL, C_NULL);
	}
	
	txScript* fxLoadScript(txMachine* the, txString path)
	{
		txPreparation* preparation = the->archive;
		txInteger c = preparation->scriptCount;
		txScript* script = preparation->scripts;
		path += preparation->baseLength;
		while (c > 0) {
			if (!c_strcmp(path, script->path))
				return script;
			c--;
			script++;
		}
		return C_NULL;
	}

--

### Promises

Promises are essentially asynchronous. The `then` method of a `Promise` object takes two arguments: a function to call when the promise is fulfilled and a function to call when the promise is rejected. Both functions have to be called by a **Job**:

> *A Job is an abstract operation that initiates an ECMAScript computation when no other ECMAScript computation is currently in progress.* (ECMAScript® 2015 Language Specification, Section 8.4).

XS takes care queuing and running Jobs but relies on platforms for their scheduling. 

--

- `void fxQueuePromiseJobs(txMachine* the)`

XS calls `fxQueuePromiseJobs` once when jobs have been queued. Platforms can use any mechanism to defer a call to `fxRunPromiseJobs`.

For instance on Mac the platform uses a run loop source and `CFRunLoopSourceSignal`:

	void fxQueuePromiseJobsCallback(void *info)
	{
		txMachine* the = info;
		fxRunPromiseJobs(the);
	}
	
	void fxQueuePromiseJobs(txMachine* the)
	{
		CFRunLoopSourceSignal(the->promiseSource);
	}

On Windows the platform uses a message window and `PostMessage`:

	LRESULT CALLBACK fxMessageWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)	{
		case WM_PROMISE: {
			txMachine* the = (txMachine*)GetWindowLongPtr(window, 0);
			fxRunPromiseJobs(the);
		} break;
		default:
			return DefWindowProc(window, message, wParam, lParam);
		}
		return 0;
	}
	
	void fxQueuePromiseJobs(txMachine* the)
	{
		PostMessage(the->window, WM_PROMISE, 0, 0);
	}

### SharedArrayBuffer & Atomics

From XS point of view, `SharedArrayBuffer` instances are host objects, i.e. instances with an internal host slot. The data of the host slot is a pointer to the data of a **shared chunk**. The destructor of the host slot is `fxReleaseSharedChunk`.

What is a shared chunk is defined by the platform. XS atomically accesses 8-bit, 16-bit or 32-bit signed or unsigned integers inside the data of a shared chunk. XS accesses integers either thru GCC atomics, or between calls to `fxLockSharedChunk` and `fxUnlockSharedChunk`

Since `Atomics.wait` and `Atomics.wake` require to synchonize the **shared cluster** of machines created or cloned by XS, platforms usually need a global synchronization mechanism, and synchronization related fields in every machine record, thru the `mxMachinePlatform` macro explained here above.

#### Shared Cluster

	void fxInitializeSharedCluster();

Applications that use `Atomics.wait` and `Atomics.wake` must call `xsInitializeSharedCluster` before creating or cloning their first machine. `xsInitializeSharedCluster` is the application programming interface, `fxInitializeSharedCluster` is the platform implementation.

`fxInitializeSharedCluster` allows the platform to setup its global synchronization mechanism.

The thread that calls `fxInitializeSharedChunks` must be the thread that runs the user interface, usually the main thread. `Atomics.wait` fails for all machines running in that thread.

	void fxTerminateSharedCluster();

Applications that use `Atomics.wait` and `Atomics.wake` must call `xsTerminateSharedCluster` after deleting their last machine. `xsTerminateSharedCluster` is the application programming interface, `fxTerminateSharedCluster` is the platform implementation.

`fxTerminateSharedCluster` allows the platform to cleanup its global synchronization mechanism.

#### Shared Chunk

	void* fxCreateSharedChunk(txInteger byteLength);

`fxCreateSharedChunk` allocates a shared chunk, `byteLength` is the size of its data, which must be initialised to zero. 

Typically platforms use a reference count to track how many machines are referencing the shared chunk. `fxCreateSharedChunk` must initialise the reference count to one.

`fxCreateSharedChunk` returns a pointer to the data.

	void fxLockSharedChunk(void* data);

`fxLockSharedChunk` locks the shared chunk, `data` is a pointer to the data of the shared chunk. 

`fxLockSharedChunk` is never called if the platform supports GCC atomics.

	txInteger fxMeasureSharedChunk(void* data);

`fxMeasureSharedChunk` returns the size of the data of the chunk, `data` is a pointer to the data of the shared chunk.

	void fxReleaseSharedChunk(void* data);

Machines call `fxReleaseSharedChunk` when they do not reference the shared chunk anymore, `data` is a pointer to the data of the shared chunk.

Typically platforms use an atomic operation to decrement the reference count of the shared chunk and free the shared chunk when the reference count is zero.

`fxReleaseSharedChunk` is the destructor of the host slot. 

	void* fxRetainSharedChunk(void* data);

A machine calls `fxRetainSharedChunk` when marshalling a shared chunk to another machine. `data` is a pointer to the data of the shared chunk.

Typically platforms use an atomic operation to increment the reference count of the shared chunk.

	void fxUnlockSharedChunk(void* data);

`fxUnlockSharedChunk` unlocks the shared chunk. `data` is a pointer to the data of the shared chunk. 

`fxUnlockSharedChunk` is never called if the platform supports GCC atomics.

	txInteger fxWaitSharedChunk(txMachine* the, void* data, txInteger byteOffset, txInteger value, txNumber timeout);

If the application did not call `fxInitializeSharedCluster` or if the current thread is the thread that called `fxInitializeSharedCluster`, `fxWaitSharedChunk` throws a `TypeError` object.

If the 32-bit signed integer at `byteOffset` in `data` is not equal to `value`, `fxWaitSharedChunk` returns `-1` immediately. Else `fxWaitSharedChunk` suspends the current thread. 

If a matching call to `fxWakeSharedChunk` resumed the thread, `fxWaitSharedChunk` returns `1`. A matching call is a call with the same `data` and `byteOffset`.

If `timeout` expired, `fxWaitSharedChunk` returns `0`. `timeout` is a number between `Date.now()` and `C_INFINITY`. 

	txInteger fxWakeSharedChunk(txMachine* the, void* data, txInteger byteOffset, txInteger count);

If the application did not call `fxInitializeSharedCluster`, `fxWakeSharedChunk` returns `0`.

`fxWakeSharedChunk` resumes at most `count` threads that have been suspended by a matching call to `fxWaitSharedChunk `. A matching call is a call with the same `data` and `byteOffset`.

`fxWakeSharedChunk` returns the number of threads that resumed.

#### Default Implementations

XS provides four default implementations of shared cluster and chunks:

- For systems with Linux futex and GCC atomics
	- define `mxUseLinuxFutex`
	- define `mxUseGCCAtomics`
	- define `mxUseDefaultSharedChunks`
- For systems with POSIX threads, with or without GCC atomics
	- define `mxUsePOSIXThreads`
	- define `mxUseGCCAtomics` if the tool chain supports GCC atomics
	- define `mxUseDefaultSharedChunks`
- For Windows
	- define `mxUseDefaultSharedChunks`
- For systems with a single thread
	- define `mxUseDefaultSharedChunks`

All default implementations use `c_malloc` and `c_free` to create and delete shared chunks.

On systems with POSIX threads and on Windows, to use the default implementation of shared cluster and chunks, the platform must define the `mxMachinePlatform` macro with at least the following fields:

		#define mxMachinePlatform \
			void* waiterCondition; \
			void* waiterData; \
			txMachine* waiterLink;

Obviously, on systems with a single thread, `Atomics.wait` always fails and `Atomics.wake` always returns zero.
