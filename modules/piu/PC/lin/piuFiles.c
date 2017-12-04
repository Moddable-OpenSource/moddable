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

static void PiuSystem_getFileInfoAux(xsMachine* the, char *path);

void PiuSystem_get_platform(xsMachine* the)
{
	xsResult = xsString("lin");
}

void PiuSystem_deleteDirectory(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	if (rmdir(path))
		xsThrowErrno();
}

void PiuSystem_deleteFile(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	if (unlink(path))
		xsThrowErrno();
}

void PiuSystem_ensureDirectory(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	int result = mkdir(path, 0755);
	if (result && (errno != EEXIST))
		xsThrowErrno();
}

void PiuSystem_fileExists(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	struct stat a_stat;
	xsResult = (stat(path, &a_stat) == 0) ? xsTrue : xsFalse;
}

void PiuSystem_getFileInfo(xsMachine* the)
{
	char path[PATH_MAX];
	xsToStringBuffer(xsArg(0), path, sizeof(path));
	PiuSystem_getFileInfoAux(the, path);
	if (xsTest(xsResult)) {
		xsStringValue slash = c_strrchr(path, '/');
		xsDefine(xsResult, xsID_name, xsString(slash + 1), xsDefault);
		xsDefine(xsResult, xsID_path, xsArg(0), xsDefault);
	}
}

void PiuSystem_getFileInfoAux(xsMachine* the, char *path)
{
	struct stat _stat;
	if (stat(path, &_stat) == 0) {
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_date, xsNumber((double)_stat.st_mtime), xsDefault);
		if (S_ISDIR(_stat.st_mode)) {
			xsDefine(xsResult, xsID_directory, xsTrue, xsDefault);
		}
		else {
			xsDefine(xsResult, xsID_size, xsNumber((double)_stat.st_mode), xsDefault);
		}
	}
}

void PiuSystem_getPathDirectory(xsMachine* the)
{
	xsStringValue path = xsToString(xsArg(0));
	xsStringValue slash = c_strrchr(path, '/');
	if (slash)
		xsResult = xsCall2(xsArg(0), xsID_slice, xsInteger(0), xsInteger(slash - path));
	else
		xsResult = xsString("");
}

void PiuSystem_getPathExtension(xsMachine* the)
{
	xsStringValue path = xsToString(xsArg(0));
	xsStringValue slash = c_strrchr(path, '/');
	xsStringValue dot = c_strrchr(slash ? slash : path, '.');
	if (slash)
		xsResult = xsCall1(xsArg(0), xsID_slice, xsInteger(dot - path + 1));
	else
		xsResult = xsString("");
}

void PiuSystem_getPathName(xsMachine* the)
{
	xsStringValue path = xsToString(xsArg(0));
	xsStringValue slash = c_strrchr(path, '/');
	if (slash)
		xsResult = xsCall1(xsArg(0), xsID_slice, xsInteger(slash - path + 1));
	else
		xsResult = xsArg(0);
}

void PiuSystem_preferenceAux(xsMachine* the)
{
	char path[PATH_MAX];
	PiuConfigPath(path);
	strcat(path, ".");
	strcat(path, xsToString(xsArg(0)));
	strcat(path, ".json");
	xsArg(0) = xsString(path);
}

