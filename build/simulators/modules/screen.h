/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#include <stdint.h>
#if mxLinux
#include <glib.h>
#elif mxWindows
#include <process.h>
#else
#include "pthread.h"
#endif

#if mxLinux
	typedef GCond txCondition;
	typedef GMutex txMutex;
	typedef GThread* txThread;
	#define mxCreateCondition(CONDITION) g_cond_init(CONDITION)
	#define mxCreateMutex(MUTEX) g_mutex_init(MUTEX)
	#define mxCurrentThread() g_thread_self()
	#define mxDeleteCondition(CONDITION) g_cond_clear(CONDITION)
	#define mxDeleteMutex(MUTEX) g_mutex_clear(MUTEX)
	#define mxLockMutex(MUTEX) g_mutex_lock(MUTEX)
	#define mxSignalCondition(CONDITION) g_cond_signal(CONDITION)
	#define mxUnlockMutex(MUTEX) g_mutex_unlock(MUTEX)
	#define mxWaitCondition(CONDITION,MUTEX) g_cond_wait(CONDITION,MUTEX)
#elif mxWindows
	typedef CONDITION_VARIABLE txCondition;
	typedef CRITICAL_SECTION txMutex;
	typedef DWORD txThread;
	#define mxCreateCondition(CONDITION) InitializeConditionVariable(CONDITION)
	#define mxCreateMutex(MUTEX) InitializeCriticalSection(MUTEX)
	#define mxCurrentThread() GetCurrentThreadId()
	#define mxDeleteCondition(CONDITION) (void)(CONDITION)
	#define mxDeleteMutex(MUTEX) DeleteCriticalSection(MUTEX)
	#define mxLockMutex(MUTEX) EnterCriticalSection(MUTEX)
	#define mxSignalCondition(CONDITION) WakeConditionVariable(CONDITION)
	#define mxUnlockMutex(MUTEX) LeaveCriticalSection(MUTEX)
	#define mxWaitCondition(CONDITION,MUTEX) SleepConditionVariableCS(CONDITION,MUTEX,INFINITE)
#else	
	typedef pthread_cond_t txCondition;
	typedef pthread_mutex_t txMutex;
	typedef pthread_t txThread;
	#define mxCreateCondition(CONDITION) pthread_cond_init(CONDITION,NULL)
	#define mxCreateMutex(MUTEX) pthread_mutex_init(MUTEX,NULL)
	#define mxCurrentThread() pthread_self()
	#define mxDeleteCondition(CONDITION) pthread_cond_destroy(CONDITION)
	#define mxDeleteMutex(MUTEX) pthread_mutex_destroy(MUTEX)
	#define mxLockMutex(MUTEX) pthread_mutex_lock(MUTEX)
	#define mxSignalCondition(CONDITION) pthread_cond_signal(CONDITION)
	#define mxUnlockMutex(MUTEX) pthread_mutex_unlock(MUTEX)
	#define mxWaitCondition(CONDITION,MUTEX) pthread_cond_wait(CONDITION,MUTEX)
#endif

typedef struct sxScreen txScreen;

typedef void (*txScreenAbortProc)(txScreen* screen, int status);
typedef void (*txScreenBufferChangedProc)(txScreen* screen);
typedef void (*txScreenFormatChangedProc)(txScreen* screen);
typedef void (*txScreenIdleProc)(txScreen* screen);
typedef void (*txScreenKeyProc)(txScreen* screen, int kind, char* string, int modifiers, double when);
typedef void (*txScreenLaunchProc)(txScreen* screen);
typedef void (*txScreenMessageProc)(txScreen* screen, char* buffer, int size);
typedef void (*txScreenQuitProc)(txScreen* screen);
typedef void (*txScreenStartProc)(txScreen* screen, double interval);
typedef void (*txScreenStopProc)(txScreen* screen);
typedef void (*txScreenTouchProc)(txScreen* screen, int kind, int index, int x, int y, double when);
typedef void (*txScreenWorkerCallbackProc)(void* machine, void* job);

#define screenBytesPerPixel 4

#define mxScreenIdling 1
#define mxScreenLED 2

struct sxScreen {
	void* machine;
	void* view;
	void* archive;
	txScreenAbortProc abort;
	txScreenBufferChangedProc bufferChanged;
	txScreenFormatChangedProc formatChanged;
	txScreenIdleProc idle;
	txScreenMessageProc invoke;
	txScreenKeyProc key;
	txScreenMessageProc post;
	txScreenQuitProc quit;
	txScreenStartProc start;
	txScreenStopProc stop;
	txScreenTouchProc touch;
	void* firstWorker;
	txMutex workersMutex;
	txThread mainThread;
	int flags;
	long instrumentTime;
	int pixelFormat;
	int rotation;
	int width;
	int height;
	uint16_t *clut;
	uint8_t palette[16 * screenBytesPerPixel];
	void *frameBuffer;				// only used when kPocoFrameBuffer
	uint32_t frameBufferLength;		// only used when kPocoFrameBuffer
	unsigned char *rowAddress;
	int rowCount;
	int rowDelta;
	int rowIndex;
	unsigned char buffer[1];
};

enum {
	rgb565le = 0,
	rgb565be,
	gray8,
	rgb332,
	gray4,
	clut4,
	pixelFormatCount
};

enum {
	touchEventBeganKind = 0,
	touchEventCancelledKind,
	touchEventEndedKind,
	touchEventMovedKind,
};

enum {
	keyEventDown = 0,
	keyEventUp = 1,
	keyEventRepeat = 1,
	keyEventCommand = 2,
	keyEventOption = 4,
	keyEventShift = 8,
};
