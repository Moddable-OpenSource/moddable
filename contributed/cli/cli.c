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
	This file implements common services across all platforms to implement the "cli-XXX" runtime for Moddable,
	including support for starting/stopping the XS virtual machine, and providing loading of archives (mods).
*/

#include "xsAll.h"
#include "mc.xs.h"
#include "cli.h"
#include "screen.h"

// txMachine* machine = NULL;
// set up context (using a txScreen, from screen.h, because that is what the base/Worker module needs - it is
// a subset of what a simulator would use, containing mostly support for finding the machine and mutex controls)
txScreen screen;

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
	Callback function when the debugger is executed (see machine->onBreak).  Updates the instrumentation so
	breakpoints have accurate instrumentation just prior to stopping.
*/
void debugBreak(xsMachine* the, uint8_t stop) {
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

	// set up our "screen" context
	mxCreateMutex(&screen.workersMutex);
	screen.firstWorker = NULL;
	screen.mainThread = mxCurrentThread();

	// instantiate the VM
	txMachine *machine = fxPrepareMachine(NULL, preparation, "machine", &screen, archive);
	if (!machine) {
		fprintf(stderr, "Failed to instantiate the VM\n");
		return 1;	
	}

	// instruct the machine to call us prior to entering the debugger, so we can update instrumentation
#ifdef mxDebug
	machine->onBreak = debugBreak;
#endif

	// link our screen and machine objects together
	screen.machine = (void *) machine;
	machine->host = (void *) &screen;

	// set up the stack context for XS
	xsBeginHost(machine);
	{
		xsVars(1);

#ifdef mxInstrument
		// send the instrumentation headers, only if the debugger is connected
#ifdef mxDebug
		if (fxIsConnected(the))
#endif
			fxDescribeInstrumentation(machine, 0, NULL, NULL);

		// send the initial instrumentation data, only if the debugger is connected
#ifdef mxDebug
		if (fxIsConnected(the))
#endif
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
	if (screen.machine)
		xsDeleteMachine(screen.machine);	
}

/*
	Updates instrumentation (debugging statistics) for a running XS machine
*/
void instrumentMachine() {
#ifdef mxInstrument
	xsBeginHost(screen.machine);
	{
		// is the debugger connected?  If not, skip to avoid console output
#ifdef mxDebug
		if (fxIsConnected(the))
#endif
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
