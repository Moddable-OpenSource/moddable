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

typedef struct AudioOutputBufferRecord AudioOutputBufferRecord;
typedef struct AudioOutputBufferRecord *AudioOutputBuffer;

struct AudioOutputBufferRecord {
	AudioOutputBuffer nextBuffer;
	uint32_t i;
	uint32_t size;
	int8_t data[1];
};

typedef struct AudioOutputRecord AudioOutputRecord;
typedef struct AudioOutputRecord *AudioOutput;
struct AudioOutputRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onWritable;
	
	AudioOutputBuffer mainBuffer;
	CRITICAL_SECTION mainMutex;
	HWND window;
	
	HANDLE thread;
	AudioOutputBuffer threadBuffer;
	CONDITION_VARIABLE threadCondition;
	CRITICAL_SECTION threadMutex;
	
	uint32_t offset;
	uint32_t size;
	uint8_t* data;
	uint8_t calling;
	uint8_t destructing;
	uint8_t running;
	xsNumberValue volume;
	
	uint8_t bitsPerSample;
	uint8_t numChannels;
	unsigned int sampleRate;
	uint16_t bytesPerFrame;
	
	uint16_t queueLength;
	uint32_t queueBufferSize;
	AudioOutputBuffer queueBuffers[1];
};

static void xs_audioout_mark_(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks xsAudioOutputHooks = {
	xs_audioout_destructor_,
	xs_audioout_mark_,
	NULL
};


static LRESULT CALLBACK xs_audioout_WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

AudioOutputBuffer xs_audioout_dequeueBuffer(AudioOutputBuffer* address)
{
	AudioOutputBuffer result = *address;
	if (result) {
		*address = result->nextBuffer;
		result->nextBuffer = C_NULL;
	}
	return result;
}

void xs_audioout_enqueueBuffer(AudioOutputBuffer* address, AudioOutputBuffer buffer)
{
	AudioOutputBuffer former;
	while ((former = *address))
		address = &(former->nextBuffer);
	*address = buffer;
}

#define xs_audioout_WindowClass "xs_audioout_WindowClass"
LRESULT CALLBACK xs_audioout_WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_CALLBACK)
		return DefWindowProc(window, message, wParam, lParam);
		
	AudioOutput output = (AudioOutput)GetWindowLongPtr(window, 0);
	AudioOutputBuffer buffer;
	
	EnterCriticalSection(&(output->mainMutex));
	buffer = output->mainBuffer;
	output->mainBuffer = C_NULL;
	LeaveCriticalSection(&(output->mainMutex));
	
	while (buffer) {
		AudioOutputBuffer nextBuffer = buffer->nextBuffer;
		buffer->nextBuffer = C_NULL;
	
		output->offset = 0;
		output->size = buffer->size;
		output->data = (uint8_t*)(buffer->data);
		if (output->onWritable && output->running) {
			output->calling = 1;
			xsBeginHost(output->the);
			xsResult = xsAccess(output->object);
			xsCallFunction2(xsReference(output->onWritable), xsResult, xsInteger(output->size), xsInteger(output->size / output->bytesPerFrame));
			xsEndHost(output->the);
			if (output->calling)
				output->calling = 0;
			else {
				xs_audioout_destructor_(output);
				return 0;
			}
		}
		c_memset(output->data + output->offset, 0, output->size - output->offset);
		EnterCriticalSection(&(output->threadMutex));
		if (output->threadBuffer == NULL) {
			output->threadBuffer = buffer;
			WakeConditionVariable(&(output->threadCondition));
		}
		else {
			xs_audioout_enqueueBuffer(&(output->threadBuffer), buffer);
		}
		LeaveCriticalSection(&(output->threadMutex));
		
		buffer = nextBuffer;
	}
	output->offset = 0;
	output->size = 0;
	output->data = C_NULL;
	return 0;
}

#define bailIfError(IT) hr = IT; if (FAILED(hr)) { fprintf(stderr, "ERROR %d\n", __LINE__); goto bail; }
#define bailIfNULL(IT) if (IT == NULL) { hr = E_FAIL; goto bail; }
#define SAFE_RELEASE(IT) \
	if ((IT) != NULL) {  \
		(IT)->Release();  \
		(IT) = NULL; \
	}

