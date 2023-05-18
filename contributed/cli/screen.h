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



struct sxScreen {
	void *machine;
	void *firstWorker;
	txMutex workersMutex;
	txThread mainThread;
};