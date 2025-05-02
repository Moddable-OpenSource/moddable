/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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
#include "xsmc.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "commodettoBitmapFormat.h"
#include "builtinCommon.h"
#include "commodettoConvert.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <process.h>

typedef struct CameraBufferRecord CameraBufferRecord;
typedef struct CameraBufferRecord *CameraBuffer;

struct CameraBufferRecord {
	CameraBuffer nextBuffer;
	IMFMediaBuffer* mediaBuffer;
	uint32_t size;
	uint8_t* data;
};

typedef struct CameraRecord CameraRecord;
typedef struct CameraRecord *Camera;

struct CameraRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onReadable;
	
	IMFSourceReader* pReader;
	
	CameraBuffer mainBuffer;
	CRITICAL_SECTION mainMutex;
	HWND window;
	
	HANDLE thread;
	CameraBuffer threadBuffer;
	CONDITION_VARIABLE threadCondition;
	CRITICAL_SECTION threadMutex;

	uint8_t calling;
	uint8_t destructing;
	uint8_t running;
	
	uint8_t format;
	uint32_t width;
	uint32_t height;
	uint32_t imageType;
	
	CommodettoConverter yuvToRGB;
	
	uint32_t size;
	uint8_t* data;
	
	uint16_t queueLength;
	CameraBufferRecord queueBuffers[1];
};

static void xs_camera_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks xsCameraHooks = {
	xs_camera_destructor,
	xs_camera_mark,
	NULL
	
};
static LRESULT CALLBACK xs_camera_WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

CameraBuffer xs_camera_dequeueBuffer(CameraBuffer* address)
{
	CameraBuffer result = *address;
	if (result) {
		*address = result->nextBuffer;
		result->nextBuffer = C_NULL;
	}
	return result;
}

void xs_camera_enqueueBuffer(CameraBuffer* address, CameraBuffer buffer)
{
	CameraBuffer former;
	while ((former = *address))
		address = &(former->nextBuffer);
	*address = buffer;
}

xsBooleanValue xs_camera_releaseBuffer(Camera camera, void* data)
{
	int i;
	CameraBuffer buffer;
	for (i = 0; i < camera->queueLength; i++) {
		buffer = &(camera->queueBuffers[i]);
		if (buffer->data == data) {
			break;
		}
	}
	if (i == camera->queueLength)
		return 0;
	EnterCriticalSection(&(camera->threadMutex));
	if (camera->threadBuffer == NULL) {
		camera->threadBuffer = buffer;
		WakeConditionVariable(&(camera->threadCondition));
	}
	else {
		xs_camera_enqueueBuffer(&(camera->threadBuffer), buffer);
	}
	LeaveCriticalSection(&(camera->threadMutex));
	return 1;
}

#define xs_camera_WindowClass "xs_camera_WindowClass"
LRESULT CALLBACK xs_camera_WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_CALLBACK)
		return DefWindowProc(window, message, wParam, lParam);
	Camera camera = (Camera)GetWindowLongPtr(window, 0);
	CameraBuffer buffer;
	
	EnterCriticalSection(&(camera->mainMutex));
	buffer = camera->mainBuffer;
	camera->mainBuffer = C_NULL;
	LeaveCriticalSection(&(camera->mainMutex));
	while (buffer) {
		CameraBuffer nextBuffer = buffer->nextBuffer;
		buffer->nextBuffer = C_NULL;
		camera->data = buffer->data;
		camera->size = buffer->size;
		if (camera->onReadable && camera->running && (camera->size > 0)) {
			camera->calling = 1;
			xsBeginHost(camera->the);
			xsResult = xsAccess(camera->object);
			xsCallFunction1(xsReference(camera->onReadable), xsResult, xsInteger(camera->size));
			xsEndHost(camera->the);
			if (camera->calling)
				camera->calling = 0;
			else {
				xs_camera_destructor(camera);
				return 0;
			}
		}
		else
			xs_camera_releaseBuffer(camera, camera->data);
		camera->size = 0;
		camera->data = NULL;
		buffer = nextBuffer;
	}
	return 0;
}

