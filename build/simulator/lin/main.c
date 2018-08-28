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

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "../screen.h"

extern GResource *screens_get_resource (void);

typedef struct {
	int x;
	int y;
	int width;
	int height;
	gchar* title;
	gchar path[1];
} txMockup;

enum {
	JSON_FILE = 0,
	JSON_OBJECT, 
	JSON_NAME,
	JSON_COLON,
	JSON_VALUE,
	JSON_NUMBER,
	JSON_STRING,
	JSON_COMMA,
};

static void fxLibraryOpen();
static void fxLibraryClose();

static txMockup* fxMockupCreate(gchar* string, gsize size, gchar* path);
static void fxMockupCreateAux(txMockup* mockup, gchar* nameFrom, gchar* nameTo, gchar* valueFrom, gchar* valueTo);

static void fxScreenAbort(txScreen* screen);
static gboolean fxScreenAbortAux(gpointer it);
static void fxScreenBufferChanged(txScreen* screen);
static void fxScreenFormatChanged(txScreen* screen);
static gboolean fxScreenIdle(gpointer data);
static void fxScreenStart(txScreen* screen, double interval);
static void fxScreenStop(txScreen* screen);

static void onApplicationAbout(GSimpleAction *action, GVariant *parameter, gpointer app);
static void onApplicationActivate(GtkApplication *app, gpointer it);
static void onApplicationOpen(GtkApplication *app, GFile **files, gint n_files, const gchar *hint, gpointer it);
static void onApplicationQuit(GSimpleAction *action, GVariant *parameter, gpointer app);
static void onApplicationShutdown(GtkApplication *app, gpointer it);
static void onApplicationStartup(GtkApplication *app);
static void onApplicationSupport(GSimpleAction *action, GVariant *parameter, gpointer app);

static void onFileClose(GSimpleAction *action, GVariant *parameter, gpointer app);
static void onFileOpen(GSimpleAction *action, GVariant *parameter, gpointer app);