void PiuSystem_readFileBuffer(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	size_t size;
	xsTry {
		file = fopen(path, "rb");
		xsElseThrow(file);
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		xsResult = xsArrayBuffer(NULL, size);
		fread(xsToArrayBuffer(xsResult), 1, size, file);	
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void PiuSystem_readFileString(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	size_t size;
	xsTry {
		file = fopen(path, "r");
		xsElseThrow(file);
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		xsResult = xsStringBuffer(NULL, size);
		fread(xsToString(xsResult), 1, size, file);	
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void PiuSystem_readPreferenceString(xsMachine* the)
{
	PiuSystem_preferenceAux(the);
	PiuSystem_readFileString(the);
}

void PiuSystem_renameDirectory(xsMachine* the)
{
	xsStringValue from = xsToString(xsArg(0));
	xsStringValue slash = c_strrchr(from, '/');
	xsStringValue to;
	xsVars(1);
	if (!slash) xsURIError("No path");
	xsVar(0) = xsCall2(xsArg(0), xsID_slice, xsInteger(0), xsInteger(slash - from + 1));
	xsVar(0) = xsCall1(xsVar(0), xsID_concat, xsArg(1));
	to = xsToString(xsVar(0));
	if (rename(from, to))
		xsThrowErrno();
}

void PiuSystem_renameFile(xsMachine* the)
{
	xsStringValue from = xsToString(xsArg(0));
	xsStringValue slash = c_strrchr(from, '/');
	xsStringValue to;
	xsVars(1);
	if (!slash) xsURIError("No path");
	xsVar(0) = xsCall2(xsArg(0), xsID_slice, xsInteger(0), xsInteger(slash - from + 1));
	xsVar(0) = xsCall1(xsVar(0), xsID_concat, xsArg(1));
	to = xsToString(xsVar(0));
	if (rename(from, to))
		xsThrowErrno();
}

void PiuSystem_writeFileBuffer(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char* buffer = xsToArrayBuffer(xsArg(1));
	size_t size = xsGetArrayBufferLength(xsArg(1));
	FILE* file = NULL;
	xsTry {
		file = fopen(path, "wb");
		xsElseThrow(file);
		fwrite(buffer, 1, size, file);		
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void PiuSystem_writeFileString(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char* buffer = xsToString(xsArg(1));
	size_t size = strlen(buffer);
	FILE* file = NULL;
	xsTry {
		file = fopen(path, "w");
		xsElseThrow(file);
		fwrite(buffer, 1, size, file);		
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void PiuSystem_writePreferenceString(xsMachine* the)
{
	PiuSystem_preferenceAux(the);
	PiuSystem_writeFileString(the);
}

void PiuSystem_DirectoryIteratorCreate(xsMachine* the)
{
	xsSet(xsThis, xsID_path, xsArg(0));
	xsSetHostData(xsThis, NULL);
}

void PiuSystem_DirectoryIteratorDelete(void* it)
{
	GDir* iterator = it;
	if (iterator != NULL)
		g_dir_close(iterator);
}

void PiuSystem_DirectoryIterator_next(xsMachine* the)
{
	GDir* iterator = xsGetHostData(xsThis);
	char path[PATH_MAX];
	char* name;
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID_path);
	xsToStringBuffer(xsVar(0), path, sizeof(path));
	if (iterator == NULL) {
		GError* error = NULL;
		iterator = g_dir_open(path, 0, &error);
		if (iterator == NULL)
			xsUnknownError(error->message);
		xsSetHostData(xsThis, iterator);
	}
	name = (char*)g_dir_read_name(iterator);
	if (name) {
		strcat(path, "/");
		strcat(path, name);
		PiuSystem_getFileInfoAux(the, path);
		xsDefine(xsResult, xsID_name, xsString(name), xsDefault);
		xsDefine(xsResult, xsID_path, xsString(path), xsDefault);
	}
}

typedef struct PiuDirectoryHelperStruct PiuDirectoryHelperRecord, *PiuDirectoryHelper;
typedef struct PiuDirectoryNotifierStruct PiuDirectoryNotifierRecord, *PiuDirectoryNotifier;

struct PiuDirectoryHelperStruct {
	PiuDirectoryHelper nextHelper;
	PiuDirectoryNotifier firstNotifier;
	GFileMonitor* monitor;
};

struct PiuDirectoryNotifierStruct {
	PiuDirectoryHelper helper;
	PiuDirectoryNotifier nextNotifier;
	xsMachine* the;
	xsSlot* reference;
	xsSlot* path;
	xsSlot* callback;
};

static void PiuSystem_DirectoryNotifierMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static PiuDirectoryHelper gFirstDirectoryHelper = NULL;

static xsHostHooks PiuDirectoryNotifierHooks ICACHE_RODATA_ATTR = {
	PiuSystem_DirectoryNotifierDelete,
	PiuSystem_DirectoryNotifierMark,
	NULL
};

static void PiuDirectoryNotifierCallback(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	PiuDirectoryHelper helper = user_data;
	PiuDirectoryNotifier self = helper->firstNotifier;
	while (self) {
		xsBeginHost(self->the);
		xsVars(3);
		xsVar(0) = xsReference(self->callback);
		xsVar(1) = xsReference(self->reference);
		xsVar(2) = *(self->path);
		xsCallFunction1(xsVar(0), xsVar(1), xsVar(2));
		xsEndHost(self->the);
		self = self->nextNotifier;
	}
}

void PiuSystem_DirectoryNotifierCreate(xsMachine* the)
{
	PiuDirectoryNotifier self = NULL;
	PiuDirectoryHelper helper = NULL;
	GFile* file = NULL;
	GError* error = NULL;
	
	xsTry {
		self = (PiuDirectoryNotifier)calloc(1, sizeof(PiuDirectoryNotifierRecord));
		xsElseThrow(self != NULL);
		self->the = the;
		self->reference = xsToReference(xsThis);
		self->path = PiuString(xsArg(0));
		self->callback = xsToReference(xsArg(1));
		xsSetHostData(xsThis, (void*)self);
		xsSetHostHooks(xsThis, &PiuDirectoryNotifierHooks);
		helper = gFirstDirectoryHelper;
		while (helper) {
			if (!strcmp(PiuToString(helper->firstNotifier->path), PiuToString(self->path)))
				break;
			helper = helper->nextHelper;
		}
		if (!helper) {
			helper = (PiuDirectoryHelper)calloc(1, sizeof(PiuDirectoryHelperRecord));
			xsElseThrow(helper != NULL);
			file = g_file_new_for_path(xsToString(xsArg(0)));
			helper->monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL, &error);
			if (helper->monitor == NULL)
				xsUnknownError(error->message);
			g_signal_connect(G_OBJECT(helper->monitor), "changed", G_CALLBACK(PiuDirectoryNotifierCallback), helper);
			g_object_unref(file);
			file = NULL;
			helper->nextHelper = gFirstDirectoryHelper;
			gFirstDirectoryHelper = helper;
		}
		self->helper = helper;
		self->nextNotifier = helper->firstNotifier;
		helper->firstNotifier = self;
	}
	xsCatch {
		if (error)
			g_clear_error(&error);
		if (file)
			g_object_unref(file);
		if (helper) {
			if (!helper->firstNotifier) {
				if (helper->monitor)
					g_object_unref(helper->monitor);
				free(helper);
			}
		}
		if (self)
			free(self);
		xsSetHostData(xsThis, NULL);
	}
}

void PiuSystem_DirectoryNotifierDelete(void* it)
{
	if (!it) return;
	PiuDirectoryNotifier self = (PiuDirectoryNotifier)it;
	PiuDirectoryHelper helper = self->helper;
	PiuDirectoryNotifier* notifierAddress = &helper->firstNotifier;
	PiuDirectoryNotifier notifier;
	while ((notifier = *notifierAddress)) {
		if (notifier == self) {
			*notifierAddress = self->nextNotifier;
			break;
		}
		notifierAddress = &notifier->nextNotifier;
	}
	free(self);
	if (!helper->firstNotifier) {
		PiuDirectoryHelper* helperAddress = &gFirstDirectoryHelper;
		PiuDirectoryHelper current;
		while ((current = *helperAddress)) {
			if (current == helper) {
				*helperAddress = helper->nextHelper;
				break;
			}
			helperAddress = &current->nextHelper;
		}
		if (helper->monitor)
			g_object_unref(helper->monitor);
		free(helper);
	}
}

void PiuSystem_DirectoryNotifierMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuDirectoryNotifier self = (PiuDirectoryNotifier)it;
	PiuMarkString(the, self->path);
	PiuMarkReference(the, self->callback);
}

void PiuSystem_DirectoryNotifier_close(xsMachine* the)
{
	PiuDirectoryNotifier self = (PiuDirectoryNotifier)xsGetHostData(xsThis);
	PiuSystem_DirectoryNotifierDelete(self);
	xsSetHostData(xsThis, NULL);
}



