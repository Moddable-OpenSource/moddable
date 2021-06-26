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
    This file implements a cli-lin platform for Moddable that supports an event loop and dynamically loaded
    archives (mods).  It implements the C main function, and starts up GTK with an invisible window to run
	the actual program.  It manages clearn termination upon receipt of ^C, and performs instrumentaton updates
	once/second.

	This code is built upon GTK3 because the core of the Moddable "lin" platform is built upon GTK.  This means
	that this "cli" application will not work on SSH or headless Linux without installing X and likely using
	xvfb (x frame buffer).  

*/

#include "xsAll.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "cli.h"
#include <signal.h>
#include <gtk/gtk.h>
#include <sys/mman.h>
#include <fcntl.h>

static GtkApplication *gxApplication = NULL;
static GtkWindow *gxWindow;
static GtkApplication *app;

#ifdef mxInstrument
/*
	Updates Moddable instrumentation once/second - called from GTK timer (see onApplicationActivate for where
	the timer is started)
*/
gboolean  sendInstrumentation(gpointer userData) {
    instrumentMachine();   
	return TRUE;
}
#endif

/*
	Map an archive (mod) into memory.  Accepts the path to the archive, and returns either a pointer to the 
	memory mapped image, or NULL if there was a failure during the loading of the file.
*/
void *loadArchive(char *archivePath) {
	struct stat statbuf;
	int archiveFile;

	archiveFile = open(archivePath, O_RDWR);
	if (archiveFile < 0) {
		fprintf(stderr, "Filed to load archive %s (error %s)\n", archivePath, strerror(errno));
		return NULL;
	}
	fstat(archiveFile, &statbuf);
	void *archiveMapped = mmap(NULL, statbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, archiveFile, 0);
	if (archiveMapped == MAP_FAILED) {
		fprintf(stderr, "Filed to map archive %s (error %s)\n", archivePath, strerror(errno));
		return NULL;
	}
	return archiveMapped;
}

/*
	GTK handler for when the application is shutdown.  Cleanly terminates the XS machine.
*/
void onApplicationShutdown(GtkApplication *app, gpointer it) {
    // done - end our XS machine
    endMachine();
}

/*
	GTK handler for application startup, which is the first event we process when the app starts up.  Creates a window (but does not show it)
	in order to keep the GTK application running until we are ready for a shutdown.  Also sets up the instrumentation timer (once/second for
	instrumentation updates in the debugger)
*/
void onApplicationStartup(GtkApplication *app) {
	// create a window to keep our application running, but don't show it (so we logically remain a console app)
	gxWindow = (GtkWindow*)gtk_application_window_new(app);

	// set up a timer for instrumentation updates once/second
#ifdef mxInstrument
	g_timeout_add(1000, sendInstrumentation, NULL);
#endif
}

/*
	GTK handler for when the application is activated, which happens if there are no commmand line arguments. We simply start up the 
	VM with no mods.
*/
void onApplicationActivate(GtkApplication *app, gpointer it) {
    startMachine(NULL);

}

/*
	GTK handler for when a command line is provided.  We start up the VM using the path to the mod from the command line
*/
void onApplicationOpen(GtkApplication *app, GFile **files, gint c, const gchar *hint, gpointer it) {
    // start up our VM, optionally with path to archive (mod)
    startMachine(g_file_get_path(files[0]));
}


/*
    Handler for intercepting the ctrl-C shutdown of the process, and instructs the CFRunLoop to shutdown
*/
void ctrlHandler(int sig) {
	fprintf(stderr, "\nShutting down\n");
    g_application_quit(G_APPLICATION(app));
}

/*
    main - accepts a single optional command line argument that is the path to a mod to load

    Sets a ^C handler (for clearn shutdown) and starts up the GTK application
*/
int main(int argc, char** argv) {
	int status;

    // take control over ^C handling
    signal(SIGINT, &ctrlHandler);

	app = gxApplication = gtk_application_new("tech.moddable.cli", G_APPLICATION_HANDLES_OPEN);
	g_signal_connect(app, "startup", G_CALLBACK(onApplicationStartup), NULL);
  	g_signal_connect(app, "activate", G_CALLBACK(onApplicationActivate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(onApplicationShutdown), NULL);
  	g_signal_connect(app, "open", G_CALLBACK(onApplicationOpen), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);
	return status;
}
