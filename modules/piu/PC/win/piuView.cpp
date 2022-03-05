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

#define GDIPVER     0x0110
#include "piuPC.h"
#include <dbt.h>

class PiuDropTarget : public IDropTarget {
public:
	PiuDropTarget(HWND window) {
		m_dwRef = 0;
		m_pdth = NULL;
		m_window = window;
		CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper, (LPVOID*)&m_pdth);
	}
	~PiuDropTarget() {
		if (m_pdth != NULL)
			m_pdth->Release(); 
	}
	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) {
		*ppvObject = NULL;
		if( IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget) ) {
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() { 
		return ++m_dwRef;
	}
	STDMETHOD_(ULONG, Release)() {
		return --m_dwRef;
	}
	STDMETHOD(DragEnter)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
		*pdwEffect &= DROPEFFECT_COPY;
		return S_OK;
	}
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
		*pdwEffect &= DROPEFFECT_COPY;
		return S_OK;
	}
	STDMETHOD(DragLeave)() {
		return S_OK;
	}
	STDMETHOD(Drop)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
		FORMATETC fmtetc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		if (pDataObject->QueryGetData(&fmtetc) == S_OK) {
			STGMEDIUM stgmed;
			if (pDataObject->GetData(&fmtetc, &stgmed) == S_OK) {
				HDROP hdrop = (HDROP)GlobalLock(stgmed.hGlobal);
				if (hdrop != NULL) {
					UINT c = DragQueryFile(hdrop, (UINT)-1, NULL, 0);
					for (UINT i = 0; i < c; i++) {
						COPYDATASTRUCT cds;
						cds.cbData = (DragQueryFileW(hdrop, i, NULL, 0) + 1) * sizeof(wchar_t);
						cds.lpData = c_malloc(cds.cbData);
						DragQueryFileW(hdrop, i, (LPWSTR)cds.lpData, cds.cbData);
						SendMessage(m_window, WM_COPYDATA, 0, (LPARAM)&cds);
						c_free(cds.lpData);
					}
					GlobalUnlock(hdrop);
				}
				ReleaseStgMedium(&stgmed);
			}
		}
		return S_OK;
	}
private:
	IDropTargetHelper *m_pdth;
	DWORD m_dwRef;
	HWND m_window;
};

