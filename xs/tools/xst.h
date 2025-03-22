/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __XST__
#define __XST__

#if defined(_MSC_VER)
	#if defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM64) || defined(_M_ARM64EC)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#undef mxWindows
		#define mxWindows 1
		#define mxExport extern
		#define mxImport extern
		#define XS_FUNCTION_NORETURN
	#else 
		#error unknown Microsoft compiler
	#endif
#elif defined(__GNUC__) 
	#if defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__k8__) || defined(__x86_64__) || defined(__aarch64__)
		#undef mxLittleEndian
		#define mxLittleEndian 1
		#if defined(__linux__) || defined(linux)
			#undef mxLinux
			#define mxLinux 1
		#else
			#undef mxMacOSX
			#define mxMacOSX 1
		#endif
		#define mxExport extern
		#define mxImport extern
		#define XS_FUNCTION_NORETURN __attribute__((noreturn))
	#else 
		#error unknown GNU compiler
	#endif
#else 
	#error unknown compiler
#endif

#if mxWindows
	#define _USE_MATH_DEFINES
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fdlibm.h>

#if mxWindows
	#include <winsock2.h>
	typedef SOCKET txSocket;
	#define mxNoSocket INVALID_SOCKET
#else
	#include <fcntl.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <pthread.h>
	#include <signal.h>
	#include <unistd.h>
	typedef int txSocket;
	#define mxNoSocket -1
#if mxLinux
	#if GNUC > 11
		#define mxUseFloat16 1
	#endif
#else
	#define mxUseFloat16 1
#endif
	#define mxUseGCCAtomics 1
	#define mxUsePOSIXThreads 1
#endif
#define mxMachinePlatform \
	txSocket connection; \
	int promiseJobs; \
	void* rejection; \
	void *script;		// txScript*

#define XS_NO_MODULE 1

#define mxAliasInstance 0
#define mxCESU8 1
#define mxCanonicalNaN 1
#define mxHostFunctionPrimitive 0
#define mxKeysGarbageCollection 1
#define mxLockdown 1
#define mxSnapshot 1

#define mxExplicitResourceManagement 1
#define mxImmutableArrayBuffers 1
#define mxModuleStuff 1

#define mxMinusZero 1
#define mxRegExpUnicodePropertyEscapes 1
#define mxStringNormalize 1
#define mxWithHasGetSequence 1

#define mxUseDefaultBuildKeys 1
#define mxUseDefaultChunkAllocation 1
#define mxUseDefaultSlotAllocation 1
#define mxUseDefaultParseScript 1
#define mxUseDefaultSharedChunks 1

typedef struct sxSharedTimer txSharedTimer;
typedef void (*txSharedTimerCallback)(txSharedTimer* timer, void *refcon, int refconSize);

extern void fxInitializeSharedTimers();
extern void fxTerminateSharedTimers();
extern void fxRescheduleSharedTimer(txSharedTimer* timer, double timeout, double interval);
extern void* fxScheduleSharedTimer(double timeout, double interval, txSharedTimerCallback callback, void* refcon, int refconSize);
extern void fxUnscheduleSharedTimer(txSharedTimer* timer);

#define mxInitializeSharedTimers fxInitializeSharedTimers
#define mxTerminateSharedTimers fxTerminateSharedTimers
#define mxRescheduleSharedTimer fxRescheduleSharedTimer
#define mxScheduleSharedTimer fxScheduleSharedTimer
#define mxUnscheduleSharedTimer fxUnscheduleSharedTimer

#if INTPTR_MAX == INT64_MAX
	#define mx32bitID 1
#endif

#if FUZZING
extern void *fxMemMalloc(size_t size);
extern void *fxMemCalloc(size_t a, size_t b);
extern void *fxMemRealloc(void *a, size_t b);
extern void fxMemFree(void *m);

#define c_malloc(a) fxMemMalloc(a)
#define c_calloc(a, b) fxMemCalloc(a, b)
#define c_realloc(a, b) fxMemRealloc(a, b)
#define c_malloc_uint32(a) c_malloc(a)
#define c_free(p) fxMemFree(p)
#define c_free_uint32(p) c_free(p)
#endif

#if mxWindows
	#include <direct.h>
	#include <errno.h>
	#include <process.h>
	typedef CONDITION_VARIABLE txCondition;
	typedef CRITICAL_SECTION txMutex;
    typedef DWORD txThread;
	#define fxCreateCondition(CONDITION) InitializeConditionVariable(CONDITION)
	#define fxCreateMutex(MUTEX) InitializeCriticalSection(MUTEX)
	#define fxDeleteCondition(CONDITION) (void)(CONDITION)
	#define fxDeleteMutex(MUTEX) DeleteCriticalSection(MUTEX)
	#define fxLockMutex(MUTEX) EnterCriticalSection(MUTEX)
	#define fxUnlockMutex(MUTEX) LeaveCriticalSection(MUTEX)
	#define fxSleepCondition(CONDITION,MUTEX) SleepConditionVariableCS(CONDITION,MUTEX,INFINITE)
	#define fxWakeAllCondition(CONDITION) WakeAllConditionVariable(CONDITION)
	#define fxWakeCondition(CONDITION) WakeConditionVariable(CONDITION)
	#define mxCurrentThread() GetCurrentThreadId()
	#define mxMonotonicNow() ((txNumber)GetTickCount64())
#else
	#include <dirent.h>
	#include <pthread.h>
	#include <sys/stat.h>
	typedef pthread_cond_t txCondition;
	typedef pthread_mutex_t txMutex;
	typedef pthread_t txThread;
	#define fxCreateCondition(CONDITION) pthread_cond_init(CONDITION,NULL)
	#define fxCreateMutex(MUTEX) pthread_mutex_init(MUTEX,NULL)
	#define fxDeleteCondition(CONDITION) pthread_cond_destroy(CONDITION)
	#define fxDeleteMutex(MUTEX) pthread_mutex_destroy(MUTEX)
	#define fxLockMutex(MUTEX) pthread_mutex_lock(MUTEX)
	#define fxUnlockMutex(MUTEX) pthread_mutex_unlock(MUTEX)
	#define fxSleepCondition(CONDITION,MUTEX) pthread_cond_wait(CONDITION,MUTEX)
	#define fxWakeAllCondition(CONDITION) pthread_cond_broadcast(CONDITION)
	#define fxWakeCondition(CONDITION) pthread_cond_signal(CONDITION)
	#define mxCurrentThread() pthread_self()
	#define mxMonotonicNow() fxDateNow()
#endif

typedef struct sxAgent txAgent;
typedef struct sxAgentCluster txAgentCluster;
typedef struct sxAgentReport txAgentReport;
typedef struct sxJob txJob;

struct sxAgent {
	txAgent* next;
#if mxWindows
    HANDLE thread;
#else
	pthread_t thread;
#endif
	int scriptLength;
	char script[1];
};

struct sxAgentReport {
	txAgentReport* next;
	char message[1];
};

struct sxAgentCluster {
	txMutex mainMutex;

	txAgent* firstAgent;
	txAgent* lastAgent;
	
	int count;
	txCondition countCondition;
	txMutex countMutex;

	void* dataBuffer;
	txCondition dataCondition;
	txMutex dataMutex;
	int dataValue;

	txAgentReport* firstReport;
	txAgentReport* lastReport;
	txMutex reportMutex;
};

extern char *gxAbortStrings[];
extern txAgentCluster gxAgentCluster;

#endif /* __XST__ */
