/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc. and Chris Midgley
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

/*
	This file implements services for the x-cli-win platform to start/stop the XS virtual machine, provide loading of
	archives (mods), and handling of instrumentation updates. 
*/

#include "xsAll.h"
#include "mc.xs.h"

HANDLE archiveFile = INVALID_HANDLE_VALUE;
HANDLE archiveMapping = INVALID_HANDLE_VALUE;
static txMachine root;
txMachine* machine = &root;


/*
	Service used by fxMapArchive to read a section of the archive (mod) from memory 
*/
xsBooleanValue fxArchiveRead(void* src, size_t offset, void* buffer, size_t size) {
	c_memcpy(buffer, ((txU1*)src) + offset, size);
	return 1;
}

/*
	Service used by fxMapArchive to write a section of the archive (mod) in memory 
*/
xsBooleanValue fxArchiveWrite(void* dst, size_t offset, void* buffer, size_t size) {
	c_memcpy(((txU1*)dst) + offset, buffer, size);
	return 1;
}

/*
	Windows specific implementation to map an archive (mod) into memory.  Accepts the path to the 
	archive, and returns either a pointer to the memory mapped image, or NULL if there was a failure during the loading
	of the file.
*/
void *loadArchive(char *archivePath) {
	if (archivePath[0]) {
		DWORD size;
		archiveFile = CreateFile(archivePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (archiveFile == INVALID_HANDLE_VALUE) {
			fprintf(stderr, "Failed to open archive %s (error %d)\n", archivePath, GetLastError());
			return NULL;
		}
		size = GetFileSize(archiveFile, &size);
		if (size == INVALID_FILE_SIZE) {
			fprintf(stderr, "Failed to get file size for %s (error %d)\n", archivePath, GetLastError());
			return NULL;
		}
		archiveMapping = CreateFileMapping(archiveFile, NULL, PAGE_READWRITE, 0, (SIZE_T)size, NULL);
		if (archiveMapping == INVALID_HANDLE_VALUE) {
			fprintf(stderr, "Failed to create file mapping for %s (error %d)\n", archivePath, GetLastError());
			return NULL;
		}
		void *memArchive = MapViewOfFile(archiveMapping, FILE_MAP_WRITE, 0, 0, (SIZE_T)size);
		if (memArchive == NULL) {
			fprintf(stderr, "Failed to map view of file for %s (error %d)\n", archivePath, GetLastError());
			return NULL;
		}
		return memArchive;
	}
	return NULL;
}

/*
	Callback function when the debugger is executed (see machine->onBreak).  Updates the instrumentation so
	breakpoints have accurate instrumentation just prior to stopping.
*/
void debugBreak(xsMachine* the, uint8_t stop)
{
#ifdef mxInstrument
	if (stop) {
		// fxCollectGarbage(the);
		// the->garbageCollectionCount -= 1;
		fxSampleInstrumentation(the, 0, NULL);
	}
#endif
}

/*
	Starts up an XS machine, including loading and mapping the archive (mod) and calling the exported 
	function "main".
*/
int startMachine(char *archivePath) {
	int error = 0;
	txPreparation* preparation = xsPreparation();
	void *memArchive = NULL;		// memory mapped instance of the mod (archive)
	void *archive = NULL;			// XS symbol-mapped archive

	// set up the XS VM
	c_memset(machine, 0, sizeof(txMachine));
	machine->preparation = preparation;
	machine->keyArray = preparation->keys;
	machine->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.keyCount;
	machine->keyIndex = (txID)preparation->keyCount;
	machine->nameModulo = preparation->nameModulo;
	machine->nameTable = preparation->names;
	machine->symbolModulo = preparation->symbolModulo;
	machine->symbolTable = preparation->symbols;
	
	machine->stack = &preparation->stack[0];
	machine->stackBottom = &preparation->stack[0];
	machine->stackTop = &preparation->stack[preparation->stackCount];
	
	machine->firstHeap = &preparation->heap[0];
	machine->freeHeap = &preparation->heap[preparation->heapCount - 1];
	machine->aliasCount = (txID)preparation->aliasCount;

	machine->onBreak = debugBreak;

	// memory map the archive using the first argument on the command line
	if (archivePath) {
		memArchive = loadArchive(archivePath);
	}

	// map the symbols for the mod ("archive")
	if (memArchive) {
		archive = fxMapArchive(preparation, memArchive, memArchive, 4 * 1024, fxArchiveRead, fxArchiveWrite);
		if (!archive) {
			fprintf(stderr, "Failed to map the archive; continuing with no archive\n");
		}
	}

	// instantiate the VM
	machine = fxPrepareMachine(NULL, preparation, "machine", NULL, archive);
	if (!machine) {
		fprintf(stderr, "Failed to instantiate the VM\n");
		return 1;	
	}


	// set up the stack context for XS
	xsBeginHost(machine);
	{
		xsVars(1);

#ifdef mxInstrument
		// send the instrumentation headers, only if the debugger is connected
		if (fxIsConnected(the))
			fxDescribeInstrumentation(machine, 0, NULL, NULL);

		// send the initial instrumentation data, only if the debugger is connected
		if (fxIsConnected(the))
			fxSampleInstrumentation(the, 0, NULL);
#endif
		{
			// attempt to locate the "main" export from the host, and call it's function to start it
			xsTry {
                xsVar(0) = xsAwaitImport("main", XS_IMPORT_DEFAULT);
                if (xsTest(xsVar(0))) {
                    if (xsIsInstanceOf(xsVar(0), xsFunctionPrototype)) {
                        xsCallFunction0(xsVar(0), xsGlobal);
                    }
                }
			}
			xsCatch {
				xsStringValue message = xsToString(xsException);
				fprintf(stderr, "Uncaught error from VM: %s\n", message);
				error = 1;
			}
		}
	}
	xsEndHost(the);
	return error;
}

/*
	Terminates a running XS machine
*/
void endMachine() {
	xsDeleteMachine(machine);	
}

/*
	Updates instrumentation (debugging statistics) for a running XS machine
*/
void instrumentMachine() {
#ifdef mxDebug
	xsBeginHost(machine);
	{
		// is the debugger connected?  If not, skip to avoid console output
		if (fxIsConnected(the))
			fxSampleInstrumentation(the, 0, NULL);
	}
	xsEndHost(the);
#endif
}

/*
	End point for XS to terminate execution
*/
void fxAbort(xsMachine* the, int status) {
	if (status == xsNotEnoughMemoryExit)
		fprintf(stderr, "Abort: not enough memory\n");
	else if (status == xsStackOverflowExit)
		fprintf(stderr, "Abort: stack overflow\n");
	else if (status == xsDeadStripExit)
		fprintf(stderr, "Abort: dead strip\n");
	else if (status == xsUnhandledExceptionExit)
		fprintf(stderr, "Abort: unhandled exception\n");
	else if (status != 0)
		fprintf(stderr, "Abort: Error code %d\n", status);
	exit(status);
}