LRESULT CALLBACK PiuWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	static BOOL tracking = FALSE;
	switch(message)	{
	case WM_CREATE: {
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(window, 0, (LONG_PTR)cs->lpCreateParams);
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		(*piuView)->window = window;
		HKEY key;
		DWORD size = 4;
		char szSubKey[1024];
		char title[64];
		c_strcpy(szSubKey, "SOFTWARE\\moddable.tech\\");
		title[0] = 0;
		GetWindowText(window, title, sizeof(title));
		if (0 != c_strlen(title))
			c_strcat(szSubKey, title);
		else
			c_strcat(szSubKey, "Screen Test");
		c_strcat(szSubKey, "\\window");
		if (RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0, KEY_READ, &key) == ERROR_SUCCESS) {
			WINDOWPLACEMENT placement;
			placement.length = sizeof(WINDOWPLACEMENT);
			placement.flags = 0;
			RegQueryValueEx(key, "show", 0, NULL, (BYTE*)&placement.showCmd, &size);
			RegQueryValueEx(key, "xMin", 0, NULL, (BYTE*)&placement.ptMinPosition.x, &size);
			RegQueryValueEx(key, "yMin", 0, NULL, (BYTE*)&placement.ptMinPosition.y, &size);
			RegQueryValueEx(key, "xMax", 0, NULL, (BYTE*)&placement.ptMaxPosition.x, &size);
			RegQueryValueEx(key, "yMax", 0, NULL, (BYTE*)&placement.ptMaxPosition.y, &size);
			RegQueryValueEx(key, "left", 0, NULL, (BYTE*)&placement.rcNormalPosition.left, &size);
			RegQueryValueEx(key, "right", 0, NULL, (BYTE*)&placement.rcNormalPosition.right, &size);
			RegQueryValueEx(key, "top", 0, NULL, (BYTE*)&placement.rcNormalPosition.top, &size);
			RegQueryValueEx(key, "bottom", 0, NULL, (BYTE*)&placement.rcNormalPosition.bottom, &size);
			SetWindowPos(window, NULL, placement.rcNormalPosition.left, placement.rcNormalPosition.top, placement.rcNormalPosition.right - placement.rcNormalPosition.left, placement.rcNormalPosition.bottom - placement.rcNormalPosition.top, SWP_NOZORDER);
			RegCloseKey(key);
		}
		SetTimer(window, 0, USER_TIMER_MINIMUM, NULL);
	} break;
	case WM_COPYDATA: {
		COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			xsVars(3);
			xsVar(0) = xsReference((*application)->behavior);
			if (xsFindResult(xsVar(0), xsID_onOpenFile)) {
				xsVar(1) = xsReference((*application)->reference);
				xsVar(2) = xsStringW((wchar_t*)cds->lpData);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
			}
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	} break;
	case WM_CLOSE: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		HKEY key;
		char szSubKey[1024];
		char title[64];
		c_strcpy(szSubKey, "SOFTWARE\\moddable.tech\\");
		title[0] = 0;
		GetWindowText(window, title, sizeof(title));
		if (0 != c_strlen(title))
			c_strcat(szSubKey, title);
		else
			c_strcat(szSubKey, "Screen Test");
		c_strcat(szSubKey, "\\window");
		if (RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS) {
			WINDOWPLACEMENT placement;
			placement.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(window, &placement);
			RegSetValueEx(key, "show", 0, REG_DWORD, (const BYTE*)&placement.showCmd, 4);
			RegSetValueEx(key, "xMin", 0, REG_DWORD, (const BYTE*)&placement.ptMinPosition.x, 4);
			RegSetValueEx(key, "yMin", 0, REG_DWORD, (const BYTE*)&placement.ptMinPosition.y, 4);
			RegSetValueEx(key, "xMax", 0, REG_DWORD, (const BYTE*)&placement.ptMaxPosition.x, 4);
			RegSetValueEx(key, "yMax", 0, REG_DWORD, (const BYTE*)&placement.ptMaxPosition.y, 4);
			RegSetValueEx(key, "left", 0, REG_DWORD, (const BYTE*)&placement.rcNormalPosition.left, 4);
			RegSetValueEx(key, "right", 0, REG_DWORD, (const BYTE*)&placement.rcNormalPosition.right, 4);
			RegSetValueEx(key, "top", 0, REG_DWORD, (const BYTE*)&placement.rcNormalPosition.top, 4);
			RegSetValueEx(key, "bottom", 0, REG_DWORD, (const BYTE*)&placement.rcNormalPosition.bottom, 4);
			RegCloseKey(key);
		}
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			xsVars(2);
			xsVar(0) = xsReference((*application)->behavior);
			xsVar(1) = xsReference((*application)->reference);
			if (xsFindResult(xsVar(0), xsID_onQuit))
				(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			else
				(void)xsCall0(xsVar(1), xsID_quit);
		}
		xsEndHost((*piuView)->the);
	} break;
		
	case WM_COMMAND: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		UINT id = LOWORD(wParam);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			xsVars(3);
			PiuApplicationDoMenu(application, id);
		}
		xsEndHost((*piuView)->the);
	} break;
	case WM_INITMENUPOPUP: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		HMENU menu = (HMENU)wParam;
		int c = GetMenuItemCount(menu), i;
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			xsVars(4);
			for (i = 0; i < c; i++) {
				UINT id = GetMenuItemID(menu, i);
				xsIntegerValue result = PiuApplicationCanMenu(application, id);
				if (result & piuMenuTitled) {
					char buffer[256];
					xsStringValue value;
					MENUITEMINFO mii;
					c_strcpy(buffer, xsToString(xsGet(xsResult, xsID_title)));
					if (xsFindString(xsResult, xsID_key, &value)) {
						c_strcat(buffer, "\tCtrl+");
						if (xsTest(xsGet(xsResult, xsID_shift)))
							c_strcat(buffer, "Shift+");
						if (xsTest(xsGet(xsResult, xsID_option)))
							c_strcat(buffer, "Alt+");
						c_strcat(buffer, value);
					}
					memset(&mii, 0, sizeof(mii));
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_STRING;
					mii.dwTypeData = buffer;
					mii.cch = c_strlen(buffer);
					SetMenuItemInfo(menu, i, MF_BYPOSITION, &mii);
				}
				CheckMenuItem(menu, i, MF_BYPOSITION | ((result & piuMenuChecked) ? MF_CHECKED : MF_UNCHECKED));
				EnableMenuItem(menu, i, MF_BYPOSITION | ((result & piuMenuEnabled) ? MF_ENABLED : MF_GRAYED));
			}
		}
		xsEndHost((*piuView)->the);
	} break;
	
	case WM_LBUTTONDOWN: {
		SetFocus(window);
		SetCapture(window);
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationModifiersChanged(application, (wParam & MK_CONTROL) ? 1 : 0, (GetKeyState(VK_MENU) & 0x8000) ? 1 : 0, (wParam & MK_SHIFT) ? 1 : 0);
			PiuApplicationTouchBegan(application, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GetTickCount());
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	} break;
	case WM_LBUTTONUP: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationModifiersChanged(application, (wParam & MK_CONTROL) ? 1 : 0, (GetKeyState(VK_MENU) & 0x8000) ? 1 : 0, (wParam & MK_SHIFT) ? 1 : 0);
			PiuApplicationTouchEnded(application, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GetTickCount());
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
		ReleaseCapture();
	} break;
	case WM_MOUSEMOVE: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationModifiersChanged(application, (wParam & MK_CONTROL) ? 1 : 0, (GetKeyState(VK_MENU) & 0x8000) ? 1 : 0, (wParam & MK_SHIFT) ? 1 : 0);
            if (wParam & MK_LBUTTON) {
				PiuApplicationTouchMoved(application, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GetTickCount());
			}
			else {
				if (!tracking) {
					TRACKMOUSEEVENT eventTrack;
					eventTrack.cbSize = sizeof(eventTrack);
					eventTrack.dwFlags = TME_LEAVE;
					eventTrack.dwHoverTime = HOVER_DEFAULT;
					eventTrack.hwndTrack = window;
					TrackMouseEvent(&eventTrack);
					tracking = TRUE;
					PiuApplicationMouseEntered(application, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				}
				else
					PiuApplicationMouseMoved(application, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
		}
		xsEndHost((*piuView)->the);
	} break;
	case WM_MOUSELEAVE: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationModifiersChanged(application, (wParam & MK_CONTROL) ? 1 : 0, (GetKeyState(VK_MENU) & 0x8000) ? 1 : 0, (wParam & MK_SHIFT) ? 1 : 0);
			PiuApplicationMouseExited(application, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			tracking = FALSE;
		}
		xsEndHost((*piuView)->the);
	} break;
	case WM_MOUSEHWHEEL: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		INT delta = GET_WHEEL_DELTA_WPARAM(wParam);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationMouseScrolled(application, delta, 0);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	} break;
	case WM_MOUSEWHEEL: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		INT delta = GET_WHEEL_DELTA_WPARAM(wParam);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationMouseScrolled(application, 0, delta);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	} break;
	case WM_SETCURSOR: {
		if (LOWORD(lParam) == HTCLIENT)
			return FALSE;
		return DefWindowProc(window, message, wParam, lParam);
	} break;
	
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);
		Graphics graphics(hdc);
   		graphics.SetCompositingQuality(CompositingQualityHighQuality);
		graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
		graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
		PiuRectangleRecord area;
		area.x = ps.rcPaint.left;
		area.y = ps.rcPaint.top;
		area.width = ps.rcPaint.right - ps.rcPaint.left;
		area.height = ps.rcPaint.bottom - ps.rcPaint.top;
		//fprintf(stderr, "update %d %d %d %d\n", area.x, area.y, area.width, area.height);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			(*piuView)->graphics = &graphics;
			(*(*application)->dispatch->update)(application, piuView, &area);
			(*piuView)->graphics = NULL;
		}
		xsEndHost((*piuView)->the);
		EndPaint(window, &ps);
		return TRUE;
	} break;
	
	case WM_SIZE: {
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		if (wParam != SIZE_MINIMIZED) {
			xsBeginHost((*piuView)->the);
			{
				PiuApplication* application = (*piuView)->application;
				PiuApplicationResize(application);
			}
			xsEndHost((*piuView)->the);
		}
	} break;
	
	case WM_TIMER: {
		SleepEx(0, TRUE); // directory notifications
		PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			xsVars(2);
			PiuApplicationDeferContents(the, application);
			PiuApplicationIdleContents(application);
			PiuApplicationTouchIdle(application);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	} break;
	
	case WM_DEVICECHANGE: {
		if (wParam == DBT_DEVNODES_CHANGED) {
			PiuView* piuView = (PiuView*)GetWindowLongPtr(window, 0);
			xsBeginHost((*piuView)->the);
			{
				PiuApplication* application = (*piuView)->application;
				xsVars(3);
				xsVar(0) = xsReference((*application)->behavior);
				if (xsFindResult(xsVar(0), xsID_onDevicesChanged)) {
					xsVar(1) = xsReference((*application)->reference);
					(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
				}
				PiuApplicationAdjust(application);
			}
			xsEndHost((*piuView)->the);
		}
		return TRUE;
	} break;
	
	default:
		return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;

}  

