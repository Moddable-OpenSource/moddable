/*
 * Copyright (c) 2016-2017 Chris Midgley
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
    This file implements a x-cli-lin platform for Moddable that supports an event loop and dynamically loaded
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
#include <gtk/gtk.h>

static GtkApplication *gxApplication = NULL;
static GtkWindow *gxWindow;
static GtkApplication *app;

#ifdef mxInstrument
/*
    Called from a once/second timer (see runMessageLoop) and is used to trigger instrumentation for debugging
*/
gboolean  sendInstrumentation(gpointer userData) {
    instrumentMachine();   
	return TRUE;
}
#endif


void onApplicationActivate(GtkApplication *app, gpointer it)
{
    // start up our VM
    startMachine(NULL);

	// set up a timer for instrumentation updates once/second
	g_timeout_add(1000, sendInstrumentation, NULL);
}

void onApplicationShutdown(GtkApplication *app, gpointer it)
{
    // done - end our XS machine
    endMachine();
}

void onApplicationStartup(GtkApplication *app)
{
	// create a window to keep our application running, but don't show it (so we logically remain a console app)
	gxWindow = (GtkWindow*)gtk_application_window_new(app);
}

/*
    Handler for intercepting the ctrl-C shutdown of the process, and instructs the CFRunLoop to shutdown
*/
void ctrlHandler(int sig) {
    g_application_quit(G_APPLICATION(app));
	fprintf(stderr, "\nShutting down\n");
}


/*
    main - accepts a single optional command line argument that is the path to a mod to load

    Sets a ^C handler (for clearn shutdown) and starts up the GTK application

	Note: This depends on GTK, which also means X, as the Moddable 'lin' machine depends deeply on GTK.  This means
	that this "cli" application will not work on SSH or headless Linux without installing X and likely using
	xvfb (x frame buffer).  
*/
int main(int argc, char** argv)
{
	int status;

    // take control over ^C handling
    signal(SIGINT, &ctrlHandler);

	app = gxApplication = gtk_application_new("tech.moddable.cli", G_APPLICATION_HANDLES_OPEN);
	g_signal_connect(app, "startup", G_CALLBACK(onApplicationStartup), NULL);
  	g_signal_connect(app, "activate", G_CALLBACK(onApplicationActivate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(onApplicationShutdown), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);
	return status;
}
