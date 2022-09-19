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

static void PiuSystem_getFileInfoAux(xsMachine* the, LPWIN32_FIND_DATAW data);

void PiuSystem_get_applicationPath(xsMachine* the)
{
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(GetModuleHandleW(NULL), path, MAX_PATH);
	xsResult = xsStringW(path);
}

void PiuSystem_get_localDirectory(xsMachine* the)
{
	char path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path);
	strcat(path, "\\");
	strcat(path, PIU_DOT_SIGNATURE);
	CreateDirectory(path, NULL);
	xsResult = xsString(path);
}

void PiuSystem_get_platform(xsMachine* the)
{
	xsResult = xsString("win");
}

void PiuSystem_buildPath(xsMachine* the)
{
 	xsIntegerValue argc = xsToInteger(xsArgc);
	xsResult = xsCall2(xsArg(0), xsID_concat, xsString("\\"), xsArg(1));
	if ((argc > 2) && xsTest(xsArg(2)))
		xsResult = xsCall2(xsResult, xsID_concat, xsString("."), xsArg(2));
}

void PiuSystem_copyFile(xsMachine* the)
{
	wchar_t* from = NULL;
	wchar_t* to = NULL;
	xsTry {
		from = xsToStringCopyW(xsArg(0));
		to = xsToStringCopyW(xsArg(1));
		xsElseThrow(CopyFileW(from, to, FALSE));
		free(to);
		free(from);
	}
	xsCatch {
		if (to != NULL)
			free(to);
		if (from != NULL)
			free(from);
		xsThrow(xsException);
	}
}

