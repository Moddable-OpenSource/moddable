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

static PWSTR xsStringToWideString(xsStringValue string)
{
	xsIntegerValue stringLength;
	xsIntegerValue wideStringLength;
	stringLength = c_strlen(string);
	PWSTR wideString;
	wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	wideString = (PWSTR)CoTaskMemAlloc(wideStringLength * 2);
	if (wideString) {
		MultiByteToWideChar(CP_UTF8, 0, string, -1, wideString, wideStringLength);
		//wideString[wideStringLength - 1] = 0;
	}
	return wideString;
}
	
void PiuSystem_alert(xsMachine* the)
{
	int argc = xsToInteger(xsArgc);
	MSGBOXPARAMSW params;
	xsStringValue string;
	xsIntegerValue result;
	xsVars(1);
	c_memset(&params, 0, sizeof(params));
	params.cbSize = sizeof(params);
	params.hwndOwner = GetForegroundWindow();
	params.hInstance = gInstance;
	params.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_type, &string)) {
			if (!c_strcmp(string, "about")) {
				params.lpszCaption = L"About";
				params.dwStyle = MB_USERICON;
				params.lpszIcon = (LPCWSTR)MAKEINTRESOURCE(100);
			}
			else if (!c_strcmp(string, "stop"))
				params.dwStyle = MB_ICONSTOP;
			else if (!c_strcmp(string, "note"))
				params.dwStyle = MB_ICONEXCLAMATION;
		}
		if (xsFindResult(xsArg(0), xsID_prompt)) {
			xsVar(0) = xsResult;
		}
		if (xsFindResult(xsArg(0), xsID_info)) {
			xsVar(0) = xsCall1(xsVar(0), xsID_concat, xsString("\n\n"));
			xsVar(0) = xsCall1(xsVar(0), xsID_concat, xsResult);
		}
		params.lpszText = xsStringToWideString(xsToString(xsVar(0)));
		if (xsFindResult(xsArg(0), xsID_buttons)) {
			if (xsIsInstanceOf(xsResult, xsArrayPrototype)) {
				xsIntegerValue c = xsToInteger(xsGet(xsResult, xsID_length));
				if (c == 3)
					params.dwStyle |= MB_YESNOCANCEL;
				else if (c == 2)
					params.dwStyle |= MB_OKCANCEL;
				else
					params.dwStyle |= MB_OK;
			}
		}
	}
	result = MessageBoxIndirectW(&params);
	if (params.lpszText)
		CoTaskMemFree((LPVOID *)params.lpszText);
	if ((argc > 1) && xsTest(xsArg(1)))
		(void)xsCallFunction1(xsArg(1), xsNull, ((result == IDYES) || (result == IDOK)) ? xsTrue : (result == IDNO) ? xsFalse : xsUndefined);
}

void PiuSystem_open(xsMachine* the, xsBooleanValue flag)
{
	HRESULT hr;
	xsIntegerValue argc = xsToInteger(xsArgc);
	IFileOpenDialog *pFileOpen = NULL;
	IShellItem *pItem = NULL;
	xsStringValue string;
	PWSTR wideString = NULL;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (LPVOID *)&pFileOpen);
	if (!SUCCEEDED(hr)) goto bail;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_prompt, &string)) {
			wideString = xsStringToWideString(string);
			hr = pFileOpen->SetOkButtonLabel(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			hr = pFileOpen->SetTitle(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			CoTaskMemFree(wideString);
			wideString = NULL;
		}
		if (xsFindString(xsArg(0), xsID_message, &string)) {
			wideString = xsStringToWideString(string);
			hr = pFileOpen->SetTitle(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			CoTaskMemFree(wideString);
			wideString = NULL;
		}
	}
	if (flag) {
		hr = pFileOpen->SetOptions(FOS_PICKFOLDERS);
		if (!SUCCEEDED(hr)) goto bail;
	}
	hr = pFileOpen->Show(GetForegroundWindow());
	if (!SUCCEEDED(hr)) goto bail;
	hr = pFileOpen->GetResult(&pItem);
	if (!SUCCEEDED(hr)) goto bail;
	hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &wideString);
	if (!SUCCEEDED(hr)) goto bail;
	xsResult = xsStringW(wideString);
	(void)xsCallFunction1(xsArg(1), xsNull, xsResult);
	
bail:
	if (wideString)
		CoTaskMemFree(wideString);
	if (pItem)
		pItem->Release();
	if (pFileOpen)
		pFileOpen->Release();
}

void PiuSystem_openDirectory(xsMachine* the)
{
	PiuSystem_open(the, 1);
}

void PiuSystem_openFile(xsMachine* the)
{
	PiuSystem_open(the, 0);
}

void PiuSystem_save(xsMachine* the, xsBooleanValue flag)
{
	HRESULT hr;
	xsIntegerValue argc = xsToInteger(xsArgc);
	IFileSaveDialog *pFileSave = NULL;
	IShellItem *pItem = NULL;
	xsStringValue string;
	PWSTR wideString = NULL;
	hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, (LPVOID *)&pFileSave);
	if (!SUCCEEDED(hr)) goto bail;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_prompt, &string)) {
			wideString = xsStringToWideString(string);
			hr = pFileSave->SetOkButtonLabel(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			hr = pFileSave->SetTitle(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			CoTaskMemFree(wideString);
			wideString = NULL;
		}
		if (xsFindString(xsArg(0), xsID_name, &string)) {
			wideString = xsStringToWideString(string);
			hr = pFileSave->SetFileName(wideString);
			if (!SUCCEEDED(hr)) goto bail;
			CoTaskMemFree(wideString);
			wideString = NULL;
		}
	}
	if (flag) {
		hr = pFileSave->SetOptions(FOS_PICKFOLDERS);
		if (!SUCCEEDED(hr)) goto bail;
	}
	hr = pFileSave->Show(GetForegroundWindow());
	if (!SUCCEEDED(hr)) goto bail;
	hr = pFileSave->GetResult(&pItem);
	if (!SUCCEEDED(hr)) goto bail;
	hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &wideString);
	if (!SUCCEEDED(hr)) goto bail;
	xsResult = xsStringW(wideString);
	(void)xsCallFunction1(xsArg(1), xsNull, xsResult);
	
bail:
	if (wideString)
		CoTaskMemFree(wideString);
	if (pItem)
		pItem->Release();
	if (pFileSave)
		pFileSave->Release();
}

void PiuSystem_saveDirectory(xsMachine* the)
{
	PiuSystem_save(the, 1);
}

void PiuSystem_saveFile(xsMachine* the)
{
	PiuSystem_save(the, 0);
}
