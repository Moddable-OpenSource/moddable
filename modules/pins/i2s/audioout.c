/*
 * Copyright (c) 2018-2023 Moddable Tech, Inc.
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
/*
	WIN32 implementation modeled on KplAudioWin.c (Apache License, Marvell Semiconductor)
	https://github.com/Kinoma/kinomajs/blob/master/build/win/Kpl/KplAudioWin.c
*/

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"
#include "sbc_decoder.h"

#ifndef MODDEF_AUDIOOUT_STREAMS
	#define MODDEF_AUDIOOUT_STREAMS (4)
#endif
#ifndef MODDEF_AUDIOOUT_BITSPERSAMPLE
	#define MODDEF_AUDIOOUT_BITSPERSAMPLE (16)
#endif
#ifndef MODDEF_AUDIOOUT_NUMCHANNELS
	#define MODDEF_AUDIOOUT_NUMCHANNELS (1)
#endif
#ifndef MODDEF_AUDIOOUT_QUEUELENGTH
	#define MODDEF_AUDIOOUT_QUEUELENGTH (8)
#endif
#ifndef MODDEF_AUDIOOUT_MIXERBYTES
	#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 8
		#define MODDEF_AUDIOOUT_MIXERBYTES (256 * 2)
	#else
		#define MODDEF_AUDIOOUT_MIXERBYTES (384 * 2)
	#endif
#endif
#if ESP32
	#ifndef MODDEF_AUDIOOUT_I2S_NUM
		#define MODDEF_AUDIOOUT_I2S_NUM (1)
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_MCK_PIN
		#define MODDEF_AUDIOOUT_I2S_MCK_PIN I2S_PIN_NO_CHANGE
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_BCK_PIN
		#define MODDEF_AUDIOOUT_I2S_BCK_PIN (26)
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_LR_PIN
		#define MODDEF_AUDIOOUT_I2S_LR_PIN (25)
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_DATAOUT_PIN
		#define MODDEF_AUDIOOUT_I2S_DATAOUT_PIN (22)
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
		#define MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE (16)
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_DAC
		#define MODDEF_AUDIOOUT_I2S_DAC (0)
	#endif
	#if MODDEF_AUDIOOUT_I2S_DAC && (16 != MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE)
		#error must be 16 bit samples
	#endif
	#if MODDEF_AUDIOOUT_I2S_DAC
		#include "driver/dac_continuous.h"
		#ifndef MODDEF_AUDIOOUT_I2S_DAC_CHANNEL
			// I2S_DAC_CHANNEL_BOTH_EN = 3
			#define MODDEF_AUDIOOUT_I2S_DAC_CHANNEL 3
		#endif
	#endif
#endif

#if PICO_BUILD
	#include "modTimer.h"
	static void picoAudioCallback(modTimer timer, void *refcon, int refconSize);

	#define SAMPLES_PER_BUFFER (882 * 2)
	#define NUM_SAMPLES_BUFFERS	3
	#ifndef MODDEF_AUDIOOUT_I2S_BCK_PIN
		#define MODDEF_AUDIOOUT_I2S_BCK_PIN (10)
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_LR_PIN
		#define MODDEF_AUDIOOUT_I2S_LR_PIN (11)
	#endif
	#ifndef MODDEF_AUDIOOUT_I2S_DATAOUT_PIN
		#define MODDEF_AUDIOOUT_I2S_DATAOUT_PIN (9)
	#endif
#endif

#ifndef MODDEF_AUDIOOUT_VOLUME_DIVIDER
	#define MODDEF_AUDIOOUT_VOLUME_DIVIDER (1)
#endif

#ifndef MODDEF_AUDIOOUT_I2S_PDM
	#define MODDEF_AUDIOOUT_I2S_PDM (0)
#elif !defined(__ets__)
	#error "PDM on ESP8266 & ESP32 only"
#elif MODDEF_AUDIOOUT_I2S_PDM == 0
	// esp8266 direct i2s output
#elif !ESP32 && (MODDEF_AUDIOOUT_I2S_PDM != 32) && (MODDEF_AUDIOOUT_I2S_PDM != 64) && (MODDEF_AUDIOOUT_I2S_PDM != 128)
	#error "invalid PDM oversampling"
#endif

#if ESP32 && MODDEF_AUDIOOUT_I2S_PDM && !defined(MODDEF_AUDIOOUT_I2S_PDM_PIN) 
	#error must define MODDEF_AUDIOOUT_I2S_PDM_PIN
#endif

#if MODDEF_AUDIOOUT_STREAMS > 4
	#error "can't mix over 4 streams"
#endif
#if (8 != MODDEF_AUDIOOUT_BITSPERSAMPLE) && (16 != MODDEF_AUDIOOUT_BITSPERSAMPLE)
	#error "bitsPerSample must be 8 or 16"
#endif

#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 8
	#define OUTPUTSAMPLETYPE int8_t
#elif MODDEF_AUDIOOUT_BITSPERSAMPLE == 16
	#define OUTPUTSAMPLETYPE int16_t
#endif

#if defined(__APPLE__)
	#import <AudioUnit/AudioUnit.h>
	#import <AudioToolbox/AudioToolbox.h>
	#define kAudioQueueBufferCount (2)
#elif defined(_WIN32)
	#define INITGUID
	#include "windows.h"
	#include "mmreg.h"
	#include "dsound.h"
	#define kAudioQueueBufferCount (2)
	enum {
		kStateIdle = 0,
		kStatePlaying = 1
	};
#elif ESP32
	#include "xsHost.h"
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "freertos/semphr.h"
#ifdef MODDEF_AUDIOOUT_I2S_PDM
	#include "driver/i2s_pdm.h"
	#include "driver/i2s_std.h"
#else
	#error
#endif

	enum {
		kStateIdle = 0,
		kStatePlaying = 1,
		kStateClosing = 2,
		kStateTerminated = 3
	};
#elif defined(__ets__)
	#include "xsHost.h"
	#include "tinyi2s.h"
#elif PICO_BUILD
	#include "xsHost.h"
	#include "pico/audio_i2s.h"
	enum {
		kStateIdle = 0,
		kStatePlaying = 1,
		kStateClosing = 2,
		kStateTerminated = 3
	};
#endif

#define kIMABytesPerChunk (68)
#define kIMASamplesPerChunk (129)	/* (kIMABytesPerChunk - 4) / 2 + 1 */
extern int dvi_adpcm_decode(void *in_buf, int in_size, void *out_buf);

#define kSBCSamplesPerChunk (128)
#define kToneSamplesPerChunk (128)

// kDecompressBufferSize based on maximum of generated buffer sizes
// 		kIMASamplesPerChunk * sizeof(int16_t)
// 		kSBCSamplesPerChunk * sizeof(int16_t)
// 		kToneSamplesPerChunk * sizeof(int16_t));
#define kDecompressBufferSize (129 * 2)

typedef struct {
	void		*samples;
	int			sampleCount;		// 0 means this is a callback or volume command with value of (uintptr_t)samples
	int			position;			// less than zero if callback, else
	int16_t		repeat;				// always 1 for callback, negative for infinite
	int8_t		sampleFormat;		// kSampleFormat
	int8_t		reserved;
	union {
		struct {
			uint32_t	remaining;
			uint16_t	total;
			uint8_t		*data;
			uint8_t		*initial;
		} compressed;
		struct {
			uint32_t			position;
			uint32_t			max;
			int32_t				count;
			OUTPUTSAMPLETYPE	value;
		} tone;
		struct {
			uint32_t			remaining;
		} silence;
	};
} modAudioQueueElementRecord, *modAudioQueueElement;

typedef struct {
	uint16_t		volume;				// 8.8 fixed
	uint16_t		reserved;
	int16_t			*decompressed;
	void			*decompressor;		//@@ merge into decompressed block
	int				elementCount;
	modAudioQueueElementRecord	element[MODDEF_AUDIOOUT_QUEUELENGTH];
} modAudioOutStreamRecord, *modAudioOutStream;

typedef struct {
	xsIntegerValue		id;
	xsIntegerValue		stream;
} modAudioOutPendingCallbackRecord, *modAudioOutPendingCallback;


