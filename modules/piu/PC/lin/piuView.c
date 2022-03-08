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

struct _GtkPiuClip {
	GtkFixed parent;
	PiuContent* piuContent;
	GtkWidget* widget;
};

struct _GtkPiuClipClass {
	GtkFixedClass parent;
};

G_DEFINE_TYPE (GtkPiuClip, gtk_piu_clip, GTK_TYPE_FIXED);

void gtk_piu_clip_class_init(GtkPiuClipClass *class)
{
}

void gtk_piu_clip_init(GtkPiuClip *object)
{
	object->piuContent = NULL;
}

GtkPiuClip* gtk_piu_clip_new(PiuContent* piuContent, GtkWidget* widget)
{
	GtkPiuClip* result = g_object_new(GTK_TYPE_PIU_CLIP, NULL);
	result->piuContent = piuContent;
	result->widget = widget;
	return result;
}

struct _GtkPiuView {
	GtkDrawingArea parent;
	PiuView* piuView;
};

struct _GtkPiuViewClass {
	GtkDrawingAreaClass parent;
};

G_DEFINE_TYPE (GtkPiuView, gtk_piu_view, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_piu_view_draw(GtkWidget *widget, cairo_t *cairo)
{
	GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
	PiuView* view = gtkView->piuView;
	xsBeginHost((*view)->the);
	{
		PiuApplication* application = (*view)->application;
		double left, top, right, bottom;
		PiuRectangleRecord area;
		(*view)->cairo = cairo;
		cairo_clip_extents(cairo, &left, &top, &right, &bottom);
		
		area.x = floor(left);
		area.y = floor(top);
		area.width = ceil(right) - area.x;
		area.height = ceil(bottom) - area.y;
		(*(*application)->dispatch->update)(application, view, &area);
		(*view)->cairo = NULL;
	}
	xsEndHost(the);
	return FALSE;
}

static gboolean gtk_piu_view_button_press_event(GtkWidget *widget, GdkEventButton *event)
{
	GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
	PiuView* view = gtkView->piuView;
	(*view)->touching = 1;
	gtk_widget_grab_focus(widget);
	xsBeginHost((*view)->the);
	{
		PiuApplication* application = (*view)->application;
		PiuApplicationTouchBegan(application, 0, event->x, event->y, event->time);
		PiuApplicationAdjust(application);
	}
	xsEndHost((*view)->the);
	return TRUE;
}

static gboolean gtk_piu_view_button_release_event(GtkWidget *widget, GdkEventButton *event)
{
	GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
	PiuView* view = gtkView->piuView;
	xsBeginHost((*view)->the);
	{
		PiuApplication* application = (*view)->application;
		PiuApplicationTouchEnded(application, 0, event->x, event->y, event->time);
		PiuApplicationAdjust(application);
	}
	xsEndHost((*view)->the);
	(*view)->touching = 0;
	return TRUE;
}

static void gtk_piu_view_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint type, guint time)
{
	GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
	PiuView* view = gtkView->piuView;
	PiuApplication* application = (*view)->application;
	gchar** uris;
	char* uri;
	char buffer[PATH_MAX];
	uris = gtk_selection_data_get_uris(data);
	if (uris != NULL) {
		while ((uri = *uris++)) {
			GFile* file = g_file_new_for_uri(uri);
			char* path = realpath(g_file_get_path(file), buffer);
			g_object_unref(file);
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
	gtk_drag_finish(context, TRUE, FALSE, time);
}

static gboolean gtk_piu_view_enter_notify_event(GtkWidget *widget, GdkEventCrossing *event)
{
	GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
	PiuView* view = gtkView->piuView;
	xsBeginHost((*view)->the);
	{
		PiuApplication* application = (*view)->application;
		PiuApplicationMouseEntered(application, event->x, event->y);
		PiuApplicationAdjust(application);
	}
	xsEndHost((*view)->the);
	return TRUE;
}

static gboolean gtk_piu_view_leave_notify_event(GtkWidget *widget, GdkEventCrossing *event)
{
	GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
	PiuView* view = gtkView->piuView;
	xsBeginHost((*view)->the);
	{
		PiuApplication* application = (*view)->application;
		PiuApplicationMouseExited(application, event->x, event->y);
		PiuApplicationAdjust(application);
	}
	xsEndHost((*view)->the);
	return TRUE;
}

static gboolean gtk_piu_view_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
	GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
	PiuView* view = gtkView->piuView;
	xsBeginHost((*view)->the);
	{
		PiuApplication* application = (*view)->application;
		if ((*view)->touching)
			PiuApplicationTouchMoved(application, 0, event->x, event->y, event->time);
		else
			PiuApplicationMouseMoved(application, event->x, event->y);
		PiuApplicationAdjust(application);
	}
	xsEndHost((*view)->the);
	return TRUE;
}

static gboolean gtk_piu_view_scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
	gdouble delta_x = 0, delta_y = 0;
	switch (event->direction) {
	case GDK_SCROLL_LEFT: delta_x = 5; break;
	case GDK_SCROLL_RIGHT: delta_x = -5; break;
	case GDK_SCROLL_UP: delta_y = 5; break;
	case GDK_SCROLL_DOWN: delta_y = -5; break;
	case GDK_SCROLL_SMOOTH: gdk_event_get_scroll_deltas((GdkEvent*)event, &delta_x, &delta_y); break;
	}
	if (delta_x || delta_y) {
		GtkPiuView* gtkView = GTK_PIU_VIEW(widget);
		PiuView* view = gtkView->piuView;
		gtk_widget_grab_focus(widget);
		xsBeginHost((*view)->the);
		{
			PiuApplication* application = (*view)->application;
			PiuApplicationMouseScrolled(application, delta_x, delta_y);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*view)->the);
	}
	return TRUE;
}