static gboolean onScreenConfigure(GtkWidget *widget, GdkEvent *event, gpointer refcon);
static gboolean onScreenDraw(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean onScreenMouseDown(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean onScreenMouseMoved(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean onScreenMouseUp(GtkWidget *widget, GdkEvent *event, gpointer data);
static void onScreenSelect(GSimpleAction *action, GVariant *parameter, gpointer app);

static gboolean onWindowConfigure(GtkWidget *widget, GdkEvent *event, gpointer refcon);
static gboolean onWindowStateEvent(GtkWidget *widget, GdkEventWindowState *event);

static GtkApplication *gxApplication = NULL;
static GSimpleAction* gxCloseAction = NULL;

static char gxConfigPath[PATH_MAX] = "";

static char* gxFormatNames[pixelFormatCount] = {
	"16-bit RGB 565 Little Endian",
	"16-bit RGB 565 Big Endian",
	"8-bit Gray",
	"8-bit RGB 332",
	"4-bit Gray",
	"4-bit Color Look-up Table",
};

static void* gxLibrary = NULL;
static char gxLibraryPath[PATH_MAX] = "";

static txMockup* gxMockups[64];
static GtkWidget *gxMockupImage = NULL;
static int gxMockupIndex = -1; 

static const char* gxResourcePrefix = "/tech/moddable/simulator/screens";
static GResource *gxResource = NULL;

static txScreen* gxScreen = NULL;
static cairo_surface_t *gxScreenSurface = NULL;

static guint gxTimer = 0;
static gboolean gxTimerRunning = FALSE;
static gboolean gxTouching = FALSE;

static GtkWindow *gxWindow;
static gboolean gxWindowFullscreen = FALSE;
static gboolean gxWindowMaximized = FALSE;
static int gxWindowX = -1;
static int gxWindowY = -1;
static int gxWindowWidth = -1;
static int gxWindowHeight = -1;

void fxLibraryOpen()
{
	GtkWidget *dialog;
	gxLibrary = dlopen(gxLibraryPath, RTLD_NOW);
	if (!gxLibrary) {
		goto error;
	}
	txScreenLaunchProc launch = (txScreenLaunchProc)dlsym(gxLibrary, "fxScreenLaunch");
	if (!launch) {
    	dlclose(gxLibrary);
    	gxLibrary = NULL;
		goto error;
	}
    (*launch)(gxScreen);
	g_simple_action_set_enabled(gxCloseAction, TRUE);
	return;
error:
    dialog = gtk_message_dialog_new(gxWindow, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", dlerror());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
} 

void fxLibraryClose()
{
	if (gxScreen->quit) 
		(*gxScreen->quit)(gxScreen);
	if (gxLibrary) {
    	dlclose(gxLibrary);
    	gxLibrary = NULL;
    }
    fxScreenBufferChanged(gxScreen);
	g_simple_action_set_enabled(gxCloseAction, FALSE);
	gtk_window_set_title(gxWindow, "Screen Test");
}

txMockup* fxMockupCreate(gchar* string, gsize size, gchar* path)
{
	gsize offset = 0;
	txMockup* mockup = (txMockup*)calloc(1, sizeof(txMockup) + strlen(path));
	gchar* nameFrom;
	gchar* nameTo;
	gchar* valueFrom;
	gchar* valueTo;
	int state = JSON_FILE;
	while (offset < size) {
		gchar c = *string;
		switch (c) {
		case '{':
			state = JSON_OBJECT;
			break;
		case '}':
			state = JSON_FILE;
			break;
		case ':':
			if (state == JSON_COLON)
				state = JSON_VALUE;
			break;
		case ',':
			if (state == JSON_NUMBER) {
				valueTo = string;
				fxMockupCreateAux(mockup, nameFrom, nameTo, valueFrom, valueTo);
				state = JSON_OBJECT;
			}
			else if (state == JSON_COMMA) {
				state = JSON_OBJECT;
			}
			break;
		case '"':
			if (state == JSON_OBJECT) {
				nameFrom = string + 1;
				state = JSON_NAME;
			}
			else if (state == JSON_NAME) {
				nameTo = string;
				state = JSON_COLON;
			}
			else if (state == JSON_VALUE) {
				valueFrom = string + 1;
				state = JSON_STRING;
			}
			else if (state == JSON_STRING) {
				valueTo = string;
				fxMockupCreateAux(mockup, nameFrom, nameTo, valueFrom, valueTo);
				state = JSON_COMMA;
			}
			break;
		default:
			if (32 < c) {
				if (state == JSON_VALUE) {
					valueFrom = string;
					state = JSON_NUMBER;
				}
			}
			else {
				if (state == JSON_NUMBER) {
					valueTo = string;
					fxMockupCreateAux(mockup, nameFrom, nameTo, valueFrom, valueTo);
					state = JSON_COMMA;
				}
			}
			break;
		}
		string++;
		offset++;
	}
	strcpy(mockup->path, path);
	return mockup;
}

void fxMockupCreateAux(txMockup* mockup, gchar* nameFrom, gchar* nameTo, gchar* valueFrom, gchar* valueTo)
{
	if (!strncmp("title", nameFrom, nameTo - nameFrom)) {
		long length = valueTo - valueFrom;
		mockup->title = (char*)malloc(length + 1);
		memcpy(mockup->title, valueFrom, length);
		mockup->title[length] = 0;
	}
	else if (!strncmp("x", nameFrom, nameTo - nameFrom))
		mockup->x = atoi(valueFrom);
	else if (!strncmp("y", nameFrom, nameTo - nameFrom))
		mockup->y = atoi(valueFrom);
	else if (!strncmp("width", nameFrom, nameTo - nameFrom))
		mockup->width = atoi(valueFrom);
	else if (!strncmp("height", nameFrom, nameTo - nameFrom))
		mockup->height = atoi(valueFrom);
}

void fxScreenAbort(txScreen* screen)
{
	g_idle_add(fxScreenAbortAux, NULL);
}

gboolean fxScreenAbortAux(gpointer it)
{
	fxLibraryClose();
    gxLibraryPath[0] = 0;
    return FALSE;
}

void fxScreenBufferChanged(txScreen* screen)
{
	unsigned char* p = screen->buffer;
	int x, y, width = screen->width, height = screen->height;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			unsigned char r = p[0];
			unsigned char b = p[2];
			*p++ = b;
			p++;
			*p++ = r;
			p++;
		}
	}
	gtk_widget_queue_draw_area(screen->view, 0, 0, width, height);
}

void fxScreenFormatChanged(txScreen* screen)
{
	char title[PATH_MAX];
	char* begin;
	char* end;
	end = strrchr(gxLibraryPath, '/');
	*end = 0;
	begin = strrchr(gxLibraryPath, '/');
	strcpy(title, "Screen Test - ");
	strcat(title, begin + 1);
	strcat(title, " - ");
	strcat(title, gxFormatNames[screen->pixelFormat]);
	*end = '/';
	gtk_window_set_title(gxWindow, title);
}

gboolean fxScreenIdle(gpointer data)
{
	if (gxScreen->idle) 
		(*gxScreen->idle)(gxScreen);
	return TRUE;
}

void fxScreenStart(txScreen* screen, double interval)
{
    if (!gxTimerRunning) {
        gxTimer = g_timeout_add(20, fxScreenIdle, NULL);
        gxTimerRunning = TRUE;
	}
}

void fxScreenStop(txScreen* screen)
{
    if (gxTimerRunning) {
        g_source_remove(gxTimer);
        gxTimerRunning = FALSE;
	}
}

void onApplicationAbout(GSimpleAction *action, GVariant *parameter, gpointer app)
{
    GtkWidget *dialog = gtk_message_dialog_new(gxWindow, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Screen Test");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Copyright 2017 Moddable Tech, Inc.\nAll rights reserved.\n\nThis application incorporates open source software from Marvell, Inc. and others.");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void onApplicationActivate(GtkApplication *app, gpointer it)
{
	gtk_window_present(gxWindow);
}

void onApplicationOpen(GtkApplication *app, GFile **files, gint n_files, const gchar *hint, gpointer it)
{
    fxLibraryClose();
	realpath(g_file_get_path(files[0]), gxLibraryPath);
	fxLibraryOpen();
	gtk_window_present(gxWindow);
}

void onApplicationQuit(GSimpleAction *action, GVariant *parameter, gpointer app)
{
    g_application_quit(G_APPLICATION(app));
}

void onApplicationShutdown(GtkApplication *app, gpointer it)
{
	GKeyFile* keyfile = g_key_file_new();
	g_key_file_set_boolean(keyfile, "window", "fullscreen", gxWindowFullscreen);
	g_key_file_set_boolean(keyfile, "window", "maximized", gxWindowMaximized);
	g_key_file_set_integer(keyfile, "window", "x", gxWindowX);
	g_key_file_set_integer(keyfile, "window", "y", gxWindowY);
	g_key_file_set_integer(keyfile, "window", "width", gxWindowWidth);
	g_key_file_set_integer(keyfile, "window", "height", gxWindowHeight);
	g_key_file_set_integer(keyfile, "mockup", "index", gxMockupIndex);
	g_key_file_save_to_file(keyfile, gxConfigPath, NULL);
	g_key_file_unref(keyfile);
	g_resources_unregister(gxResource);
}

void onApplicationStartup(GtkApplication *app)
{
	static const GActionEntry app_actions[] = {
	  { "about", onApplicationAbout },
	  { "open", onFileOpen },
	  { "quit", onApplicationQuit },
	  { "support", onApplicationSupport }
	};

	GMenu *menubar, *menu, *section;
	GMenuItem* item;
	
	// CONFIG
	char* home = getenv("XDG_CONFIG_HOME");
	if (home)
		strcpy(gxConfigPath, home);
	else {
		home = getenv("HOME");
		strcpy(gxConfigPath, home);
		strcat(gxConfigPath, "/.config");
	}
	strcat(gxConfigPath, "/tech.moddable.simulator.ini");
	
	GKeyFile* keyfile = g_key_file_new();
	if (g_key_file_load_from_file(keyfile, gxConfigPath, G_KEY_FILE_NONE, NULL)) {
		GError *error;
		error = NULL;
		gxWindowFullscreen = g_key_file_get_boolean(keyfile, "window", "fullscreen", &error);
		if (error) {
			g_clear_error(&error);
			gxWindowFullscreen = 0;
		}
		gxWindowMaximized = g_key_file_get_boolean(keyfile, "window", "maximized", &error);
		if (error) {
			g_clear_error(&error);
			gxWindowMaximized = 0;
		}
		gxWindowX = g_key_file_get_integer(keyfile, "window", "x", &error);
		if (error) {
			g_clear_error(&error);
			gxWindowX = -1;
		}
		gxWindowY = g_key_file_get_integer(keyfile, "window", "y", &error);
		if (error) {
			g_clear_error(&error);
			gxWindowY = -1;
		}
		gxWindowWidth = g_key_file_get_integer(keyfile, "window", "width", &error);
		if (error) {
			g_clear_error(&error);
			gxWindowWidth = -1;
		}
		gxWindowHeight = g_key_file_get_integer(keyfile, "window", "height", &error);
		if (error) {
			g_clear_error(&error);
			gxWindowHeight = -1;
		}
		gxMockupIndex = g_key_file_get_integer(keyfile, "mockup", "index", &error);
		if (error) {
			g_clear_error(&error);
			gxMockupIndex = -1;
		}
	}
	g_key_file_unref(keyfile);
		
	// RESOURCES
	gxResource = screens_get_resource();
	g_resources_register(gxResource);

	// MENU BAR
	menubar = g_menu_new();
	menu = g_menu_new();
	section = g_menu_new();
	item = g_menu_item_new("Open", "app.open");
	g_menu_item_set_attribute(item, "accel", "s", "<Ctrl>O");
	g_menu_append_item(section, item);
	item = g_menu_item_new("Close", "app.close");
	g_menu_item_set_attribute(item, "accel", "s", "<Ctrl>W");
	g_menu_append_item(section, item);
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_object_unref(section);
	section = g_menu_new();
	item = g_menu_item_new("Quit", "app.quit");
	g_menu_item_set_attribute(item, "accel", "s", "<Ctrl>Q");
	g_menu_append_item(section, item);
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_object_unref(section);
	g_menu_insert_submenu(menubar, 0, "File", G_MENU_MODEL(menu));
	g_object_unref(menu);
	menu = g_menu_new();
	int index = 0;
	char **names = g_resource_enumerate_children(gxResource, gxResourcePrefix, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	char **name = names;
	while (*name) {
		char path[PATH_MAX], *dot;
		strcpy(path, gxResourcePrefix);
		strcat(path, "/");
		strcat(path, *name);
		dot = strrchr(path, '.');
		if (dot && !strcmp(dot, ".json")) {
			GBytes *bytes = g_resources_lookup_data(path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
			strcpy(dot, ".png");
			if (bytes) {
				gsize size;
				gconstpointer data = g_bytes_get_data(bytes, &size);
				txMockup* mockup = fxMockupCreate((gchar*)data, size, path);
				if (mockup) {
					gxMockups[index] = mockup;
					item = g_menu_item_new(mockup->title, NULL);
					g_menu_item_set_action_and_target_value(item, "app.size", g_variant_new_int32(index));
					g_menu_append_item(menu, item);
					g_object_unref(item);
					index++;
				}
			}
			g_bytes_unref(bytes);
		}
		name++;
	}
	g_strfreev(names);
	g_menu_insert_submenu(menubar, 1, "Size", G_MENU_MODEL(menu));
	if ((gxMockupIndex < 0) || (index <= gxMockupIndex))
		gxMockupIndex = 4;
	g_object_unref(menu);
	
	menu = g_menu_new();
	section = g_menu_new();
	g_menu_append(section, "Moddable Developer", "app.support");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_object_unref(section);
	section = g_menu_new();
	g_menu_append(section, "About", "app.about");
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_object_unref(section);
	g_menu_insert_submenu(menubar, 2, "Help", G_MENU_MODEL(menu));
	g_object_unref(menu);
	gtk_application_set_menubar(app, G_MENU_MODEL(menubar));
	g_object_unref(menubar);
	
	// ACTIONS
	g_action_map_add_action_entries(G_ACTION_MAP(app), app_actions, G_N_ELEMENTS(app_actions), app);
	gxCloseAction =  g_simple_action_new("close", NULL);
	g_signal_connect(G_OBJECT(gxCloseAction), "activate", G_CALLBACK(onFileClose), app);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(gxCloseAction));
	g_simple_action_set_enabled(gxCloseAction, FALSE);
	GSimpleAction* action =  g_simple_action_new_stateful("size", G_VARIANT_TYPE("i"), g_variant_new_int32(gxMockupIndex));
	g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(onScreenSelect), app);
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
	g_object_unref(action);
	
	// WINDOW
	gxWindow = (GtkWindow*)gtk_application_window_new(app);
	g_signal_connect(G_OBJECT(gxWindow), "configure-event", G_CALLBACK(onWindowConfigure), NULL);
	g_signal_connect(G_OBJECT(gxWindow), "window-state-event", G_CALLBACK(onWindowStateEvent), NULL);
	gtk_window_set_icon_name(gxWindow, "tech-moddable-simulator");
	gtk_window_set_title(gxWindow, "Screen Test");
	gtk_window_set_default_size(gxWindow, gxWindowWidth, gxWindowHeight);
	gtk_window_move(gxWindow, gxWindowX, gxWindowY);
	if (gxWindowMaximized)
		gtk_window_maximize(gxWindow);
	if (gxWindowFullscreen)
		gtk_window_fullscreen(gxWindow);

    GtkWidget *box = gtk_fixed_new();
 	gtk_widget_set_halign(GTK_WIDGET(box), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(box), GTK_ALIGN_CENTER);

	txMockup* mockup = gxMockups[gxMockupIndex];
	gxMockupImage = gtk_image_new_from_resource(mockup->path);
  	gtk_fixed_put(GTK_FIXED(box), gxMockupImage, 0, 0);
	GtkWidget *drawing_area = gtk_drawing_area_new();
	gtk_widget_add_events(GTK_WIDGET(drawing_area), GDK_ALL_EVENTS_MASK);
	gtk_widget_set_size_request (drawing_area, mockup->width, mockup->height);
	g_signal_connect(G_OBJECT(drawing_area), "configure-event", G_CALLBACK(onScreenConfigure), NULL);
	g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(onScreenDraw), NULL);
	g_signal_connect(G_OBJECT(drawing_area), "button-press-event", G_CALLBACK(onScreenMouseDown), NULL);
	g_signal_connect(G_OBJECT(drawing_area), "button-release-event", G_CALLBACK(onScreenMouseUp), NULL);
	g_signal_connect(G_OBJECT(drawing_area), "motion-notify-event", G_CALLBACK(onScreenMouseMoved), NULL);
	gtk_fixed_put(GTK_FIXED(box), drawing_area, mockup->x, mockup->y);
	gtk_container_add(GTK_CONTAINER(gxWindow), box);
  
	gtk_widget_show_all (GTK_WIDGET(gxWindow));
}

void onApplicationSupport(GSimpleAction *action, GVariant *parameter, gpointer app)
{
	system("xdg-open http://moddable.tech");
}

void onFileClose(GSimpleAction *action, GVariant *parameter, gpointer app)
{
	fxScreenAbortAux(NULL);
}

void onFileOpen(GSimpleAction *action, GVariant *parameter, gpointer app)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", gxWindow, GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Moddable apps");
	gtk_file_filter_add_pattern (filter, "mc.so");	
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name(filter, "All files");
	gtk_file_filter_add_pattern (filter, "*");	
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    	fxLibraryClose();
		strcpy(gxLibraryPath, filename);
    	fxLibraryOpen();
		g_free(filename);
	  }
	gtk_widget_destroy (dialog);
}

