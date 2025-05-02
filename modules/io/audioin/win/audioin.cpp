/*
 * Copyright (c) 2024 Moddable Tech, Inc.
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
#include "builtinCommon.h"

#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>
#include <process.h>

typedef struct AudioInputBufferRecord AudioInputBufferRecord;
typedef struct AudioInputBufferRecord *AudioInputBuffer;

struct AudioInputBufferRecord {
	AudioInputBuffer nextBuffer;
	uint32_t i;
	uint32_t size;
	int8_t data[1];
};

typedef struct AudioInputRecord AudioInputRecord;
typedef struct AudioInputRecord *AudioInput;
struct AudioInputRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onReadable;
	
	AudioInputBuffer mainBuffer;
	CRITICAL_SECTION mainMutex;
	HWND window;
	
	HANDLE thread;
	AudioInputBuffer threadBuffer;
	CONDITION_VARIABLE threadCondition;
	CRITICAL_SECTION threadMutex;
	
	uint32_t offset;
	uint32_t size;
	uint8_t* data;
	uint8_t calling;
	uint8_t destructing;
	uint8_t running;
	
	uint8_t bitsPerSample;
	uint8_t numChannels;
	unsigned int sampleRate;
	uint16_t bytesPerFrame;
	
	uint16_t queueLength;
	AudioInputBuffer queueBuffers[1];
};

static void xs_audioin_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks xsAudioInputHooks = {
	xs_audioin_destructor,
	xs_audioin_mark,
	NULL
};


static LRESULT CALLBACK xs_audioin_WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

AudioInputBuffer xs_audioin_dequeueBuffer(AudioInputBuffer* address)
{
	AudioInputBuffer result = *address;
	if (result) {
		*address = result->nextBuffer;
		result->nextBuffer = C_NULL;
	}
	return result;
}

void xs_audioin_enqueueBuffer(AudioInputBuffer* address, AudioInputBuffer buffer)
{
	AudioInputBuffer former;
	while ((former = *address))
		address = &(former->nextBuffer);
	*address = buffer;
}

#define xs_audioin_WindowClass "xs_audioin_WindowClass"
LRESULT CALLBACK xs_audioin_WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_CALLBACK)
		return DefWindowProc(window, message, wParam, lParam);
		
	AudioInput input = (AudioInput)GetWindowLongPtr(window, 0);
	AudioInputBuffer buffer;
	
	EnterCriticalSection(&(input->mainMutex));
	buffer = input->mainBuffer;
	input->mainBuffer = C_NULL;
	LeaveCriticalSection(&(input->mainMutex));
	
	while (buffer) {
		AudioInputBuffer nextBuffer = buffer->nextBuffer;
		buffer->nextBuffer = C_NULL;
	
		input->offset = 0;
		input->size = buffer->size;
		input->data = (uint8_t*)(buffer->data);
		if (input->onReadable && input->running && (input->size > 0)) {
			input->calling = 1;
			xsBeginHost(input->the);
			xsResult = xsAccess(input->object);
			xsCallFunction2(xsReference(input->onReadable), xsResult, xsInteger(input->size), xsInteger(input->size / input->bytesPerFrame));
			xsEndHost(input->the);
			if (input->calling)
				input->calling = 0;
			else {
				xs_audioin_destructor(input);
				return 0;
			}
		}

		EnterCriticalSection(&(input->threadMutex));
		if (input->threadBuffer == NULL) {
			input->threadBuffer = buffer;
			WakeConditionVariable(&(input->threadCondition));
		}
		else {
			xs_audioin_enqueueBuffer(&(input->threadBuffer), buffer);
		}
		LeaveCriticalSection(&(input->threadMutex));
		
		buffer = nextBuffer;
	}
	input->offset = 0;
	input->size = 0;
	input->data = C_NULL;
	return 0;
}

#define bailIfError(IT) hr = IT; if (FAILED(hr)) { fprintf(stderr, "ERROR %d\n", __LINE__); goto bail; }
#define bailIfNULL(IT) if (IT == NULL) { hr = E_FAIL; goto bail; }
#define SAFE_RELEASE(IT) \
	if ((IT) != NULL) {  \
		(IT)->Release();  \
		(IT) = NULL; \
	}

unsigned int __stdcall xs_audioin_loop(void* it)
{
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	const IID IID_IAudioClient = __uuidof(IAudioClient);
	const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

	AudioInput input = (AudioInput)it;
	AudioInputBuffer buffer;
	
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    IAudioClient *pAudioClient = NULL;
    REFERENCE_TIME duration;
	WAVEFORMATEX wfx;
    HANDLE hEvent = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
	
	UINT32 framesCount, framesAvailable, bufferSize;
    BYTE *frames;
    DWORD flags = 0;

	CoInitialize(NULL);
    bailIfError(CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator));
    bailIfError(pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice));
    bailIfError(pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient));
    bailIfError(pAudioClient->GetDevicePeriod(&duration, NULL));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = input->numChannels;
	wfx.nSamplesPerSec = input->sampleRate;
	wfx.wBitsPerSample = input->bitsPerSample;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = 0;
    bailIfError(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
        duration, 0, &wfx, NULL));

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    bailIfNULL(hEvent);
    bailIfError(pAudioClient->SetEventHandle(hEvent));

    bailIfError(pAudioClient->GetBufferSize(&framesCount));
    bufferSize = framesCount * wfx.nBlockAlign;
	for (int i = 0; i < input->queueLength; i++) {
		input->queueBuffers[i] = (AudioInputBuffer)c_calloc(1, sizeof(AudioInputBufferRecord) + (bufferSize - 1));
    	bailIfNULL(input->queueBuffers[i]);
    	input->queueBuffers[i]->i = i;
		xs_audioin_enqueueBuffer(&(input->threadBuffer), input->queueBuffers[i]);
	}
    duration = ((1000 * framesCount) / wfx.nSamplesPerSec) / 2;

    bailIfError(pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient));

    bailIfError(pAudioClient->Start());

	for (;;) {
		EnterCriticalSection(&(input->threadMutex));
		while ((input->threadBuffer == NULL) && (input->destructing == 0))
			SleepConditionVariableCS(&(input->threadCondition), &(input->threadMutex), INFINITE);
		buffer = xs_audioin_dequeueBuffer(&(input->threadBuffer));
		LeaveCriticalSection(&(input->threadMutex));
		if (input->destructing)
			break;
 		if (WaitForSingleObject(hEvent, 2000) != WAIT_OBJECT_0) {
			pAudioClient->Stop();
			hr = ERROR_TIMEOUT;
			goto bail;
		}
 		if (input->destructing)
			break;
      
		int8_t* p = (int8_t*)buffer->data;
		buffer->size = 0;
    	UINT32 packetLength = 0;
		bailIfError(pCaptureClient->GetNextPacketSize(&packetLength));
		while (packetLength != 0) {
			bailIfError(pCaptureClient->GetBuffer(&frames, &framesAvailable, &flags, NULL, NULL));
			bufferSize = framesAvailable * wfx.nBlockAlign;
			c_memcpy(p, frames, bufferSize);
			p += bufferSize;
			buffer->size += bufferSize;
			bailIfError(pCaptureClient->ReleaseBuffer(framesAvailable));
			bailIfError(pCaptureClient->GetNextPacketSize(&packetLength));
		}
		
		EnterCriticalSection(&(input->mainMutex));
		if (input->mainBuffer == C_NULL) {
			input->mainBuffer = buffer;
			PostMessage(input->window, WM_CALLBACK, 0, 0);
		}
		else {
			xs_audioin_enqueueBuffer(&(input->mainBuffer), buffer);
		}
		LeaveCriticalSection(&(input->mainMutex));
	}    

    bailIfError(pAudioClient->Stop());

bail:
	for (int i = 0; i < input->queueLength; i++)
		if (input->queueBuffers[i])
			c_free(input->queueBuffers[i]);
    if (hEvent != NULL)
        CloseHandle(hEvent);
    SAFE_RELEASE(pEnumerator)
    SAFE_RELEASE(pDevice)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(pCaptureClient)
    return 0;
}

void xs_audioin_constructor(xsMachine *the)
{
	uint8_t format = kIOFormatBuffer;
	xsStringValue type;
	uint8_t bitsPerSample = 0;
	uint8_t numChannels = 0;
	unsigned int sampleRate = 0;
	uint16_t queueLength = 8;
	uint16_t bytesPerFrame = 0;
	uint32_t bufferSize = 0;
	AudioInput input;
	xsmcVars(1);

#ifdef MODDEF_AUDIOIN_BITSPERSAMPLE
	bitsPerSample = MODDEF_AUDIOIN_BITSPERSAMPLE;
#endif
#ifdef MODDEF_AUDIOIN_NUMCHANNELS
	numChannels = MODDEF_AUDIOIN_NUMCHANNELS;
#endif
#ifdef MODDEF_AUDIOIN_SAMPLERATE
	sampleRate = MODDEF_AUDIOIN_SAMPLERATE;
#endif
#ifdef MODDEF_AUDIOIN_QUEUELENGTH
	queueLength = MODDEF_AUDIOIN_QUEUELENGTH;
#endif
	format = builtinInitializeFormat(the, format);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
	if (xsmcHas(xsArg(0), xsID_type)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_type);
		type = xsmcToString(xsVar(0));
		if (c_strcmp(type, "LPCM"))
			xsRangeError("invalid type");
	}
	if (xsmcHas(xsArg(0), xsID_bitsPerSample)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_bitsPerSample);
		bitsPerSample = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_channels)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channels);
		numChannels = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_sampleRate)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_sampleRate);
		sampleRate = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_queueLength)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_queueLength);
		queueLength = xsmcToInteger(xsVar(0));
	}
	if ((8 != bitsPerSample) && (16 != bitsPerSample))
		xsRangeError("invalid bits per sample");
	if ((1 != numChannels) && (2 != numChannels))
		xsRangeError("invalid number of channels");
	if ((sampleRate < 8000) || (sampleRate > 48000))
		xsRangeError("invalid sample rate");
	if (queueLength < 2)
		xsRangeError("invalid queue length");
	bytesPerFrame = (bitsPerSample * numChannels) >> 3;
	bufferSize = (bytesPerFrame * sampleRate) >> 5; //??

	input = (AudioInput)c_calloc(1, sizeof(AudioInputRecord) + ((queueLength - 1) * sizeof(AudioInputBuffer)));
	if (!input)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, input);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAudioInputHooks);
	
	input->bitsPerSample = bitsPerSample;
	input->numChannels = numChannels;
	input->sampleRate = sampleRate;
	input->bytesPerFrame = bytesPerFrame;
	input->queueLength = queueLength;
	
	input->the = the;
	input->object = xsThis;
	xsRemember(input->object);
	input->onReadable = builtinGetCallback(the, xsID_onReadable);	
	builtinInitializeTarget(the);
	
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(AudioInput);
	wcex.lpfnWndProc = xs_audioin_WindowProc;
	wcex.lpszClassName = xs_audioin_WindowClass;
	RegisterClassEx(&wcex);
	input->window = CreateWindowEx(0, xs_audioin_WindowClass, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongPtr(input->window, 0, (LONG_PTR)input);
	
	InitializeCriticalSection(&(input->mainMutex));
	InitializeConditionVariable(&(input->threadCondition));
	InitializeCriticalSection(&(input->threadMutex));
	input->thread = (HANDLE)_beginthreadex(NULL, 0, xs_audioin_loop, input, 0, NULL);
}

void xs_audioin_destructor(void *it)
{
	if (it) {
		AudioInput input = (AudioInput)it;
		EnterCriticalSection(&(input->threadMutex));
		input->destructing = 1;
		WakeConditionVariable(&(input->threadCondition));
		LeaveCriticalSection(&(input->threadMutex));
		WaitForSingleObject(input->thread, INFINITE);
		CloseHandle(input->thread);
		DeleteCriticalSection(&(input->threadMutex));
		DeleteCriticalSection(&(input->mainMutex));
		if (input->window) {
			DestroyWindow(input->window);
			input->window = NULL;
		}
		
		c_free(input);
	}
}

void xs_audioin_close(xsMachine *the)
{
	AudioInput input = (AudioInput)xsmcGetHostData(xsThis);
	if (input && xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(input->object);
		if (input->calling)
			input->calling = 0;
		else
			xs_audioin_destructor(input);
	}
}

void xs_audioin_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	AudioInput input = (AudioInput)it;
	if (input->onReadable)
		(*markRoot)(the, input->onReadable);
}

void xs_audioin_read(xsMachine *the)
{
	AudioInput input = (AudioInput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	xsUnsignedValue available, requested;
	xsBooleanValue allocate = 1;
	void* buffer;
	available = input->size - input->offset;
	if (0 == xsmcArgc)
		requested = available;
	else if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsResult = xsArg(0);
		xsmcGetBufferWritable(xsResult, (void **)&buffer, &requested);
		xsmcSetInteger(xsResult, requested);
		allocate = 0;
	}
	else
		requested = xsmcToInteger(xsArg(0));
		
	if ((requested <= 0) || (requested > available)) 
		xsUnknownError("invalid size");
	if (allocate)
		buffer = xsmcSetArrayBuffer(xsResult, NULL, requested);
	c_memcpy(buffer, input->data + input->offset, requested);
	input->offset += requested;
}

void xs_audioin_start(xsMachine *the)
{
	AudioInput input = (AudioInput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	input->running = 1;
}

void xs_audioin_stop(xsMachine *the)
{
	AudioInput input = (AudioInput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	input->running = 0;
}

void xs_audioin_get_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	builtinGetFormat(the, kIOFormatBuffer);
}

void xs_audioin_set_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	uint8_t format = builtinSetFormat(the);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
}

void xs_audioin_get_bitsPerSample(xsMachine *the)
{
	AudioInput input = (AudioInput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	xsResult = xsInteger(input->bitsPerSample);
}

void xs_audioin_get_numChannels(xsMachine *the)
{
	AudioInput input = (AudioInput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	xsResult = xsInteger(input->numChannels);
}

void xs_audioin_get_sampleRate(xsMachine *the)
{
	AudioInput input = (AudioInput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	xsResult = xsInteger(input->sampleRate);
}
