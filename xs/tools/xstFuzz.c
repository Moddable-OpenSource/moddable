/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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

#include "xsAll.h"
#include "xsScript.h"
#include "xs.h"

extern int fuzz(int argc, char* argv[]);
extern void fx_print(xsMachine* the);
extern void fxBuildAgent(xsMachine* the);
extern void fxBuildFuzz(xsMachine* the);
extern void fxRunLoop(txMachine* the);
extern void fxRunModuleFile(txMachine* the, txString path);
extern void fxRunProgramFile(txMachine* the, txString path, txUnsigned flags);

#if OSSFUZZ
static int fuzz_oss(const uint8_t *Data, size_t script_size);
#endif

#if FUZZING
static void fx_fillBuffer(txMachine *the);
static void fx_fuzz_gc(xsMachine* the);
static void fx_fuzz_doMarshall(xsMachine* the);
#if OSSFUZZ
static void fx_nop(xsMachine *the);
static void fx_assert_throws(xsMachine *the);
#endif
#if FUZZILLI
static void fx_memoryFail(txMachine *the);
#endif
extern int gxStress;
int gxMemoryFail;		// not thread safe
#endif
/* native memory stress */

void fxBuildFuzz(xsMachine* the) 
{
#if FUZZING
	xsResult = xsNewHostFunction(fx_fuzz_gc, 0);
	xsSet(xsGlobal, xsID("gc"), xsResult);
	xsResult = xsNewHostFunction(fx_fillBuffer, 2);
	xsSet(xsGlobal, xsID("fillBuffer"), xsResult);
	xsResult = xsNewHostFunction(fx_fuzz_doMarshall, 1);
	xsSet(xsGlobal, xsID("doMarshall"), xsResult);
#if FUZZILLI
	xsResult = xsNewHostFunction(fx_memoryFail, 1);
	xsSet(xsGlobal, xsID("memoryFail"), xsResult);
#endif

	xsResult = xsNewHostFunction(fx_petrify, 1);
	xsDefine(xsGlobal, xsID("petrify"), xsResult, xsDontEnum);
	xsResult = xsNewHostFunction(fx_mutabilities, 1);
	xsDefine(xsGlobal, xsID("mutabilities"), xsResult, xsDontEnum);

	gxStress = 0;
	gxMemoryFail = 0;
#endif
}

#if OSSFUZZ
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    fuzz_oss(Data, Size);
    return 0;
}
#endif

#if FUZZING

// oss-fuzz limits to 2.5 GB, so 2 GB here to be comfortably under that
//#define mxXSMemoryLimit 0x80000000

#if mxXSMemoryLimit

struct sxMemoryBlock {
	struct sxMemoryBlock	*next;
	struct sxMemoryBlock	*prev;
	struct sxMemoryBlock	*address;
	size_t					size;
};
typedef struct sxMemoryBlock txMemoryBlock;

#define kMemoryBlockCount (256)		// update hashAddress if this is changed
static txMemoryBlock *gBlocks[kMemoryBlockCount];
static size_t gBlocksSize = 0;

static uint8_t hashAddress(void *addr)
{
	txU8 sum = (uintptr_t)addr;

	sum = (~sum) + (sum << 18); // sum = (sum << 18) - sum - 1;
	sum = sum ^ (sum >> 31);
	sum = sum * 21; // sum = (sum + (sum << 2)) + (sum << 4);
	sum = sum ^ (sum >> 11);
	sum = sum + (sum << 6);
	sum = sum ^ (sum >> 22);

	return (uint8_t)sum;
}

#define kBlockOverhead (64)

static txMutex gLinkMemoryMutex;

