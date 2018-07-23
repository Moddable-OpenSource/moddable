/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "piuPC.h"
#include "screen.h"
#include <dlfcn.h>

typedef struct PiuScreenStruct PiuScreenRecord, *PiuScreen;
typedef struct PiuScreenMessageStruct PiuScreenMessageRecord, *PiuScreenMessage;

struct PiuScreenStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	GtkWidget* gtkClip;
	GtkWidget* gtkDrawingArea;
	cairo_surface_t *surface;
    txScreen* screen;
	void* library;
	guint timer;
	gboolean timerRunning;
	gboolean touching;
	PiuScreenMessage firstMessage;
};

struct PiuScreenMessageStruct {
	PiuScreenMessage nextMessage;
    PiuScreen* self;
	int size;
	char* buffer;
};

static void PiuScreenBind(void* it, PiuApplication* application, PiuView* view);
static PiuScreenMessage PiuScreenCreateMessage(PiuScreen* self, char* buffer, int size);
static void PiuScreenDelete(void* it);
static void PiuScreenDeleteMessage(PiuScreen* self, PiuScreenMessage message);
static void PiuScreenDictionary(xsMachine* the, void* it);
static void PiuScreenMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuScreenQuit(PiuScreen* self);
static void PiuScreenUnbind(void* it, PiuApplication* application, PiuView* view);

static gboolean PiuScreen_postMessageAux(gpointer data);

static void fxScreenAbort(txScreen* screen);
static gboolean fxScreenAbortAux(gpointer data);
static void fxScreenBufferChanged(txScreen* screen);
static void fxScreenFormatChanged(txScreen* screen);
static gboolean fxScreenIdle(gpointer data);
static void fxScreenPost(txScreen* screen, char* message, int size);
static gboolean fxScreenPostAux(gpointer data);
static void fxScreenStart(txScreen* screen, double interval);
static void fxScreenStop(txScreen* screen);


gboolean onScreenDraw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	PiuScreen* self = (PiuScreen*)data;
	cairo_surface_t *surface = (*self)->surface;
	gint width = cairo_image_surface_get_width(surface);
	gint height = cairo_image_surface_get_height(surface);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	return TRUE;
}

gboolean onScreenMouseDown(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	PiuScreen* self = (PiuScreen*)data;
	txScreen* screen = (*self)->screen;
    (*self)->touching = TRUE;
	if (screen && screen->touch) 
		(*screen->touch)(screen, touchEventBeganKind, 0, ((GdkEventButton*)event)->x, ((GdkEventButton*)event)->y, 0);
	return TRUE;
}

gboolean onScreenMouseMoved(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	PiuScreen* self = (PiuScreen*)data;
	txScreen* screen = (*self)->screen;
	if ((*self)->touching) {
		if (screen && screen->touch) 
			(*screen->touch)(screen, touchEventMovedKind, 0, ((GdkEventButton*)event)->x, ((GdkEventButton*)event)->y, 0);
	}
	return FALSE;
}

