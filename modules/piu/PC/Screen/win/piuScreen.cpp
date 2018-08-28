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

#define WM_QUIT_MACHINE WM_USER
#define WM_MESSAGE_TO_APPLICATION WM_USER + 1
#define WM_MESSAGE_TO_HOST WM_USER + 2

typedef struct PiuScreenStruct PiuScreenRecord, *PiuScreen;

struct PiuScreenStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	HWND window;
	HWND control;
    txScreen* screen;
    HDC dc;
    HBITMAP bitmap;
    BITMAPINFO* bitmapInfo;
	HMODULE library;
};

static void PiuScreenBind(void* it, PiuApplication* application);
static void PiuScreenDelete(void* it);
static void PiuScreenDictionary(xsMachine* the, void* it);
static void PiuScreenQuit(PiuScreen* self);
static void PiuScreenMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuScreenUnbind(void* it, PiuApplication* application);

static void fxScreenAbort(txScreen* screen);
static void fxScreenBufferChanged(txScreen* screen);
static void fxScreenFormatChanged(txScreen* screen);
static void fxScreenPost(txScreen* screen, char* message, int size);
static void fxScreenStart(txScreen* screen, double interval);
static void fxScreenStop(txScreen* screen);

LRESULT CALLBACK PiuScreenControlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	static BOOL dragging = FALSE;
	switch(message)	{
	case WM_CREATE: {
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(window, 0, (LONG_PTR)cs->lpCreateParams);
	} break;
	case WM_LBUTTONDOWN: {
		SetCapture(window);
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		if (screen && screen->touch) 
			(*screen->touch)(screen, touchEventBeganKind, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
	} break;
	case WM_LBUTTONUP: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		if (screen && screen->touch) 
			(*screen->touch)(screen, touchEventEndedKind, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		ReleaseCapture();
	} break;
	case WM_MOUSEMOVE: {
        if (wParam & MK_LBUTTON) {
			PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
			txScreen* screen = (*self)->screen;
			if (screen && screen->touch) 
				(*screen->touch)(screen, touchEventMovedKind, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		}
	} break;
	case WM_PAINT: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);
		HGDIOBJ object = SelectObject((*self)->dc, (*self)->bitmap);
		SetDIBits((*self)->dc, (*self)->bitmap, 0, screen->height, screen->buffer, (*self)->bitmapInfo, DIB_RGB_COLORS);
		BitBlt(hdc, 0, 0, screen->width, screen->height, (*self)->dc, 0, 0, SRCCOPY);
		SelectObject((*self)->dc, object);
		EndPaint(window, &ps);
		return TRUE;
	} break;
	case WM_TIMER: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		if (screen && screen->idle) 
			(*screen->idle)(screen);
	} break;
	case WM_QUIT_MACHINE: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		PiuScreenQuit(self);
	} break;
	case WM_MESSAGE_TO_APPLICATION: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		if (screen && screen->invoke)
			(*screen->invoke)(screen, (char*)lParam, (int)wParam);
		free((void*)lParam);
	} break;
	case WM_MESSAGE_TO_HOST: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		if ((*self)->behavior) {
			xsBeginHost((*self)->the);
			xsVars(3);
			xsVar(0) = xsReference((*self)->behavior);
			if (xsFindResult(xsVar(0), xsID_onMessage)) {
				xsVar(1) = xsReference((*self)->reference);
				if (wParam)
					xsVar(2) = xsArrayBuffer((xsStringValue)lParam, (xsIntegerValue)wParam);
				else
					xsVar(2) = xsString((xsStringValue*)lParam);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
			}
			xsEndHost((*self)->the);
		}
		free((void*)lParam);
	} break;
	default:
		return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;
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