LRESULT CALLBACK PiuClipWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	static BOOL tracking = FALSE;
	switch(message)	{
	case WM_CREATE: {
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		SetWindowLongPtr(window, 0, (LONG_PTR)cs->lpCreateParams);
	} break;
	case WM_COMMAND: {
		PiuContent* content = (PiuContent*)GetWindowLongPtr(window, 0);
		switch (HIWORD(wParam)) {
		case EN_CHANGE: {
			if ((*content)->behavior) {
				xsBeginHost((*content)->the);
				xsVars(2);
				xsVar(0) = xsReference((*content)->behavior);
				if (xsFindResult(xsVar(0), xsID_onStringChanged)) {
					xsVar(1) = xsReference((*content)->reference);
					(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
				}
				xsEndHost((*content)->the);
			}
		} break;
		}
		return TRUE;
	} break;
	case WM_CTLCOLOREDIT: {
		PiuField* self = (PiuField*)GetWindowLongPtr(window, 0);
		PiuSkin* skin = (*self)->skin;
		PiuStyle* style = (*self)->computedStyle;
		SetBkMode((HDC)wParam,OPAQUE);
		SetBkColor((HDC)wParam, RGB((*skin)->data.color.fill[0].r, (*skin)->data.color.fill[0].g, (*skin)->data.color.fill[0].b));
		SetTextColor((HDC)wParam, RGB((*style)->color[0].r, (*style)->color[0].g, (*style)->color[0].b));
		return (LRESULT)((*self)->solidBrush);
	} break;
	default:
		return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;
}

static void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
static void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuViewHooks = {
	PiuViewDelete,
	PiuViewMark,
	NULL
};

BOOL CALLBACK PiuViewAdjustAux(HWND window, LPARAM lParam)
{
	PiuView* self = (PiuView*)lParam;
	if (GetParent(window) == (*self)->window) {
		PiuContent* content = (PiuContent*)GetWindowLongPtr(window, 0);
		PiuRectangleRecord bounds = (*content)->bounds;
		PiuRectangleRecord clipBounds = bounds;
		PiuContainer* container = (*content)->container;
		while (container) {
			bounds.x += (*container)->bounds.x;
			bounds.y += (*container)->bounds.y;
			clipBounds.x += (*container)->bounds.x;
			clipBounds.y += (*container)->bounds.y;
			if ((*container)->flags & piuClip)
				PiuRectangleIntersect(&clipBounds, &clipBounds, &(*container)->bounds);
			container = (*container)->container;
		}
		MoveWindow(window, clipBounds.x, clipBounds.y, clipBounds.width, clipBounds.height, TRUE);
		window = GetWindow(window, GW_CHILD);
		MoveWindow(window, bounds.x - clipBounds.x, bounds.y - clipBounds.y, bounds.width, bounds.height, TRUE);
	}
	return TRUE;
}

void PiuViewAdjust(PiuView* self) 
{
	EnumChildWindows((*self)->window, PiuViewAdjustAux, (LPARAM)self);
}

void PiuViewChangeCursor(PiuView* self, int32_t shape)
{
	LPCTSTR name;
	HCURSOR cursor;
	switch (shape) {
	case 1:
		name = IDC_CROSS;
		break;
	case 2:
		name = IDC_IBEAM;
		break;
	case 3:
		name = IDC_HAND;
		break;
	case 4:
		name = IDC_NO;
		break;
	case 5:
		name = IDC_SIZEWE;
		break;
	case 6:
		name = IDC_SIZENS;
		break;
	default:
		name = IDC_ARROW;
		break;
	}
	cursor = LoadCursor(NULL, name);
	SetCursor(cursor);
}

void PiuViewCreate(xsMachine* the) 
{
	PiuView* self;
	PiuApplication* application;
	xsVars(5);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuViewRecord));
	self = PIU(View, xsThis);
	(*self)->reference = xsToReference(xsThis);
	(*self)->the = the;
	xsSetHostHooks(xsThis, &PiuViewHooks);
	application = (*self)->application = PIU(Application, xsArg(0));
	(*application)->view = self;

	HMENU menuBar = CreateMenu();
	WORD accelerators[256][3]; // ACCEL misaligned with /Zp1
	int acceleratorsCount = 0;
	if (xsFindResult(xsArg(1), xsID_menus)) {
		xsIntegerValue c, i, d, j;
		(void)xsCall0(xsResult, xsID_shift);
		c = xsToInteger(xsGet(xsResult, xsID_length));
		for (i = 0; i < c; i++) {
			HMENU menu = CreateMenu();
			xsVar(0) = xsGetAt(xsResult, xsInteger(i));
			xsVar(1) = xsGet(xsVar(0), xsID_title);
			AppendMenu(menuBar, MF_POPUP, (UINT)menu, xsToString(xsVar(1)));
			xsVar(1) = xsGet(xsVar(0), xsID_items);
			d = xsToInteger(xsGet(xsVar(1), xsID_length));
			for (j = 0; j < d; j++) {
				xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
				if (xsTest(xsVar(2))) {
					WORD id = (WORD)((i << 8) | (j + 1));
					xsStringValue value;
					char buffer[256];
					xsIdentifier index;

					value = xsToString(xsGet(xsVar(2), xsID_command));
					c_strcpy(buffer, "can");
					c_strcat(buffer, value);
					index = xsID(buffer);
					xsSet(xsVar(2), xsID_canID, xsInteger(index));
					
					value = xsToString(xsGet(xsVar(2), xsID_command));
					c_strcpy(buffer, "do");
					c_strcat(buffer, value);
					index = xsID(buffer);
					xsSet(xsVar(2), xsID_doID, xsInteger(index));
						
					xsVar(3) = xsGet(xsVar(2), xsID_titles);
					if (xsTest(xsVar(3))) {
						xsVar(4) = xsGet(xsVar(2), xsID_state);
						xsVar(4) = xsGetAt(xsVar(3), xsVar(4));
						xsSet(xsVar(2), xsID_title, xsVar(4));
					}
					
					if ((acceleratorsCount < 256)  && xsFindString(xsVar(2), xsID_key, &value)) {
						xsStringValue title = xsToString(xsGet(xsVar(2), xsID_title));	
						BYTE mask = FCONTROL | FVIRTKEY;
						c_strcpy(buffer, title);
						c_strcat(buffer, "\tCtrl+");
						if (xsTest(xsGet(xsVar(2), xsID_shift))) {
							mask |= FSHIFT;
							c_strcat(buffer, "Shift+");
						}
						if (xsTest(xsGet(xsVar(2), xsID_option))) {
							mask |= FALT;
							c_strcat(buffer, "Alt+");
						}
						c_strcat(buffer, value);
						accelerators[acceleratorsCount][0] = mask;
						accelerators[acceleratorsCount][1] = LOBYTE(VkKeyScan(value[0]));
						accelerators[acceleratorsCount][2] = id;
						acceleratorsCount++;
						AppendMenu(menu, MF_STRING, id, buffer);
					}
					else {
						wchar_t* title = xsToStringCopyW(xsGet(xsVar(2), xsID_title));
						AppendMenuW(menu, MF_STRING, id, title);
						c_free(title);
					}
				}
				else
					AppendMenu(menu, MF_SEPARATOR, -1, NULL);
			}
		}
		xsSet(xsArg(0), xsID_menus, xsResult);
	}
	xsStringValue title = NULL;
	if (xsFindResult(xsArg(1), xsID_window))
		title = xsToString(xsGet(xsResult, xsID_title));
	(*self)->acceleratorTable = CreateAcceleratorTable((LPACCEL)accelerators, acceleratorsCount);
	(*self)->colorMatrix = new ColorMatrix();
	(*self)->imageAttributes = new ImageAttributes();
	(*self)->solidBrush = new SolidBrush(Color(255, 0, 0, 0));
	(*self)->window = CreateWindowEx(WS_EX_COMPOSITED | WS_EX_LAYERED, "PiuWindow", title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
		NULL, menuBar, gInstance, (LPVOID)self);
	
	RegisterDragDrop((*self)->window, new PiuDropTarget((*self)->window));

	xsResult = xsThis;
}