void gtk_piu_view_class_init(GtkPiuViewClass *class)
{
	GTK_WIDGET_CLASS(class)->button_press_event = gtk_piu_view_button_press_event;
	GTK_WIDGET_CLASS(class)->button_release_event = gtk_piu_view_button_release_event;
	GTK_WIDGET_CLASS(class)->drag_data_received = gtk_piu_view_drag_data_received;
	GTK_WIDGET_CLASS(class)->draw = gtk_piu_view_draw;
	GTK_WIDGET_CLASS(class)->enter_notify_event = gtk_piu_view_enter_notify_event;
	GTK_WIDGET_CLASS(class)->leave_notify_event = gtk_piu_view_leave_notify_event;
	GTK_WIDGET_CLASS(class)->motion_notify_event = gtk_piu_view_motion_notify_event;
	GTK_WIDGET_CLASS(class)->scroll_event = gtk_piu_view_scroll_event;
}

void gtk_piu_view_init(GtkPiuView *object)
{
	object->piuView = NULL;
}

GtkPiuView* gtk_piu_view_new(PiuView* piuView)
{
	GtkPiuView* result = g_object_new(GTK_TYPE_PIU_VIEW, NULL);
	result->piuView = piuView;
	return result;
}

struct _GtkPiuWindow {
	GtkApplicationWindow parent;
	PiuView* piuView;
	gboolean fullscreen;
	gboolean maximized;
	int x;
	int y;
	int width;
	int height;
	GtkAllocation allocation;
};

struct _GtkPiuWindowClass {
	GtkApplicationWindowClass parent;
};

G_DEFINE_TYPE (GtkPiuWindow, gtk_piu_window, GTK_TYPE_APPLICATION_WINDOW);

static gboolean gtk_piu_window_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
	gboolean result = GTK_WIDGET_CLASS(gtk_piu_window_parent_class)->configure_event(widget, event);
	GtkPiuWindow* gtkWindow = GTK_PIU_WINDOW(widget);
    if (!gtkWindow->fullscreen && !gtkWindow->maximized) {
    	gtk_window_get_position(GTK_WINDOW(widget), &gtkWindow->x, &gtkWindow->y);
    	gtk_window_get_size(GTK_WINDOW(widget), &gtkWindow->width, &gtkWindow->height);
    }
	return result;
}

