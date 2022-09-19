/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#define GDIPVER     0x0110
#include "piuPC.h"
#include "screen.h"

#define WM_ABORT_MACHINE WM_USER
#define WM_QUIT_MACHINE WM_USER + 1
#define WM_MESSAGE_TO_APPLICATION WM_USER + 2
#define WM_MESSAGE_TO_HOST WM_USER + 3

typedef struct PiuScreenStruct PiuScreenRecord, *PiuScreen;
typedef struct PiuScreenMessageStruct PiuScreenMessageRecord, *PiuScreenMessage;

struct PiuScreenStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	HWND window;
	HWND control;
    txScreen* screen;
    Bitmap* bitmap;
	ColorMatrix* colorMatrix;
	ImageAttributes* imageAttributes;
	HMODULE library;
	HANDLE archiveFile;
	HANDLE archiveMapping;
	PiuRectangleRecord hole;
	xsIntegerValue rotation;
	xsNumberValue transparency;
};

struct PiuScreenMessageStruct {
	int size;
	char* buffer;
};

static void PiuScreenBind(void* it, PiuApplication* application, PiuView* view);
static PiuScreenMessage PiuScreenCreateMessage(PiuScreen* self, char* buffer, int size);
static void PiuScreenDelete(void* it);
static void PiuScreenDeleteMessage(PiuScreen* self, PiuScreenMessage message);
static void PiuScreenDictionary(xsMachine* the, void* it);
static void PiuScreenMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuScreenQuit(PiuScreen* self);
static void PiuScreenRotatePoint(PiuScreen* self, PiuCoordinate x, PiuCoordinate y, PiuPoint result);
static void PiuScreenUnbind(void* it, PiuApplication* application, PiuView* view);