void PiuSystem_deleteDirectory(xsMachine* the)
{
	wchar_t* path = NULL;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		xsElseThrow(RemoveDirectoryW(path)); //@@ only if empty
		free(path);
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_deleteFile(xsMachine* the)
{
	wchar_t* path = NULL;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		xsElseThrow(DeleteFileW(path));
		free(path);
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_ensureDirectory(xsMachine* the)
{
	wchar_t* path = NULL;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
			wchar_t* p = path;
			while ((p = wcschr(p, '\\'))) {
				*p = 0;
				if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
					xsElseThrow(CreateDirectoryW(path, NULL) != 0);
				}
				*p = '\\';
				p++;
			}
			xsElseThrow(CreateDirectoryW(path, NULL) != 0);
		}
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_fileExists(xsMachine* the)
{
	wchar_t* path = NULL;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		xsResult = (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) ? xsTrue : xsFalse;
		free(path);
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_getFileInfo(xsMachine* the)
{
	wchar_t* path = NULL;
	WIN32_FIND_DATAW data;
	xsVars(1);
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		if (GetFileAttributesExW(path, GetFileExInfoStandard, (LPWIN32_FILE_ATTRIBUTE_DATA)&data)) {
			PiuSystem_getFileInfoAux(the, &data);
			xsVar(0) = xsCall1(xsThis, xsID_getPathName, xsArg(0));
			xsDefine(xsResult, xsID_name, xsVar(0), xsDefault);
			xsDefine(xsResult, xsID_path, xsArg(0), xsDefault);
		}
		free(path);
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_getFileInfoAux(xsMachine* the, LPWIN32_FIND_DATAW data)
{
	static const __int64 BETWEEN_EPOCHS = 116444736000000000;
	__int64 value;
	xsResult = xsNewObject();
	value = data->ftLastWriteTime.dwHighDateTime;
	value = (value << 32) + data->ftLastWriteTime.dwLowDateTime;
	value = (value - BETWEEN_EPOCHS) / 10000;
	xsDefine(xsResult, xsID_date, xsNumber((double)value), xsDefault);
	if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		xsDefine(xsResult, xsID_directory, xsTrue, xsDefault);
	}
	else {
		wchar_t* dot = wcsrchr(data->cFileName, '.');
		if (dot && !wcscmp(dot, L".lnk"))
			xsDefine(xsResult, xsID_symbolicLink, xsTrue, xsDefault);
		value = data->nFileSizeHigh;
		value = (value << 32) + data->nFileSizeLow;
		xsDefine(xsResult, xsID_size, xsNumber((double)value), xsDefault);
	}
}

void PiuSystem_getPathDirectory(xsMachine* the)
{
	wchar_t* path = NULL;
	wchar_t* slash;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		slash = wcsrchr(path, '\\');
		if (slash) {
			*slash = 0;
			xsResult = xsStringW(path);
		}
		else
			xsResult = xsString("");
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_getPathExtension(xsMachine* the)
{
	wchar_t* path = NULL;
	wchar_t* slash;
	wchar_t* dot;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		slash = wcsrchr(path, '\\');
		dot = wcsrchr(slash ? slash : path, '.');
		if (dot)
			xsResult = xsStringW(dot + 1);
		else 
			xsResult = xsString("");
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_getPathName(xsMachine* the)
{
	wchar_t* path = NULL;
	wchar_t* slash;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		slash = wcsrchr(path, '\\');
		if (slash)
			xsResult = xsStringW(slash + 1);
		else
			xsResult = xsString("");
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_getSymbolicLinkInfo(xsMachine* the)
{
	HRESULT hr;
	wchar_t* from = NULL;
	IShellLinkW *iShellLink = NULL;
	IPersistFile* iPersistFile = NULL;
	wchar_t to[MAX_PATH];
	WIN32_FIND_DATAW data;
	xsTry {
		from = xsToStringCopyW(xsArg(0));
		hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&iShellLink);
		if (!SUCCEEDED(hr)) goto bail;
		hr = iShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&iPersistFile);
		if (!SUCCEEDED(hr)) goto bail;
		hr = iPersistFile->Load(from, STGM_READ);
		if (!SUCCEEDED(hr)) goto bail;
   		hr = iShellLink->Resolve((HWND) 0, 0);
		if (!SUCCEEDED(hr)) goto bail;
   		hr = iShellLink->GetPath(&to[0], MAX_PATH, &data, SLGP_SHORTPATH);
		if (!SUCCEEDED(hr)) goto bail;
		PiuSystem_getFileInfoAux(the, &data);
		xsDefine(xsResult, xsID_name, xsStringW(data.cFileName), xsDefault);
		xsDefine(xsResult, xsID_path, xsStringW(to), xsDefault);
	}
	xsCatch {
	}
bail:
	if (from != NULL)
		free(from);
	if (iPersistFile)
		iPersistFile->Release();
	if (iShellLink)
		iShellLink->Release();
}

void PiuSystem_readFileBuffer(xsMachine* the)
{
	wchar_t* path = NULL;
	HANDLE file = INVALID_HANDLE_VALUE;
	DWORD size, result;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		xsElseThrow(file != INVALID_HANDLE_VALUE);
		size = GetFileSize(file, NULL);
		xsResult = xsArrayBuffer(NULL, (xsIntegerValue)size);
		xsElseThrow(ReadFile(file, xsToArrayBuffer(xsResult), size, &result, NULL));
		CloseHandle(file);
		free(path);
	}
	xsCatch {
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_readFileString(xsMachine* the)
{
	wchar_t* path = NULL;
	HANDLE file = INVALID_HANDLE_VALUE;
	DWORD size, result;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		xsElseThrow(file != INVALID_HANDLE_VALUE);
		size = GetFileSize(file, NULL);
		xsResult = xsStringBuffer(NULL, (xsIntegerValue)size);
		xsElseThrow(ReadFile(file, xsToString(xsResult), size, &result, NULL));
		CloseHandle(file);
		free(path);
	}
	xsCatch {
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

void PiuSystem_readPreferenceString(xsMachine* the)
{
	HKEY key;
	xsStringValue name = xsToString(xsArg(0));
	DWORD size;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, gPreferencesRegistryPath, 0, KEY_READ, &key) == ERROR_SUCCESS) {
		RegQueryValueEx(key, name, 0, NULL, NULL, &size);
		xsResult = xsStringBuffer(NULL, size - 1);
		RegQueryValueEx(key, name, 0, NULL, (LPBYTE)xsToString(xsResult), &size);
		RegCloseKey(key);
	}
}

void PiuSystem_renameDirectory(xsMachine* the)
{
	wchar_t* from = NULL;
	wchar_t* slash;
	wchar_t* name = NULL;
	wchar_t* to = NULL;
	xsVars(1);
	xsTry {
		from = xsToStringCopyW(xsArg(0));
		slash = wcsrchr(from, '\\');
		if (!slash) xsURIError("No path");
		*(slash + 1) = 0;
		name = xsToStringCopyW(xsArg(1));
		to = (wchar_t*)malloc((slash - from  + wcslen(name) + 1) * 2);
		wcscpy(to, from);
		wcscat(to, name);
		xsElseThrow(MoveFileW(from, to));
		free(to);
		free(name);
		free(from);
	}
	xsCatch {
		if (to != NULL)
			free(to);
		if (name != NULL)
			free(name);
		if (from != NULL)
			free(from);
		xsThrow(xsException);
	}
}

void PiuSystem_renameFile(xsMachine* the)
{
	PiuSystem_renameDirectory(the);
}

void PiuSystem_writeFileBuffer(xsMachine* the)
{
	wchar_t* path = NULL;
	HANDLE file = INVALID_HANDLE_VALUE;
	void* buffer;
	DWORD size, result;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		xsElseThrow(file != INVALID_HANDLE_VALUE);
		buffer = xsToArrayBuffer(xsArg(1));
		size = (DWORD)xsGetArrayBufferLength(xsArg(1));
		xsElseThrow(WriteFile(file, buffer, size, &result, NULL));
		CloseHandle(file);
		free(path);
	}
	xsCatch {
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
		if (path != NULL)
			free(path);
	}
}

void PiuSystem_writeFileString(xsMachine* the)
{
	wchar_t* path = NULL;
	HANDLE file = INVALID_HANDLE_VALUE;
	xsStringValue buffer;
	DWORD size, result;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		xsElseThrow(file != INVALID_HANDLE_VALUE);
		buffer = xsToString(xsArg(1));
		size = (DWORD)c_strlen(buffer);
		xsElseThrow(WriteFile(file, buffer, size, &result, NULL));
		CloseHandle(file);
		free(path);
	}
	xsCatch {
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
		if (path != NULL)
			free(path);
	}
}


void PiuSystem_writePreferenceString(xsMachine* the)
{
	HKEY key;
	xsStringValue name = xsToString(xsArg(0));
	xsStringValue value = xsToString(xsArg(1));
	if (RegCreateKeyEx(HKEY_CURRENT_USER, gPreferencesRegistryPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS) {
		RegSetValueEx(key, name, 0, REG_SZ, (const BYTE*)value, c_strlen(value) + 1);
		RegCloseKey(key);
	}
}

void PiuSystem_DirectoryIteratorCreate(xsMachine* the)
{
	xsVars(1);
	xsVar(0) = xsCall1(xsArg(0), xsID_concat, xsString("\\"));
	xsSet(xsThis, xsID_path, xsVar(0));
	xsSetHostData(xsThis, INVALID_HANDLE_VALUE);
}

void PiuSystem_DirectoryIteratorDelete(void* it)
{
	HANDLE iterator = it;
	if (iterator != INVALID_HANDLE_VALUE)
		FindClose(iterator);
}

void PiuSystem_DirectoryIterator_next(xsMachine* the)
{
	HANDLE iterator;
	wchar_t* path = NULL;
	WIN32_FIND_DATAW data;
	BOOL result = FALSE;
	xsVars(2);
	xsTry {
		xsVar(0) = xsGet(xsThis, xsID_path);
		iterator = xsGetHostData(xsThis);
		if (iterator == INVALID_HANDLE_VALUE) {
			xsVar(1) = xsCall1(xsVar(0), xsID_concat, xsString("*"));
			path = xsToStringCopyW(xsVar(1));
			iterator = FindFirstFileW(path, &data);
			free(path);
			path = NULL;
			if (iterator != INVALID_HANDLE_VALUE)  {
				xsSetHostData(xsThis, iterator);
				result = TRUE;
			}
		}
		else
			result = FindNextFileW(iterator, &data);
		while (result) {
			if (((data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0) && wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..")) {
				PiuSystem_getFileInfoAux(the, &data);
				xsVar(1) = xsStringW(data.cFileName);
				xsDefine(xsResult, xsID_name, xsVar(1), xsDefault);
				xsVar(0) = xsCall1(xsVar(0), xsID_concat, xsVar(1));
				xsDefine(xsResult, xsID_path, xsVar(0), xsDefault);
				break;
			}
			result = FindNextFileW(iterator, &data);
		}
	}
	xsCatch {
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}

typedef struct PiuDirectoryHelperStruct PiuDirectoryHelperRecord, *PiuDirectoryHelper;
typedef struct PiuDirectoryNotifierStruct PiuDirectoryNotifierRecord, *PiuDirectoryNotifier;

struct PiuDirectoryHelperStruct {
	PiuDirectoryHelper nextHelper;
	PiuDirectoryNotifier firstNotifier;
	HANDLE handle;
	OVERLAPPED overlapped;
	DWORD dummy[256];
	BOOL queued;
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

VOID CALLBACK PiuSystem_DirectoryNotifierCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	PiuDirectoryHelper helper = (PiuDirectoryHelper)(lpOverlapped->hEvent);
	if (!helper->queued) {
		if (helper->handle != INVALID_HANDLE_VALUE)
			CloseHandle(helper->handle);
		c_free(helper);
		return;
	}
	helper->queued = ReadDirectoryChangesW(helper->handle, helper->dummy, sizeof(helper->dummy), FALSE,
		FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
		NULL, &helper->overlapped, PiuSystem_DirectoryNotifierCallback);
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
	wchar_t* path = NULL;
	
	xsTry {
		self = (PiuDirectoryNotifier)c_calloc(1, sizeof(PiuDirectoryNotifierRecord));
		xsElseThrow(self != NULL);
		self->the = the;
		self->reference = xsToReference(xsThis);
		self->path = PiuString(xsArg(0));
		self->callback = xsToReference(xsArg(1));
		xsSetHostData(xsThis, (void*)self);
		xsSetHostHooks(xsThis, &PiuDirectoryNotifierHooks);
		helper = gFirstDirectoryHelper;
		while (helper) {
			if (!c_strcmp(PiuToString(helper->firstNotifier->path), PiuToString(self->path)))
				break;
			helper = helper->nextHelper;
		}
		
		if (!helper) {
			helper = (PiuDirectoryHelper)c_calloc(1, sizeof(PiuDirectoryHelperRecord));
			xsElseThrow(helper != NULL);
			helper->handle = INVALID_HANDLE_VALUE;
			helper->overlapped.hEvent = (HANDLE)helper;
			path = xsToStringCopyW(xsArg(0));
			helper->handle = CreateFileW(path, 
				FILE_LIST_DIRECTORY,
				FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 
				NULL);
			xsElseThrow(helper->handle != INVALID_HANDLE_VALUE);
			helper->queued = ReadDirectoryChangesW(helper->handle, helper->dummy, sizeof(helper->dummy), FALSE,
				FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
				NULL, &helper->overlapped, PiuSystem_DirectoryNotifierCallback);
			xsElseThrow(helper->queued);
			c_free(path);
			path = NULL;
			helper->nextHelper = gFirstDirectoryHelper;
			gFirstDirectoryHelper = helper;
		}
		self->helper = helper;
		self->nextNotifier = helper->firstNotifier;
		helper->firstNotifier = self;

	}
	xsCatch {
		if (path)
			c_free(path);
		if (helper) {
			if (!helper->firstNotifier) {
				if (helper->queued)
					helper->queued = FALSE;
				else {
					if (helper->handle != INVALID_HANDLE_VALUE)
						CloseHandle(helper->handle);
					c_free(helper);
				}
			}
		}
		if (self)
			c_free(self);
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
	c_free(self);
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
		if (helper->queued)
			helper->queued = FALSE;
		else {
			if (helper->handle != INVALID_HANDLE_VALUE)
				CloseHandle(helper->handle);
			c_free(helper);
		}
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