typedef struct {
	xsMachine				*the;
	xsSlot					obj;

	uint16_t				sampleRate;
	uint8_t					numChannels;
	uint8_t					bitsPerSample;
	uint8_t					bytesPerFrame;
	uint8_t					applyVolume;		// one or more active streams is not at 1.0 volume
	uint8_t					built;
	int8_t					useCount;

	int						activeStreamCount;
	modAudioOutStream		activeStream[MODDEF_AUDIOOUT_STREAMS];

	int						streamCount;

#if defined(__APPLE__)
	AudioQueueRef			audioQueue;
	AudioQueueBufferRef		buffer[kAudioQueueBufferCount];
	pthread_mutex_t			mutex;
	CFRunLoopTimerRef		callbackTimer;
	CFRunLoopRef			runLoop;
#elif defined(_WIN32)
	HWND					hWnd;
	LPDIRECTSOUND8			dS;
	LPDIRECTSOUNDBUFFER8	dSBuffer;
	HANDLE					hNotify;
	HANDLE					hEndThread;
	HANDLE					hNotifyThread;
	uint32_t				bufferSize;
	uint32_t				bytesWritten;
	uint32_t				bufferCount;
	uint32_t				samplesNeeded;
	uint8_t					*buffer;
	CRITICAL_SECTION		cs;
	uint8_t					state;		// 0 idle, 1 playing
#elif ESP32
	SemaphoreHandle_t 		mutex;
	TaskHandle_t			task;

	uint8_t					state;		// 0 idle, 1 playing, 2 terminated
#if MODDEF_AUDIOOUT_I2S_DAC
	dac_continuous_handle_t	dacHandle;
	uint8_t					*buffer8;
#else
	i2s_chan_handle_t 		tx_handle;
#endif
	uint32_t				buffer[MODDEF_AUDIOOUT_MIXERBYTES >> 2];
#elif defined(__ets__)
	uint8_t					i2sActive;
	OUTPUTSAMPLETYPE		buffer[64];		// size assumes DMA Buffer size of I2S
#elif PICO_BUILD
	uint8_t					state;
	modTimer				timer;
	struct audio_buffer_pool	*ap;
	audio_format_t 				format;
	struct audio_buffer_format	producer_format;
	struct audio_buffer_pool	*producer_pool;
	struct audio_i2s_config config;
#endif

#if MODDEF_AUDIOOUT_I2S_PDM
	int32_t					prevSample;
	int32_t					error;
#endif

	int									pendingCallbackCount;
	modAudioOutPendingCallbackRecord	pendingCallbacks[MODDEF_AUDIOOUT_QUEUELENGTH];

	modAudioOutStreamRecord	stream[1];		// must be last
} modAudioOutRecord, *modAudioOut;

static void updateActiveStreams(modAudioOut out);
#if defined(__APPLE__)
	static void audioQueueCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer);
	static void queueCallback(modAudioOut out, xsIntegerValue id, xsIntegerValue stream);
	static void invokeCallbacks(CFRunLoopTimerRef timer, void *info);
#elif defined(_WIN32)
	static LRESULT CALLBACK modAudioWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI directSoundProc(LPVOID lpParameter);
	static HRESULT createDirectSoundBuffer(modAudioOut out, uint32_t bufferSize, uint32_t bufferCount);
	static void releaseDirectSoundBuffer(modAudioOut out);
	static void doRenderSamples(modAudioOut out);
#elif ESP32
	static void audioOutLoop(void *pvParameter);
#elif defined(__ets__)
	static void doRenderSamples(void *refcon, int16_t *lr, int count);
#endif
#if PICO_BUILD || defined(__ets__) || ESP32 || defined(_WIN32)
	void deliverCallbacks(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
	void queueCallback(modAudioOut out, xsIntegerValue id, xsIntegerValue stream);
#endif

static void doLock(modAudioOut out);
static void doUnlock(modAudioOut out);

static void audioMix(modAudioOut out, int samplesToGenerate, OUTPUTSAMPLETYPE *output);
static void endOfElement(modAudioOut out, modAudioOutStream stream);
static void setStreamVolume(modAudioOut out, modAudioOutStream stream, int volume);
static int streamDecompressNext(modAudioOutStream stream);

static const xsHostHooks ICACHE_RODATA_ATTR xsAudioOutHooks = {
	xs_audioout_destructor,
	NULL,
	NULL
};

#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 8
	#define MIXSAMPLETYPE int16_t
	#define ClampSample(s, streams) ((s < -128) ? -128 : ((s > 127) ? 127 : s))
//	#define ClampSample(s, streams) (s / streams)
//	#define ClampSample(s, streams) (s)
#elif MODDEF_AUDIOOUT_BITSPERSAMPLE == 16
	#define MIXSAMPLETYPE int32_t
	#define ClampSample(s, streams) ((s < -32768) ? -32768 : ((s > 32767) ? 32767 : s))
//	#define ClampSample(s, streams) (s / streams)
//	#define ClampSample(s, streams) (s)
#endif

void xs_audioout_destructor(void *data)
{
	modAudioOut out = data;
	int i;

	if (!out)
		return;

#if defined(__APPLE__)
	if (out->callbackTimer)
		CFRunLoopTimerInvalidate(out->callbackTimer);

	if (out->audioQueue) {
		int i;

		AudioQueueStop(out->audioQueue, true);

		for (i = 0; i < kAudioQueueBufferCount; i++)
			if (out->buffer[i])
				AudioQueueFreeBuffer(out->audioQueue, out->buffer[i]);

		AudioQueueDispose(out->audioQueue, true);
	}

	pthread_mutex_destroy(&out->mutex);
#elif defined(_WIN32)
	if (NULL != out->dSBuffer)
		releaseDirectSoundBuffer(out);
	if (NULL != out->dS) 
		IDirectSound8_Release(out->dS);
	if (out->hWnd)
		DestroyWindow(out->hWnd);
	DeleteCriticalSection(&out->cs);
	UnregisterClass("modAudioWindowClass", NULL);
#elif ESP32
	out->state = kStateClosing;
	if (out->task) {
		xTaskNotify(out->task, kStateClosing, eSetValueWithOverwrite);
		while (kStateClosing == out->state)
			modDelayMilliseconds(1);

		vSemaphoreDelete(out->mutex);
	}
#if MODDEF_AUDIOOUT_I2S_DAC
	if (out->buffer8)
		free(out->buffer8);
#endif

#elif defined(__ets__)
	if (out->i2sActive)
		i2s_end();
#endif

	for (i = 0; i < out->streamCount; i++) {
		if (out->stream[i].decompressed)
			c_free(out->stream[i].decompressed);
		if (out->stream[i].decompressor)
			c_free(out->stream[i].decompressor);
	}

	c_free(out);
}

void xs_audioout(xsMachine *the)
{
	int i;
	modAudioOut out;
	uint16_t sampleRate = 0;
	uint8_t numChannels = 0;
	uint8_t bitsPerSample = 0;
	int streamCount = 0;

	xsmcVars(1);

	if (xsReferenceType != xsmcTypeOf(xsArg(0)))
		xsSyntaxError("no options");

	if (xsmcHas(xsArg(0), xsID_sampleRate)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_sampleRate);
		sampleRate = xsmcToInteger(xsVar(0));
	}
#ifdef MODDEF_AUDIOOUT_SAMPLERATE
	else
		sampleRate = MODDEF_AUDIOOUT_SAMPLERATE;
#endif
	if ((sampleRate < 8000) || (sampleRate > 48000))
		xsRangeError("invalid sample rate");

	if (xsmcHas(xsArg(0), xsID_numChannels)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_numChannels);
		numChannels = xsmcToInteger(xsVar(0));
	}
#ifdef MODDEF_AUDIOOUT_NUMCHANNELS
	else
		numChannels = MODDEF_AUDIOOUT_NUMCHANNELS;
#endif
	if ((1 != numChannels) && (2 != numChannels))
		xsRangeError("bad numChannels");

	if (xsmcHas(xsArg(0), xsID_bitsPerSample)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_bitsPerSample);
		bitsPerSample = xsmcToInteger(xsVar(0));
	}
#ifdef MODDEF_AUDIOOUT_BITSPERSAMPLE
	else
		bitsPerSample = MODDEF_AUDIOOUT_BITSPERSAMPLE;
#endif
	if ((8 != bitsPerSample) && (16 != bitsPerSample))
		xsRangeError("bad bitsPerSample");

	streamCount = MODDEF_AUDIOOUT_STREAMS;
	if (xsmcHas(xsArg(0), xsID_streams)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_streams);
		streamCount = xsmcToInteger(xsVar(0));
	}
	if ((streamCount < 1) || (streamCount > MODDEF_AUDIOOUT_STREAMS))
		xsRangeError("bad streamCount");

	out = (modAudioOut)c_calloc(sizeof(modAudioOutRecord) + (sizeof(modAudioOutStreamRecord) * (streamCount - 1)), 1);
	if (!out)
		xsUnknownError("no memory");
	xsmcSetHostData(xsThis, out);
	xsSetHostHooks(xsThis, (void *)&xsAudioOutHooks);

	out->the = the;
	out->obj = xsThis;
	out->useCount = 1;

	out->streamCount = streamCount;
	out->sampleRate = sampleRate;
	out->numChannels = numChannels;
	out->bitsPerSample = bitsPerSample;
	out->bytesPerFrame = (bitsPerSample * numChannels) >> 3;
	out->streamCount = streamCount;

	for (i = 0; i < streamCount; i++)
		out->stream[i].volume = 256 / MODDEF_AUDIOOUT_VOLUME_DIVIDER;

#if defined(__APPLE__)
	out->runLoop = CFRunLoopGetCurrent();
#endif
}

void xs_audioout_build(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);

#if defined(__APPLE__)
	OSStatus err;
	AudioStreamBasicDescription desc = {0};
	int i;

	desc.mBitsPerChannel = out->bitsPerSample;
	desc.mBytesPerFrame = out->bytesPerFrame;
	desc.mBytesPerPacket = desc.mBytesPerFrame;
	desc.mChannelsPerFrame = out->numChannels;
#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 8
	desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
#elif MODDEF_AUDIOOUT_BITSPERSAMPLE == 16
	desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
