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

#ifndef __XSLOPT__
#define __XSLOPT__
#ifdef EMSCRIPTEN
	#undef mxLinux
	#define mxLinux 1
#else
	#if defined(_MSC_VER)
		#if defined(_M_IX86) || defined(_M_X64)
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
		#if defined(__i386__) || defined(i386) || defined(intel) || defined(arm) || defined(__arm__) || defined(__k8__) || defined(__x86_64__) || defined(__aarch64__) || defined(EMSCRIPTEN)
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
	#define mxUseGCCAtomics 1
	#define mxUsePOSIXThreads 1
#endif
#define mxMachinePlatform \
	txSocket connection; \
	void* host; \
	void* waiterCondition; \
	void* waiterData; \
	txMachine* waiterLink; \
	txCallback fakeCallback;

#define mxUseDefaultChunkAllocation 1
#define mxUseDefaultSlotAllocation 1
#define mxUseDefaultSharedChunks 1
#define mxUseDefaultAbort 1

#endif /* __XSLOPT__ */