gboolean onScreenConfigure(GtkWidget *widget, GdkEvent *event, gpointer refcon)
{
	gint width, height;
	gtk_widget_get_size_request(widget, &width, &height);
	if (gxScreen) {
		if ((gxScreen->width == width) && (gxScreen->height == height))
			return TRUE;
		if (gxLibraryPath[0])
			fxLibraryClose();
		cairo_surface_destroy(gxScreenSurface);
		gxScreenSurface = NULL;
		free(gxScreen);
		gxScreen = NULL;
	}
	gxScreen = (txScreen*)malloc(sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
	memset(gxScreen, 0, sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
	gxScreen->view = widget;
	gxScreen->abort = fxScreenAbort;
	gxScreen->bufferChanged = fxScreenBufferChanged;
	gxScreen->formatChanged = fxScreenFormatChanged;
	gxScreen->start = fxScreenStart;
	gxScreen->stop = fxScreenStop;
	gxScreen->width = width;
	gxScreen->height = height;
	gxScreenSurface = cairo_image_surface_create_for_data(gxScreen->buffer, CAIRO_FORMAT_RGB24, width, height, width * screenBytesPerPixel);
	if (gxLibraryPath[0])
		fxLibraryOpen();
 	return TRUE;
}

gboolean onScreenDraw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	gint width = cairo_image_surface_get_width(gxScreenSurface);
	gint height = cairo_image_surface_get_height(gxScreenSurface);
	cairo_set_source_surface(cr, gxScreenSurface, 0, 0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	return TRUE;
}

gboolean onScreenMouseDown(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gxTouching = TRUE;
	if (gxScreen && gxScreen->touch) 
		(*gxScreen->touch)(gxScreen, touchEventBeganKind, 0, ((GdkEventButton*)event)->x, ((GdkEventButton*)event)->y, 0);
	return TRUE;
}

gboolean onScreenMouseMoved(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	if (gxTouching) {
		if (gxScreen && gxScreen->touch) 
			(*gxScreen->touch)(gxScreen, touchEventMovedKind, 0, ((GdkEventButton*)event)->x, ((GdkEventButton*)event)->y, 0);
	}
	return FALSE;
}

gboolean onScreenMouseUp(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	if (gxScreen && gxScreen->touch) 
		(*gxScreen->touch)(gxScreen, touchEventEndedKind, 0, ((GdkEventButton*)event)->x, ((GdkEventButton*)event)->y, 0);
    gxTouching = FALSE;
	return TRUE;
}

void onScreenSelect(GSimpleAction *action, GVariant *parameter, gpointer app)
{
	int index = g_variant_get_int32(parameter);
	if (gxMockupIndex != index) {
		txMockup* mockup = gxMockups[index];
		gxMockupIndex = index;
		GtkWidget *view = gxScreen->view;
		g_simple_action_set_state(action, parameter);
		gtk_image_set_from_resource(GTK_IMAGE(gxMockupImage), mockup->path);
		gtk_widget_set_size_request(view, mockup->width, mockup->height);
		gtk_fixed_move(GTK_FIXED(gtk_widget_get_parent(view)), view, mockup->x, mockup->y);
	}
}

gboolean onWindowConfigure(GtkWidget *widget, GdkEvent *event, gpointer refcon)
{
    if (!gxWindowFullscreen && !gxWindowMaximized) {
    	gtk_window_get_position(GTK_WINDOW(widget), &gxWindowX, &gxWindowY);
    	gtk_window_get_size(GTK_WINDOW(widget), &gxWindowWidth, &gxWindowHeight);
    }
	return FALSE;
}

gboolean onWindowStateEvent(GtkWidget *widget, GdkEventWindowState *event)
{
    gxWindowFullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
    gxWindowMaximized = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
	return FALSE;
}

int main(int argc, char** argv)
{
	int status;
	GtkApplication *app = gxApplication = gtk_application_new("tech.moddable.simulator", G_APPLICATION_HANDLES_OPEN);
	g_signal_connect(app, "startup", G_CALLBACK(onApplicationStartup), NULL);
  	g_signal_connect(app, "activate", G_CALLBACK(onApplicationActivate), NULL);
  	g_signal_connect(app, "open", G_CALLBACK(onApplicationOpen), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(onApplicationShutdown), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	return status;
}
