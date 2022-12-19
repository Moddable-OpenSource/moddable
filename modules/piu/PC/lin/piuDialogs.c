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
	
void PiuSystem_alert(xsMachine* the)
{
	PiuApplication* application = xsGetContext(the);
	PiuView* view = (*application)->view;
 	xsIntegerValue argc = xsToInteger(xsArgc);
	GtkMessageType type = GTK_MESSAGE_OTHER;
	xsStringValue string;
	xsStringValue prompt = NULL;
	GtkWidget *dialog;
 	GtkResponseType result;
	xsVars(1);
	xsVar(0) = xsString("");
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_type, &string)) {
			if (!strcmp(string, "about"))
				type = GTK_MESSAGE_INFO;
			else if (!c_strcmp(string, "stop"))
				type = GTK_MESSAGE_ERROR;
			else if (!c_strcmp(string, "note"))
				type = GTK_MESSAGE_WARNING;
		}
		if (xsFindString(xsArg(0), xsID_prompt, &string)) {
			prompt = string;
		}
	}
	dialog = gtk_message_dialog_new((*view)->gtkWindow, GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_NONE, prompt, NULL);
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_info, &string)) {
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), string, NULL);
		}
		if (xsFindResult(xsArg(0), xsID_buttons)) {
			if (xsIsInstanceOf(xsResult, xsArrayPrototype)) {
				xsIntegerValue c = xsToInteger(xsGet(xsResult, xsID_length)), i;
				for (i = 0; i < c; i++) {
					string = xsToString(xsGetIndex(xsResult, i));
					gtk_dialog_add_button(GTK_DIALOG(dialog), string, i);
				}
			}
		}
	}
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if ((argc > 1) && xsTest(xsArg(1)))
		(void)xsCallFunction1(xsArg(1), xsNull, (result == 0) ? xsTrue : (result == 1) ? xsUndefined : xsFalse);
}

void PiuSystem_open(xsMachine* the, GtkFileChooserAction action)
{
	PiuApplication* application = xsGetContext(the);
	PiuView* view = (*application)->view;
 	xsIntegerValue argc = xsToInteger(xsArgc);
	xsStringValue string;
 	xsStringValue message = NULL;
 	xsStringValue name = NULL;
 	xsStringValue prompt = NULL;
	GtkWidget *dialog;
	GtkFileFilter *filter = NULL;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_message, &string)) {
			message = strdup(string);
		}
		if (xsFindString(xsArg(0), xsID_name, &string)) {
			name = strdup(string);
		}
		if (xsFindString(xsArg(0), xsID_prompt, &string)) {
			prompt = strdup(string);
		}
	}
	dialog = gtk_file_chooser_dialog_new((message) ? message : prompt, (*view)->gtkWindow, action, "Cancel", GTK_RESPONSE_CANCEL, prompt, GTK_RESPONSE_ACCEPT, NULL);
// 	if ((argc > 0) && xsTest(xsArg(0))) {
// 		if (xsFindString(xsArg(0), xsID_fileType, &string)) {
// 			filter = gtk_file_filter_new();
// 			gtk_file_filter_set_name(filter, string);
// 			gtk_file_filter_add_pattern(filter, string);	
// 			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
// 		}	
// 	}
	if (name)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), name);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
 		(void)xsCallFunction1(xsArg(1), xsNull, xsString(filename));
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
	if (message) free(message);
	if (name) free(name);
	if (prompt) free(prompt);
}

void PiuSystem_openDirectory(xsMachine* the)
{
	PiuSystem_open(the, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
}

void PiuSystem_openFile(xsMachine* the)
{
	PiuSystem_open(the, GTK_FILE_CHOOSER_ACTION_OPEN);
}

void PiuSystem_saveDirectory(xsMachine* the)
{
	PiuSystem_open(the, GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER);
}

void PiuSystem_saveFile(xsMachine* the)
{
	PiuSystem_open(the, GTK_FILE_CHOOSER_ACTION_SAVE);
}