static gboolean gtk_piu_window_delete_event(GtkWidget *widget, GdkEventAny *event)
{
	GtkPiuWindow* gtkWindow = GTK_PIU_WINDOW(widget);
	PiuView* view = gtkWindow->piuView;
	xsBeginHost((*view)->the);
	{
		PiuApplication* application = (*view)->application;
		xsVars(2);
		xsVar(0) = xsReference((*application)->behavior);
		xsVar(1) = xsReference((*application)->reference);
		if (xsFindResult(xsVar(0), xsID_onQuit))
			(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
		else
			(void)xsCall0(xsVar(1), xsID_quit);
	}
	xsEndHost((*view)->the);
	return TRUE;
}

static void gtk_piu_window_get_preferred_height(GtkWidget *widget, gint *minimum_height, gint *natural_height)
{
	GTK_WIDGET_CLASS(gtk_piu_window_parent_class)->get_preferred_height(widget, minimum_height, natural_height);
	*minimum_height = 480;
	if (*natural_height < *minimum_height)
		*natural_height = *minimum_height;
}

static void gtk_piu_window_get_preferred_width(GtkWidget *widget, gint *minimum_width, gint *natural_width)
{
	GTK_WIDGET_CLASS(gtk_piu_window_parent_class)->get_preferred_width(widget, minimum_width, natural_width);
	*minimum_width = 640;
	if (*natural_width < *minimum_width)
		*natural_width = *minimum_width;
}

static void gtk_piu_window_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	GTK_WIDGET_CLASS(gtk_piu_window_parent_class)->size_allocate(widget, allocation);
	GtkPiuWindow* gtkWindow = GTK_PIU_WINDOW(widget);
	if ((gtkWindow->allocation.width != allocation->width) || (gtkWindow->allocation.height != allocation->height)) {
		PiuView* view = gtkWindow->piuView;
		xsBeginHost((*view)->the);
		{
			PiuApplication* application = (*view)->application;
			PiuApplicationResize(application);
		}
		xsEndHost(the);
		gtkWindow->allocation.width = allocation->width;
		gtkWindow->allocation.height = allocation->height;
	}
}

static gboolean gtk_piu_window_state_event(GtkWidget *widget, GdkEventWindowState *event)
{
 	GtkPiuWindow* gtkWindow = GTK_PIU_WINDOW(widget);
	gtkWindow->fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
    gtkWindow->maximized = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
	return GTK_WIDGET_CLASS(gtk_piu_window_parent_class)->window_state_event(widget, event);
}

void gtk_piu_window_class_init(GtkPiuWindowClass *class)
{
	GTK_WIDGET_CLASS(class)->configure_event = gtk_piu_window_configure_event;
	GTK_WIDGET_CLASS(class)->delete_event = gtk_piu_window_delete_event;
	GTK_WIDGET_CLASS(class)->size_allocate = gtk_piu_window_size_allocate;
	GTK_WIDGET_CLASS(class)->get_preferred_height = gtk_piu_window_get_preferred_height;
	GTK_WIDGET_CLASS(class)->get_preferred_width = gtk_piu_window_get_preferred_width;
	GTK_WIDGET_CLASS(class)->window_state_event = gtk_piu_window_state_event;
}

void gtk_piu_window_init(GtkPiuWindow *gtkWindow)
{
	gtkWindow->fullscreen = FALSE;
	gtkWindow->maximized = FALSE;
	gtkWindow->x = -1;
	gtkWindow->y = -1;
	gtkWindow->width = -1;
	gtkWindow->height = -1;
}

GtkPiuWindow* gtk_piu_window_new(GtkApplication* application, PiuView* piuView)
{
	GtkPiuWindow* result = g_object_new(GTK_TYPE_PIU_WINDOW, "application", application, NULL);
	result->piuView = piuView;
	return result;
}

