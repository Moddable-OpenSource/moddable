/*
 * Copyright (c) 2020 Chris Midgley
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

/*
    This file implements a cli-win platform for Moddable that supports an event loop and dynamically loaded
    archives (mods).  It implements the C main function, sets up XS machine and starts a Windows message pump.
    Upon a WM_CLOSE message (which is intercepted and sent upon ^C) it cleanly shuts down the XS machine.  It also
    maintains the instrumentation updates, using a timer, on a once/second interval.
*/

#include "xsAll.h"
#include <process.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "cli.h"
#include "sysinfoapi.h"
#include "screen.h"

extern txScreen screen;

unsigned int messagePumpThreadId = 0;       // thread ID of the thread running the windows message pump

#ifdef mxInstrument
#define INSTRUMENTATION_FREQUENCY 1000       // frequency of instrumentation updates in ms
#define WM_APP_INSTRUMENTATION (WM_APP + 1) // ID of our private instrumentation message

static VOID CALLBACK sendInstrumentationMessage(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    PostMessage(NULL, WM_APP_INSTRUMENTATION, 0, 0);
}
#endif

/*
	Windows specific implementation to map an archive (mod) into memory.  Accepts the path to the 
	archive, and returns either a pointer to the memory mapped image, or NULL if there was a failure during the loading
	of the file.
*/
void *loadArchive(char *archivePath) {
    HANDLE archiveFile = INVALID_HANDLE_VALUE;
    HANDLE archiveMapping = INVALID_HANDLE_VALUE;

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
    Message pump that manages the XS virtual machine.  The machine is started up, then the Windows message
    pump is executed allow for modules such as Timer to be used.  Also maintains instrumentation to xsbug once
    per second, triggered by the WM_APP_INSTRUMENTATION message from a timer.  Shuts down the VM when a WM_CLOSE
    message is received, and signals completion using the terminateEvent event.
*/
void messagePump(char *pathToMod) {
    MSG msg;

    // save away our thread ID - this is done so that the ^C interception can send us a WM_CLOSE message
    messagePumpThreadId = GetCurrentThreadId();

    // start up our VM
    int error = startMachine(pathToMod);

    // set up a timer for the instrumentation
#ifdef mxInstrument
    SetTimer(NULL, 1, INSTRUMENTATION_FREQUENCY, sendInstrumentationMessage);
#endif

    // process the windows message loop until terminated
    while( GetMessage(&msg, NULL, 0, 0) > 0 ) {
#ifdef mxInstrument
        if (msg.message == WM_APP_INSTRUMENTATION) {
            // make sure we don't issue instrumentation more often than once/INSTRUMENTATION_FREQUENCY - if 
            // the host gets busy, a bunch of messages can be queued up, and this drops the extra ones on the floor
            static DWORD instrumentTime = 0;
            DWORD currentTime = GetTickCount();
            
            if (instrumentTime < currentTime) {
                instrumentTime = currentTime + INSTRUMENTATION_FREQUENCY;
                instrumentMachine();
            }

            continue;
        }
#endif	

        // do we need to shut down?
        if (msg.message == WM_CLOSE) {
            break;
        }

        // go ahead and let the message get dispatched
		TranslateMessage(&msg);
		DispatchMessage(&msg);

        // if the running operation never stops sending messages (this can happen such as with workers that
        // create workers on every loop), the timer gets starved because Windows does not process timer
        // events unless the queue is empty.  This will ensure that timers continue to operate even
        // when the loop is busy
        while (PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // similar problem as WM_TIMER for the WM_CLOSE message - it does get processed eventually, but to 
        // speed it up we check here
        if (PeekMessage(&msg, NULL, WM_CLOSE, WM_CLOSE, PM_REMOVE)) 
            break;
	}
    // done - end our XS machine
    endMachine();
}

/*
    Handler for intercepting the ctrl-C shutdown of the process, and sends the WM_CLOSE message so the message pump can
    initiate a shutdown
*/
BOOL WINAPI ctrlHandler(_In_ DWORD dwCtrlType) {
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
    	fprintf(stderr, "\nShutting down\n");
        PostMessage(((txMachine *) screen.machine)->window, WM_CLOSE, 0, 0);

        return TRUE;
    default:
        // Pass signal on to the next handler
        return FALSE;
    }
}


/*
    main - accepts a single optional command line argument that is the path to a mod to load

    Sets a ^C handler (for clearn shutdown) and starts up the VM and message pump
*/
int main(int argc, char* argv[]) {

    // take control over ^C handling
    SetConsoleCtrlHandler(ctrlHandler, TRUE);

    // start up the XS VM and run the message pump
    messagePump((argc > 1) ? argv[1] : NULL);

    return 0;
}