unsigned int __stdcall xs_audioout_loop(void* it)
{
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	const IID IID_IAudioClient = __uuidof(IAudioClient);
	const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
	const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);

	AudioOutput output = (AudioOutput)it;
	AudioOutputBuffer buffer;
	
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    IAudioClient *pAudioClient = NULL;
    REFERENCE_TIME duration;
	WAVEFORMATEX wfx;
    HANDLE hEvent = NULL;
    IAudioRenderClient *pRenderClient = NULL;
    IAudioStreamVolume *pStreamVolume = NULL;
	UINT32 volumeCount, volumeIndex;
	float* volumes = NULL;
	
	UINT32 framesCount, framesAvailable, bufferSize;
    BYTE *frames;
    DWORD flags = 0;

	CoInitialize(NULL);
    bailIfError(CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator));
    bailIfError(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice));
    bailIfError(pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient));
    bailIfError(pAudioClient->GetDevicePeriod(NULL, &duration));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = output->numChannels;
	wfx.nSamplesPerSec = output->sampleRate;
	wfx.wBitsPerSample = output->bitsPerSample;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = 0;
    bailIfError(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
        duration, duration, &wfx, NULL));

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    bailIfNULL(hEvent);
    bailIfError(pAudioClient->SetEventHandle(hEvent));
    
    bailIfError(pAudioClient->GetService(IID_IAudioStreamVolume, (void**)&pStreamVolume));
    bailIfError(pStreamVolume->GetChannelCount(&volumeCount));
    volumes = (float*)c_malloc(volumeCount * sizeof(float));
    bailIfNULL(volumes);

    bailIfError(pAudioClient->GetBufferSize(&framesCount));
    bufferSize = framesCount * wfx.nBlockAlign;
	for (int i = 0; i < output->queueLength; i++) {
		output->queueBuffers[i] = (AudioOutputBuffer)c_calloc(1, sizeof(AudioOutputBufferRecord) + (bufferSize - 1));
    	bailIfNULL(output->queueBuffers[i]);
    	output->queueBuffers[i]->i = i;
    	output->queueBuffers[i]->size = bufferSize;
		xs_audioout_enqueueBuffer(&(output->threadBuffer), output->queueBuffers[i]);
	}

    bailIfError(pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pRenderClient));

    bailIfError(pRenderClient->GetBuffer(framesCount, &frames));
	c_memset(frames, 0, framesCount * wfx.nBlockAlign);
    bailIfError(pRenderClient->ReleaseBuffer(framesCount, flags));

    bailIfError(pAudioClient->Start());

	for (;;) {
		EnterCriticalSection(&(output->threadMutex));
		while ((output->threadBuffer == NULL) && (output->destructing == 0))
			SleepConditionVariableCS(&(output->threadCondition), &(output->threadMutex), INFINITE);
		buffer = xs_audioout_dequeueBuffer(&(output->threadBuffer));
		LeaveCriticalSection(&(output->threadMutex));
		if (output->destructing)
			break;
			
		for (volumeIndex = 0; volumeIndex < volumeCount; volumeIndex++)
			volumes[volumeIndex] = (float)output->volume;
      	bailIfError(pStreamVolume->SetAllVolumes(volumeCount, volumes));
     
		int8_t* p = (int8_t*)buffer->data;
		UINT32 c = buffer->size / wfx.nBlockAlign;
		while (c > 0) {
			if (WaitForSingleObject(hEvent, 2000) != WAIT_OBJECT_0) {
				pAudioClient->Stop();
				hr = ERROR_TIMEOUT;
				goto bail;
			}
			bailIfError(pAudioClient->GetCurrentPadding(&framesAvailable));
			framesAvailable = framesCount - framesAvailable;
		
			UINT32 result = c;
			if (result > framesAvailable)
				result = framesAvailable;
			bailIfError(pRenderClient->GetBuffer(result, &frames));
			c_memcpy(frames, p, result * wfx.nBlockAlign);
			bailIfError(pRenderClient->ReleaseBuffer(result, flags));
			p += (result * wfx.nBlockAlign);
			c -= result;
		}
	
		EnterCriticalSection(&(output->mainMutex));
		if (output->mainBuffer == C_NULL) {
			output->mainBuffer = buffer;
			PostMessage(output->window, WM_CALLBACK, 0, 0);
		}
		else {
			xs_audioout_enqueueBuffer(&(output->mainBuffer), buffer);
		}
		LeaveCriticalSection(&(output->mainMutex));
	}    

    bailIfError(pAudioClient->Stop());

bail:
	for (int i = 0; i < output->queueLength; i++)
		if (output->queueBuffers[i])
			c_free(output->queueBuffers[i]);
    if (volumes != NULL)
        c_free((void*)volumes);
    if (hEvent != NULL)
        CloseHandle(hEvent);
    SAFE_RELEASE(pEnumerator)
    SAFE_RELEASE(pDevice)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(pRenderClient)
    return 0;
}