static void linkMemoryBlock(void *address, size_t size)
{
	static uint8_t first = 1;
	if (first) {
		first = 0;
		fxCreateMutex(&gLinkMemoryMutex);
	}
	uint8_t index = hashAddress(address);
	txMemoryBlock *block = malloc(sizeof(txMemoryBlock));		// assuming this will never fail (nearly true)

	block->address = address;
	block->prev = C_NULL;
	block->size = size;

    fxLockMutex(&gLinkMemoryMutex);

	block->next = gBlocks[index];
	if (gBlocks[index])
		gBlocks[index]->prev = block;
	gBlocks[index] = block;
	
	gBlocksSize += (size + kBlockOverhead) + (sizeof(txMemoryBlock) + kBlockOverhead);

    fxUnlockMutex(&gLinkMemoryMutex);
}

static void unlinkMemoryBlock(void *address)
{
	uint8_t index = hashAddress(address);

    fxLockMutex(&gLinkMemoryMutex);
	txMemoryBlock *block = gBlocks[index];
	while (block && (block->address != address))
		block = block->next;

	if (block->next)
		block->next->prev = block->prev;

	if (block->prev)
		block->prev->next = block->next;
	else
		gBlocks[index] = block->next;

	gBlocksSize -= (block->size + kBlockOverhead) + (sizeof(txMemoryBlock) + kBlockOverhead);

    fxUnlockMutex(&gLinkMemoryMutex);

	free(block);
}

static size_t getMemoryBlockSize(void *address)
{
	uint8_t index = hashAddress(address);
    fxLockMutex(&gLinkMemoryMutex);
	txMemoryBlock *block = gBlocks[index];
	while (block && (block->address != address))
		block = block->next;
	int size = block->size;
    fxUnlockMutex(&gLinkMemoryMutex);
    return size;
}

void freeMemoryBlocks(void)
{
	int i;
	for (i = 0; i < kMemoryBlockCount; i++) {
		while (gBlocks[i])
			fxMemFree(gBlocks[i]->address);
	}
}

#if mxNoChunks
void *fxMemMalloc_noforcefail(size_t size)
{
	if ((size + gBlocksSize) > mxXSMemoryLimit)
		return NULL;

	void *result = malloc(size);
	linkMemoryBlock(result, size);
	return result;
}
#endif

void *fxMemMalloc(size_t size)
{
	if (gxMemoryFail && !--gxMemoryFail)
		return NULL;

	if ((size + gBlocksSize) > mxXSMemoryLimit)
		return NULL;

	void *result = malloc(size);
	linkMemoryBlock(result, size);
	
	return result;
}

void *fxMemCalloc(size_t a, size_t b)
{
	if (gxMemoryFail && !--gxMemoryFail)
		return NULL;

	size_t size = a * b;
	if ((size + gBlocksSize) > mxXSMemoryLimit)
		return NULL;

	void *result = calloc(a, b);
	linkMemoryBlock(result, size);

	return result;
}

void *fxMemRealloc(void *a, size_t b)
{
	if (gxMemoryFail && !--gxMemoryFail)
		return NULL;
 
	if ((b - getMemoryBlockSize(a) + gBlocksSize) > mxXSMemoryLimit)
		return NULL;

	unlinkMemoryBlock(a);
	a = realloc(a, b);
	linkMemoryBlock(a, b);
	
	return a;
}

void fxMemFree(void *m)
{
	unlinkMemoryBlock(m);
	free(m);
}

#else // 0 == mxXSMemoryLimit

#if mxNoChunks
void *fxMemMalloc_noforcefail(size_t size)
{
	return malloc(size);
}
#endif

void *fxMemMalloc(size_t size)
{
	if (gxMemoryFail && !--gxMemoryFail)
		return NULL;

	return malloc(size);
}

void *fxMemCalloc(size_t a, size_t b)
{
	if (gxMemoryFail && !--gxMemoryFail)
		return NULL;

	return calloc(a, b);
}

void *fxMemRealloc(void *a, size_t b)
{
	if (gxMemoryFail && !--gxMemoryFail)
		return NULL;
 
	return realloc(a, b);
}

void fxMemFree(void *m)
{
	free(m);
}

#endif // mxXSMemoryLimit

#endif	// FUZZING

/* FUZZILLI */

