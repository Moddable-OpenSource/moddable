/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#ifndef __WIN_XS__
#define __WIN_XS__

#undef mxWindows
#define mxWindows 1

#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

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
#include <stdbool.h>

#include <winsock2.h>

#define mxUseDefaultBuildKeys 1
#define mxUseDefaultChunkAllocation 1
#define mxUseDefaultSlotAllocation 1
#define mxUseDefaultFindModule 1
#define mxUseDefaultLoadModule 1
#define mxUseDefaultParseScript 1
#define mxUseDefaultSharedChunks 1

typedef struct sxWorkerJob txWorkerJob;
typedef void (*txWorkerCallback)(void* machine, void* job);

struct sxWorkerJob {
	txWorkerJob* next;
	txWorkerCallback callback;
};

extern void fxQueueWorkerJob(void* machine, void* job);

#define mxMachinePlatform \
	SOCKET connection; \
	HWND window; \
	void* host; \
	void* thread; \
	void (*threadCallback)(void*); \
	void* waiterCondition; \
	void* waiterData; \
	void* waiterLink; \
	CRITICAL_SECTION workerMutex; \
	txWorkerJob* workerQueue; \
	void* demarshall;

#define WM_PROMISE WM_USER
#define WM_SERVICE WM_USER + 1
#define WM_XSBUG WM_USER + 2
#define WM_CALLBACK WM_USER + 3
#define WM_WORKER WM_USER + 4
#define WM_MODTIMER WM_USER + 5
	
#ifdef mxDebug
	#define MODDEF_XS_TEST 1
#endif

#endif /* __WIN_XS__ */