void gtk_piu_window_load(GtkPiuWindow* gtkWindow)
{
	GKeyFile* keyfile = g_key_file_new();
	char path[PATH_MAX];
	PiuConfigPath(path);
	strcat(path, ".ini");
	if (g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)) {
		GError *error;
		error = NULL;
		gtkWindow->fullscreen = g_key_file_get_boolean(keyfile, "window", "fullscreen", &error);
		if (error) {
			g_clear_error(&error);
			gtkWindow->fullscreen = FALSE;
		}
		gtkWindow->maximized = g_key_file_get_boolean(keyfile, "window", "maximized", &error);
		if (error) {
			g_clear_error(&error);
			gtkWindow->maximized = FALSE;
		}
		gtkWindow->x = g_key_file_get_integer(keyfile, "window", "x", &error);
		if (error) {
			g_clear_error(&error);
			gtkWindow->x = -1;
		}
		gtkWindow->y = g_key_file_get_integer(keyfile, "window", "y", &error);
		if (error) {
			g_clear_error(&error);
			gtkWindow->y = -1;
		}
		gtkWindow->width = g_key_file_get_integer(keyfile, "window", "width", &error);
		if (error) {
			g_clear_error(&error);
			gtkWindow->width = -1;
		}
		gtkWindow->height = g_key_file_get_integer(keyfile, "window", "height", &error);
		if (error) {
			g_clear_error(&error);
			gtkWindow->height = -1;
		}
	}
	g_key_file_unref(keyfile);
}

void gtk_piu_window_save(GtkPiuWindow* gtkWindow)
{
	GKeyFile* keyfile = g_key_file_new();
	char path[PATH_MAX];
	g_key_file_set_boolean(keyfile, "window", "fullscreen", gtkWindow->fullscreen);
	g_key_file_set_boolean(keyfile, "window", "maximized", gtkWindow->maximized);
	g_key_file_set_integer(keyfile, "window", "x", gtkWindow->x);
	g_key_file_set_integer(keyfile, "window", "y", gtkWindow->y);
	g_key_file_set_integer(keyfile, "window", "width", gtkWindow->width);
	g_key_file_set_integer(keyfile, "window", "height", gtkWindow->height);
	PiuConfigPath(path);
	strcat(path, ".ini");
	g_key_file_save_to_file(keyfile, path, NULL);
	g_key_file_unref(keyfile);
}

static void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
static void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuViewHooks = {
	PiuViewDelete,
	PiuViewMark,
	NULL
};

static void PiuViewAdjustAux(GtkWidget *widget, gpointer it)
{
	PiuView* self = it;
	GtkFixed* gtkFixed = (*self)->gtkFixed;
	GtkWidget* gtkView = (*self)->gtkView;
	if (gtkView == widget) {
		PiuApplication* application = (*self)->application;
		gtk_fixed_move(gtkFixed, widget, 0, 0);
		gtk_widget_set_size_request(widget, (*application)->bounds.width, (*application)->bounds.height);
	}
	else {
		GtkPiuClip* gtkClip = GTK_PIU_CLIP(widget);
		PiuContent* content = gtkClip->piuContent;
		PiuRectangleRecord bounds = (*content)->bounds;
		PiuRectangleRecord clipBounds = bounds;
		PiuContainer* container = (*content)->container;
		while (container) {
			bounds.x += (*container)->bounds.x;
			bounds.y += (*container)->bounds.y;
			clipBounds.x += (*container)->bounds.x;
			clipBounds.y += (*container)->bounds.y;
			if ((*container)->flags & piuClip)
				PiuRectangleIntersect(&clipBounds, &clipBounds, &(*container)->bounds);
			container = (*container)->container;
		}
		gtk_fixed_move(gtkFixed, widget, clipBounds.x, clipBounds.y);
		gtk_widget_set_size_request(widget, clipBounds.width, clipBounds.height);
		widget = gtkClip->widget;
		gtk_widget_set_size_request(widget, bounds.width, bounds.height);
		if (PiuRectangleContains(&clipBounds, &bounds))
    		gtk_widget_show(widget);
		else
	    	gtk_widget_hide(widget);
    	
	}
}