static void fxScreenAbort(txScreen* screen, int status);
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
		if (screen && screen->touch)  {
			PiuPointRecord point;
			PiuScreenRotatePoint(self, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &point);
			(*screen->touch)(screen, touchEventBeganKind, 0, point.x, point.y, 0);
		}
	} break;
	case WM_LBUTTONUP: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		if (screen && screen->touch)  {
			PiuPointRecord point;
			PiuScreenRotatePoint(self, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &point);
			(*screen->touch)(screen, touchEventEndedKind, 0, point.x, point.y, 0);
		}
		ReleaseCapture();
	} break;
	case WM_MOUSEMOVE: {
        if (wParam & MK_LBUTTON) {
			PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
			txScreen* screen = (*self)->screen;
			if (screen && screen->touch)  {
				PiuPointRecord point;
				PiuScreenRotatePoint(self, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &point);
				(*screen->touch)(screen, touchEventMovedKind, 0, point.x, point.y, 0);
			}
		}
	} break;
	case WM_ERASEBKGND: {
		return 1;
	} break;
	case WM_PAINT: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);
		Graphics graphics(hdc);
 		if (!PiuRectangleIsEmpty(&(*self)->hole)) {
 			const Region region(Rect((*self)->hole.x, (*self)->hole.y, (*self)->hole.width, (*self)->hole.height));
   			HRGN hRegion = region.GetHRGN(&graphics);
 			graphics.SetClip(hRegion, CombineModeExclude);
		}
		graphics.TranslateTransform(((REAL)(*self)->bounds.width)/2.0, ((REAL)(*self)->bounds.height)/2.0);
		graphics.RotateTransform((REAL)(*self)->rotation);
		graphics.TranslateTransform(-((REAL)screen->width)/2.0, -((REAL)screen->height)/2.0);
		if ((*self)->transparency) {
			RectF bounds(0, 0, screen->width, screen->height);
			(*self)->colorMatrix->m[0][0] = 1;
			(*self)->colorMatrix->m[1][1] = 1;
			(*self)->colorMatrix->m[2][2] = 1;
			(*self)->colorMatrix->m[3][3] = 1 - (REAL)((*self)->transparency);
			(*self)->colorMatrix->m[4][4] = 1;
			(*self)->imageAttributes->SetColorMatrix((*self)->colorMatrix);
			graphics.DrawImage((*self)->bitmap, bounds, bounds, UnitPixel, (*self)->imageAttributes);
		}
		else
  			graphics.DrawImage((*self)->bitmap, PointF(0.0f, 0.0f));
		EndPaint(window, &ps);
		return TRUE;
	} break;
	case WM_TIMER: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		if (screen && screen->idle) 
			(*screen->idle)(screen);
	} break;
	case WM_ABORT_MACHINE: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		if ((*self)->behavior) {
			xsBeginHost((*self)->the);
			xsVars(3);
			xsVar(0) = xsReference((*self)->behavior);
			if (xsFindResult(xsVar(0), xsID_onAbort)) {
				xsVar(1) = xsReference((*self)->reference);
				xsVar(2) = xsInteger(wParam);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
			}
			xsEndHost((*self)->the);
		}
	} break;
	case WM_QUIT_MACHINE: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		PiuScreenQuit(self);
	} break;
	case WM_MESSAGE_TO_APPLICATION: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		txScreen* screen = (*self)->screen;
		PiuScreenMessage message = (PiuScreenMessage)lParam;
		if (screen && screen->invoke) {
			(*screen->invoke)(screen, message->buffer, message->size);
		}
		PiuScreenDeleteMessage(self, message);
	} break;
	case WM_MESSAGE_TO_HOST: {
		PiuScreen* self = (PiuScreen*)GetWindowLongPtr(window, 0);
		PiuScreenMessage message = (PiuScreenMessage)lParam;
		if ((*self)->behavior) {
			xsBeginHost((*self)->the);
			xsVars(3);
			xsVar(0) = xsReference((*self)->behavior);
			if (xsFindResult(xsVar(0), xsID_onMessage)) {
				xsVar(1) = xsReference((*self)->reference);
				if (message->size > 0)
					xsVar(2) = xsArrayBuffer(message->buffer, message->size);
				else
					xsVar(2) = xsString(message->buffer);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
			}
			xsEndHost((*self)->the);
		}
		PiuScreenDeleteMessage(self, message);
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

void PiuScreenBind(void* it, PiuApplication* application, PiuView* view)
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
	PiuContentBind(it, application, view);
	
	PiuDimension width = (*self)->coordinates.width;
	PiuDimension height = (*self)->coordinates.height;
	if (((*self)->rotation == 90) || ((*self)->rotation == 270)) {
		PiuDimension tmp = width;
		width = height;
		height = tmp;
	}
    txScreen* screen = (txScreen*)malloc(sizeof(txScreen) - 1 + (width * height * 4));
    memset(screen, 0, sizeof(txScreen) - 1 + (width * height * 4));
    screen->view = self;
    screen->abort = fxScreenAbort;
    screen->bufferChanged = fxScreenBufferChanged;
	screen->formatChanged = fxScreenFormatChanged;
    screen->post = fxScreenPost;
    screen->start = fxScreenStart;
    screen->stop = fxScreenStop;
	mxCreateMutex(&screen->workersMutex);
    screen->width = width;
    screen->height = height;
    (*self)->screen = screen;
    
    (*self)->bitmap = new Bitmap(width, height, 4 * width, PixelFormat32bppRGB, screen->buffer);
    (*self)->colorMatrix = new ColorMatrix();
    (*self)->imageAttributes = new ImageAttributes();
    
	(*self)->window = CreateWindowEx(0, "PiuClipWindow", NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, 0, 0, 0, (*view)->window, NULL, gInstance, (LPVOID)self);
	(*self)->control = CreateWindowEx(0, "PiuScreenControl", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, (*self)->window, NULL, gInstance, (LPVOID)self);
}

PiuScreenMessage PiuScreenCreateMessage(PiuScreen* self, char* buffer, int size) 
{
	PiuScreenMessage message = (PiuScreenMessage)malloc(sizeof(PiuScreenMessageRecord));
	if (!message)
		return NULL;
	message->size = size;
	if (size < 0)
		message->buffer = buffer;
	else {
		if (!size)
			size = strlen(buffer) + 1;;
		message->buffer = (char*)malloc(size);
		if (!message->buffer) {
			free(message);
			return NULL;
		}
		memcpy(message->buffer, buffer, size);
	}
	return message;
}

void PiuScreenDelete(void* it) 
{
}

void PiuScreenDeleteMessage(PiuScreen* self, PiuScreenMessage message) 
{
	free(message->buffer);
	free(message);
}