unsigned int __stdcall xs_camera_loop(void* it)
{
	Camera camera = (Camera)it;
	CameraBuffer buffer;
	HRESULT hr;
	
	for (;;) {
		EnterCriticalSection(&(camera->threadMutex));
		while ((camera->threadBuffer == NULL) && (camera->destructing == 0))
			SleepConditionVariableCS(&(camera->threadCondition), &(camera->threadMutex), INFINITE);
		buffer = xs_camera_dequeueBuffer(&(camera->threadBuffer));
		LeaveCriticalSection(&(camera->threadMutex));
		if (camera->destructing)
			break;
		if (buffer->mediaBuffer) {
			buffer->mediaBuffer->Unlock();
			buffer->mediaBuffer->Release();
			buffer->mediaBuffer = C_NULL;
			buffer->data = C_NULL;
			buffer->size = 0;
		}

		DWORD streamIndex;
		DWORD controlFlags;
		LONGLONG timeStamp;
		IMFSample* pSample;
		hr = camera->pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &controlFlags, &timeStamp, &pSample);
		if (hr == S_OK) {
			if (pSample != NULL) {
				IMFMediaBuffer* pBuffer;
				pSample->ConvertToContiguousBuffer(&pBuffer);
				if (hr == S_OK) {
					DWORD length;
					hr = pBuffer->GetCurrentLength(&length);
					if (hr == S_OK) {
						uint8_t* data;
						hr = pBuffer->Lock(&data, NULL, &length);
						if (hr == S_OK) {
							buffer->mediaBuffer = pBuffer;
							buffer->size = length;
							buffer->data = data;
							if (camera->yuvToRGB)
								(camera->yuvToRGB)(camera->width * camera->height, buffer->data, buffer->data, NULL);
							EnterCriticalSection(&(camera->mainMutex));
							if (camera->mainBuffer == C_NULL) {
								camera->mainBuffer = buffer;
								PostMessage(camera->window, WM_CALLBACK, 0, 0);
							}
							else {
								xs_camera_enqueueBuffer(&(camera->mainBuffer), buffer);
							}
							LeaveCriticalSection(&(camera->mainMutex));
						}
						else
							pBuffer->Release();
					}
					else
						pBuffer->Release();
				}
				pSample->Release();
			}
      	}
	}
    return 0;
}

void xs_camera_buffer_close(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	if (!data)
		return;
	xsmcVars(1);
	xsmcGet(xsVar(0), xsThis, xsID_camera);
    Camera camera = (Camera)xsmcGetHostDataValidate(xsVar(0), (void *)&xsCameraHooks);
	if (!xs_camera_releaseBuffer(camera, data))
		xsUnknownError("unknown buffer");
	xsmcSetHostBuffer(xsThis, NULL, 0);
}

void xs_camera_constructor(xsMachine *the)
{
	#define xsResultThrow(RESULT,MESSAGE) if (S_OK != (RESULT)) xsUnknownError(MESSAGE);

	uint8_t format = kIOFormatBuffer;
	uint32_t width = 320;
	uint32_t height = 240;
	uint32_t imageType = kCommodettoBitmapRGB565LE;
	CommodettoConverter yuvToRGB = NULL;
	uint16_t queueLength = 3;
	
	HRESULT hr;
	IMFAttributes* attributes = NULL;
	UINT32 deviceCount = 0;
	IMFActivate** devices = NULL;
	IMFMediaSource* mediaSource = NULL;
	IMFSourceReader* pReader;
	IMFMediaType* pType = NULL;
	
	Camera camera = NULL;
	xsmcVars(1);

	xsTry {
		format = builtinInitializeFormat(the, format);
		if ((kIOFormatBuffer != format) && (kIOFormatBufferDisposable != format))
			xsRangeError("invalid format");
		if (xsmcHas(xsArg(0), xsID_width)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_width);
			width = xsmcToInteger(xsVar(0));
		}
		if (xsmcHas(xsArg(0), xsID_height)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_height);
			height = xsmcToInteger(xsVar(0));
		}
		if (xsmcHas(xsArg(0), xsID_imageType)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_imageType);
			imageType = xsmcToInteger(xsVar(0));
		}
		if (imageType == kCommodettoBitmapRGB565LE)
			yuvToRGB = CommodettoPixelsConverterGet(kCommodettoBitmapYUV422, kCommodettoBitmapRGB565LE);
		else if (imageType != kCommodettoBitmapYUV422)
			xsRangeError("invalid imageType");
		
		xsResultThrow(MFCreateAttributes(&attributes, 1), "cannot create attributes");
		xsResultThrow(attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID), "cannot set source type attribute");
		xsResultThrow(MFEnumDeviceSources(attributes, &devices, &deviceCount), "cannot enumerate cameras");
		if (deviceCount == 0) xsUnknownError("no cameras");
		xsResultThrow(devices[0]->ActivateObject(IID_PPV_ARGS(&mediaSource)), "cannot connect camera[0] to source");
		xsResultThrow(MFCreateSourceReaderFromMediaSource(mediaSource, attributes, &pReader), "cannot create source reader");
		mediaSource->Release();
		mediaSource = NULL;
		attributes->Release();
		attributes = NULL;
		for (unsigned int i = 0; i < deviceCount; i++)
			devices[i]->Release();
		CoTaskMemFree(devices);
		devices = NULL;
		
		DWORD dwMediaTypeIndex = 0;
		uint32_t minWidth = 0xFFFFFFFF;
		uint32_t minHeight = 0xFFFFFFFF;
		for (;;) {
			if (S_OK != pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex, &pType))
				break;
			UINT32 frameWidth, frameHeight;
			MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &frameWidth, &frameHeight);
			if ((width <= frameWidth) && (height <= frameHeight)) {
				if ((minWidth > frameWidth) || (minHeight > frameHeight)) {
					minWidth = frameWidth;
					minHeight = frameHeight;
				}
			}
			pType->Release();
			pType = NULL;
			dwMediaTypeIndex++;
		}
		width = minWidth;
		height = minHeight;
		
		hr = MFCreateMediaType(&pType);
		hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);
		MFSetAttributeSize(pType, MF_MT_FRAME_SIZE, width, height);
		hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
		pType->Release();
		pType = NULL;
		
		camera = (Camera)c_calloc(1, sizeof(CameraRecord) + ((queueLength - 1) * sizeof(CameraBufferRecord)));
		if (!camera)
			xsRangeError("not enough memory");
			
		camera->pReader = pReader;
		camera->format = format;
		camera->width = width;
		camera->height = height;
		camera->imageType = imageType;
		camera->yuvToRGB = yuvToRGB;
		
		camera->queueLength = queueLength;
		for (int i = 0; i < camera->queueLength; i++)
			xs_camera_enqueueBuffer(&(camera->threadBuffer), &(camera->queueBuffers[i]));
		
		xsmcSetHostData(xsThis, camera);
		xsSetHostHooks(xsThis, (xsHostHooks *)&xsCameraHooks);
		camera->the = the;
		camera->object = xsThis;
		xsRemember(camera->object);
		camera->onReadable = builtinGetCallback(the, xsID_onReadable);	
		builtinInitializeTarget(the);
	
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(WNDCLASSEX));
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.cbWndExtra = sizeof(Camera);
		wcex.lpfnWndProc = xs_camera_WindowProc;
		wcex.lpszClassName = xs_camera_WindowClass;
		RegisterClassEx(&wcex);
		camera->window = CreateWindowEx(0, xs_camera_WindowClass, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
		SetWindowLongPtr(camera->window, 0, (LONG_PTR)camera);
		
		InitializeCriticalSection(&(camera->mainMutex));
		InitializeConditionVariable(&(camera->threadCondition));
		InitializeCriticalSection(&(camera->threadMutex));
		camera->thread = (HANDLE)_beginthreadex(NULL, 0, xs_camera_loop, camera, 0, NULL);
	}
	xsCatch {
		xs_camera_destructor(camera);
		xsThrow(xsException);
	}
}