void PiuScreenBind(void* it, PiuApplication* application)
{
	static ATOM windowClass = 0;
	if (!windowClass) {
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = PiuScreenControlProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(txScreen*);
		wcex.hInstance = gInstance;
		wcex.hIcon = NULL;
		wcex.hIconSm = NULL;
		wcex.hCursor = NULL;
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = "PiuScreenControl";
		windowClass = RegisterClassEx(&wcex);
	}
	PiuScreen* self = (PiuScreen*)it;
	PiuContentBind((PiuContent*)it, application);
	
	PiuDimension width = (*self)->coordinates.width;
	PiuDimension height = (*self)->coordinates.height;
    txScreen* screen = (txScreen*)malloc(sizeof(txScreen) - 1 + (width * height * 4));
    memset(screen, 0, sizeof(txScreen) - 1 + (width * height * 4));
    screen->view = self;
    screen->abort = fxScreenAbort;
    screen->bufferChanged = fxScreenBufferChanged;
    screen->formatChanged = fxScreenFormatChanged;
    screen->post = fxScreenPost;
    screen->start = fxScreenStart;
    screen->stop = fxScreenStop;
    screen->width = width;
    screen->height = height;
    (*self)->screen = screen;
    
	HDC hdc = GetDC(NULL);
	(*self)->dc = CreateCompatibleDC(hdc); 
	(*self)->bitmap = CreateCompatibleBitmap(hdc, width, height);
	(*self)->bitmapInfo = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO) + (2 * sizeof(RGBQUAD)));
	(*self)->bitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	(*self)->bitmapInfo->bmiHeader.biWidth = width;
	(*self)->bitmapInfo->bmiHeader.biHeight = -height;
	(*self)->bitmapInfo->bmiHeader.biPlanes = 1;
	(*self)->bitmapInfo->bmiHeader.biBitCount = 32;
	(*self)->bitmapInfo->bmiHeader.biCompression = BI_BITFIELDS;
	(*self)->bitmapInfo->bmiColors[0].rgbBlue = 0xFF;
	(*self)->bitmapInfo->bmiColors[1].rgbGreen = 0xFF;
	(*self)->bitmapInfo->bmiColors[2].rgbRed = 0xFF;
    
	PiuView* view = (*application)->view;
	(*self)->window = CreateWindowEx(0, "PiuClipWindow", NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, 0, 0, 0, (*view)->window, NULL, gInstance, (LPVOID)self);
	(*self)->control = CreateWindowEx(0, "PiuScreenControl", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, (*self)->window, NULL, gInstance, (LPVOID)self);
}

void PiuScreenDictionary(xsMachine* the, void* it) 
{
}

void PiuScreenDelete(void* it) 
{
}

void PiuScreenMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuContentMark(the, it, markRoot);
}

void PiuScreenQuit(PiuScreen* self) 
{
	MSG msg;
	txScreen* screen = (*self)->screen;
	if (screen && screen->quit)
		(*screen->quit)(screen);
    while (PeekMessage(&msg, (*self)->control, WM_MESSAGE_TO_APPLICATION, WM_MESSAGE_TO_HOST, PM_REMOVE)) {
		free((void*)msg.lParam);
	}		
	KillTimer((*self)->control, 0);
	if ((*self)->library) {
		FreeLibrary((*self)->library);
		(*self)->library = NULL;
	}
	InvalidateRect((*self)->control, NULL, FALSE);
}

void PiuScreenUnbind(void* it, PiuApplication* application)
{
	PiuScreen* self = (PiuScreen*)it;
	PiuScreenQuit(self);
	DestroyWindow((*self)->window);
	(*self)->control = NULL;
	(*self)->window = NULL;
	
	free((*self)->bitmapInfo);
	(*self)->bitmapInfo = NULL;
	DeleteObject((*self)->bitmap);
	(*self)->bitmap = NULL;
	DeleteObject((*self)->dc);
	(*self)->dc = NULL;
	free((*self)->screen);
	(*self)->screen = NULL;
	PiuContentUnbind(it, application);
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
	wchar_t* path = NULL;
	HMODULE library = NULL;
	txScreenLaunchProc launch;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		library = LoadLibraryW(path);
		xsElseThrow(library != NULL);
		launch = (txScreenLaunchProc)GetProcAddress(library, "fxScreenLaunch");
		xsElseThrow(launch != NULL);
		(*self)->library = library;
		(*launch)((*self)->screen);
	}
	xsCatch {
		if (library != NULL)
			FreeLibrary(library);
		if (path != NULL)
			free(path);
		xsThrow(xsException);
	}
}
	
void PiuScreen_postMessage(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	int size;
	char* buffer;
	if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		size = (int)xsGetArrayBufferLength(xsArg(0));
		buffer = (char*)c_malloc(size);
		xsElseThrow(buffer != NULL);
		c_memcpy(buffer, xsToArrayBuffer(xsArg(0)), size);
	}
	else {
		size = 0;
		buffer = xsToStringCopy(xsArg(0));
	}
	PostMessage((*self)->control, WM_MESSAGE_TO_APPLICATION, (WPARAM)size, (LPARAM)buffer);
}

void PiuScreen_quit(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	PiuScreenQuit(self);
}

void fxScreenAbort(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	PostMessage((*self)->control, WM_QUIT_MACHINE, 0, 0);
}

void fxScreenBufferChanged(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	InvalidateRect((*self)->control, NULL, FALSE);
}

void fxScreenFormatChanged(txScreen* screen)
{
}

void fxScreenPost(txScreen* screen, char* message, int size)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	char* buffer;
	if (size) {
		buffer = (char*)c_malloc(size);
		if (buffer)
			c_memcpy(buffer, message, size);
	}
	else {
		buffer = _strdup(message);
	}
	if (buffer)
		PostMessage((*self)->control, WM_MESSAGE_TO_HOST, (WPARAM)size, (LPARAM)buffer);
}

void fxScreenStart(txScreen* screen, double interval)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	SetTimer((*self)->control, 0, (UINT)interval, NULL);
}

void fxScreenStop(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	KillTimer((*self)->control, 0);
}



