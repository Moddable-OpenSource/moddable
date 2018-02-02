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

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h> 
#include <shlguid.h>
#include <objidl.h>
#include <ole2.h>
#include <oleidl.h>
#include <gdiplus.h>
using namespace Gdiplus;

struct PiuFontStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuFont* next;
	PiuFlags flags;
	xsIndex family;
	PiuCoordinate size;
	PiuCoordinate weight;
	PrivateFontCollection* collection;
	Font* object;
	REAL ascent;
	REAL delta;
	REAL height;
};

struct PiuTextureStruct {
	PiuHandlePart;
	PiuAssetPart;
	Image* image;
	xsNumberValue scale;
	PiuDimension width;
	PiuDimension height;
};

struct PiuViewStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuApplication* application;
	HWND window;
	HACCEL acceleratorTable;
	PrivateFontCollection* fontCollection;
	SolidBrush* solidBrush;
	Graphics* graphics;
	xsIntegerValue graphicsStatesIndex;
	GraphicsState graphicsStates[256];
};

#ifdef __cplusplus
extern "C" {
#endif
extern xsMachine* ServiceThreadMain();
extern void fxAbort(xsMachine *the);
#ifdef __cplusplus
}
#endif

extern LRESULT CALLBACK PiuWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK PiuClipWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

extern HINSTANCE gInstance;
extern char gPreferencesRegistryPath[];
extern char gWindowRegistryPath[];

