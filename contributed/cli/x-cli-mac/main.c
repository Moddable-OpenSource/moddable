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
    This file implements a x-cli-mac platform for Moddable that supports an event loop and dynamically loaded
    archives (mods).  It implements the C main function, sets up XS machine and starts a Mac CFRunLoop message loop.
    Upon a close message (which is intercepted and sent upon ^C) it cleanly shuts down the XS machine.  It also
    maintains the instrumentation updates, using a timer, on a once/second interval.
*/

#include "xsAll.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "machine.h"
#include <signal.h>

#ifdef mxInstrument
/*
    Called from a once/second timer (see runMessageLoop) and is used to trigger instrumentation for debugging
*/
static void sendInstrumentation(CFRunLoopTimerRef cfTimer, void *info) {
    instrumentMachine();   
}
#endif


/*
    Message loop that manages the XS virtual machine.  The machine is started up, a timer is created to update
    instrumentation every second, and then the Mac message loop is executed.
*/
int runMessageLoop(char *pathToMod)
{
    // start up our VM
    int error = startMachine(pathToMod);

    // set up a timer for the instrumentation
#ifdef mxInstrument
	CFRunLoopTimerRef timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 1,
					1, 0, 0, sendInstrumentation, NULL);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
#endif

    // process the message loop until terminated
    CFRunLoopRun();

    // done - end our XS machine
    endMachine();

    return error;
}

/*
    Handler for intercepting the ctrl-C shutdown of the process, and instructs the CFRunLoop to shutdown
*/
void ctrlHandler(int sig) {
    CFRunLoopStop(CFRunLoopGetCurrent());
}


/*
    main - accepts a single optional command line argument that is the path to a mod to load

    Sets a ^C handler (for clearn shutdown) and starts up the VM and message pump
*/
int main(int argc, char* argv[]) {

    // take control over ^C handling
    signal(SIGINT, &ctrlHandler);

    // start up the XS VM and run the message loop
    int error = runMessageLoop((argc > 1) ? argv[1] : NULL);

    return error;
}
