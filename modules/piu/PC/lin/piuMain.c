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

extern GResource *mc_get_resource(void);

struct _GtkPiuApplication {
	GtkApplication parent;
	xsMachine* machine;
	PiuApplication* piuApplication;
};

struct _GtkPiuApplicationClass {
	GtkApplicationClass parent;
};

G_DEFINE_TYPE (GtkPiuApplication, gtk_piu_application, GTK_TYPE_APPLICATION);

static void gtk_piu_application_activate(GApplication *app)
{
	GtkPiuApplication* gtkApplication = GTK_PIU_APPLICATION(app);
	PiuApplication* application = gtkApplication->piuApplication;
	PiuView* view = (*application)->view;
	GtkWindow *gtkWindow = (*view)->gtkWindow;
	gtk_window_present(gtkWindow);
}

static void gtk_piu_application_command(GSimpleAction *action, GVariant *parameter, gpointer it)
{
	PiuApplication* self = it;
	xsBeginHost((*self)->the);
	{
		const char* name = g_action_get_name(G_ACTION(action));
		xsIdentifier doID = xsID(name);
		PiuContent* content = (*self)->focus;
		xsVars(3);
		while (content) {
			if ((*content)->behavior) {
				xsVar(0) = xsReference((*content)->behavior);
				if (xsFindResult(xsVar(0), doID)) {
					xsVar(1) = xsReference((*content)->reference);
					if (parameter && g_variant_is_of_type(parameter, G_VARIANT_TYPE_INT32))
						xsVar(2) = xsInteger(g_variant_get_int32(parameter));
					(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
					PiuApplicationAdjust(self);
					break;
				}
			}
			content = (PiuContent*)(*content)->container;
		}
	}
	xsEndHost((*self)->the);
}

static void gtk_piu_application_open(GApplication *app, GFile **files, gint c, const gchar *hint)
{
	GtkPiuApplication* gtkApplication = GTK_PIU_APPLICATION(app);
	PiuApplication* application = gtkApplication->piuApplication;
	PiuView* view = (*application)->view;
	GtkWindow *gtkWindow = (*view)->gtkWindow;
	char buffer[PATH_MAX];
	gtk_window_present(gtkWindow);
	gint i;
	for (i = 0; i < c; i++) {
		char* path = realpath(g_file_get_path(files[i]), buffer);
		xsBeginHost((*view)->the);
		{
			xsVars(2);
			xsVar(0) = xsReference((*application)->behavior);
			if (xsFindResult(xsVar(0), xsID_onOpenFile)) {
				xsVar(1) = xsReference((*application)->reference);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsString(path));
			}
			PiuApplicationAdjust(application);
		}
		xsEndHost((*view)->the);
	}
}

static void gtk_piu_application_shutdown(GApplication *app)
{
	GtkPiuApplication* gtkApplication = GTK_PIU_APPLICATION(app);
	PiuApplication* application = gtkApplication->piuApplication;
	PiuView* view = (*application)->view;
	GtkWindow *gtkWindow = (*view)->gtkWindow;
	gtk_piu_window_save(GTK_PIU_WINDOW(gtkWindow));
	G_APPLICATION_CLASS(gtk_piu_application_parent_class)->shutdown(app);
}

static void gtk_piu_application_startup(GApplication *app)
{
	G_APPLICATION_CLASS(gtk_piu_application_parent_class)->startup(app);
	GtkPiuApplication* gtkApplication = GTK_PIU_APPLICATION(app);
	GResource* resource = mc_get_resource();
	g_resources_register(resource);
	xsMachine* machine = gtkApplication->machine = ServiceThreadMain(NULL);
	xsBeginHost(machine);
	{
		xsVars(2);
		xsResult = xsAwaitImport("main", XS_IMPORT_DEFAULT);
		gtkApplication->piuApplication = PIU(Application, xsResult);

		PiuApplication* application = gtkApplication->piuApplication;
		if ((*application)->behavior) {
			xsVar(0) = xsReference((*application)->behavior);
			if (xsFindResult(xsVar(0), xsID_onAppearanceChanged)) {
				xsVar(1) = xsReference((*application)->reference);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsUndefined);
			}
		}
		
		PiuApplicationAdjust(gtkApplication->piuApplication);
		xsCollectGarbage();
	}
	xsEndHost(machine);
}

static void gtk_piu_application_class_init(GtkPiuApplicationClass *class)
{
	G_APPLICATION_CLASS(class)->activate = gtk_piu_application_activate;
	G_APPLICATION_CLASS(class)->open = gtk_piu_application_open;
	G_APPLICATION_CLASS(class)->shutdown = gtk_piu_application_shutdown;
	G_APPLICATION_CLASS(class)->startup = gtk_piu_application_startup;
}

static void gtk_piu_application_init(GtkPiuApplication *app)
{
	app->machine = NULL;
	app->piuApplication = NULL;
}

