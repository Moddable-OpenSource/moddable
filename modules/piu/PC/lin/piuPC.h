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

#include "piuAll.h"

#include <gtk/gtk.h>

#include "mc.defines.h"

#define GTK_TYPE_PIU_APPLICATION (gtk_piu_application_get_type())
#define GTK_PIU_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_PIU_APPLICATION, GtkPiuApplication))
#define GTK_IS_PIU_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_PIU_APPLICATION))
typedef struct _GtkPiuApplication GtkPiuApplication;
typedef struct _GtkPiuApplicationClass GtkPiuApplicationClass;
extern GType gtk_piu_application_get_type();

#define GTK_TYPE_PIU_WINDOW (gtk_piu_window_get_type())
#define GTK_PIU_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_PIU_WINDOW, GtkPiuWindow))
#define GTK_IS_PIU_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_PIU_WINDOW))
typedef struct _GtkPiuWindow GtkPiuWindow;
typedef struct _GtkPiuWindowClass GtkPiuWindowClass;
extern GType gtk_piu_window_get_type();
extern GtkPiuWindow* gtk_piu_window_new(GtkApplication* application, PiuView* piuView);
extern void gtk_piu_window_save(GtkPiuWindow* gtkWindow);

#define GTK_TYPE_PIU_VIEW (gtk_piu_view_get_type())
#define GTK_PIU_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PIU_VIEW, GtkPiuView))
#define GTK_IS_PIU_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PIU_VIEW))
typedef struct _GtkPiuView GtkPiuView;
typedef struct _GtkPiuViewClass GtkPiuViewClass;
extern GType gtk_piu_view_get_type();
extern GtkPiuView* gtk_piu_view_new(PiuView* piuView);

#define GTK_TYPE_PIU_CLIP (gtk_piu_clip_get_type())
#define GTK_PIU_CLIP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_PIU_CLIP, GtkPiuClip))
#define GTK_IS_PIU_CLIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_PIU_CLIP))
typedef struct _GtkPiuClip GtkPiuClip;
typedef struct _GtkPiuClipClass GtkPiuClipClass;
extern GType gtk_piu_clip_get_type();
extern GtkPiuClip* gtk_piu_clip_new(PiuContent* content, GtkWidget* widget);

extern GtkApplication *gtkApplication;

struct PiuFontStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuFont* next;
	PiuFlags flags;
	xsIdentifier family;
	PiuCoordinate size;
	PiuCoordinate weight;
	cairo_font_face_t* face;
	double ascent;
	double delta;
	double height;
	PangoAttrList* pangoAttributes;
};

struct PiuTextureStruct {
	PiuHandlePart;
	PiuAssetPart;
	cairo_surface_t* image;
	xsNumberValue scale;
	PiuDimension width;
	PiuDimension height;
};

struct PiuViewStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuApplication* application;
	GtkWindow* gtkWindow;
	GtkFixed* gtkFixed;
	GtkWidget* gtkView;
	cairo_t* cairo;
	guint timer;
	PiuBoolean running;
	PiuBoolean touching;
	PiuBoolean filtered;
	PiuBoolean transparent;
	PiuBoolean appearanceChanged;
};

extern xsMachine* ServiceThreadMain(void* context);
extern void PiuViewDrawRoundContent(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuDimension radius, PiuDimension border, PiuVariant variant, PiuColor fillColor, PiuColor strokeColor);
extern void fxAbort(xsMachine *the, int status);
extern void PiuConfigPath(char* path);
extern char gtkApplicationPath[];