gboolean onScreenMouseUp(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	PiuScreen* self = (PiuScreen*)data;
	txScreen* screen = (*self)->screen;
	if (screen && screen->touch) 
		(*screen->touch)(screen, touchEventEndedKind, 0, ((GdkEventButton*)event)->x, ((GdkEventButton*)event)->y, 0);
    (*self)->touching = FALSE;
	return TRUE;
}

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuScreenDispatchRecord = {
	"Screen",
	PiuScreenBind,
	PiuContentCascade,
	PiuContentDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuContentMeasureHorizontally,
	PiuContentMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuScreenUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuScreenHooks = {
	PiuScreenDelete,
	PiuScreenMark,
	NULL
};

void PiuScreenBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuScreen* self = (PiuScreen*)it;
	GtkFixed* gtkFixed = (*view)->gtkFixed;
	PiuContentBind(it, application, view);
	
	PiuDimension width = (*self)->coordinates.width;
	PiuDimension height = (*self)->coordinates.height;
	
    txScreen* screen = (txScreen*)malloc(sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
    if (screen == NULL) goto bail;
    memset(screen, 0, sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
    screen->view = self;
    screen->abort = fxScreenAbort;
    screen->bufferChanged = fxScreenBufferChanged;
    screen->formatChanged = fxScreenFormatChanged;
    screen->post = fxScreenPost;
    screen->start = fxScreenStart;
    screen->stop = fxScreenStop;
    screen->width = width;
    screen->height = height;

	cairo_surface_t *surface = cairo_image_surface_create_for_data(screen->buffer, CAIRO_FORMAT_RGB24, width, height, width * screenBytesPerPixel);
    if (surface == NULL) goto bail;
    
	GtkWidget* gtkDrawingArea = gtk_drawing_area_new();
    if (gtkDrawingArea == NULL) goto bail;
	gtk_widget_add_events(GTK_WIDGET(gtkDrawingArea), GDK_ALL_EVENTS_MASK);
	gtk_widget_set_size_request (gtkDrawingArea, width, height);
	g_signal_connect(G_OBJECT(gtkDrawingArea), "draw", G_CALLBACK(onScreenDraw), self);
	g_signal_connect(G_OBJECT(gtkDrawingArea), "button-press-event", G_CALLBACK(onScreenMouseDown), self);
	g_signal_connect(G_OBJECT(gtkDrawingArea), "button-release-event", G_CALLBACK(onScreenMouseUp), self);
	g_signal_connect(G_OBJECT(gtkDrawingArea), "motion-notify-event", G_CALLBACK(onScreenMouseMoved), self);
    
	GtkPiuClip* gtkClip = gtk_piu_clip_new((PiuContent*)self, GTK_WIDGET(gtkDrawingArea));
    if (gtkClip == NULL) goto bail;
    
	(*self)->gtkClip = GTK_WIDGET(gtkClip);
	(*self)->gtkDrawingArea = gtkDrawingArea;
	(*self)->surface = surface;
	(*self)->screen = screen;
	
	gtk_container_add(GTK_CONTAINER(gtkClip), GTK_WIDGET(gtkDrawingArea));
	gtk_container_add(GTK_CONTAINER(gtkFixed), GTK_WIDGET(gtkClip));
    gtk_widget_show(gtkDrawingArea);
    gtk_widget_show(GTK_WIDGET(gtkClip));
	return;
bail:
	if (gtkClip)
		gtk_widget_destroy(GTK_WIDGET(gtkClip));
	if (gtkDrawingArea)
		gtk_widget_destroy(gtkDrawingArea);
	if (surface)
		cairo_surface_destroy(surface);
	if (screen)
		free(screen);
}

PiuScreenMessage PiuScreenCreateMessage(PiuScreen* self, char* buffer, int size) 
{
	PiuScreenMessage message = (PiuScreenMessage)malloc(sizeof(PiuScreenMessageRecord));
	if (!message)
		return NULL;
	message->size = size;
	if (size < 0)
		message->buffer = buffer;
	else {
		if (!size)
			size = strlen(buffer) + 1;;
		message->buffer = malloc(size);
		if (!message->buffer) {
			free(message);
			return NULL;
		}
		memcpy(message->buffer, buffer, size);
	}
	message->nextMessage = (*self)->firstMessage;
	(*self)->firstMessage = message;
	message->self = self;
	return message;
}

void PiuScreenDelete(void* it) 
{
}

void PiuScreenDeleteMessage(PiuScreen* self, PiuScreenMessage message) 
{
	PiuScreenMessage* address = &((*self)->firstMessage);
	PiuScreenMessage current;
	while ((current = *address)) {
		if (current == message) {
			*address = message->nextMessage;
			free(message->buffer);
			free(message);
			break;
		}
		address = &(current->nextMessage);
	}
}

void PiuScreenDictionary(xsMachine* the, void* it) 
{
}

void PiuScreenMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuContentMark(the, it, markRoot);
}

void PiuScreenQuit(PiuScreen* self) 
{
	txScreen* screen = (*self)->screen;
	if (screen && screen->quit)
		(*screen->quit)(screen);
	gtk_widget_queue_draw_area((*self)->gtkDrawingArea, 0, 0, screen->width, screen->height);
    if ((*self)->firstMessage) {
		PiuScreenMessage current = (*self)->firstMessage;
		while (current) {
			PiuScreenMessage next = current->nextMessage;
			g_idle_remove_by_data(current);
			free(current);
			current = next;
		}
		(*self)->firstMessage = NULL;
	}
    if ((*self)->timerRunning) {
        g_source_remove((*self)->timer);
        (*self)->timerRunning = FALSE;
	}
	if ((*self)->library) {
		dlclose((*self)->library);
		(*self)->library = NULL;
	}
}

void PiuScreenUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuScreen* self = (PiuScreen*)it;
	GtkWidget* gtkClip = (*self)->gtkClip;
	GtkWidget* gtkDrawingArea = (*self)->gtkDrawingArea;
	cairo_surface_t *surface = (*self)->surface;
	txScreen* screen = (*self)->screen;
	PiuScreenQuit(self);
	if (screen) {
		free(screen);
		(*self)->screen = NULL;
	}
	if (surface) {
		cairo_surface_destroy(surface);
		(*self)->surface = NULL;
	}
	if (gtkDrawingArea) {
		gtk_widget_destroy(gtkDrawingArea);
		(*self)->gtkDrawingArea = NULL;
	}
	if (gtkClip) {
		gtk_widget_destroy(gtkClip);
		(*self)->gtkClip = NULL;
	}
	PiuContentUnbind(it, application, view);
}

void PiuScreen_create(xsMachine* the)
{
	PiuScreen* self;
	xsVars(2);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuScreenRecord));
	self = PIU(Screen, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuScreenHooks);
	(*self)->dispatch = (PiuDispatch)&PiuScreenDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuScreenDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuScreen_get_running(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = (*self)->library ? xsTrue : xsFalse;
}
	