void PiuViewAdjust(PiuView* self) 
{
	GtkFixed* gtkFixed = (*self)->gtkFixed;
	gtk_container_foreach(GTK_CONTAINER(gtkFixed), PiuViewAdjustAux, self);
}

void PiuViewChangeCursor(PiuView* self, int32_t shape)
{
	GtkWindow* gtkWindow = (*self)->gtkWindow;
	GdkCursorType cursor_type;
	GdkCursor *cursor;
	switch (shape) {
	case 1:
		cursor_type = GDK_TCROSS;
		break;
	case 2:
		cursor_type = GDK_XTERM;
		break;
	case 3:
		cursor_type = GDK_HAND1;
		break;
	case 4:
		cursor_type = GDK_X_CURSOR;
		break;
	case 5:
		cursor_type = GDK_SB_H_DOUBLE_ARROW;
		break;
	case 6:
		cursor_type = GDK_SB_V_DOUBLE_ARROW;
		break;
	default:
		cursor_type = GDK_LEFT_PTR;
		break;
	}
	cursor = gdk_cursor_new_for_display(gdk_display_get_default(), cursor_type);
	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(gtkWindow)), cursor);
}

void PiuViewCreate(xsMachine* the) 
{
	PiuView* self;
	PiuApplication* application;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuViewRecord));
	self = PIU(View, xsThis);
	(*self)->reference = xsToReference(xsThis);
	(*self)->the = the;
	xsSetHostHooks(xsThis, &PiuViewHooks);
	application = (*self)->application = PIU(Application, xsArg(0));
	(*application)->view = self;

	if (xsFindResult(xsArg(1), xsID_menus))
		xsCall1(xsArg(0), xsID_createMenus, xsResult);
	
	GtkPiuWindow* gtkWindow = gtk_piu_window_new(gtkApplication, self);
	gtk_window_set_icon_name(GTK_WINDOW(gtkWindow), PIU_DASH_SIGNATURE);
	if (xsFindResult(xsArg(1), xsID_window)) {
		xsStringValue title = xsToString(xsGet(xsResult, xsID_title));
		gtk_window_set_title(GTK_WINDOW(gtkWindow), title);
	}
	gtk_piu_window_load(GTK_PIU_WINDOW(gtkWindow));
	gtk_window_set_default_size(GTK_WINDOW(gtkWindow), gtkWindow->width, gtkWindow->height);
	gtk_window_move(GTK_WINDOW(gtkWindow), gtkWindow->x, gtkWindow->y);
	if (gtkWindow->maximized)
		gtk_window_maximize(GTK_WINDOW(gtkWindow));
	if (gtkWindow->fullscreen)
		gtk_window_fullscreen(GTK_WINDOW(gtkWindow));
	
	GtkWidget* gtkFixed = gtk_fixed_new();
 	gtk_widget_set_halign(gtkFixed, GTK_ALIGN_FILL);
	gtk_widget_set_valign(gtkFixed, GTK_ALIGN_FILL);
	gtk_container_add(GTK_CONTAINER(gtkWindow), gtkFixed);

	GtkPiuView* gtkView = gtk_piu_view_new(self);
	gtk_widget_add_events(GTK_WIDGET(gtkView), GDK_ALL_EVENTS_MASK);
	gtk_drag_dest_set(GTK_WIDGET(gtkView), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
	GtkTargetList *target_list = gtk_target_list_new (NULL, 0);
	gtk_target_list_add_uri_targets (target_list, 0);
	gtk_drag_dest_set_target_list(GTK_WIDGET(gtkView), target_list);
	gtk_container_add(GTK_CONTAINER(gtkFixed), GTK_WIDGET(gtkView));
	
	(*self)->gtkFixed = GTK_FIXED(gtkFixed);
	(*self)->gtkView = GTK_WIDGET(gtkView);
	(*self)->gtkWindow = GTK_WINDOW(gtkWindow);
	
	gtk_widget_show_all(GTK_WIDGET(gtkWindow));
	gtk_widget_set_can_focus(GTK_WIDGET(gtkView), TRUE);
	gtk_widget_grab_focus(GTK_WIDGET(gtkView));
	
	xsResult = xsThis;
}