void PiuViewDelete(void* it)
{
}

void PiuViewDictionary(xsMachine* the, void* it)
{
	
}

void PiuViewDrawRoundContent(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuDimension radius, PiuDimension border, PiuVariant variant, PiuColor fillColor, PiuColor strokeColor)
{
	Graphics* graphics = (*self)->graphics;
 	graphics->SetSmoothingMode(SmoothingModeAntiAlias);
	REAL lx = (REAL)x, ty = (REAL)y, rx = lx + (REAL)w, by = ty + (REAL)h, r = (REAL)radius, t, u = (REAL)border;
// 	if (r > fw / 2)
// 		r = fw / 2;
// 	if (r > fh / 2)
// 		r = fh / 2;
// 	
	if (variant == 1)
		lx += r;
	else if (variant == 2)
		rx -= r;
	if (u > 0) {
		REAL delta = u / 2;
		lx += delta;
		ty += delta;
		rx -= delta;
		by -= delta;
		r -= delta;
	}
	t = r * 0.552284749831;
	GraphicsPath path;
	path.AddBezier(lx, ty + r, lx, ty + r - t, lx + r - t, ty, lx + r, ty);
	path.AddBezier(rx - r, ty, rx - r + t, ty, rx, ty + r - t, rx, ty + r);
	if (variant == 2)
		path.AddBezier(rx, by - r, rx, by - r + t, rx + r - t, by, rx + r, by);
	else
		path.AddBezier(rx, by - r, rx, by - r + t, rx - r + t, by, rx - r, by);
	if (variant == 1)
		path.AddBezier(lx - r, by, lx - r + t, by, lx, by - r + t, lx, by - r);
	else
		path.AddBezier(lx + r, by, lx + r - t, by, lx, by - r + t, lx, by - r);
    path.CloseFigure();
	if (fillColor->a) {
		(*self)->solidBrush->SetColor(Color(fillColor->a, fillColor->r, fillColor->g, fillColor->b));
		graphics->FillPath((*self)->solidBrush, &path);
	}
	if ((border > 0) && (strokeColor->a)) {
		Pen pen(Color(strokeColor->a, strokeColor->r, strokeColor->g, strokeColor->b), u);
		graphics->DrawPath(&pen, &path);
	}
}