#endif
	desc.mFormatID = kAudioFormatLinearPCM;
	desc.mFramesPerPacket = 1;
	desc.mSampleRate = out->sampleRate;

	pthread_mutex_init(&out->mutex, NULL);

	err = AudioQueueNewOutput(&desc, audioQueueCallback, out, NULL, NULL, 0, &out->audioQueue);
	if (noErr != err)
		xsUnknownError("can't create output");

	// 2 buffers, 1/32 of a second each
	for (i = 0; i < kAudioQueueBufferCount; i++)
		AudioQueueAllocateBuffer(out->audioQueue, (((out->bitsPerSample * out->numChannels) >> 3) * out->sampleRate) >> 5, &out->buffer[i]);
#elif defined(_WIN32)
	LPDIRECTSOUND8 lpDirectSound = NULL;
	WNDCLASSEX wcex = {0};
	HRESULT hr;

	out->state = kStateIdle;

	InitializeCriticalSection(&out->cs);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = modAudioWindowProc;
	wcex.lpszClassName = "modAudioWindowClass";
	RegisterClassEx(&wcex);
	out->hWnd = CreateWindowEx(0, "modAudioWindowClass", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

	hr = DirectSoundCreate8(NULL, &lpDirectSound, NULL);
	if (FAILED(hr))
		xsUnknownError("can't create output");

	hr = IDirectSound8_SetCooperativeLevel(lpDirectSound, out->hWnd, DSSCL_PRIORITY);
	if (FAILED(hr))
		xsUnknownError("can't create output");

	out->dS = lpDirectSound;

	// 2 buffers, 1/8 of a second each
	hr = createDirectSoundBuffer(out, (out->sampleRate * out->bytesPerFrame) >> 3, kAudioQueueBufferCount);
	if (FAILED(hr))
		xsUnknownError("can't create output");
#elif ESP32
	out->state = kStateIdle;
	out->mutex = xSemaphoreCreateMutex();

	xTaskCreate(audioOutLoop, "audioOut", 1536 + XT_STACK_EXTRA_CLIB, out, 10, &out->task);
#if MODDEF_AUDIOOUT_I2S_DAC
	out->buffer8 = malloc((sizeof(out->buffer) / sizeof(uint16_t)) * sizeof(uint8_t));
	if (!out->buffer8)
		xsUnknownError("out of memory");
#endif
#elif PICO_BUILD
	out->format.format = AUDIO_BUFFER_FORMAT_PCM_S16;
	out->format.sample_freq = out->sampleRate;
	out->format.channel_count = out->numChannels;
	out->producer_format.format = &out->format;
	out->producer_format.sample_stride = 2;

	out->producer_pool = audio_new_producer_pool(&out->producer_format, NUM_SAMPLES_BUFFERS, SAMPLES_PER_BUFFER);

	out->config.data_pin = MODDEF_AUDIOOUT_I2S_DATAOUT_PIN;
	out->config.clock_pin_base = MODDEF_AUDIOOUT_I2S_BCK_PIN;		// uses this pin for BCK and pin + 1 for LR
	out->config.dma_channel = 3;
	out->config.pio_sm = 1;

	const struct audio_format *output_format;
	output_format = audio_i2s_setup(&out->format, &out->config);
	if (!output_format) {
		modLog("can't open audio device");
		return;
	}

	audio_i2s_connect(out->producer_pool);

	out->ap = out->producer_pool;
	out->state = kStateIdle;

#endif

	xsRemember(out->obj);

	out->built = 1;
}

#define upUseCount(out) (out)->useCount += 1

static void downUseCount(modAudioOut out)
{
	xsMachine *the = out->the;

	out->useCount -= 1;
	if (out->useCount > 0)
		return;

	if (out->built)
		xsForget(out->obj);

	xs_audioout_destructor(out);
}

void xs_audioout_close(xsMachine *the)
{
	modAudioOut out = xsmcGetHostData(xsThis);
	if (out && xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);

#if ESP32
		out->state = kStateClosing;
		if (out->task) {
			xTaskNotify(out->task, kStateClosing, eSetValueWithOverwrite);
			while (kStateClosing == out->state)
				modDelayMilliseconds(1);

			vSemaphoreDelete(out->mutex);
		}
#endif

		downUseCount(out);
	}
}

void xs_audioout_start(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);

#if MODDEF_AUDIOOUT_I2S_PDM
	out->prevSample = 0;
	out->error = 0;
#endif

#if defined(__APPLE__)
	for (int i = 0; i < kAudioQueueBufferCount; i++)
		audioQueueCallback(out, out->audioQueue, out->buffer[i]);

	AudioQueueStart(out->audioQueue, NULL);
#elif defined(_WIN32)
	out->state = kStatePlaying;
	doRenderSamples(out);
	IDirectSoundBuffer_Play(out->dSBuffer, 0, 0, DSBPLAY_LOOPING);
#elif ESP32
	out->state = kStatePlaying;
	xTaskNotify(out->task, kStatePlaying, eSetValueWithOverwrite);
#elif defined(__ets__)
	#if MODDEF_AUDIOOUT_I2S_PDM
//		i2s_begin(doRenderSamples, out, out->sampleRate * (MODDEF_AUDIOOUT_I2S_PDM >> 5));
		#error
	#else
//		i2s_begin(doRenderSamples, out, out->sampleRate);
		#error
	#endif
	out->i2sActive = true;
#elif PICO_BUILD
	out->state = kStatePlaying;
	if (!out->timer) {
		out->timer = modTimerAdd(1, 20, picoAudioCallback, &out, 4);
	}
#endif
}

void xs_audioout_stop(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);

#if defined(__APPLE__)
	AudioQueueStop(out->audioQueue, true);
#elif defined(_WIN32)
	out->state = kStateIdle;
	IDirectSoundBuffer_Stop(out->dSBuffer);
#elif ESP32
	out->state = kStateIdle;
	xTaskNotify(out->task, kStateIdle, eSetValueWithOverwrite);
#elif defined(__ets__)
	i2s_end();
	out->i2sActive = false;
#elif PICO_BUILD
	out->state = kStateIdle;
#endif
}

enum {
	kKindSamples = 1,
	kKindFlush = 2,
	kKindCallback = 3,
	kKindVolume = 4,
	kKindRawSamples = 5,
	kKindTone = 6,
	kKindSilence = 7,
};

enum {
	kSampleFormatUncompressed = 0,
	kSampleFormatIMA = 1,
	kSampleFormatSBC = 2,
	kSampleFormatTone = 3,
	kSampleFormatSilence = 4
};