void PiuViewDelete(void* it)
{
}

void PiuViewDictionary(xsMachine* the, void* it)
{
	
}

void PiuViewDrawRoundContent(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuDimension radius, PiuDimension border, PiuVariant variant, PiuColor fillColor, PiuColor strokeColor)
{
	cairo_t* cr = (*self)->cairo;
	double lx = x, ty = y, rx = x + w, by = y + h, r = radius, t, u = border;
	if (variant == 1)
		lx += r;
	else if (variant == 2)
		rx -= r;
	if (u > 0) {
		double delta = u / 2;
		lx += delta;
		ty += delta;
		rx -= delta;
		by -= delta;
		r -= delta;
	}
	t = r * 0.552284749831;
	cairo_new_sub_path(cr);
	cairo_move_to(cr, lx, ty + r);
	cairo_curve_to(cr, lx, ty + r - t, lx + r - t, ty, lx + r, ty);
	cairo_line_to(cr, rx - r, ty);
	cairo_curve_to(cr, rx - r + t, ty, rx, ty + r - t, rx, ty + r);
	cairo_line_to(cr, rx, by - r);
	if (variant == 2)
		cairo_curve_to(cr, rx, by - r + t, rx + r - t, by, rx + r, by);
	else
		cairo_curve_to(cr, rx, by - r + t, rx - r + t, by, rx - r, by);
	if (variant == 1) {
		cairo_line_to(cr, lx - r, by);
		cairo_curve_to(cr, lx - r + t, by, lx, by - r + t, lx, by - r);
	}
	else {
		cairo_line_to(cr, lx + r, by);
		cairo_curve_to(cr, lx + r - t, by, lx, by - r + t, lx, by - r);
	}
	cairo_close_path(cr);
	cairo_set_source_rgba(cr, ((double)fillColor->r) / 255.0, ((double)fillColor->g) / 255.0, ((double)fillColor->b) / 255.0, ((double)fillColor->a) / 255.0);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba(cr, ((double)strokeColor->r) / 255.0, ((double)strokeColor->g) / 255.0, ((double)strokeColor->b) / 255.0, ((double)strokeColor->a) / 255.0);
	cairo_set_line_width(cr, u);
	cairo_stroke (cr);
}

void PiuViewDrawString(PiuView* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension sw)
{
	PiuViewDrawStringSubPixel(self, slot, offset, length, font, x, y, w, sw);
}

void PiuViewDrawStringSubPixel(PiuView* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, double x, double y, PiuDimension w, PiuDimension sw)
{
	xsMachine* the = (*self)->the;
	cairo_t* cr = (*self)->cairo;
	xsStringValue string = PiuToString(slot);
	if (length < 0)
		length = c_strlen(string + offset);
	char* text = malloc(length + 1);
	memcpy(text, string + offset, length);
	text[length] = 0;
	cairo_set_font_face(cr, (*font)->face);
	cairo_set_font_size(cr, (*font)->size);
	cairo_move_to(cr, x, y + (*font)->ascent);
	cairo_show_text(cr, text);
	free(text);
}

void PiuViewDrawTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		x -= sx;
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw)
		sw = tw - sx;
	if (sy < 0) {
		y -= sy;
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th)
		sh = th - sy;
	if ((sw <= 0) || (sh <= 0)) return;
	PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, sh);
}