void xs_audioout_constructor_(xsMachine* the)
{
	uint8_t format = kIOFormatBuffer;
	xsStringValue type;
	int bitsPerSample = 0;
	int numChannels = 0;
	int sampleRate = 0;
	uint16_t queueLength = 2;
	uint16_t bytesPerFrame = 0;
	uint32_t bufferSize = 0;
	AudioOutput output;
	xsmcVars(1);

#ifdef MODDEF_AUDIOOUT_BITSPERSAMPLE
	bitsPerSample = MODDEF_AUDIOOUT_BITSPERSAMPLE;
#endif
#ifdef MODDEF_AUDIOOUT_NUMCHANNELS
	numChannels = MODDEF_AUDIOOUT_NUMCHANNELS;
#endif
#ifdef MODDEF_AUDIOOUT_SAMPLERATE
	sampleRate = MODDEF_AUDIOOUT_SAMPLERATE;
#endif
#ifdef MODDEF_AUDIOOUT_QUEUELENGTH
	queueLength = MODDEF_AUDIOOUT_QUEUELENGTH;
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

	output = (AudioOutput)c_calloc(1, sizeof(AudioOutputRecord) + ((queueLength - 1) * sizeof(AudioOutputBuffer)));
	if (!output)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, output);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAudioOutputHooks);
	
	output->volume = 1;
	output->bitsPerSample = (uint8_t)bitsPerSample;
	output->numChannels = (uint8_t)numChannels;
	output->sampleRate = (unsigned int)sampleRate;
	output->bytesPerFrame = bytesPerFrame;
	
	output->queueBufferSize = bufferSize;
	output->queueLength = queueLength;
	
	output->the = the;
	output->object = xsThis;
	xsRemember(output->object);
	output->onWritable = builtinGetCallback(the, xsID_onWritable);	
	builtinInitializeTarget(the);
	
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = sizeof(AudioOutput);
	wcex.lpfnWndProc = xs_audioout_WindowProc;
	wcex.lpszClassName = xs_audioout_WindowClass;
	RegisterClassEx(&wcex);
	output->window = CreateWindowEx(0, xs_audioout_WindowClass, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
	SetWindowLongPtr(output->window, 0, (LONG_PTR)output);
	
	InitializeCriticalSection(&(output->mainMutex));
	InitializeConditionVariable(&(output->threadCondition));
	InitializeCriticalSection(&(output->threadMutex));
	output->thread = (HANDLE)_beginthreadex(NULL, 0, xs_audioout_loop, output, 0, NULL);
}

void xs_audioout_destructor_(void *it)
{
	if (it) {
		AudioOutput output = (AudioOutput)it;
		EnterCriticalSection(&(output->threadMutex));
		output->destructing = 1;
		WakeConditionVariable(&(output->threadCondition));
		LeaveCriticalSection(&(output->threadMutex));
		WaitForSingleObject(output->thread, INFINITE);
		CloseHandle(output->thread);
		DeleteCriticalSection(&(output->threadMutex));
		DeleteCriticalSection(&(output->mainMutex));
		if (output->window) {
			DestroyWindow(output->window);
			output->window = NULL;
		}
		c_free(output);
	}
}

void xs_audioout_close_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostData(xsThis);
	if (output && xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(output->object);
		if (output->calling)
			output->calling = 0;
		else
			xs_audioout_destructor_(output);
	}
}

void xs_audioout_mark_(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	AudioOutput output = (AudioOutput)it;
	if (output->onWritable)
		(*markRoot)(the, output->onWritable);
}

void xs_audioout_start_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	if (output->running)
		xsUnknownError("already started");
	output->running = 1;
}

void xs_audioout_stop_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	output->running = 0;
}

void xs_audioout_write_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsUnsignedValue available = output->size - output->offset, length;
	void* buffer;
	xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
	xsmcSetInteger(xsResult, length);
	if ((length <= 0) || (length > available)) 
		xsUnknownError("invalid size");
	c_memcpy(output->data + output->offset, buffer, length);
	output->offset += length;
}

void xs_audioout_get_format_(xsMachine* the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	builtinGetFormat(the, kIOFormatBuffer);
}

void xs_audioout_set_format_(xsMachine* the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	uint8_t format = builtinSetFormat(the);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
}

void xs_audioout_get_bitsPerSample_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsResult = xsInteger(output->bitsPerSample);
}

void xs_audioout_get_numChannels_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsResult = xsInteger(output->numChannels);
}

void xs_audioout_get_sampleRate_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsResult = xsInteger(output->sampleRate);
}

void xs_audioout_get_volume_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsNumberValue volume = output->volume;
	xsResult = xsNumber(output->volume);
}

void xs_audioout_set_volume_(xsMachine* the)
{
	AudioOutput output = (AudioOutput)xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsNumberValue volume = xsmcToNumber(xsArg(0));
	if ((volume < 0) || (volume > 1))
		xsRangeError("invalid volume");
	output->volume = volume;
}