void PiuViewDrawString(PiuView* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension sw)
{
	PiuViewDrawStringSubPixel(self, slot, offset, length, font, x, y, w, sw);
}

void PiuViewDrawStringSubPixel(PiuView* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, double x, double y, PiuDimension w, PiuDimension sw)
{
	xsMachine* the = (*self)->the;
	Graphics* graphics = (*self)->graphics;
   const StringFormat* pStringFormat = StringFormat::GenericTypographic();
	xsStringValue string = PiuToString(slot);
	if (length < 0)
		length = c_strlen(string + offset);
	xsIntegerValue wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string + offset, length, NULL, 0);
	PWSTR wideString = (PWSTR)c_malloc(wideStringLength * 2);
	xsElseThrow(wideString != NULL);
	MultiByteToWideChar(CP_UTF8, 0, string + offset, length, wideString, wideStringLength);
	PointF origin((REAL)x, (REAL)y + (*font)->delta);
	graphics->DrawString(wideString, wideStringLength, (*font)->object, origin, pStringFormat, (*self)->solidBrush);
	c_free(wideString);
}

void PiuViewDrawTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		x -= sx;
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw)
		sw = tw - sx;
	if (sy < 0) {
		y -= sy;
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th)
		sh = th - sy;
	if ((sw <= 0) || (sh <= 0)) return;
	PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, sh);
}