void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	cairo_t* cr = (*self)->cairo;
	double scale = (*texture)->scale;
	if ((*self)->filtered) {
		if ((*self)->transparent) return;
		double w = sw * scale;
		double h = sh * scale;
		cairo_surface_t* maskImage = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
		cairo_t* maskContext = cairo_create(maskImage);
		cairo_set_source_surface(maskContext, (*texture)->image, 0, 0);
		cairo_pattern_t *pattern = cairo_get_source(maskContext);	
		cairo_matrix_t matrix;
		cairo_matrix_init_identity(&matrix);
		cairo_matrix_translate(&matrix, sx * scale, sy * scale);
		cairo_pattern_set_matrix (pattern, &matrix);
		cairo_rectangle(maskContext, 0, 0, w, h);
		cairo_fill(maskContext);
	
		cairo_save(cr);
		cairo_get_matrix(cr, &matrix);
  		cairo_matrix_scale(&matrix, 1 / scale, 1 / scale);
		cairo_set_matrix(cr, &matrix);
		cairo_mask_surface(cr, maskImage, x, y);
//  		cairo_rectangle(cr, x, y, sw, sh);
 		cairo_fill(cr);   
		cairo_restore(cr);
  		
  		cairo_destroy(maskContext);   
  		cairo_surface_destroy(maskImage);   
	}
	else {
		cairo_set_source_surface(cr, (*texture)->image, 0, 0);
		cairo_pattern_t *pattern = cairo_get_source(cr);	
		cairo_matrix_t matrix;
		cairo_matrix_init_identity(&matrix);
		cairo_matrix_scale(&matrix, scale, scale);
		cairo_matrix_translate(&matrix, sx - x, sy - y);
		cairo_pattern_set_matrix (pattern, &matrix);
		cairo_rectangle(cr, x, y, sw, sh);
		cairo_fill(cr);
	}
}

void PiuViewFillColor(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	if ((w <= 0) || (h <= 0)) return;
	if ((*self)->transparent) return;
	cairo_t* cr = (*self)->cairo;
	cairo_rectangle(cr, x, y, w, h);
	cairo_fill(cr);
}

void PiuViewFillTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		if (w == sw) {
			x -= sx;
			w += sx;
		}
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw) {
		if (w == sw)
			w = tw - sx;
		sw = tw - sx;
	}
	if (sy < 0) {
		if (h == sh) {
			y -= sy;
			h += sy;
		}
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th) {
		if (h == sh)
			h = th - sy;
		sh = th - sy;
	}
	if ((w <= 0) || (h <= 0) || (sw <= 0) || (sh <= 0)) return;
	PiuCoordinate xx, ww;
	while (h >= sh) {
		xx = x;
		ww = w;
		while (ww >= sw) {
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, sw, sh);
			xx += sw;
			ww -= sw;
		}
		if (ww)
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, ww, sh);
		y += sh;
		h -= sh;
	}
	if (h) {
		while (w >= sw) {
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, h);
			x += sw;
			w -= sw;
		}
		if (w)
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, w, h);
	}
}

void PiuViewGetSize(PiuView* self, PiuDimension *width, PiuDimension *height)
{
	GtkFixed* gtkFixed = (*self)->gtkFixed;
	*width = gtk_widget_get_allocated_width(GTK_WIDGET(gtkFixed));
	*height = gtk_widget_get_allocated_height(GTK_WIDGET(gtkFixed));
	//fprintf(stderr, "PiuViewGetSize %d %d\n", *width, *height);
}

static gboolean PiuViewIdle(gpointer data)
{
	PiuView* self = data;
	xsBeginHost((*self)->the);
	{
		PiuApplication* application = (*self)->application;
		xsVars(2);
		PiuApplicationDeferContents(the, application);
		PiuApplicationIdleContents(application);
		PiuApplicationTouchIdle(application);
		PiuApplicationAdjust(application);
	}
	xsEndHost(the);
	return TRUE;
}

void PiuViewIdleCheck(PiuView* self, PiuInterval idle)
{
	PiuBoolean running = (idle > 0) ? 1 : 0;
	if ((*self)->running != running) {
		(*self)->running = running;
		if (idle)
			(*self)->timer = g_timeout_add(20, PiuViewIdle, self);
		else
			g_source_remove((*self)->timer);
	}
}

void PiuViewInvalidate(PiuView* self, PiuRectangle area) 
{
	GtkWidget* gtkView = (*self)->gtkView;
	if (area) {
		if ((area->width > 0) && (area->height > 0))
			gtk_widget_queue_draw_area(gtkView, area->x, area->y, area->width, area->height);
	}
	else {
		gtk_widget_queue_draw(gtkView);
	}
}