void xs_camera_destructor(void *it)
{
	if (it) {
		Camera camera = (Camera)it;
		EnterCriticalSection(&(camera->threadMutex));
		camera->destructing = 1;
		WakeConditionVariable(&(camera->threadCondition));
		LeaveCriticalSection(&(camera->threadMutex));
		WaitForSingleObject(camera->thread, INFINITE);
		CloseHandle(camera->thread);
		DeleteCriticalSection(&(camera->threadMutex));
		DeleteCriticalSection(&(camera->mainMutex));
		if (camera->window) {
			DestroyWindow(camera->window);
			camera->window = NULL;
		}
		for (int i = 0; i < camera->queueLength; i++) {
			CameraBuffer buffer = &(camera->queueBuffers[i]);
			if (buffer->mediaBuffer) {
				buffer->mediaBuffer->Unlock();
				buffer->mediaBuffer->Release();
			}
		}
		c_free(camera);
	}
}

void xs_camera_close(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostData(xsThis);
	if (camera && xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(camera->object);
		if (camera->calling)
			camera->calling = 0;
		else
			xs_camera_destructor(camera);
	}
}

void xs_camera_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Camera camera = (Camera)it;
	if (camera->onReadable)
		(*markRoot)(the, camera->onReadable);
}

void xs_camera_read(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	if (camera->data == NULL)
		return;
	if (camera->format == kIOFormatBuffer) {
		if ((xsmcArgc > 0) && (xsReferenceType == xsmcTypeOf(xsArg(0)))) {
			void *data;
			xsUnsignedValue size;
			xsmcGetBufferWritable(xsArg(0), &data, &size);
			if (size < camera->size) {
				xsRangeError("buffer too small");
			}
			xsmcSetInteger(xsResult, camera->size);
			memcpy(data, camera->data, camera->size);
		}
		else {
			xsmcSetArrayBuffer(xsResult, camera->data, camera->size);
		}
		xs_camera_releaseBuffer(camera, camera->data);
	}
	else {
		xsmcVars(1);
		xsResult = xsNewHostObject(C_NULL);
		xsVar(0) = xsNewHostFunction(xs_camera_buffer_close, 0);
		xsmcDefine(xsResult, xsID_close, xsVar(0), xsDefault);
		xsmcDefine(xsResult, xsID_camera, xsThis, xsDefault);
		xsmcSetHostBuffer(xsResult, camera->data, camera->size);
	}
}

void xs_camera_start(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	camera->running = 1;
}

void xs_camera_stop(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	camera->running = 0;
}

void xs_camera_get_format(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	builtinGetFormat(the, camera->format);
}

void xs_camera_set_format(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatBuffer != format) && (kIOFormatBufferDisposable != format))
		xsRangeError("invalid format");
	camera->format = format;
}

void xs_camera_get_width(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->width);
}

void xs_camera_get_height(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->height);
}

void xs_camera_get_imageType(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->imageType);
}