#if FUZZILLI
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define SHM_SIZE 0x100000
#define MAX_EDGES ((SHM_SIZE - 4) * 8)

struct shmem_data {
	uint32_t num_edges;
	unsigned char edges[];
};

struct shmem_data* __shmem;
uint32_t *__edges_start, *__edges_stop;

void __sanitizer_cov_reset_edgeguards()
{
	uint64_t N = 0;
	for (uint32_t *x = __edges_start; x < __edges_stop && N < MAX_EDGES; x++)
		*x = ++N;
}

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
	// Avoid duplicate initialization
	if (start == stop || *start)
		return;

	if (__edges_start != NULL || __edges_stop != NULL) {
		fprintf(stderr, "Coverage instrumentation is only supported for a single module\n");
		c_exit(-1);
	}

	__edges_start = start;
	__edges_stop = stop;

	// Map the shared memory region
	const char* shm_key = getenv("SHM_ID");
	if (!shm_key) {
		puts("[COV] no shared memory bitmap available, skipping");
		__shmem = (struct shmem_data*) malloc(SHM_SIZE);
	} else {
		int fd = shm_open(shm_key, O_RDWR, S_IREAD | S_IWRITE);
		if (fd <= -1) {
			fprintf(stderr, "Failed to open shared memory region: %s\n", strerror(errno));
			c_exit(-1);
		}

		__shmem = (struct shmem_data*) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (__shmem == MAP_FAILED) {
			fprintf(stderr, "Failed to mmap shared memory region\n");
			c_exit(-1);
		}
	}

	__sanitizer_cov_reset_edgeguards();

	__shmem->num_edges = stop - start;
	printf("[COV] edge counters initialized. Shared memory: %s with %u edges\n", shm_key, __shmem->num_edges);
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
	// There's a small race condition here: if this function executes in two threads for the same
	// edge at the same time, the first thread might disable the edge (by setting the guard to zero)
	// before the second thread fetches the guard value (and thus the index). However, our
	// instrumentation ignores the first edge (see libcoverage.c) and so the race is unproblematic.
	uint32_t index = *guard;
	// If this function is called before coverage instrumentation is properly initialized we want to return early.
	if (!index) return;
	__shmem->edges[index / 8] |= 1 << (index % 8);
	*guard = 0;
}

#define REPRL_CRFD 100
#define REPRL_CWFD 101
#define REPRL_DRFD 102
#define REPRL_DWFD 103

void fx_fuzzilli(xsMachine* the)
{
	const char* str = xsToString(xsArg(0));
	if (!strcmp(str, "FUZZILLI_CRASH")) {
 		switch (xsToInteger(xsArg(1))) {
 			case 0:
				// check crash
				*((volatile char *)0) = 0;
				break;
 			case 1: {
				// check sanitizer
				// this code is so buggy its bound to trip
				// different sanitizers
				size_t s = -1;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
				txSize txs = s + 1;
#pragma GCC diagnostic pop
				char buf[2];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
				char* bufptr = &buf;
#pragma GCC diagnostic pop
				bufptr[4] = (buf[0] == buf[1]) ? 0 : 1;
				*((volatile char *)0) = 0;

				// check ASAN
				char *data = malloc(64);
				free(data);
				data[0]++;
				} break;
 			case 2:
				// check assert
				assert(0);
				break;
		}
 	}
	else if (!strcmp(str, "FUZZILLI_PRINT")) {
		const char* print_str = xsToString(xsArg(1));
		FILE* fzliout = fdopen(REPRL_DWFD, "w");
		if (!fzliout) {
			fprintf(stderr, "Fuzzer output channel not available, printing to stdout instead\n");
			fzliout = stdout;
		}
		fprintf(fzliout, "%s\n", print_str);
		fflush(fzliout);
	}
}

#ifdef mxMetering
static xsBooleanValue xsAlwaysWithinComputeLimit(xsMachine* machine, uint64_t index)
{
	return 1;
}
#endif