static GtkPiuApplication* gtk_piu_application_new(void)
{
	return g_object_new(GTK_TYPE_PIU_APPLICATION, "application-id", PIU_DOT_SIGNATURE, "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}

GtkApplication *gtkApplication;
char gtkApplicationPath[PATH_MAX];

int main(int argc, char** argv)
{
	realpath(argv[0], gtkApplicationPath);
	gtkApplication = GTK_APPLICATION(gtk_piu_application_new());
  	return g_application_run(G_APPLICATION(gtkApplication), argc, argv);
}

void fxAbort(xsMachine *the, int status)
{
    g_application_quit(G_APPLICATION(gtkApplication));
}

void PiuApplication_createMenus(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	GMenu* menubar = g_menu_new();
	xsIntegerValue c, i, d, j;
	xsVars(5);
	(void)xsCall0(xsArg(0), xsID_shift);
	c = xsToInteger(xsGet(xsArg(0), xsID_length));
	for (i = 0; i < c; i++) {
		GMenu* menu = g_menu_new();
		GMenu* section = g_menu_new();
		xsIntegerValue position = 0;
		xsVar(0) = xsGetAt(xsArg(0), xsInteger(i));
		xsVar(1) = xsGet(xsVar(0), xsID_items);
		d = xsToInteger(xsGet(xsVar(1), xsID_length));
		for (j = 0; j < d; j++) {
			xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
			if (xsTest(xsVar(2))) {
				char buffer[256];
				xsStringValue value;
				xsIdentifier index;
				xsIntegerValue target;
				GSimpleAction* action;
				GMenuItem* item;
				
				value = xsToString(xsGet(xsVar(2), xsID_command));
				c_strcpy(buffer, "can");
				c_strcat(buffer, value);
				index = xsID(buffer);
				xsSet(xsVar(2), xsID_canID, xsInteger(index));
				
				value = xsToString(xsGet(xsVar(2), xsID_command));
				c_strcpy(buffer, "do");
				c_strcat(buffer, value);
				index = xsID(buffer);
				xsSet(xsVar(2), xsID_doID, xsInteger(index));
				
				if (xsFindInteger(xsVar(2), xsID_value, &target)) {
					action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(gtkApplication), buffer));
					if (!action) {
						action =  g_simple_action_new_stateful(buffer, G_VARIANT_TYPE("i"), g_variant_new_int32(target));
						g_action_map_add_action(G_ACTION_MAP(gtkApplication), G_ACTION(action));
						g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(gtk_piu_application_command), self);
					}
					xsVar(3) = xsNewHostObject(NULL);
					xsSetHostData(xsVar(3), action);
					xsSet(xsVar(2), xsID_action, xsVar(3));
				
					value = xsToString(xsGet(xsVar(2), xsID_command));
					c_strcpy(buffer, "app.do");
					c_strcat(buffer, value);
				
					value = xsToString(xsGet(xsVar(2), xsID_title));	
					item = g_menu_item_new(value, NULL);
					g_menu_item_set_action_and_target_value(item, buffer, g_variant_new_int32(target));
				}
				else {
					action = g_simple_action_new(buffer, NULL);
					g_action_map_add_action(G_ACTION_MAP(gtkApplication), G_ACTION(action));
					g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(gtk_piu_application_command), self);
					xsVar(3) = xsNewHostObject(NULL);
					xsSetHostData(xsVar(3), action);
					xsSet(xsVar(2), xsID_action, xsVar(3));
					xsVar(3) = xsGet(xsVar(2), xsID_titles);
				
					value = xsToString(xsGet(xsVar(2), xsID_command));
					c_strcpy(buffer, "app.do");
					c_strcat(buffer, value);
				
					if (xsTest(xsVar(3))) {
						xsVar(4) = xsGet(xsVar(2), xsID_state);
						value = xsToString(xsGetAt(xsVar(3), xsVar(4)));
						item = g_menu_item_new(value, buffer);
						xsVar(3) = xsNewHostObject(NULL);
						xsSetHostData(xsVar(3), item);
						xsSet(xsVar(2), xsID_item, xsVar(3));
						xsSet(xsVar(2), xsID_position, xsInteger(position));
						xsVar(3) = xsNewHostObject(NULL);
						xsSetHostData(xsVar(3), section);
						xsSet(xsVar(2), xsID_section, xsVar(3));
					}
					else {
						value = xsToString(xsGet(xsVar(2), xsID_title));	
						item = g_menu_item_new(value, buffer);
					}
				}
				
				if (xsFindString(xsVar(2), xsID_key, &value)) {
					char key = value[0];
					if (('A' <= key) && (key <= 'Z')) {
						c_strcpy(buffer, "<Ctrl>");
						xsVar(3) = xsGet(xsVar(2), xsID_shift);
						if (xsTest(xsVar(3)))
							c_strcat(buffer, "<Shift>");
						xsVar(3) = xsGet(xsVar(2), xsID_option);
						if (xsTest(xsVar(3)))
							c_strcat(buffer, "<Alt>");
						c_strcat(buffer, value);
						g_menu_item_set_attribute(item, "accel", "s", buffer);
					}
				}
				g_menu_append_item(section, item);
				position++;
			}
			else {
				g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
				g_object_unref(section);
				section = g_menu_new();
				position = 0;
			}
		}
		g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
		xsStringValue title = xsToString(xsGet(xsVar(0), xsID_title));	
		g_menu_insert_submenu(menubar, i, title, G_MENU_MODEL(menu));
		g_object_unref(menu);
	}	
	gtk_application_set_menubar(gtkApplication, G_MENU_MODEL(menubar));
	g_object_unref(menubar);
	xsSet(xsThis, xsID_menus, xsArg(0));
}