void xs_audioout_enqueue(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	int streamIndex, argc = xsmcArgc;
	int repeat = 1, sampleOffset = 0, samplesToUse = -1, volume, i, samplesInBuffer;
	uint8_t kind;
	uint8_t *buffer;
	xsUnsignedValue bufferLength;
	uint16_t sampleRate;
	uint8_t numChannels;
	uint8_t bitsPerSample;
	uint8_t sampleFormat = kSampleFormatUncompressed;
	modAudioOutStream stream;
	modAudioQueueElement element;
	int activeStreamCount = out->activeStreamCount;

	xsmcVars(1);

	streamIndex = xsmcToInteger(xsArg(0));
	if ((streamIndex < 0) || (streamIndex >= out->streamCount))
		xsRangeError("invalid stream");
	stream = &out->stream[streamIndex];

	kind = xsmcToInteger(xsArg(1));
	if (kKindFlush != kind) {
		if (MODDEF_AUDIOOUT_QUEUELENGTH == stream->elementCount)
			xsUnknownError("queue full");
	}

	switch (kind) {
		case kKindSamples:
		case kKindRawSamples:
			if (argc > 3) {
				if ((xsNumberType == xsmcTypeOf(xsArg(3))) && (C_INFINITY == xsmcToNumber(xsArg(3))))
					repeat = -1;
				else {
					repeat = xsmcToInteger(xsArg(3));
					if (repeat <= 0)
						xsUnknownError("invalid repeat");
				}
				if (argc > 4) {
					sampleOffset = xsmcToInteger(xsArg(4));
					if (argc > 5)
						samplesToUse = xsmcToInteger(xsArg(5));
				}
			}

			if (xsBufferNonrelocatable != xsmcGetBufferReadable(xsArg(2), (void **)&buffer, &bufferLength))
				xsUnknownError("non-relocatable block required");

			if (kKindSamples == kind) {
				if (('m' != c_read8(buffer + 0)) || ('a' != c_read8(buffer + 1)) || (1 != c_read8(buffer + 2)))
					xsUnknownError("bad header");

				bitsPerSample = c_read8(buffer + 3);
				sampleRate = c_read16(buffer + 4);
				numChannels = c_read8(buffer + 6);
				sampleFormat = c_read8(buffer + 7);
				int bufferSamples = c_read32(buffer + 8);
				if ((kSampleFormatUncompressed == sampleFormat) && (bitsPerSample != out->bitsPerSample))
					xsUnknownError("format doesn't match output");
				if ((sampleRate != out->sampleRate) || (numChannels != out->numChannels))
					xsUnknownError("rate/channels doesn't match output");
				if ((kSampleFormatUncompressed != sampleFormat) && (kSampleFormatIMA != sampleFormat) && (kSampleFormatSBC != sampleFormat))
					xsUnknownError("unsupported compression");
				
				if ((kSampleFormatSBC == sampleFormat) && (sampleOffset || (-1 != samplesToUse))) 
					xsUnknownError("no offset/use for SBC");

				buffer += 12;
				bufferLength -= 12;

				if (sampleOffset >= bufferSamples)
					xsUnknownError("invalid offset");

				if ((samplesToUse < 0) || ((sampleOffset + samplesToUse) > bufferSamples))
					samplesToUse = bufferSamples - sampleOffset;
			}
			else {
				int bufferSamples = bufferLength / out->bytesPerFrame;
				if ((samplesToUse < 0) || ((sampleOffset + samplesToUse) > bufferSamples))
					samplesToUse = bufferSamples - sampleOffset;

				sampleFormat = kSampleFormatUncompressed;
			}

			if (kSampleFormatUncompressed == sampleFormat) {
				samplesInBuffer = bufferLength / out->bytesPerFrame;
				if (sampleOffset >= samplesInBuffer)
					xsUnknownError("buffer too small");
				samplesInBuffer -= sampleOffset;
			}
			else if (kSampleFormatIMA == sampleFormat) {
				samplesInBuffer = (bufferLength / kIMABytesPerChunk) * kIMASamplesPerChunk;
				if (sampleOffset >= samplesInBuffer)
					xsUnknownError("buffer too small");
				samplesInBuffer -= sampleOffset;
			}
			else if (kSampleFormatSBC == sampleFormat)
				;
			else
				xsUnknownError("unhandled compression");

			if (kSampleFormatSBC != sampleFormat) {
				if (samplesToUse > samplesInBuffer)
					xsUnknownError("buffer too small");
			}

			doLock(out);

			element = &stream->element[stream->elementCount];
			element->position = 0;
			element->repeat = repeat;
			element->sampleFormat = sampleFormat;
			if (kSampleFormatUncompressed == sampleFormat) {
				element->samples = buffer + (sampleOffset * out->bytesPerFrame);
				element->sampleCount = samplesToUse;
			}
			else if (kSampleFormatIMA == sampleFormat) {
				if (NULL == stream->decompressed) {
					stream->decompressed = c_malloc(kDecompressBufferSize);
					if (NULL == stream->decompressed)
						xsUnknownError("out of memory");
				}
				element->samples = stream->decompressed;
				element->sampleCount = kIMASamplesPerChunk;
				// calculations quantize to chunk boundaries
				element->compressed.initial = buffer + ((sampleOffset / kIMASamplesPerChunk) * kIMABytesPerChunk);
				element->compressed.data = element->compressed.initial;
				element->compressed.total = samplesToUse / kIMASamplesPerChunk;
				element->compressed.remaining = element->compressed.total;
			}
			else if (kSampleFormatSBC == sampleFormat) {
				if (NULL == stream->decompressor) {
					stream->decompressor = c_malloc(sizeof(SBC_Decode));
					if (NULL == stream->decompressor)
						xsUnknownError("out of memory");
					sbc_init((SBC_Decode *)stream->decompressor);
				}

				if (NULL == stream->decompressed) {
					stream->decompressed = c_malloc(kDecompressBufferSize);
					if (NULL == stream->decompressed)
						xsUnknownError("out of memory");
				}
				element->samples = stream->decompressed;
				element->sampleCount = kSBCSamplesPerChunk;
				element->compressed.initial = buffer;
				element->compressed.data = element->compressed.initial;
				element->compressed.total = bufferLength;
				element->compressed.remaining = element->compressed.total;
			}
			else
				xsUnknownError("invalid format");


		enqueueSamples:
			// stop infinite element
			for (i = stream->elementCount - 1; i >= 0; i--) {
				if (0 == stream->element[i].repeat)
					continue;

				if (stream->element[i].repeat < 0) {
					if (kSampleFormatTone == element->sampleFormat)
						element[-1].tone.count = 0;;
					stream->element[i].repeat = 1;
				}
				break;
			}

		enqueue:
			stream->elementCount += 1;

			if (1 == stream->elementCount) {
				if (kSampleFormatUncompressed != sampleFormat)
					streamDecompressNext(stream);
				updateActiveStreams(out);
			}

			doUnlock(out);
			break;

		case kKindTone:
		case kKindSilence: {
			xsNumberValue frequency;
			int count;
			OUTPUTSAMPLETYPE value;

			if (kKindTone == kind) {
				frequency = xsmcToNumber(xsArg(2));
				count = -1;
				if ((argc > 3) && (C_INFINITY != xsmcToNumber(xsArg(3))))
					count = xsmcToInteger(xsArg(3));
				if (frequency < 10)
					xsUnknownError("invalid frequency");
				if (argc > 4)
					value = xsmcToInteger(xsArg(4));
				else {
#if 8 == MODDEF_AUDIOOUT_BITSPERSAMPLE
					value = 127 >> 2;
#else
					value = 32767 >> 2;
#endif
				}
				sampleFormat = kSampleFormatTone;
			}
			else {
				count = xsmcToInteger(xsArg(2));
				if (0 == count)
					xsUnknownError("invalid count");
				frequency = 1;
				value = 0;
				sampleFormat = kSampleFormatSilence;

			}

			if (NULL == stream->decompressed) {
				stream->decompressed = c_malloc(kDecompressBufferSize);
				if (NULL == stream->decompressed)
					xsUnknownError("out of memory");
			}

			doLock(out);

			element = &stream->element[stream->elementCount];
			element->position = 0;
			element->sampleFormat = sampleFormat;
			element->samples = stream->decompressed;
			element->repeat = (count < 0) ? -1 : 1;

			if (kSampleFormatTone == sampleFormat) {
				element->sampleCount = kToneSamplesPerChunk;

				element->tone.position = 0;
				element->tone.max = (uint32_t)(32768.0 * (xsNumberValue)out->sampleRate / frequency);		// 16.16 fixed
				element->tone.value = value;
				element->tone.count = count;
			}
			else {
				element->sampleCount = 0;
				element->silence.remaining = count;
			}

			goto enqueueSamples;
		}

		case kKindFlush: {
			int elementCount, i;
			modAudioQueueElement element;

			doLock(out);

			elementCount = stream->elementCount;
			stream->elementCount = 0;		// flush queue
			updateActiveStreams(out);

			for (i = 0, element = stream->element; i < elementCount; i++, element++) {
				if ((0 == element->repeat) && (element->position < 0) && (0 == element->sampleCount))
					queueCallback(out, (xsIntegerValue)(uintptr_t)element->samples, streamIndex);
			}

			if (stream->decompressed)
				c_free(stream->decompressed);
			stream->decompressed = NULL;
			if (stream->decompressor)
				c_free(stream->decompressor);
			stream->decompressed = NULL;
			stream->decompressor = NULL;

			doUnlock(out);

#if defined(__APPLE__)
			invokeCallbacks(NULL, out);
#elif PICO_BUILD || ESP || defined(__ets__) || defined(_WIN32)
			deliverCallbacks(the, out, NULL, 0);
#endif
		} break;

		case kKindCallback:
			doLock(out);

			element = &stream->element[stream->elementCount];
			element->samples = (void *)(uintptr_t)xsmcToInteger(xsArg(2));
			element->sampleCount = 0;
			element->position = -1;
			element->repeat = 0;		//@@
			goto enqueue;

		case kKindVolume:
			volume = xsmcToInteger(xsArg(2));
			if (0 == stream->elementCount) {
				setStreamVolume(out, stream, volume);
				break;
			}

			doLock(out);

			element = &stream->element[stream->elementCount];
			element->samples = NULL;
			element->sampleCount = 0;
			element->position = volume;
			element->repeat = 0;		//@@
			goto enqueue;

		default:
			xsUnknownError("bad kind");
	}

	if (out->activeStreamCount != activeStreamCount) {
		uint8_t started = 0 == activeStreamCount;
		if (started) {
#if ESP32
			if (out->task)
				xTaskNotify(out->task, kStatePlaying, eSetValueWithOverwrite);
#endif
		}
	}

	xsResult = xsThis;
}

void xs_audioout_mix(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	int samplesNeeded;
	void *result;

	if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsUnsignedValue bytesAvailable;
		xsmcGetBufferReadable(xsArg(0), (void **)&result, &bytesAvailable);
		samplesNeeded = bytesAvailable / out->bytesPerFrame;
	}
	else {
		samplesNeeded = xsmcToInteger(xsArg(0));
		if (samplesNeeded < 0)
			xsRangeError("invalid count");
		
		int bytesNeeded = samplesNeeded * out->bytesPerFrame;
		result = c_malloc(bytesNeeded);
		if (!result)
			xsUnknownError("no memory");

		xsResult = xsNewHostObject(c_free);
		xsmcSetHostBuffer(xsResult, result, bytesNeeded);

		xsmcVars(1);
		xsmcSetInteger(xsVar(0), bytesNeeded);
		xsmcDefine(xsResult, xsID_byteLength, xsVar(0), xsDefault);
	}

	audioMix(out, samplesNeeded, result);
}

void xs_audioout_length(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	int streamIndex = xsmcToInteger(xsArg(0));
	if ((streamIndex < 0) || (streamIndex >= out->streamCount))
		xsRangeError("invalid stream");

	xsmcSetInteger(xsResult, MODDEF_AUDIOOUT_QUEUELENGTH - out->stream[streamIndex].elementCount);
}

void xs_audioout_get_sampleRate(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetInteger(xsResult, out->sampleRate);
}

void xs_audioout_get_numChannels(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetInteger(xsResult, out->numChannels);
}

void xs_audioout_get_bitsPerSample(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetInteger(xsResult, out->bitsPerSample);
}

