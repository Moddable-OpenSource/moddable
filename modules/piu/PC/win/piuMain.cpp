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

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE gInstance;
char gWindowRegistryPath[MAX_PATH] = "SOFTWARE\\moddable.tech\\";
char gPreferencesRegistryPath[MAX_PATH] = "SOFTWARE\\moddable.tech\\";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	wchar_t* commandLine = GetCommandLineW();
	int argc = 0, argi;
	LPWSTR* argv = CommandLineToArgvW(commandLine, &argc);
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	char* name = strrchr(buffer, '\\') + 1;
	char* dot = strrchr(buffer, '.');
	*dot = 0;
	
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, name);
    if (hMutex) {
		HWND window = FindWindow("PiuWindow", name);
		if ((nCmdShow != SW_SHOWMINIMIZED) && (nCmdShow != SW_MINIMIZE) && (nCmdShow != SW_SHOWMINNOACTIVE))
			SetForegroundWindow(window);
		for (argi = 1; argi < argc; argi++) {
			wchar_t path[MAX_PATH];
			COPYDATASTRUCT cds;
			_wfullpath(path, argv[argi], MAX_PATH);
			cds.cbData = 2 * (wcslen(path) + 1);
			cds.lpData = path;
			SendMessage(window, WM_COPYDATA, 0, (LPARAM)&cds);
		}
    	return 0;
    }
    hMutex = CreateMutex(0, 0, name);
	strcat(gWindowRegistryPath, name);
	strcat(gWindowRegistryPath, "\\window");
	strcat(gPreferencesRegistryPath, name);
	strcat(gPreferencesRegistryPath, "\\preferences");
    
	ULONG_PTR m_gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
	
	INITCOMMONCONTROLSEX initCtrls = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
	InitCommonControlsEx(&initCtrls);
	
	UINT dwErrMode;
	dwErrMode = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
	OleInitialize(NULL);
	SetErrorMode(dwErrMode);
	
	gInstance = hInstance;

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = PiuWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(PiuView*);
	wcex.hInstance = gInstance;
	wcex.hIcon = LoadIcon(gInstance, MAKEINTRESOURCE(100));
	wcex.hIconSm = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "PiuWindow";
	RegisterClassEx(&wcex);
	
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = PiuClipWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(PiuContent*);
	wcex.hInstance = gInstance;
	wcex.hIcon = NULL;
	wcex.hIconSm = NULL;
	wcex.hCursor = NULL;
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "PiuClipWindow";
	RegisterClassEx(&wcex);
	
	
	PiuApplication* application;
	xsMachine* machine = ServiceThreadMain(NULL);
	xsBeginHost(machine);
	{
		xsVars(2);
		xsResult = xsAwaitImport("main", XS_IMPORT_DEFAULT);
		application = PIU(Application, xsResult);
		xsCollectGarbage();

		if ((*application)->behavior) {
			xsVar(0) = xsReference((*application)->behavior);
			if (xsFindResult(xsVar(0), xsID_onAppearanceChanged)) {
				xsVar(1) = xsReference((*application)->reference);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsUndefined);
			}
		}
	}
	xsEndHost(machine);

	PiuView* view = (*application)->view;
	HWND window = (*view)->window;
	ShowWindow(window, nCmdShow);
	SetFocus(window);

	for (argi = 1; argi < argc; argi++) {
		wchar_t path[MAX_PATH];
		COPYDATASTRUCT cds;
		_wfullpath(path, argv[argi], MAX_PATH);
		cds.cbData = 2 * (wcslen(path) + 1);
		cds.lpData = path;
		SendMessage(window, WM_COPYDATA, 0, (LPARAM)&cds);
	}
		
	MSG msg;
    while( GetMessage(&msg, NULL, 0, 0) > 0 ) {
		if (TranslateAccelerator(window, (*view)->acceleratorTable, &msg))
			continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	//DestroyAcceleratorTable((*piuApplication)->acceleratorTable);
	OleUninitialize();
    GdiplusShutdown(m_gdiplusToken);
    return 0;
}

void fxAbort(xsMachine *the, int status)
{
	PostQuitMessage(0);
}

void fxStringW(xsMachine* the, xsSlot* slot, wchar_t* wideString)
{
	xsIntegerValue stringLength;
	xsStringValue string;
	stringLength = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
	fxStringBuffer(the, slot, NULL, stringLength + 1);
	string = xsToString(*slot);
	WideCharToMultiByte(CP_UTF8, 0, wideString, -1, string, stringLength, NULL, NULL);
	string[stringLength] = 0;
}

wchar_t* fxToStringBufferW(xsMachine* the, xsSlot* slot, wchar_t* wideString, xsIntegerValue theSize)
{
	xsStringValue string = fxToString(the, slot);
	xsIntegerValue wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	if (wideStringLength > theSize)
		xsRangeError("Cannot buffer string");
	MultiByteToWideChar(CP_UTF8, 0, string, -1, wideString, theSize);
	return wideString;
}

wchar_t* fxToStringCopyW(xsMachine* the, xsSlot* slot)
{
	xsStringValue string = fxToString(the, slot);
	xsIntegerValue wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	wchar_t* wideString = (wchar_t*)malloc(wideStringLength * 2);
	xsElseThrow(wideString != NULL);
	MultiByteToWideChar(CP_UTF8, 0, string, -1, wideString, wideStringLength);
	return wideString;
}

wchar_t* str2wcs(char* string)
{
	xsIntegerValue wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	wchar_t* wideString = (wchar_t*)CoTaskMemAlloc(wideStringLength * 2);
	if (wideString)
		MultiByteToWideChar(CP_UTF8, 0, string, -1, wideString, wideStringLength);
	return wideString;
}

wchar_t* strn2wcs(char* string, size_t length)
{
	xsIntegerValue wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string, length, NULL, 0);
	wchar_t* wideString = (wchar_t*)CoTaskMemAlloc((wideStringLength + 1) * 2);
	if (wideString) {
		MultiByteToWideChar(CP_UTF8, 0, string, length, wideString, wideStringLength);
		wideString[wideStringLength] = 0;
	}
	return wideString;
}

void PiuWideStringFree(wchar_t* wideString)
{
	if (wideString)
		CoTaskMemFree((LPVOID*)wideString);
}

void PiuApplication_createMenus(xsMachine *the)
{
}

void PiuApplication_get_title(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	wchar_t title[256];
	GetWindowTextW((*view)->window, title, sizeof(title));
	xsResult = xsStringW(title);
}

void PiuApplication_set_title(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	wchar_t* title = xsToStringCopyW(xsArg(0));
	SetWindowTextW((*view)->window, title);
	free(title);
}

void PiuApplication_gotoFront(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	SetForegroundWindow((*view)->window);
}

void PiuApplication_purge(xsMachine* the)
{
	xsCollectGarbage();
}

void PiuApplication_quit(xsMachine *the)
{
	PostQuitMessage(0);
}

void PiuApplication_updateMenus(xsMachine *the)
{
}

void fxThrowLastError(xsMachine* the, xsStringValue path, xsIntegerValue line)
{
	char buffer[2048];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
	fxThrowMessage(the, path, line, XS_UNKNOWN_ERROR, "%s", buffer);
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