void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	Graphics* graphics = (*self)->graphics;
	REAL scale = (REAL)(*texture)->scale;
	RectF source(scale * sx, scale * sy, scale * sw, scale * sh);
	RectF destination((REAL)x, (REAL)y, (REAL)sw, (REAL)sh);
	if ((*self)->filtered) {
		if ((*self)->transparent) return;
		graphics->DrawImage((*texture)->image, destination, source, UnitPixel, (*self)->imageAttributes);
	}
	else {
		graphics->DrawImage((*texture)->image, destination, source, UnitPixel, NULL);
	}
}

void PiuViewFillColor(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	if ((w <= 0) || (h <= 0)) return;
	if ((*self)->transparent) return;
	Graphics* graphics = (*self)->graphics;
	graphics->FillRectangle((*self)->solidBrush, x, y, w, h);
}

void PiuViewFillTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		if (w == sw) {
			x -= sx;
			w += sx;
		}
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw) {
		if (w == sw)
			w = tw - sx;
		sw = tw - sx;
	}
	if (sy < 0) {
		if (h == sh) {
			y -= sy;
			h += sy;
		}
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th) {
		if (h == sh)
			h = th - sy;
		sh = th - sy;
	}
	if ((w <= 0) || (h <= 0) || (sw <= 0) || (sh <= 0)) return;
	PiuCoordinate xx, ww;
	while (h >= sh) {
		xx = x;
		ww = w;
		while (ww >= sw) {
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, sw, sh);
			xx += sw;
			ww -= sw;
		}
		if (ww)
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, ww, sh);
		y += sh;
		h -= sh;
	}
	if (h) {
		while (w >= sw) {
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, h);
			x += sw;
			w -= sw;
		}
		if (w)
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, w, h);
	}
}