void PiuScreenDictionary(xsMachine* the, void* it) 
{
	PiuScreen* self = (PiuScreen*)it;
	xsIntegerValue integer;
	if (xsFindInteger(xsArg(1), xsID_rotation, &integer)) {
		(*self)->rotation = integer;
	}
}

void PiuScreenRotatePoint(PiuScreen* self, PiuCoordinate x, PiuCoordinate y, PiuPoint result)
{
    switch ((*self)->rotation) {
    case 0:
    	result->x = x;
		result->y = y;
		break;
    case 90:
    	result->x = y;
        result->y = (*self)->bounds.width - x;
		break;
    case 180:
    	result->x = (*self)->bounds.width - x;
        result->y = (*self)->bounds.height - y;
		break;
    case 270:
    	result->x = (*self)->bounds.height - y;
		result->y = x;;
		break;
    }
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
	if ((*self)->screen->archive) {
		UnmapViewOfFile((*self)->screen->archive);
		(*self)->screen->archive = NULL;
	}
	if ((*self)->archiveMapping) {
		CloseHandle((*self)->archiveMapping);
		(*self)->archiveMapping = INVALID_HANDLE_VALUE;
	}
	if ((*self)->archiveFile) {
		CloseHandle((*self)->archiveFile);
		(*self)->archiveFile = INVALID_HANDLE_VALUE;
	}
	if ((*self)->library) {
		wchar_t path[MAX_PATH];
		HANDLE file;
		GetModuleFileNameW((*self)->library, path, 1024);
		FreeLibrary((*self)->library);
		file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		while (file == INVALID_HANDLE_VALUE) {
			Sleep(1);
			file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		CloseHandle(file);
		(*self)->library = NULL;
	}
	InvalidateRect((*self)->control, NULL, FALSE);
}

void PiuScreenUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuScreen* self = (PiuScreen*)it;
	txScreen* screen = (*self)->screen;
	PiuScreenQuit(self);
	DestroyWindow((*self)->window);
	(*self)->control = NULL;
	(*self)->window = NULL;
	
	delete (*self)->colorMatrix;
	(*self)->colorMatrix = NULL;
	delete (*self)->imageAttributes;
	(*self)->imageAttributes = NULL;
	delete (*self)->bitmap;
	(*self)->bitmap = NULL;
	mxDeleteMutex(&screen->workersMutex);
	free(screen);
	(*self)->screen = NULL;
	PiuContentUnbind(it, application, view);
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

void PiuScreen_get_hole(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_x, xsPiuCoordinate((*self)->hole.x), xsDefault);
	xsDefine(xsResult, xsID_y, xsPiuCoordinate((*self)->hole.y), xsDefault);
	xsDefine(xsResult, xsID_width, xsPiuDimension((*self)->hole.width), xsDefault);
	xsDefine(xsResult, xsID_height, xsPiuDimension((*self)->hole.height), xsDefault);
}

void PiuScreen_set_hole(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsIntegerValue value;
	if (xsFindInteger(xsArg(0), xsID_x, &value)) {
		(*self)->hole.x = value;
	}
	if (xsFindInteger(xsArg(0), xsID_y, &value)) {
		(*self)->hole.y = value;
	}
	if (xsFindInteger(xsArg(0), xsID_width, &value)) {
		(*self)->hole.width = value;
	}
	if (xsFindInteger(xsArg(0), xsID_height, &value)) {
		(*self)->hole.height = value;
	}
	PiuContentInvalidate(self, NULL);
}

void PiuScreen_get_pixelFormat(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = xsInteger((*self)->screen->pixelFormat);
}

void PiuScreen_get_running(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = (*self)->library ? xsTrue : xsFalse;
}
		
void PiuScreen_get_transparency(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = xsNumber((*self)->transparency);
}

void PiuScreen_set_transparency(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	(*self)->transparency = xsToNumber(xsArg(0));
	PiuContentInvalidate(self, NULL);
}

