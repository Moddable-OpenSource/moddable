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

typedef struct PiuFieldStruct PiuFieldRecord, *PiuField;

struct PiuFieldStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* hint;
	xsSlot* string;
	PiuStyle* computedStyle;
	GtkWidget* gtkClip;
	GtkWidget* gtkField;
	GtkCssProvider* gtkCssProvider;
};

static void PiuFieldBind(void* it, PiuApplication* application, PiuView* view);
static void PiuFieldCascade(void* it);
static void PiuFieldComputeStyle(PiuField* self);
static void PiuFieldDictionary(xsMachine* the, void* it);
static void PiuFieldMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuFieldUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuFieldDispatchRecord = {
	"Field",
	PiuFieldBind,
	PiuFieldCascade,
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
	PiuFieldUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuFieldHooks = {
	PiuContentDelete,
	PiuFieldMark,
	NULL
};

static void gtk_entry_activate(GtkWidget *widget, gpointer user_data)
{
	PiuField* self = (PiuField*)user_data;
	if ((*self)->behavior) {
		xsBeginHost((*self)->the);
		xsVars(2);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onEnter)) {
			xsVar(1) = xsReference((*self)->reference);
			(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
		}
		xsEndHost((*self)->the);
	}
}

static void gtk_entry_changed(GtkWidget *widget, gpointer user_data)
{
	PiuField* self = (PiuField*)user_data;
	if ((*self)->behavior) {
		xsBeginHost((*self)->the);
		xsVars(2);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onStringChanged)) {
			xsVar(1) = xsReference((*self)->reference);
			(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
		}
		xsEndHost((*self)->the);
	}
}

static gboolean gtk_entry_focused(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	PiuField* self = (PiuField*)user_data;
    PiuApplication* application =  (*self)->application;
    PiuApplicationSetFocus(application, self);
    return FALSE;
}
               
void PiuFieldBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuField* self = (PiuField*)it;
	xsMachine* the = (*self)->the;
	PiuContentBind(it, application, view);
	PiuFieldComputeStyle(self);
	PiuSkin* skin = (*self)->skin;
	PiuStyle* style = (*self)->computedStyle;
	
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), 
		 "#field {\n"
		 "   background-color: transparent;\n"
		 "   background-image: none;\n"
		 "   border: 0 none transparent;\n"
		 "   box-shadow: none;\n"
		 "   color: #%2.2x%2.2x%2.2x;\n"
		 "   outline: transparent none 0;\n"
		 "   margin: 0;\n"
		 "   padding: 0px 4px 0px 4px;\n"
		 "}\n"
		 "#field selection {\n"
		 "   background-color: #%2.2x%2.2x%2.2x;\n"
		 "   color: #%2.2x%2.2x%2.2x;\n"
		 "}\n", 
		 (*style)->color[0].r, (*style)->color[0].g, (*style)->color[0].b, 
		 (*skin)->data.color.fill[1].r, (*skin)->data.color.fill[1].g, (*skin)->data.color.fill[1].b, 
		 (*style)->color[1].r, (*style)->color[1].g, (*style)->color[1].b);
	
	(*self)->gtkCssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_data((*self)->gtkCssProvider, buffer, -1, NULL);
	
	
	GtkFixed* gtkFixed = (*view)->gtkFixed;
	GtkWidget* gtkField = gtk_entry_new();
	g_signal_connect(G_OBJECT(gtkField), "activate", G_CALLBACK(gtk_entry_activate), self);
	g_signal_connect(G_OBJECT(gtkField), "changed", G_CALLBACK(gtk_entry_changed), self);
	g_signal_connect(G_OBJECT(gtkField), "focus-in-event", G_CALLBACK(gtk_entry_focused), self);
// 	gtk_entry_set_attributes(GTK_ENTRY(gtkField), (*font)->pangoAttributes);
	gtk_widget_set_name(GTK_WIDGET(gtkField), "field");
	GtkStyleContext* style_context = gtk_widget_get_style_context(gtkField);
    gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_VIEW );
	gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER((*self)->gtkCssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	if ((*self)->hint) {
 		xsStringValue string = PiuToString((*self)->hint);
   		gtk_entry_set_placeholder_text(GTK_ENTRY(gtkField), string);
	}
	if ((*self)->string) {
 		xsStringValue string = PiuToString((*self)->string);
   		gtk_entry_set_text(GTK_ENTRY(gtkField), string);
	}
	GtkPiuClip* gtkClip = gtk_piu_clip_new((PiuContent*)self, GTK_WIDGET(gtkField));
	gtk_container_add(GTK_CONTAINER(gtkClip), GTK_WIDGET(gtkField));
	gtk_container_add(GTK_CONTAINER(gtkFixed), GTK_WIDGET(gtkClip));
    gtk_widget_show(GTK_WIDGET(gtkClip));
	(*self)->gtkClip = GTK_WIDGET(gtkClip);
	(*self)->gtkField = gtkField;
}