void PiuApplication_get_title(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	GtkWindow *gtkWindow = (*view)->gtkWindow;
	xsResult = xsString(gtk_window_get_title(gtkWindow));
}

void PiuApplication_set_title(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	GtkWindow *gtkWindow = (*view)->gtkWindow;
	gtk_window_set_title(gtkWindow, xsToString(xsArg(0)));
}

void PiuApplication_gotoFront(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	GtkWindow *gtkWindow = (*view)->gtkWindow;
	gtk_window_present(gtkWindow);
}

void PiuApplication_purge(xsMachine* the)
{
	xsCollectGarbage();
}

void PiuApplication_quit(xsMachine *the)
{
    g_application_quit(G_APPLICATION(gtkApplication));
}

void PiuApplication_updateMenus(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	xsIntegerValue c, i, d, j;
	xsVars(6);
	xsVar(0) = xsReference((*self)->reference);
	xsVar(0) = xsGet(xsVar(0), xsID_menus);
	c = xsToInteger(xsGet(xsVar(0), xsID_length));
	for (i = 0; i < c; i++) {
		xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
		xsVar(1) = xsGet(xsVar(1), xsID_items);
		d = xsToInteger(xsGet(xsVar(1), xsID_length));
		for (j = 0; j < d; j++) {
			xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
			if (xsTest(xsVar(2))) {
				xsIdentifier canID = (xsIdentifier)xsToInteger(xsGet(xsVar(2), xsID_canID));
				PiuContent* content = (*self)->focus;
				xsIntegerValue result = 0;
				while (content) {
					if ((*content)->behavior) {
						xsVar(3) = xsReference((*content)->behavior);
						if (xsFindResult(xsVar(3), canID)) {
							xsVar(4) = xsReference((*content)->reference);
							xsVar(5) = xsCallFunction2(xsResult, xsVar(3), xsVar(4), xsVar(2));
							if (xsTest(xsVar(5)))
								result |= piuMenuEnabled;
							break;
						}
					}
					content = (PiuContent*)(*content)->container;
				}
				xsVar(3) = xsGet(xsVar(2), xsID_check);
				if (xsTest(xsVar(3)))
					result |= piuMenuChecked;
				xsVar(3) = xsGet(xsVar(2), xsID_action);
				GSimpleAction* action = xsGetHostData(xsVar(3));
				g_simple_action_set_enabled(action, result & piuMenuEnabled ? TRUE : FALSE);
				if (result & piuMenuChecked) {
					xsIntegerValue target;
					if (xsFindInteger(xsVar(2), xsID_value, &target))
						g_simple_action_set_state(action, g_variant_new_int32(target));
				}
				xsVar(3) = xsGet(xsVar(2), xsID_titles);
				if (xsTest(xsVar(3))) {
					xsVar(4) = xsGet(xsVar(2), xsID_section);
					GMenu* section = xsGetHostData(xsVar(4));
					xsVar(4) = xsGet(xsVar(2), xsID_position);
					xsIntegerValue position = xsToInteger(xsVar(4));
					xsVar(4) = xsGet(xsVar(2), xsID_item);
					GMenuItem* item = xsGetHostData(xsVar(4));
					xsVar(4) = xsGet(xsVar(2), xsID_state);
					xsStringValue title = xsToString(xsGetAt(xsVar(3), xsVar(4)));
					g_menu_remove(section, position);
					g_menu_item_set_label(item, title);
					g_menu_insert_item(section, position, item);
				}
			}
		}
	}
}

void fxThrowErrno(xsMachine* the, xsStringValue path, xsIntegerValue line)
{
	fxThrowMessage(the, path, line, XS_UNKNOWN_ERROR, "%s", strerror(errno));
}

void PiuConfigPath(char* path)
{
	char* home = getenv("XDG_CONFIG_HOME");
	if (home)
		strcpy(path, home);
	else {
		home = getenv("HOME");
		strcpy(path, home);
		strcat(path, "/.config");
	}
	strcat(path, "/");
	strcat(path, PIU_DOT_SIGNATURE);
}


int mcCountResources(xsMachine* the)
{
	return 0;
}

const char* mcGetResourceName(xsMachine* the, int i)
{
	return NULL;
}

const void *mcGetResource(xsMachine* the, const char* path, size_t* size)
{
	*size = 0;
	return NULL;\
}