void xs_audioout_get_streams(xsMachine *the)
{
	modAudioOut out = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetInteger(xsResult, out->streamCount);
}


// note: updateActiveStreams relies on caller to lock mutex
void updateActiveStreams(modAudioOut out)
{
	int i;

	out->activeStreamCount = 0;
	out->applyVolume = false;
	for (i = 0; i < out->streamCount; i++) {
		modAudioOutStream stream = &out->stream[i];

		if (0 == stream->elementCount)
			continue;

		out->activeStream[out->activeStreamCount++] = stream;
		if (stream->volume != 256)
			out->applyVolume = true;
	}
}

#if defined(__APPLE__)
void audioQueueCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef buffer)
{
	modAudioOut out = inUserData;
	int samplesToGenerate = buffer->mAudioDataBytesCapacity / out->bytesPerFrame;
	OUTPUTSAMPLETYPE *output = (OUTPUTSAMPLETYPE *)buffer->mAudioData;

	buffer->mAudioDataByteSize = samplesToGenerate * out->bytesPerFrame;

	pthread_mutex_lock(&out->mutex);

	audioMix(out, samplesToGenerate, output);

	pthread_mutex_unlock(&out->mutex);

	AudioQueueEnqueueBuffer(out->audioQueue, buffer, 0, NULL);
}

void invokeCallbacks(CFRunLoopTimerRef timer, void *info)
{
	modAudioOut out = info;

	out->callbackTimer = NULL;

	xsBeginHost(out->the);
	xsmcVars(1);

	upUseCount(out);

	while (out->pendingCallbackCount) {
		xsIntegerValue id, stream;

		pthread_mutex_lock(&out->mutex);
		id = out->pendingCallbacks[0].id;
		stream = out->pendingCallbacks[0].stream;

		out->pendingCallbackCount -= 1;
		if (out->pendingCallbackCount)
			c_memmove(out->pendingCallbacks, out->pendingCallbacks + 1, out->pendingCallbackCount * sizeof(modAudioOutPendingCallbackRecord));
		pthread_mutex_unlock(&out->mutex);

		xsmcSetInteger(xsVar(0), id);
		xsTry {
			xsmcGet(xsResult, out->obj, xsID_callbacks);
			if (xsUndefinedType != xsmcTypeOf(xsResult)) {
				xsmcGetIndex(xsResult, xsResult, stream);
				if (xsmcIsCallable(xsResult))
					xsCallFunction1(xsResult, out->obj, xsVar(0));
			}
			else
				xsCall1(out->obj, xsID_callback, xsVar(0));
		}
		xsCatch {
		}
	}

	xsEndHost(out->the);

	downUseCount(out);
}

// note: queueCallback relies on caller to lock mutex
void queueCallback(modAudioOut out, xsIntegerValue id, xsIntegerValue stream)
{
	if (out->pendingCallbackCount < MODDEF_AUDIOOUT_QUEUELENGTH) {
		out->pendingCallbacks[out->pendingCallbackCount].id = id;
		out->pendingCallbacks[out->pendingCallbackCount++].stream = stream;

		if (1 == out->pendingCallbackCount) {
			CFRunLoopTimerContext context = {0};
			context.info = out;
			out->callbackTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(), 0, 0, 0, invokeCallbacks, &context);
			CFRunLoopAddTimer(out->runLoop, out->callbackTimer, kCFRunLoopCommonModes);
		}
	}
	else
		printf("audio callback queue full\n");
}

#elif defined(_WIN32)

HRESULT createDirectSoundBuffer(modAudioOut out, uint32_t bufferSize, uint32_t bufferCount)
{
	HRESULT hr;
	WAVEFORMATEX format;
	DSBUFFERDESC dsbdsc = { 0 };
	HANDLE hNotifyEvent = NULL, hEndThread = NULL;
	LPDIRECTSOUNDBUFFER8 lpDirectSoundBuffer = NULL;
	LPDSBPOSITIONNOTIFY aPosNotify = NULL;
	LPDIRECTSOUNDNOTIFY pDSNotify = NULL;
	LPDIRECTSOUNDBUFFER lpDsb = NULL;

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.wBitsPerSample = out->bitsPerSample;
	format.nChannels = out->numChannels;
	format.nSamplesPerSec = out->sampleRate;
	format.nBlockAlign = format.wBitsPerSample / 8 * out->numChannels;
	format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
	format.cbSize = 0;

	hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEndThread = CreateEvent(NULL, FALSE, FALSE, NULL);

	aPosNotify = c_malloc(sizeof(DSBPOSITIONNOTIFY) * bufferCount);
	if (!aPosNotify) {
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto bail;
	}

	out->buffer = c_malloc(bufferSize * bufferCount);
	if (!out->buffer) {
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto bail;
	}

	for (DWORD i = 0; i < bufferCount; i++) {
		aPosNotify[i].dwOffset = ((DWORD)bufferSize * i) + (DWORD)bufferSize - 1;
		aPosNotify[i].hEventNotify = hNotifyEvent;
	}

	dsbdsc.dwSize = sizeof(dsbdsc);
	dsbdsc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
	dsbdsc.dwBufferBytes = bufferSize * bufferCount;
	dsbdsc.lpwfxFormat = &format;

	hr = IDirectSound8_CreateSoundBuffer(out->dS, &dsbdsc, &lpDsb, NULL);
	if (FAILED(hr)) goto bail;

	hr = IDirectSound8_QueryInterface(lpDsb, &IID_IDirectSoundBuffer8, (LPVOID *)&lpDirectSoundBuffer);
	if (FAILED(hr)) goto bail;

	hr = IDirectSoundBuffer_QueryInterface(lpDsb, &IID_IDirectSoundNotify, (LPVOID *)&pDSNotify);
	if (FAILED(hr)) goto bail;

	hr = IDirectSoundBuffer_SetCurrentPosition(lpDirectSoundBuffer, 0);
	if (FAILED(hr)) goto bail;

	hr = IDirectSoundNotify_SetNotificationPositions(pDSNotify, bufferCount, aPosNotify);
	if (FAILED(hr)) goto bail;

	out->dSBuffer = lpDirectSoundBuffer;
	out->hNotify = hNotifyEvent;
	out->hEndThread = hEndThread;
	out->bufferSize = bufferSize * bufferCount;
	out->bufferCount = bufferCount;
	out->samplesNeeded = out->bufferSize / out->bytesPerFrame;
	out->bytesWritten = 0;

	out->hNotifyThread = CreateThread(NULL, 0, directSoundProc, out, 0, NULL);
	if (NULL == out->hNotifyThread) {
		hr = ERROR_MAX_THRDS_REACHED;
		goto bail;
	}

bail:
	if (FAILED(hr)) {
		if (lpDirectSoundBuffer)
			IDirectSoundBuffer_Release(lpDirectSoundBuffer);

		if (hEndThread)
			CloseHandle(hEndThread);

		if (hNotifyEvent)
			CloseHandle(hNotifyEvent);

		if (out->hNotifyThread) {
			SetEvent(out->hNotifyThread);
			WaitForSingleObject(out->hNotifyThread, INFINITE);
			out->hNotifyThread = NULL;
		}

		if (out->buffer) {
			c_free(out->buffer);
			out->buffer = NULL;
		}
	}

	if (pDSNotify)
		IDirectSoundNotify_Release(pDSNotify);

	if (lpDsb)
		IDirectSoundBuffer_Release(lpDsb);

	if (aPosNotify)
		c_free(aPosNotify);

	return hr;
}

void releaseDirectSoundBuffer(modAudioOut out)
{
	if (NULL != out->dSBuffer)
		IDirectSoundBuffer_Stop(out->dSBuffer);

	if (out->hEndThread)
		SetEvent(out->hEndThread);

	if (out->hNotifyThread) {
		WaitForSingleObject(out->hNotifyThread, INFINITE);
		CloseHandle(out->hNotifyThread);
		out->hNotifyThread = NULL;
	}

	if (NULL != out->dSBuffer) {
		IDirectSoundBuffer_Release(out->dSBuffer);
		out->dSBuffer = NULL;
	}

	if (NULL != out->hNotify) {
		CloseHandle(out->hNotify);
		out->hNotify = NULL;
	}

	if (NULL != out->hEndThread) {
		CloseHandle(out->hEndThread);
		out->hEndThread = NULL;
	}

	if (NULL != out->buffer) {
		c_free(out->buffer);
		out->buffer = NULL;
	}
}

void doRenderSamples(modAudioOut out)
{
	LPBYTE p1, p2;
	DWORD b1, b2;
	HRESULT hr;
	DWORD bytesWritten = out->samplesNeeded * out->bytesPerFrame;

	EnterCriticalSection(&out->cs);

	audioMix(out, out->samplesNeeded, (OUTPUTSAMPLETYPE*)out->buffer);

	hr = IDirectSoundBuffer_Lock(out->dSBuffer, (DWORD)(out->bytesWritten % out->bufferSize), bytesWritten,
		(LPVOID *)&p1, &b1, (LPVOID *)&p2, &b2, 0);
	if (FAILED(hr))
		goto bail;

	if (NULL != p1) {
		c_memcpy(p1, out->buffer, b1);
		if (NULL != p2)
			c_memcpy(p2, b1 + out->buffer, b2);
		out->bytesWritten += bytesWritten;
		out->samplesNeeded = 0;
	}

	IDirectSoundBuffer_Unlock(out->dSBuffer, p1, b1, p2, b2);

bail:
	LeaveCriticalSection(&out->cs);
}

