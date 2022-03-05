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


LRESULT CALLBACK PiuFieldProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
	if ((message == WM_NCCALCSIZE) && wParam) {
		NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
		RECT* rect = params->rgrc;
		PiuField* self = (PiuField*)data;
		PiuStyle* style = (*self)->computedStyle;
		int delta;
		rect->left += (*style)->margins.left;
		rect->right -= (*style)->margins.right;
		rect->top += (*style)->margins.top;
		rect->bottom -= (*style)->margins.bottom;
		switch ((*style)->vertical) {
		case piuTop:
			break;
		case piuBottom:
			delta = rect->bottom - rect->top - PiuFontGetHeight((*style)->font);
			rect->top += delta;
			break;
		default:
			delta = (rect->bottom - rect->top - PiuFontGetHeight((*style)->font)) / 2;
			rect->top += delta;
			rect->bottom -= delta;
			break;
		}
	}
	if (message == WM_SETFOCUS) {
		PiuField* self = (PiuField*)data;
		if ((*self)->application)
			PiuApplicationSetFocus((*self)->application, self);
	}
   return DefSubclassProc(control, message, wParam, lParam);
}

void PiuFieldBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuField* self = (PiuField*)it;
	xsMachine* the = (*self)->the;
	PiuContentBind(it, application, view);
	PiuFieldComputeStyle(self);
	PiuSkin* skin = (*self)->skin;
	PiuStyle* style = (*self)->computedStyle;

	(*self)->solidBrush = CreateSolidBrush(RGB((*skin)->data.color.fill[0].r, (*skin)->data.color.fill[0].g, (*skin)->data.color.fill[0].b));
	(*self)->window = CreateWindowEx(0, "PiuClipWindow", NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, 0, 0, 0, (*view)->window, NULL, gInstance, (LPVOID)self);
	(*self)->control = CreateWindowEx(0, "Edit", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, (*self)->window, NULL, gInstance, NULL);
	SetWindowSubclass((*self)->control, PiuFieldProc, 0, (DWORD_PTR)self);
	if ((*self)->hint) {
		wchar_t* buffer = xsToStringCopyW(*((*self)->hint));
		Edit_SetCueBannerText((*self)->control, buffer);
		c_free(buffer);
	}
	if ((*self)->string) {
		wchar_t* buffer = xsToStringCopyW(*((*self)->string));
		SetWindowTextW((*self)->control, buffer);
		c_free(buffer);
	}
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
		PiuFont* font = (*result)->font;
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
	DestroyWindow((*self)->window);
	DeleteObject((*self)->solidBrush);
	(*self)->control = NULL;
	(*self)->window = NULL;
	(*self)->solidBrush = NULL;
	(*self)->computedStyle = NULL;
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
		HWND control = (*self)->control;
		int wideLength = GetWindowTextLengthW(control) + 1;
		wchar_t* buffer = (wchar_t*)c_malloc(wideLength * 2);
		GetWindowTextW(control, buffer, wideLength);
		xsResult = xsStringW(buffer);
		c_free(buffer);
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
		wchar_t* buffer = xsToStringCopyW(*hint);
		Edit_SetCueBannerText((*self)->control, buffer);
		c_free(buffer);
	}
}

void PiuField_set_string(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	xsSlot* string = PiuString(xsArg(0));
	(*self)->string = string;
	if ((*self)->application) {
		wchar_t* buffer = xsToStringCopyW(*string);
		SetWindowTextW((*self)->control, buffer);
		c_free(buffer);
	}
}

void PiuField_focus(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	if ((*self)->application) {
		SetFocus((*self)->control);
	}
}