void PiuViewGetSize(PiuView* self, PiuDimension *width, PiuDimension *height)
{
	RECT area;
	GetClientRect((*self)->window, &area);
	*width = area.right - area.left;
	*height = area.bottom - area.top;
}

void PiuViewIdleCheck(PiuView* self, PiuInterval idle)
{
}

void PiuViewInvalidate(PiuView* self, PiuRectangle area) 
{
	if (area) {
		//fprintf(stderr, "invalidate %d %d %d %d\n", area->x, area->y, area->width, area->height);
		RECT rect;
		rect.left = area->x;
		rect.top = area->y;
		rect.right = rect.left + area->width;
		rect.bottom = rect.top + area->height;
		InvalidateRect((*self)->window, &rect, FALSE);
	}
	else {
		InvalidateRect((*self)->window, NULL, FALSE);
	}
}

void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuViewPopClip(PiuView* self)
{
	Graphics* graphics = (*self)->graphics;
	(*self)->graphicsStatesIndex--;
	graphics->Restore((*self)->graphicsStates[(*self)->graphicsStatesIndex]);
}

void PiuViewPopColor(PiuView* self)
{
}

void PiuViewPopColorFilter(PiuView* self)
{
	(*self)->filtered = 0;
}

void PiuViewPopOrigin(PiuView* self)
{
	Graphics* graphics = (*self)->graphics;
	(*self)->graphicsStatesIndex--;
	graphics->Restore((*self)->graphicsStates[(*self)->graphicsStatesIndex]);
}

void PiuViewPushClip(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	Graphics* graphics = (*self)->graphics;
	(*self)->graphicsStates[(*self)->graphicsStatesIndex] = graphics->Save();
	(*self)->graphicsStatesIndex++;
	Rect rect(x, y, w, h);
	graphics->IntersectClip(rect);
}

void PiuViewPushColor(PiuView* self, PiuColor color)
{
	(*self)->solidBrush->SetColor(Color(color->a, color->r, color->g, color->b));
	(*self)->transparent = (color->a == 0) ? 1 : 0;
}

void PiuViewPushColorFilter(PiuView* self, PiuColor color)
{
	(*self)->colorMatrix->m[0][0] = 1;
	(*self)->colorMatrix->m[1][1] = 1;
	(*self)->colorMatrix->m[2][2] = 1;
	(*self)->colorMatrix->m[3][3] = 1;
	(*self)->colorMatrix->m[4][4] = 1;
	(*self)->colorMatrix->m[4][0] = ((REAL)(color->r)) / 255;
	(*self)->colorMatrix->m[4][1] = ((REAL)(color->g)) / 255;
	(*self)->colorMatrix->m[4][2] = ((REAL)(color->b)) / 255;
	(*self)->imageAttributes->SetColorMatrix((*self)->colorMatrix);
	(*self)->transparent = (color->a == 0) ? 1 : 0;
	(*self)->filtered = 1;
}

void PiuViewPushOrigin(PiuView* self, PiuCoordinate x, PiuCoordinate y)
{
	Graphics* graphics = (*self)->graphics;
	(*self)->graphicsStates[(*self)->graphicsStatesIndex] = graphics->Save();
	(*self)->graphicsStatesIndex++;
	graphics->TranslateTransform((REAL)x, (REAL)y, MatrixOrderAppend);
}

void PiuViewReflow(PiuView* self)
{
}

void PiuViewReschedule(PiuView* self)
{
	PiuApplicationIdleCheck((*self)->application);
}

double PiuViewTicks(PiuView* self)
{
    return GetTickCount();
}