LRESULT CALLBACK modAudioWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_CALLBACK: {
			modAudioOut out = (modAudioOut)lParam;
			switch (wParam) {
				case 0:
					deliverCallbacks(out->the, out, NULL, 0);
					break;
				case 1:
					doRenderSamples(out);
					break;
				default:
					break;
			}
			return 0;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

DWORD WINAPI directSoundProc(LPVOID lpParameter)
{
	HANDLE handles[2];
	modAudioOut out = (modAudioOut)lpParameter;
	HRESULT hr;
	DWORD playCursor;

	handles[0] = out->hEndThread;
	handles[1] = out->hNotify;
	while (true) {
		DWORD waitObject = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (WAIT_OBJECT_0 == waitObject)
			break;

		EnterCriticalSection(&out->cs);

		if (kStatePlaying != out->state) {
			LeaveCriticalSection(&out->cs);
			continue;
		}

		hr = IDirectSoundBuffer_GetCurrentPosition(out->dSBuffer, &playCursor, NULL);
		if (SUCCEEDED(hr)) {
			int32_t unplayed = (int32_t)((out->bytesWritten % out->bufferSize) - playCursor);
			if (unplayed < 0)
				unplayed += out->bufferSize;
			out->samplesNeeded = ((out->bufferSize - unplayed) / out->bytesPerFrame);
		}

		LeaveCriticalSection(&out->cs);

		PostMessage(out->hWnd, WM_CALLBACK, 1, (LPARAM)out);
	}

	return 1;
}

#elif ESP32
void audioOutLoop(void *pvParameter)
{
	modAudioOut out = pvParameter;
	uint8_t installed = false, stopped = true;

#if MODDEF_AUDIOOUT_I2S_PDM
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    tx_chan_cfg.auto_clear = true;

	i2s_pdm_tx_config_t tx_cfg = {
		.clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(out->sampleRate),
		.slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
		.gpio_cfg = {
			.clk = -1,
			.dout = MODDEF_AUDIOOUT_I2S_PDM_PIN,
			.invert_flags = {
				.clk_inv = false,
			},
		},
	};
	i2s_new_channel(&tx_chan_cfg, &out->tx_handle, NULL);
	i2s_channel_init_pdm_tx_mode(out->tx_handle, &tx_cfg);

#elif !MODDEF_AUDIOOUT_I2S_DAC
	// I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, i2s_role)
	i2s_chan_config_t chan_cfg = {
		.id = MODDEF_AUDIOOUT_I2S_NUM,
		.role = I2S_ROLE_MASTER,
		.dma_desc_num = 2,
		.dma_frame_num = sizeof(out->buffer) / out->bytesPerFrame,
		.auto_clear = true,		// This is different from I2S_CHANNEL_DEFAULT_CONFIG
	};
	i2s_new_channel(&chan_cfg, &out->tx_handle, NULL);

	i2s_std_config_t i2s_config = {
		.gpio_cfg = {
			.mclk = I2S_GPIO_UNUSED,
			.bclk = MODDEF_AUDIOOUT_I2S_BCK_PIN,
			.ws = MODDEF_AUDIOOUT_I2S_LR_PIN,
			.dout = MODDEF_AUDIOOUT_I2S_DATAOUT_PIN,
			.din = I2S_GPIO_UNUSED,
			.invert_flags = {
				.mclk_inv = false,
				.bclk_inv = false,
				.ws_inv = false,
			},
		},
	};

	// I2S_STD_CLK_DEFAULT_CONFIG(sampleRate) (i2s_std.h)
	i2s_config.clk_cfg.sample_rate_hz = out->sampleRate;
	i2s_config.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
	i2s_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

	// I2S_STD_MSB_SLOT_DEFAULT_CONFIG(bitwidth, mode) (i2s_std.h)
	int msb_right = true;
#if MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE == 32
	i2s_config.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
	i2s_config.slot_cfg.ws_width = I2S_DATA_BIT_WIDTH_32BIT;
	msb_right = false;
#elif MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE == 16
	i2s_config.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
	i2s_config.slot_cfg.ws_width = I2S_DATA_BIT_WIDTH_16BIT;
#else
	i2s_config.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_8BIT;
	i2s_config.slot_cfg.ws_width = I2S_DATA_BIT_WIDTH_8BIT;
#endif
	i2s_config.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
#if SOC_I2S_HW_VERSION_1    // For esp32/esp32-s2
	i2s_config.slot_cfg.msb_right = msb_right;
#else
	i2s_config.slot_cfg.left_align = false;
	i2s_config.slot_cfg.big_endian = false;
	i2s_config.slot_cfg.bit_order_lsb = false;
#endif

#if MODDEF_AUDIOOUT_NUMCHANNELS == 2
	i2s_config.slot_cfg.slot_mode = I2S_SLOT_MODE_STEREO;
	i2s_config.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
#else
	i2s_config.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;
	i2s_config.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
#endif
	i2s_config.slot_cfg.ws_pol = false;
	i2s_config.slot_cfg.bit_shift = false;

	i2s_channel_init_std_mode(out->tx_handle, &i2s_config);
	i2s_channel_reconfig_std_slot(out->tx_handle, &i2s_config.slot_cfg);
	i2s_channel_reconfig_std_clock(out->tx_handle, &i2s_config.clk_cfg);

#else /* MODDEF_AUDIOOUT_I2S_DAC */
	dac_continuous_config_t cont_cfg = {
#if MODDEF_AUDIOOUT_I2S_DAC_CHANNEL == 3
		.chan_mask = DAC_CHANNEL_MASK_ALL,
#elif MODDEF_AUDIOOUT_I2S_DAC_CHANNEL == 1
		.chan_mask = DAC_CHANNEL_MASK_CH1,
#elif MODDEF_AUDIOOUT_I2S_DAC_CHANNEL == 2
		.chan_mask = DAC_CHANNEL_MASK_CH0,
#else
		#error "invalid dac channel"
#endif
		.chan_mask = DAC_CHANNEL_MASK_ALL,
		.desc_num = 4,
		.buf_size = 2048,
		.freq_hz = out->sampleRate,
		.offset = 0,
		.clk_src = DAC_DIGI_CLK_SRC_APLL,   // Using APLL as clock source to get a wider frequency range
		.chan_mode = DAC_CHANNEL_MODE_SIMUL,
	};
	dac_continuous_new_channels(&cont_cfg, &out->dacHandle);
#endif

	while (true) {
		size_t bytes_written;

		if ((kStatePlaying != out->state) || (0 == out->activeStreamCount)) {
			uint32_t newState;

			if (!stopped) {
#if MODDEF_AUDIOOUT_I2S_DAC
				dac_continuous_disable(out->dacHandle);
#else
				i2s_channel_disable(out->tx_handle);
#endif
				stopped = true;
			}

			if ((kStateClosing == out->state) || (kStateTerminated == out->state))
				break;

			xTaskNotifyWait(0, 0, &newState, portMAX_DELAY);
			continue;
		}

		if (!installed) {
#if MODDEF_AUDIOOUT_I2S_DAC
			dac_continuous_enable(out->dacHandle);
#else
			i2s_channel_enable(out->tx_handle);
#endif
			installed = true;
			stopped = false;
		}
		else if (stopped) {
			stopped = false;
#if MODDEF_AUDIOOUT_I2S_DAC
			dac_continuous_enable(out->dacHandle);
#else
			i2s_channel_enable(out->tx_handle);
#endif
		}

		xSemaphoreTake(out->mutex, portMAX_DELAY);
		audioMix(out, sizeof(out->buffer) / out->bytesPerFrame, (OUTPUTSAMPLETYPE *)out->buffer);
		xSemaphoreGive(out->mutex);

#if MODDEF_AUDIOOUT_I2S_DAC
		int count = sizeof(out->buffer) / out->bytesPerFrame;
		int i = count;
		uint16_t *src = (uint16_t *)out->buffer;
		uint8_t *dst = out->buffer8;

		while (i--) {
			uint16_t s = *src++ ^ 0x8000;
			*dst++ = s >> 8;
		}

		dac_continuous_write(out->dacHandle, out->buffer8, count, NULL, -1);		// -1 for portMAX_DELAY
#elif (16 == MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE) || (32 == MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE)
		i2s_channel_write(out->tx_handle, (const char *)out->buffer, sizeof(out->buffer), &bytes_written, portMAX_DELAY);
#else
	#error invalid MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
#endif
	}

#if MODDEF_AUDIOOUT_I2S_DAC
	if (out->dacHandle) {
		dac_continuous_disable(out->dacHandle);
		dac_continuous_del_channels(out->dacHandle);
		out->dacHandle = NULL;
	}
#else
	if (out->tx_handle) {
		i2s_channel_disable(out->tx_handle);
		i2s_del_channel(out->tx_handle);
		out->tx_handle = NULL;
	}
#endif

	out->state = kStateTerminated;
	out->task = NULL;

	vTaskDelete(NULL);	// "If it is necessary for a task to exit then have the task call vTaskDelete( NULL ) to ensure its exit is clean."
}

#elif defined(__ets__) 
void doRenderSamples(void *refcon, int16_t *lr, int count)
{
	modAudioOut out = refcon;
	OUTPUTSAMPLETYPE *s = (OUTPUTSAMPLETYPE *)out->buffer;

#if MODDEF_AUDIOOUT_I2S_PDM
	count /= (MODDEF_AUDIOOUT_I2S_PDM >> 5);
#endif

	audioMix(out, count, out->buffer);

#if 0 == MODDEF_AUDIOOUT_I2S_PDM
	// expand mono to stereo
	while (count--) {
		int16_t sample = *s++;
#if 8 == MODDEF_AUDIOOUT_BITSPERSAMPLE
		sample |= sample << 8;
#endif
		lr[0] = sample;
		lr[1] = sample;
		lr += 2;
	}
#else
	// expand mono to PDM
	#define FRACTIONAL_BITS (13)
	const int32_t SAMPLE_MAX = 0x7FFF << FRACTIONAL_BITS;
	int32_t *pdm = (int32_t *)lr;
	int32_t value = 0;		// doesn't need to be initialized, but compiler will complain if it isn't
	int32_t prevSample = out->prevSample;
	int32_t error = out->error;

	while (count--) {
		int16_t i, j;
		int32_t sample = *s++, step;

#if 8 == MODDEF_AUDIOOUT_BITSPERSAMPLE
		sample |= sample << 8;
#endif
		sample <<= FRACTIONAL_BITS;		// smear high bits to shifted in low bits?
		step = (sample - prevSample) >> (4 + (MODDEF_AUDIOOUT_I2S_PDM >> 5));
		prevSample = sample;

		for (j = MODDEF_AUDIOOUT_I2S_PDM >> 5; j; j--) {
			i = 31;
			do {
				if (error < 0) {
					value = (value << 1) | 1;
					error += SAMPLE_MAX - sample;
				}
				else {
					value <<= 1;
					error -= SAMPLE_MAX + sample;
				}
				sample += step;
			} while (--i);
			*pdm++ = value;
		}
    }
	out->prevSample = prevSample;
	out->error = error;
#endif
}

#elif PICO_BUILD
static void picoAudioCallback(modTimer timer, void *refcon, int refconSize)
{
	modAudioOut out = *(modAudioOut*)refcon;

	static int i2s_enabled = 0;

	if (!i2s_enabled) {
modLog(" - enable i2s ");
		audio_i2s_set_enabled(true);
		i2s_enabled = 1;
	}

	if (out->state == kStatePlaying) {
		int j;
		for (j=0; j<3; j++) {
			struct audio_buffer *buffer = take_audio_buffer(out->ap, false);
			if (!buffer) {
//				modLog("no buffer");
				break;
			}
			int16_t *samples = (int16_t*)buffer->buffer->bytes;
			audioMix(out, buffer->max_sample_count, samples);
			buffer->sample_count = buffer->max_sample_count;
			give_audio_buffer(out->ap, buffer);
		}
	}
	else if (out->state == kStateIdle) {
	}
	else if (out->state == kStateClosing) {
		modLog("audio closing");

		audio_i2s_set_enabled(false);
		i2s_enabled = 0;
		modTimerRemove(out->timer);
		out->timer = NULL;
		out->state = kStateTerminated;
	}

}
#endif

#if PICO_BUILD || defined(__ets__) || ESP32 || defined(_WIN32)

void deliverCallbacks(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modAudioOut out = refcon;

	xsBeginHost(out->the);
	xsmcVars(1);

	upUseCount(out);

	while (out->pendingCallbackCount) {
		xsIntegerValue id;
		xsIntegerValue stream;

		doLock(out);

		id = out->pendingCallbacks[0].id;
		stream = out->pendingCallbacks[0].stream;

		out->pendingCallbackCount -= 1;
		if (out->pendingCallbackCount)
			c_memmove(out->pendingCallbacks, out->pendingCallbacks + 1, out->pendingCallbackCount * sizeof(modAudioOutPendingCallbackRecord));

		doUnlock(out);

		xsmcSetInteger(xsVar(0), id);
		xsTry {
			xsmcGet(xsResult, out->obj, xsID_callbacks);
			if (xsUndefinedType != xsmcTypeOf(xsResult)) {
				xsmcGetIndex(xsResult, xsResult, stream);
				if (xsmcIsCallable(xsResult))
					xsCallFunction1(xsResult, out->obj, xsVar(0));
			}
			else
				xsCall1(out->obj, xsID_callback, xsVar(0));
		}
		xsCatch {
		}
	}

	xsEndHost(out->the);

	downUseCount(out);
}

// note: queueCallback relies on caller to lock mutex
void queueCallback(modAudioOut out, xsIntegerValue id, xsIntegerValue stream)
{
	if (out->pendingCallbackCount < MODDEF_AUDIOOUT_QUEUELENGTH) {
		out->pendingCallbacks[out->pendingCallbackCount].id = id;
		out->pendingCallbacks[out->pendingCallbackCount++].stream = stream;

		if (1 == out->pendingCallbackCount)
#if defined(_WIN32)
			PostMessage(out->hWnd, WM_CALLBACK, 0, (LPARAM)out);
#else
			modMessagePostToMachine(out->the, NULL, 0, deliverCallbacks, out);
#endif
	}
	else
		modLog("audio callback queue full");
}
#endif

void doLock(modAudioOut out)
{
	if (out->built) {
#if defined(__APPLE__)
		pthread_mutex_lock(&out->mutex);
#elif defined(_WIN32)
		EnterCriticalSection(&out->cs);
#elif ESP32
		xSemaphoreTake(out->mutex, portMAX_DELAY);
#elif PICO_BUILD || defined(__ets__)
		modCriticalSectionBegin();
#endif
	}
}

void doUnlock(modAudioOut out)
{
	if (out->built) {
#if defined(__APPLE__)
		pthread_mutex_unlock(&out->mutex);
#elif defined(_WIN32)
		LeaveCriticalSection(&out->cs);
#elif ESP32
		xSemaphoreGive(out->mutex);
#elif PICO_BUILD || defined(__ets__)
		modCriticalSectionEnd();
#endif
	}
}

#if defined(__ets__) && !ESP32
	#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 16
		#define readSample(x) ((int16_t)c_read16(x))
	#else
		#define readSample(x) ((int8_t)c_read8(x))
	#endif
#else
	#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 16
		#define readSample(x) (*(int16_t *)(x))
	#else
		#define readSample(x) (*(int8_t *)(x))
	#endif
#endif

void audioMix(modAudioOut out, int samplesToGenerate, OUTPUTSAMPLETYPE *output)
{
	uint8_t bytesPerFrame = out->bytesPerFrame;
	int i;

	for (i = 0; i < out->activeStreamCount; i++) {
		modAudioOutStream stream = out->activeStream[i];
		modAudioQueueElement element = stream->element;
		if ((0 == element->repeat) && (0 == element->sampleCount)) {		// volume or callback 
			endOfElement(out, stream);
			i = -1;		// active streams could have changed, start again 
		}
	}

	while (samplesToGenerate) {
		switch (out->activeStreamCount) {
			case 0:
				samplesToGenerate *= out->numChannels;
				while (samplesToGenerate--)
					*output++ = 0;
				samplesToGenerate = 0;
				break;

			case 1: {
				modAudioOutStream stream = out->activeStream[0];
				modAudioQueueElement element = stream->element;
				int use = element->sampleCount - element->position;
				if (use > samplesToGenerate)
					use = samplesToGenerate;

				OUTPUTSAMPLETYPE *s0 = (OUTPUTSAMPLETYPE *)((element->position * bytesPerFrame) + (uint8_t *)element->samples);
				if (!out->applyVolume) {
					c_memmove(output, s0, use * bytesPerFrame);
					output = (OUTPUTSAMPLETYPE *)((use * bytesPerFrame) + (uint8_t *)output);
				}
				else {
					uint16_t v0 = stream->volume;
					int count = use * out->numChannels;
					while (count--)
						*output++ = (readSample(s0++) * v0) >> 8;
				}

				samplesToGenerate -= use;
				element->position += use;

				if (element->position == element->sampleCount)
					endOfElement(out, stream);
				}
				break;

#if MODDEF_AUDIOOUT_STREAMS > 1
			case 2: {
				modAudioOutStream stream0 = out->activeStream[0];
				modAudioOutStream stream1 = out->activeStream[1];
				modAudioQueueElement element0 = stream0->element;
				modAudioQueueElement element1 = stream1->element;
				int use0 = element0->sampleCount - element0->position;
				int use1 = element1->sampleCount - element1->position;
				int use = (use0 < use1) ? use0 : use1;
				if (use > samplesToGenerate)
					use = samplesToGenerate;

				OUTPUTSAMPLETYPE *s0 = (OUTPUTSAMPLETYPE *)((element0->position * bytesPerFrame) + (uint8_t *)element0->samples);
				OUTPUTSAMPLETYPE *s1 = (OUTPUTSAMPLETYPE *)((element1->position * bytesPerFrame) + (uint8_t *)element1->samples);
				int count = use * out->numChannels;
				if (!out->applyVolume) {
					while (count--) {
						MIXSAMPLETYPE s = readSample(s0++) + readSample(s1++);
						*output++ = ClampSample(s, 2);
					}
				}
				else {
					uint16_t v0 = stream0->volume;
					uint16_t v1 = stream1->volume;
					while (count--) {
						MIXSAMPLETYPE s = ((readSample(s0++) * v0) + (readSample(s1++) * v1)) >> 8;
						*output++ = ClampSample(s, 2);
					}
				}

				samplesToGenerate -= use;
				element0->position += use;
				element1->position += use;

				if (element0->position == element0->sampleCount)
					endOfElement(out, stream0);
				if (element1->position == element1->sampleCount)
					endOfElement(out, stream1);
				}
				break;
#endif

#if MODDEF_AUDIOOUT_STREAMS > 2
			case 3: {
				modAudioOutStream stream0 = out->activeStream[0];
				modAudioOutStream stream1 = out->activeStream[1];
				modAudioOutStream stream2 = out->activeStream[2];
				modAudioQueueElement element0 = stream0->element;
				modAudioQueueElement element1 = stream1->element;
				modAudioQueueElement element2 = stream2->element;
				int use0 = element0->sampleCount - element0->position;
				int use1 = element1->sampleCount - element1->position;
				int use2 = element2->sampleCount - element2->position;
				int use = (use0 < use1) ? use0 : use1;
				if (use > use2) use = use2;
				if (use > samplesToGenerate)
					use = samplesToGenerate;

				OUTPUTSAMPLETYPE *s0 = (OUTPUTSAMPLETYPE *)((element0->position * bytesPerFrame) + (uint8_t *)element0->samples);
				OUTPUTSAMPLETYPE *s1 = (OUTPUTSAMPLETYPE *)((element1->position * bytesPerFrame) + (uint8_t *)element1->samples);
				OUTPUTSAMPLETYPE *s2 = (OUTPUTSAMPLETYPE *)((element2->position * bytesPerFrame) + (uint8_t *)element2->samples);
				int count = use * out->numChannels;
				if (!out->applyVolume) {
					while (count--) {
						MIXSAMPLETYPE s = readSample(s0++) + readSample(s1++) + readSample(s2++);
						*output++ = ClampSample(s, 3);
					}
				}
				else {
					uint16_t v0 = stream0->volume;
					uint16_t v1 = stream1->volume;
					uint16_t v2 = stream2->volume;
					while (count--) {
						MIXSAMPLETYPE s = ((readSample(s0++) * v0) + (readSample(s1++) * v1) + (readSample(s2++) * v2)) >> 8;
						*output++ = ClampSample(s, 3);
					}
				}

				samplesToGenerate -= use;
				element0->position += use;
				element1->position += use;
				element2->position += use;

				if (element0->position == element0->sampleCount)
					endOfElement(out, stream0);
				if (element1->position == element1->sampleCount)
					endOfElement(out, stream1);
				if (element2->position == element2->sampleCount)
					endOfElement(out, stream2);
				}
				break;
#endif

#if MODDEF_AUDIOOUT_STREAMS > 3
			case 4: {
				modAudioOutStream stream0 = out->activeStream[0];
				modAudioOutStream stream1 = out->activeStream[1];
				modAudioOutStream stream2 = out->activeStream[2];
				modAudioOutStream stream3 = out->activeStream[3];
				modAudioQueueElement element0 = stream0->element;
				modAudioQueueElement element1 = stream1->element;
				modAudioQueueElement element2 = stream2->element;
				modAudioQueueElement element3 = stream3->element;
				int use0 = element0->sampleCount - element0->position;
				int use1 = element1->sampleCount - element1->position;
				int use2 = element2->sampleCount - element2->position;
				int use3 = element3->sampleCount - element3->position;
				int use = (use0 < use1) ? use0 : use1;
				if (use > use2) use = use2;
				if (use > use3) use = use3;
				if (use > samplesToGenerate)
					use = samplesToGenerate;

				OUTPUTSAMPLETYPE *s0 = (OUTPUTSAMPLETYPE *)((element0->position * bytesPerFrame) + (uint8_t *)element0->samples);
				OUTPUTSAMPLETYPE *s1 = (OUTPUTSAMPLETYPE *)((element1->position * bytesPerFrame) + (uint8_t *)element1->samples);
				OUTPUTSAMPLETYPE *s2 = (OUTPUTSAMPLETYPE *)((element2->position * bytesPerFrame) + (uint8_t *)element2->samples);
				OUTPUTSAMPLETYPE *s3 = (OUTPUTSAMPLETYPE *)((element3->position * bytesPerFrame) + (uint8_t *)element3->samples);
				int count = use * out->numChannels;
				if (!out->applyVolume) {
					while (count--) {
						MIXSAMPLETYPE s = readSample(s0++) + readSample(s1++) + readSample(s2++) + readSample(s3++);
						*output++ = ClampSample(s, 4);
					}
				}
				else {
					uint16_t v0 = stream0->volume;
					uint16_t v1 = stream1->volume;
					uint16_t v2 = stream2->volume;
					uint16_t v3 = stream3->volume;
					while (count--) {
						MIXSAMPLETYPE s = ((readSample(s0++) * v0) + (readSample(s1++) * v1) + (readSample(s2++) * v2) + (readSample(s3++) * v3)) >> 8;
						*output++ = ClampSample(s, 4);
					}
				}

				samplesToGenerate -= use;
				element0->position += use;
				element1->position += use;
				element2->position += use;
				element3->position += use;

				if (element0->position == element0->sampleCount)
					endOfElement(out, stream0);
				if (element1->position == element1->sampleCount)
					endOfElement(out, stream1);
				if (element2->position == element2->sampleCount)
					endOfElement(out, stream2);
				if (element3->position == element3->sampleCount)
					endOfElement(out, stream3);
				}
				break;
#endif
		}
	}
}

// note: endOfElement relies on caller to lock mutex
void endOfElement(modAudioOut out, modAudioOutStream stream)
{
	modAudioQueueElement element = stream->element;

	if (element->sampleFormat != kSampleFormatUncompressed) {
		if (streamDecompressNext(stream))
			return;
	}

	if (element->position > 0)
		element->position = 0;
	element->compressed.remaining = element->compressed.total;
	element->compressed.data = element->compressed.initial;

	if (element->repeat < 0) {		// infinity... continues until more samples queued
		int i;
		for (i = 1; i < stream->elementCount; i++) {
			if (stream->element[i].sampleCount) {
				element->repeat = 0;
				break;
			}
		}
	}
	else if (element->repeat)
		element->repeat -= 1;

	while (0 == element->repeat) {
		if (0 == element->sampleCount) {
			if (element->position < 0)
				queueCallback(out, (xsIntegerValue)(uintptr_t)element->samples, stream - out->stream);
			else if (NULL == element->samples)
				setStreamVolume(out, stream, element->position);
		}

		stream->elementCount -= 1;
		if (stream->elementCount) {
			c_memmove(element, element + 1, sizeof(modAudioQueueElementRecord) * stream->elementCount);
			if (element->repeat) {		// first element has audio. if compressed, decompress first chunk
				if (element->sampleFormat != kSampleFormatUncompressed)
					streamDecompressNext(stream);
				break;
			}
		}
		else {
			updateActiveStreams(out);
			break;
		}
	}
}

void setStreamVolume(modAudioOut out, modAudioOutStream stream, int volume)
{
	volume /= MODDEF_AUDIOOUT_VOLUME_DIVIDER;
	if (stream->volume == volume)
		return;

	stream->volume = volume;
	updateActiveStreams(out);
}

int streamDecompressNext(modAudioOutStream stream)
{
	modAudioQueueElement element = stream->element;

	if (kSampleFormatIMA == element->sampleFormat) {
		if (0 == element->compressed.remaining)
			return 0;

		dvi_adpcm_decode(element->compressed.data, kIMABytesPerChunk, stream->decompressed);

		element->compressed.remaining -= 1;
		element->compressed.data += kIMABytesPerChunk;
	}
	else if (kSampleFormatSBC == element->sampleFormat) {
		if (0 == element->compressed.remaining)
			return 0;

		int bytesUsed = sbc_decoder((SBC_Decode *)stream->decompressor, element->compressed.data, 512 /* @@ imperfect */, stream->decompressed, kSBCSamplesPerChunk * sizeof(int16_t), NULL);

		element->compressed.remaining -= bytesUsed;
		element->compressed.data += bytesUsed;
	}
	else if (kSampleFormatTone == element->sampleFormat) {
		int16_t *out = stream->decompressed;
		uint8_t remain;
		OUTPUTSAMPLETYPE value;
		int position, max;

		if (0 == element->tone.count)
			return 0;

		if (element->tone.count < 0)
			remain = kToneSamplesPerChunk;
		else {
			if (kToneSamplesPerChunk < element->tone.count)
				remain = kToneSamplesPerChunk;
			else
				remain = element->tone.count;
			element->tone.count -= remain;
		}

		value = element->tone.value;
		position = element->tone.position;
		max = element->tone.max;
		element->sampleCount = remain; 
		while (remain--) {
			*out++ = value;
			position += 0x10000;
			if (position >= max) {
				value = -value;
				position -= max;
			}
		}
		element->tone.value = value;
		element->tone.position = position;
	}
	else if (kSampleFormatSilence == element->sampleFormat) {
		uint32_t use = element->silence.remaining;
		if (!use) return 0;

		if (use > kToneSamplesPerChunk)
			use = kToneSamplesPerChunk;
		element->silence.remaining -= use;
		element->sampleCount = use;

		c_memset(stream->decompressed, 0, use * 2); 
	}

	element->position = 0;

	return 1;
}