void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuViewPopClip(PiuView* self)
{
	cairo_t* cr = (*self)->cairo;
	cairo_restore(cr);
}

void PiuViewPopColor(PiuView* self)
{
}

void PiuViewPopColorFilter(PiuView* self)
{
	(*self)->filtered = 0;
}

void PiuViewPopOrigin(PiuView* self)
{
	cairo_t* cr = (*self)->cairo;
	cairo_restore(cr);
}

void PiuViewPushClip(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	cairo_t* cr = (*self)->cairo;
	cairo_save(cr);
	cairo_rectangle (cr, x, y, w, h);
	cairo_clip(cr);
}

void PiuViewPushColor(PiuView* self, PiuColor color)
{
	cairo_t* cr = (*self)->cairo;
	cairo_set_source_rgba(cr, ((double)color->r) / 255.0, ((double)color->g) / 255.0, ((double)color->b) / 255.0, ((double)color->a) / 255.0);
	(*self)->transparent = (color->a == 0) ? 1 : 0;
}

void PiuViewPushColorFilter(PiuView* self, PiuColor color)
{
	cairo_t* cr = (*self)->cairo;
	cairo_set_source_rgba(cr, ((double)color->r) / 255.0, ((double)color->g) / 255.0, ((double)color->b) / 255.0, ((double)color->a) / 255.0);
	(*self)->transparent = (color->a == 0) ? 1 : 0;
	(*self)->filtered = 1;
}

void PiuViewPushOrigin(PiuView* self, PiuCoordinate x, PiuCoordinate y)
{
	cairo_t* cr = (*self)->cairo;
	cairo_save(cr);
	cairo_translate(cr, x, y);
}

void PiuViewReflow(PiuView* self)
{
}

void PiuViewReschedule(PiuView* self)
{
	PiuApplicationIdleCheck((*self)->application);
}

double PiuViewTicks(PiuView* self)
{
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now))
		return 0;
	return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
}

void PiuViewValidate(PiuView* self, PiuRectangle area) 
{
	//@@
}

void PiuCursors_get_arrow(xsMachine* the)
{
	xsResult = xsInteger(0);
}

void PiuCursors_get_cross(xsMachine* the)
{
	xsResult = xsInteger(1);
}

void PiuCursors_get_iBeam(xsMachine* the)
{
	xsResult = xsInteger(2);
}

void PiuCursors_get_link(xsMachine* the)
{
	xsResult = xsInteger(3);
}

void PiuCursors_get_notAllowed(xsMachine* the)
{
	xsResult = xsInteger(4);
}

void PiuCursors_get_resizeColumn(xsMachine* the)
{
	xsResult = xsInteger(5);
}

void PiuCursors_get_resizeRow(xsMachine* the)
{
	xsResult = xsInteger(6);
}

void PiuSystem_getClipboardString(xsMachine* the)
{
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	if (gtk_clipboard_wait_is_text_available(clipboard)) {
		char *string = gtk_clipboard_wait_for_text(clipboard);
		if (string) {
			xsResult = xsString(string);
			g_free(string);
			return;
		}
	}
	xsResult = xsString("");
}

void PiuSystem_setClipboardString(xsMachine* the)
{
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	if (xsToInteger(xsArgc) > 0) {
		char* string = xsToString(xsArg(0));
		gtk_clipboard_set_text(clipboard, string, strlen(string));
	}
	else
		gtk_clipboard_set_text(clipboard, "", 0);
}

void PiuSystem_launchPath(xsMachine* the)
{
	char command[PATH_MAX];
	strcpy(command, "xdg-open ");
	strcat(command, xsToString(xsArg(0)));
	xsResult = xsInteger(system(command));
}

void PiuSystem_launchURL(xsMachine* the)
{
	PiuSystem_launchPath(the);
}

void PiuNavigationBar_create(xsMachine* the)
{
	xsDebugger();
}

void PiuStatusBar_create(xsMachine* the)
{
	xsDebugger();
}