void PiuViewValidate(PiuView* self, PiuRectangle area) 
{
	if (area) {
		//fprintf(stderr, "validate %d %d %d %d\n", area->x, area->y, area->width, area->height);
		RECT rect;
		rect.left = area->x;
		rect.top = area->y;
		rect.right = rect.left + area->width;
		rect.bottom = rect.top + area->height;
		ValidateRect((*self)->window, &rect);
	}
	else {
		ValidateRect((*self)->window, NULL);
	}
}

void PiuCursors_get_arrow(xsMachine* the)
{
	xsResult = xsInteger(0);
}

void PiuCursors_get_cross(xsMachine* the)
{
	xsResult = xsInteger(1);
}

void PiuCursors_get_iBeam(xsMachine* the)
{
	xsResult = xsInteger(2);
}

void PiuCursors_get_link(xsMachine* the)
{
	xsResult = xsInteger(3);
}

void PiuCursors_get_notAllowed(xsMachine* the)
{
	xsResult = xsInteger(4);
}

void PiuCursors_get_resizeColumn(xsMachine* the)
{
	xsResult = xsInteger(5);
}

void PiuCursors_get_resizeRow(xsMachine* the)
{
	xsResult = xsInteger(6);
}

void PiuSystem_getClipboardString(xsMachine* the)
{
	HWND window = NULL;
	HGLOBAL data;
	PWSTR wideString;
	xsIntegerValue length;
	xsStringValue buffer, src, dst;
	char c;
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		if (OpenClipboard(window)) {
			data = GetClipboardData(CF_UNICODETEXT);
			wideString = (PWSTR)GlobalLock(data);
			if (wideString) {
				length = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, NULL, 0, NULL, NULL);
				buffer = (xsStringValue)c_malloc(length);
				if (buffer) {
					WideCharToMultiByte(CP_UTF8, 0, wideString, -1, buffer, length, NULL, NULL);
					src = buffer;
					dst = buffer;
					while ((c = *src++)) {
						if ((c == 13) && (*src == 10)) {
							*dst++ = 10;
							src++;
						}
						else
							*dst++ = c;
					}
					*dst = 0;
					xsResult = xsString(buffer);
					c_free(buffer);
				}
				GlobalUnlock(wideString);
			}
			CloseClipboard();
		}
	}
	if (!xsTest(xsResult))
		xsResult = xsString("");
}

void PiuSystem_setClipboardString(xsMachine* the)
{
	HWND window = NULL;
	xsStringValue string, src, buffer, dst;
	char c;
	xsIntegerValue length;
	HGLOBAL data;
	if (OpenClipboard(window)) {
		EmptyClipboard(); 
		if (xsToInteger(xsArgc) > 0) {
			string = xsToString(xsArg(0));
			src = string;
			length = 0;
			while ((c = *src++)) {
				length++;
				if (c == 10)
					length++;
			}
			length++;
			buffer = (xsStringValue)c_malloc(length);
			if (buffer) {
				src = string;
				dst = buffer;
				while ((c = *src++)) {
					if (c == 10)
						*dst++ = 13;
					*dst++ = c;
				}
				*dst = 0;
				length = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
				data = GlobalAlloc(GMEM_MOVEABLE, 2 * length);
				if (data) {
					MultiByteToWideChar(CP_UTF8, 0, buffer, -1, (LPWSTR)GlobalLock(data), length);
					GlobalUnlock(data);
					SetClipboardData(CF_UNICODETEXT, data);
				}
				c_free(buffer);
			}
		}
		CloseClipboard();
	}
}

void PiuSystem_launchPath(xsMachine* the)
{
	SHELLEXECUTEINFOW shellExecuteInfo;
	wchar_t* path = xsToStringCopyW(xsArg(0));
	memset(&shellExecuteInfo, 0, sizeof(shellExecuteInfo));
	shellExecuteInfo.cbSize = sizeof(shellExecuteInfo);
	shellExecuteInfo.fMask = 0;
	shellExecuteInfo.lpVerb = L"open";
	shellExecuteInfo.lpFile = path;
	shellExecuteInfo.hwnd = GetDesktopWindow();
	shellExecuteInfo.nShow = SW_SHOWNORMAL;
	ShellExecuteExW(&shellExecuteInfo);
	free(path);
}

void PiuSystem_launchURL(xsMachine* the)
{
	PiuSystem_launchPath(the);
}

void PiuNavigationBar_create(xsMachine* the)
{
	xsDebugger();
}

void PiuStatusBar_create(xsMachine* the)
{
	xsDebugger();
}