int fuzz(int argc, char* argv[])
{
	char helo[] = "HELO";
	if (4 != write(REPRL_CWFD, helo, 4)) {
		fprintf(stderr, "Error writing HELO\n");
		c_exit(-1);
	}
	if (4 != read(REPRL_CRFD, helo, 4)) {
		fprintf(stderr, "Error reading HELO\n");
		c_exit(-1);
	}
	if (0 != memcmp(helo, "HELO", 4)) {
		fprintf(stderr, "Invalid response from parent\n");
		c_exit(-1);
	}
	xsCreation _creation = {
		1 * 1024 * 1024, 	/* initialChunkSize */
		1 * 1024 * 1024, 	/* incrementalChunkSize */
		32768, 				/* initialHeapCount */
		32768,			 	/* incrementalHeapCount */
		64 * 1024,	 		/* stackCount */
		1024,				/* initialKeyCount */
		1024,				/* incrementalKeyCount */
		1993, 				/* nameModulo */
		127, 				/* symbolModulo */
		64 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};

	while (1) {
		int error = 0;
		char *buffer = NULL;

		gxStress = 0;
		gxMemoryFail = 0;

		xsMachine* machine = xsCreateMachine(&_creation, "xst_fuzz", NULL);
		xsBeginMetering(machine, xsAlwaysWithinComputeLimit, 0);		// interval/step of zero means "never invoke callback" 
		{
		xsBeginHost(machine);
		{
			xsTry {
				xsVars(1);

				// hardened javascript
				xsResult = xsNewHostFunction(fx_harden, 1);
				xsDefine(xsGlobal, xsID("harden"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_lockdown, 0);
				xsDefine(xsGlobal, xsID("lockdown"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_petrify, 1);
				xsDefine(xsGlobal, xsID("petrify"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_mutabilities, 1);
				xsDefine(xsGlobal, xsID("mutabilities"), xsResult, xsDontEnum);

				// fuzzilli
				xsResult = xsNewHostFunction(fx_fuzzilli, 2);
				xsSet(xsGlobal, xsID("fuzzilli"), xsResult);
				xsResult = xsNewHostFunction(fx_fuzz_gc, 0);
				xsSet(xsGlobal, xsID("gc"), xsResult);
				xsResult = xsNewHostFunction(fx_print, 1);
				xsSet(xsGlobal, xsID("print"), xsResult);
				xsResult = xsNewHostFunction(fx_fillBuffer, 2);
				xsSet(xsGlobal, xsID("fillBuffer"), xsResult);
				xsResult = xsNewHostFunction(fx_fuzz_doMarshall, 1);
				xsSet(xsGlobal, xsID("doMarshall"), xsResult);				
				xsResult = xsNewHostFunction(fx_memoryFail, 1);
				xsSet(xsGlobal, xsID("memoryFail"), xsResult);

				// wait for the script
				char action[4];
				ssize_t nread = read(REPRL_CRFD, action, 4);
				fflush(0);		//@@
				if (nread != 4 || memcmp(action, "exec", 4) != 0) {
					fprintf(stderr, "Unknown action: %s\n", action);
					c_exit(-1);
				}

				size_t script_size = 0;
				read(REPRL_CRFD, &script_size, 8);

				ssize_t remaining = (ssize_t)script_size;
				buffer = (char *)malloc(script_size + 1);
				ssize_t rv = read(REPRL_DRFD, buffer, (size_t) remaining);
				if (rv <= 0) {
					fprintf(stderr, "Failed to load script\n");
					c_exit(-1);
				}
				buffer[script_size] = 0;	// required when debugger active

				// run the script
				txSlot* realm = mxProgram.value.reference->next->value.module.realm;
				txStringCStream aStream;
				aStream.buffer = buffer;
				aStream.offset = 0;
				aStream.size = script_size;
				the->script = fxParseScript(the, &aStream, fxStringCGetter, mxProgramFlag | mxDebugFlag);
				fxRunScript(the, the->script, mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
				the->script = NULL;
				mxPullSlot(mxResult);

				fxRunLoop(the);
			}
			xsCatch {
				the->script = NULL;
				error = 1;
			}
		}
		gxMemoryFail = 0;
		fxCheckUnhandledRejections(machine, 1);
		xsEndHost(machine);
		}
		xsEndMetering(machine);
		gxMemoryFail = 0;
		fxDeleteScript(machine->script);
		int status = (machine->exitStatus & 0xff) << 8;
		if (!status && error)
			status = XS_UNHANDLED_EXCEPTION_EXIT << 8;
		if (write(REPRL_CWFD, &status, 4) != 4) {
			fprintf(stderr, "Erroring writing return value over REPRL_CWFD\n");
			exit(-1);
		}

		xsDeleteMachine(machine);

		free(buffer);

		__sanitizer_cov_reset_edgeguards();
	}


	return 0;
}
#else
int fuzz(int argc, char* argv[])
{
	fprintf(stderr, "Build xst with FUZZING=1 FUZZILLI=1\n");
	return 1;
}
#endif 
#if OSSFUZZ

#if mxMetering
#ifndef mxFuzzMeter
	// highest rate for test262 corpus was 2147483800
	#define mxFuzzMeter (214748380)
#endif

static xsBooleanValue xsWithinComputeLimit(xsMachine* machine, uint64_t index)
{
	// may be useful to print current index for debugging
//	fprintf(stderr, "Current index: %u\n", index);
	if (index > mxFuzzMeter) {
//		fprintf(stderr, "Computation limits reached (index %u). Exiting...\n", index);
		return 0;
	}
	return 1;
}
#endif

extern void modInstallTextDecoder(xsMachine *the);

int fuzz_oss(const uint8_t *Data, size_t script_size)
{
	xsCreation _creation = {
		1 * 1024 * 1024, 	/* initialChunkSize */
		1 * 1024 * 1024, 	/* incrementalChunkSize */
		32768, 				/* initialHeapCount */
		32768,			 	/* incrementalHeapCount */
		64 * 1024,	 		/* stackCount */
		1024,				/* initialKeyCount */
		1024,				/* incrementalKeyCount */
		1993, 				/* nameModulo */
		127, 				/* symbolModulo */
		64 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};
	size_t buffer_size = script_size + script_size + script_size + 1;			// (massively) over-allocate to have space if UTF-8 encoding expands (1 byte invalid byte becomes a 3-byte UTF-8 sequence)
	char* buffer = (char *)malloc(buffer_size);
	memcpy(buffer, Data, script_size);

	buffer[script_size] = 0;	// required when debugger active

	xsCreation* creation = &_creation;
	xsMachine* machine;
	machine = xsCreateMachine(creation, "xst_fuzz_oss", NULL);

	xsBeginMetering(machine, xsWithinComputeLimit, 65536);
	{
		xsBeginHost(machine);
		{
			xsTry {
				xsVars(2);
				modInstallTextDecoder(the);
				xsResult = xsArrayBuffer(buffer, script_size);
				xsVar(0) = xsNew0(xsGlobal, xsID("TextDecoder"));
				xsResult = xsCall1(xsVar(0), xsID("decode"), xsResult);
	#ifdef OSSFUZZ_JSONPARSE
				xsVar(0) = xsGet(xsGlobal, xsID("JSON"));
				xsResult = xsCall1(xsVar(0), xsID("parse"), xsResult);
	#else
				xsToStringBuffer(xsResult, buffer, buffer_size);

				// hardened javascript
				xsResult = xsNewHostFunction(fx_harden, 1);
				xsDefine(xsGlobal, xsID("harden"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_lockdown, 0);
				xsDefine(xsGlobal, xsID("lockdown"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_petrify, 1);
				xsDefine(xsGlobal, xsID("petrify"), xsResult, xsDontEnum);
				xsResult = xsNewHostFunction(fx_mutabilities, 1);
				xsDefine(xsGlobal, xsID("mutabilities"), xsResult, xsDontEnum);

				xsResult = xsNewHostFunction(fx_fuzz_gc, 0);
				xsSet(xsGlobal, xsID("gc"), xsResult);
				xsResult = xsNewHostFunction(fx_print, 1);
				xsSet(xsGlobal, xsID("print"), xsResult);

				// test262 stubs
				xsVar(0) = xsNewHostFunction(fx_nop, 1);
				xsDefine(xsGlobal, xsID("assert"), xsVar(0), xsDontEnum);
				xsDefine(xsVar(0), xsID("sameValue"), xsVar(0), xsDontEnum);
				xsDefine(xsVar(0), xsID("notSameValue"), xsVar(0), xsDontEnum);
				xsVar(1) = xsNewHostFunction(fx_assert_throws, 1);
				xsDefine(xsVar(0), xsID("throws"), xsVar(1), xsDontEnum);
				
				txStringCStream aStream;
				aStream.buffer = buffer;
				aStream.offset = 0;
				aStream.size = strlen(buffer);
				// run script
				txSlot* realm = mxProgram.value.reference->next->value.module.realm;
				the->script = fxParseScript(the, &aStream, fxStringCGetter, mxProgramFlag | mxDebugFlag);
				fxRunScript(the, the->script, mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
				the->script = NULL;
				mxPullSlot(mxResult);
				fxRunLoop(the);
	#endif
			}
			xsCatch {
				the->script = NULL;
			}
		}
		xsEndHost(machine);
	}
	xsEndMetering(machine);
	fxDeleteScript(machine->script);
#if mxXSMemoryLimit
	int exitStatus = machine->exitStatus;
#endif
	xsDeleteMachine(machine);
	free(buffer);

#if mxXSMemoryLimit
	if ((XS_TOO_MUCH_COMPUTATION_EXIT == exitStatus) || (XS_NOT_ENOUGH_MEMORY_EXIT == exitStatus) || (XS_STACK_OVERFLOW_EXIT == exitStatus))
		freeMemoryBlocks();		// clean-up if computation or memory limits exceeded, or stack overflow
#endif

	return 0;
}

#endif 

#if FUZZING || FUZZILLI

void fx_fillBuffer(txMachine *the)
{
	xsIntegerValue seed = xsToInteger(xsArg(1));
	xsIntegerValue length = xsGetArrayBufferLength(xsArg(0)), i;
	uint8_t *buffer = xsToArrayBuffer(xsArg(0));
	
	for (i = 0; i < length; i++) {
		seed = (uint64_t)seed * 48271 % 0x7fffffff;
		*buffer++ = (uint8_t)seed;
	}
}

void fx_fuzz_gc(xsMachine* the)
{
	xsResult = xsInteger(gxStress);

	xsIntegerValue c = xsToInteger(xsArgc);
	if (!c) {
		xsCollectGarbage();
		return;
	}
	
	int count = xsToInteger(xsArg(0));
	gxStress = (count < 0) ? count : -count;
}

void fx_fuzz_doMarshall(xsMachine *the)
{
	char *message;
	xsIntegerValue c = xsToInteger(xsArgc);
	if (c > 0)
		message = xsMarshallAlien(xsArg(0));
	else
		message = xsMarshallAlien(xsUndefined);
	xsResult = xsDemarshallAlien(message);
	c_free(message);
}

#if OSSFUZZ
void fx_nop(xsMachine *the)
{
}

void fx_assert_throws(xsMachine *the)
{
	mxTry(the) {
		if (xsToInteger(xsArgc) >= 2)
			xsCallFunction0(xsArg(1), xsGlobal);
	}
	mxCatch(the) {
	}
}
#endif 

#if FUZZILLI
void fx_memoryFail(txMachine *the)
{
	xsResult = xsInteger(gxMemoryFail);
	if (!xsToInteger(xsArgc))
		return;

	int count = xsToInteger(xsArg(0));
	if (count < 0)
		xsUnknownError("invalid");
	gxMemoryFail = count;
}
#endif

#endif