void PiuScreen_launch(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsStringValue path = NULL;
	void* library = NULL;
	txScreenLaunchProc launch;
	xsTry {
		path = xsToString(xsArg(0));
		library = dlopen(path, RTLD_NOW);
		if (library == NULL) {
			xsUnknownError("%s", dlerror());
		}
		launch = (txScreenLaunchProc)dlsym(library, "fxScreenLaunch");
		if (launch == NULL) {
			xsUnknownError("%s", dlerror());
		}
		(*self)->library = library;
		(*launch)((*self)->screen);
	}
	xsCatch {
		if (library != NULL)
			dlclose(library);
		xsThrow(xsException);
	}
}

void PiuScreen_postMessage(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	PiuScreenMessage message;
	if (xsToInteger(xsArgc) > 1) {
		int worker = (int)xsToInteger(xsArg(1));
		message = PiuScreenCreateMessage(self, xsMarshallAlien(xsArg(0)), 0 - worker);
	}
	else {
		if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
			int size = (int)xsGetArrayBufferLength(xsArg(0));
			message = PiuScreenCreateMessage(self, xsToArrayBuffer(xsArg(0)), size);
		}
		else {
			xsStringValue string = xsToString(xsArg(0));
			message = PiuScreenCreateMessage(self, string, 0);
		}
	}
	if (message)
		g_idle_add(PiuScreen_postMessageAux, message);
}

gboolean PiuScreen_postMessageAux(gpointer data)
{
	PiuScreenMessage message = (PiuScreenMessage)data;
	PiuScreen* self = message->self;
	txScreen* screen = (*self)->screen;
	if (screen && screen->invoke)
		(*screen->invoke)(screen, message->buffer, message->size);
	PiuScreenDeleteMessage(self, message);
    return FALSE;
}

void PiuScreen_quit(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	PiuScreenQuit(self);
}

void fxScreenAbort(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	g_idle_add(fxScreenAbortAux, self);
}

gboolean fxScreenAbortAux(gpointer data)
{
	PiuScreen* self = (PiuScreen*)data;
	PiuScreenQuit(self);
    return FALSE;
}

void fxScreenBufferChanged(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	gtk_widget_queue_draw_area((*self)->gtkDrawingArea, 0, 0, screen->width, screen->height);
}

int fxScreenCreateWorker(txScreen* screen, char* name)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	int worker = 0;
	xsBeginHost((*self)->the);
	{
		xsVars(3);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onCreateWorker)) {
			xsVar(1) = xsReference((*self)->reference);
			xsVar(2) = xsString(name);
			xsResult = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
			worker = xsToInteger(xsResult);
		}
	}
	xsEndHost((*self)->the);
	return worker;
}

void fxScreenDeleteWorker(txScreen* screen, int worker)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	xsBeginHost((*self)->the);
	{
		xsVars(3);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onDeleteWorker)) {
			xsVar(1) = xsReference((*self)->reference);
			xsVar(2) = xsInteger(worker);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
		}
	}
	xsEndHost((*self)->the);
}

void fxScreenFormatChanged(txScreen* screen)
{
}

gboolean fxScreenIdle(gpointer data)
{
	txScreen* screen = (txScreen*)data;
	if (screen->idle) 
		(*screen->idle)(screen);
	return TRUE;
}

void fxScreenPost(txScreen* screen, char* buffer, int size)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	PiuScreenMessage message = PiuScreenCreateMessage(self, buffer, size);
	if (message)
		g_idle_add(fxScreenPostAux, message);
}

gboolean fxScreenPostAux(gpointer data)
{
	PiuScreenMessage message = (PiuScreenMessage)data;
	PiuScreen* self = message->self;
	if ((*self)->behavior) {
		xsBeginHost((*self)->the);
		xsVars(4);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onMessage)) {
			xsVar(1) = xsReference((*self)->reference);
			if (message->size < 0) {
				xsVar(2) = xsDemarshallAlien(message->buffer);
				xsVar(3) = xsInteger(0 - message->size);
			}
			else if (message->size > 0)
				xsVar(2) = xsArrayBuffer(message->buffer, message->size);
			else
				xsVar(2) = xsString(message->buffer);
			(void)xsCallFunction3(xsResult, xsVar(0), xsVar(1), xsVar(2), xsVar(3));
		}
		xsEndHost((*self)->the);
	}
	PiuScreenDeleteMessage(self, message);
    return FALSE;
}

void fxScreenStart(txScreen* screen, double interval)
{
	PiuScreen* self = (PiuScreen*)screen->view;
    if (!(*self)->timerRunning) {
        (*self)->timer = g_timeout_add(20, fxScreenIdle, (*self)->screen);
        (*self)->timerRunning = TRUE;
	}
}

void fxScreenStop(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
    if ((*self)->timerRunning) {
        g_source_remove((*self)->timer);
        (*self)->timerRunning = FALSE;
	}
}