void PiuScreen_launch(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	wchar_t* libraryPath = NULL;
	wchar_t* archivePath = NULL;
	HMODULE library = NULL;
	txScreenLaunchProc launch;
	HANDLE archiveFile = INVALID_HANDLE_VALUE;
	DWORD size;
	HANDLE archiveMapping = INVALID_HANDLE_VALUE;
	void* archive = NULL;
	xsTry {
		libraryPath = xsToStringCopyW(xsArg(0));
		if (xsToInteger(xsArgc) > 1)
			archivePath = xsToStringCopyW(xsArg(1));
		library = LoadLibraryW(libraryPath);
		xsElseThrow(library != NULL);
		launch = (txScreenLaunchProc)GetProcAddress(library, "fxScreenLaunch");
		xsElseThrow(launch != NULL);
		if (archivePath) {
			archiveFile = CreateFileW(archivePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			xsElseThrow(archiveFile != INVALID_HANDLE_VALUE);
			size = GetFileSize(archiveFile, &size);
			xsElseThrow(size != INVALID_FILE_SIZE);
			archiveMapping = CreateFileMapping(archiveFile, NULL, PAGE_READWRITE, 0, (SIZE_T)size, NULL);
			xsElseThrow(archiveMapping != INVALID_HANDLE_VALUE);
			archive = MapViewOfFile(archiveMapping, FILE_MAP_WRITE, 0, 0, (SIZE_T)size);
			xsElseThrow(archive != NULL);
		}
		(*self)->library = library;
		(*self)->archiveFile = archiveFile;
		(*self)->archiveMapping = archiveMapping;
		(*self)->screen->archive = archive;
		(*launch)((*self)->screen);
		if (archivePath)
			free(archivePath);
		free(libraryPath);
	}
	xsCatch {
		if (archive)
			UnmapViewOfFile(archive);
		if (archiveMapping)
			CloseHandle(archiveMapping);
		if (archiveFile)
			CloseHandle(archiveFile);
		if (library != NULL)
			FreeLibrary(library);
		if (archivePath != NULL)
			free(archivePath);
		if (libraryPath != NULL)
			free(libraryPath);
		xsThrow(xsException);
	}
}
	
void PiuScreen_postMessage(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	PiuScreenMessage message;
	if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		int size = (int)xsGetArrayBufferLength(xsArg(0));
		message = PiuScreenCreateMessage(self, (char*)xsToArrayBuffer(xsArg(0)), size);
	}
	else {
		xsStringValue string = xsToString(xsArg(0));
		message = PiuScreenCreateMessage(self, string, 0);
	}
	if (message)
		PostMessage((*self)->control, WM_MESSAGE_TO_APPLICATION, (WPARAM)sizeof(message), (LPARAM)message);
}

void PiuScreen_quit(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	PiuScreenQuit(self);
}

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

void PiuScreen_writePNG(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	txScreen* screen = (*self)->screen;
	wchar_t* path = NULL;
	Bitmap* bitmap = NULL;
	CLSID encoderClsid;
	Status status;
	xsTry {
		path = xsToStringCopyW(xsArg(0));
		bitmap = new Bitmap(screen->width, screen->height, 4 * screen->width, PixelFormat32bppRGB, screen->buffer);
		GetEncoderClsid(L"image/png", &encoderClsid);
  		status = bitmap->Save(path, &encoderClsid, NULL);
		xsElseThrow(status == Ok);
		delete bitmap;
		free(path);
	}
	xsCatch {
		if (bitmap != NULL)
			delete bitmap;
		if (path != NULL)
			free(path);
	}
}

void fxScreenAbort(txScreen* screen, int status)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	PostMessage((*self)->control, WM_ABORT_MACHINE, status, 0);
}

void fxScreenBufferChanged(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	InvalidateRect((*self)->control, NULL, FALSE);
}

void fxScreenFormatChanged(txScreen* screen)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	if (self && (*self)->behavior) {
		xsBeginHost((*self)->the);
		xsVars(3);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onPixelFormatChanged)) {
			xsVar(1) = xsReference((*self)->reference);
			xsVar(2) = xsInteger(screen->pixelFormat);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
		}
		xsEndHost((*self)->the);
	}
}

void fxScreenPost(txScreen* screen, char* buffer, int size)
{
	PiuScreen* self = (PiuScreen*)screen->view;
	PiuScreenMessage message = PiuScreenCreateMessage(self, buffer, size);
	if (message)
		PostMessage((*self)->control, WM_MESSAGE_TO_HOST, (WPARAM)sizeof(message), (LPARAM)message);
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

extern "C" {
	extern void PiuScreenWorkerCreateAux(xsMachine* the, txScreen* screen);
}
void PiuScreenWorkerCreate(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsArg(1));
	PiuScreenWorkerCreateAux(the, (*self)->screen);
}