void PiuFieldCascade(void* it)
{
	PiuField* self = (PiuField*)it;
	PiuContentCascade((PiuContent*)it);
	PiuFieldComputeStyle(self);
	PiuContentReflow(self, piuSizeChanged);
}

void PiuFieldComputeStyle(PiuField* self)
{
	xsMachine* the = (*self)->the;
	PiuApplication* application = (*self)->application;
	PiuContainer* container = (PiuContainer*)self;
	PiuStyleLink* list = (*application)->styleList;
	PiuStyleLink* chain = NULL;
	while (container) {
		PiuStyle* style = (*container)->style;
		if (style) {
			list = PiuStyleLinkMatch(the, list, chain, style);
			chain = list;
		}
		container = (*container)->container;
	}
	if (chain) {
		PiuStyle* result = PiuStyleLinkCompute(the, chain, application);
		(*self)->computedStyle = result;
	}
}

void PiuFieldDictionary(xsMachine* the, void* it) 
{
	PiuField* self = (PiuField*)it;
	if (xsFindResult(xsArg(1), xsID_placeholder)) {
		xsSlot* hint = PiuString(xsResult);
		(*self)->hint = hint;
	}
	if (xsFindResult(xsArg(1), xsID_string)) {
		xsSlot* string = PiuString(xsResult);
		(*self)->string = string;
	}
}

void PiuFieldMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuField self = (PiuField)it;
	PiuContentMark(the, (PiuContent)it, markRoot);
	PiuMarkHandle(the, self->computedStyle);
	PiuMarkString(the, self->hint);
	PiuMarkString(the, self->string);
}

void PiuFieldUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuField* self = (PiuField*)it;
	GtkCssProvider* gtkCssProvider = (*self)->gtkCssProvider;
	GtkWidget* gtkClip = (*self)->gtkClip;
	GtkWidget* gtkField = (*self)->gtkField;
	gtk_widget_destroy(gtkField);
	(*self)->gtkField = NULL;
	gtk_widget_destroy(gtkClip);
	(*self)->gtkClip = NULL;
	(*self)->computedStyle = NULL;
	g_clear_object(&gtkCssProvider);
	(*self)->gtkCssProvider = NULL;
	PiuContentUnbind(it, application, view);
}

void PiuField_create(xsMachine* the)
{
	PiuField* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuFieldRecord));
	self = PIU(Field, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuFieldHooks);
	(*self)->dispatch = (PiuDispatch)&PiuFieldDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuFieldDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuField_get_placeholder(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	if ((*self)->hint)
		xsResult = *((*self)->hint);
}

void PiuField_get_string(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	if ((*self)->application) {
		GtkWidget* gtkField = (*self)->gtkField;
		xsStringValue string = (xsStringValue)gtk_entry_get_text(GTK_ENTRY(gtkField));
		if (string)
			xsResult = xsString(string);
		else
			xsResult = xsString("");
	}
	else if ((*self)->string)
		xsResult = *((*self)->string);
}

void PiuField_set_placeholder(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	xsSlot* hint = PiuString(xsArg(0));
	(*self)->hint = hint;
	if ((*self)->application) {
 		GtkWidget* gtkField = (*self)->gtkField;
		xsStringValue text = PiuToString(hint);
   		gtk_entry_set_placeholder_text(GTK_ENTRY(gtkField), text);
	}
}

void PiuField_set_string(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	xsSlot* string = PiuString(xsArg(0));
	(*self)->string = string;
	if ((*self)->gtkField) {
 		GtkWidget* gtkField = (*self)->gtkField;
 		xsStringValue text = PiuToString(string);
   		gtk_entry_set_text(GTK_ENTRY(gtkField), text);
	}
}

void PiuField_focus(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	if ((*self)->application) {
		gtk_widget_grab_focus((*self)->gtkField);
	}
}

