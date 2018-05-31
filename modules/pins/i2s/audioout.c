/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#ifndef MODDEF_AUDIOOUT_STREAMS
	#define MODDEF_AUDIOOUT_STREAMS (4)
#endif
#ifndef MODDEF_AUDIOOUT_BITSPERSAMPLE
	#define MODDEF_AUDIOOUT_BITSPERSAMPLE (16)
#endif
#ifndef MODDEF_AUDIOOUT_QUEUELENGTH
	#define MODDEF_AUDIOOUT_QUEUELENGTH (8)
#endif
#if ESP32
	#ifndef MODDEF_AUDIOOUT_I2S_NUM
		#define MODDEF_AUDIOOUT_I2S_NUM (0)
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
#endif

#ifndef MODDEF_AUDIOOUT_VOLUME_DIVIDER
	#define MODDEF_AUDIOOUT_VOLUME_DIVIDER (1)
#endif

#ifndef MODDEF_AUDIOOUT_I2S_PDM
	#define MODDEF_AUDIOOUT_I2S_PDM (0)
#elif !defined(__ets__)
	#error "PDM on ESP8266 only"
#elif (MODDEF_AUDIOOUT_I2S_PDM != 32) && (MODDEF_AUDIOOUT_I2S_PDM != 64) && (MODDEF_AUDIOOUT_I2S_PDM != 128)
	#error "invalid PDM oversampling"
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
#elif ESP32
	#include "xsesp.h"
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "freertos/semphr.h"
	#include "driver/i2s.h"

	enum {
		kStateIdle = 0,
		kStatePlaying = 1,
		kStateTerminated = 2
	};
#elif defined(__ets__)
	#include "xsesp.h"
	#include "tinyi2s.h"
#endif

#define kIMABytesPerChunk (68)
#define kIMASamplesPerChunk (129)
extern int dvi_adpcm_decode(void *in_buf, int in_size, void *out_buf);

typedef struct {
	void		*samples;
	int			sampleCount;		// 0 means this is a callback or volume command with value of (uintptr_t)samples
	int			position;			// less than zero if callback, else
	int16_t		repeat;				// always 1 for callback, negative for infinite
	int8_t		sampleFormat;		// kSampleFormat
	int8_t		reserved;
	uint16_t	compressedRemaining;
	uint16_t	compressedTotal;
	uint8_t		*compressed;
	uint8_t		*compressedInitial;
} modAudioQueueElementRecord, *modAudioQueueElement;

typedef struct {
	uint16_t		volume;				// 8.8 fixed
	int16_t			*decompressed;
	int				elementCount;
	modAudioQueueElementRecord	element[MODDEF_AUDIOOUT_QUEUELENGTH];
} modAudioOutStreamRecord, *modAudioOutStream;

typedef struct {
	xsMachine				*the;
	xsSlot					obj;

	uint16_t				sampleRate;
	uint8_t					numChannels;
	uint8_t					bitsPerSample;
	uint8_t					bytesPerFrame;
	uint8_t					applyVolume;		// one or more active streams is not at 1.0 volume

	int						activeStreamCount;
	modAudioOutStream		activeStream[MODDEF_AUDIOOUT_STREAMS];

	int						streamCount;

#if defined(__APPLE__)
	AudioQueueRef			audioQueue;
	AudioQueueBufferRef		buffer[kAudioQueueBufferCount];
	pthread_mutex_t			mutex;
	CFRunLoopTimerRef		callbackTimer;
	CFRunLoopRef			runLoop;
#elif ESP32
	SemaphoreHandle_t 		mutex;
	TaskHandle_t			task;

	uint8_t					state;		// 0 idle, 1 playing, 2 terminated

	uint32_t				buffer[384];
#if 32 == MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
	uint32_t				*buffer32;
#endif
#elif defined(__ets__)
	uint8_t					i2sActive;
	OUTPUTSAMPLETYPE		buffer[64];		// size assumes DMA Buffer size of I2S
#endif

#if MODDEF_AUDIOOUT_I2S_PDM
	int32_t					prevSample;
	int32_t					error;
#endif

	int						pendingCallbackCount;
	xsIntegerValue			pendingCallbacks[MODDEF_AUDIOOUT_QUEUELENGTH];

	modAudioOutStreamRecord	stream[1];		// must be last
} modAudioOutRecord, *modAudioOut;

static void updateActiveStreams(modAudioOut out);
#if defined(__APPLE__)
	static void audioQueueCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer);
	static void queueCallback(modAudioOut out, xsIntegerValue id);
	static void invokeCallbacks(CFRunLoopTimerRef timer, void *info);
#elif ESP32
	static void audioOutLoop(void *pvParameter);
#elif defined(__ets__)
	static void doRenderSamples(void *refcon, int16_t *lr, int count);
#endif
#if defined(__ets__) || ESP32
	void deliverCallbacks(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
	void queueCallback(modAudioOut out, xsIntegerValue id);
#endif
static void audioMix(modAudioOut out, int samplesToGenerate, OUTPUTSAMPLETYPE *output);
static void endOfElement(modAudioOut out, modAudioOutStream stream);
static void setStreamVolume(modAudioOut out, modAudioOutStream stream, int volume);
static int streamDecompressNext(modAudioOutStream stream);

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
#elif ESP32
	out->state = kStateTerminated;
	xTaskNotify(out->task, kStateTerminated, eSetValueWithOverwrite);

	vSemaphoreDelete(out->mutex);
#if 32 == MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
	if (out->buffer32)
		heap_caps_free(out->buffer32);
#endif

#elif defined(__ets__)
	if (out->i2sActive)
		i2s_end();
#endif

	for (i = 0; i < out->streamCount; i++)
		if (out->stream[i].decompressed)
			c_free(out->stream[i].decompressed);

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

	out->the = the;
	out->obj = xsThis;

	out->streamCount = streamCount;
	out->sampleRate = sampleRate;
	out->numChannels = numChannels;
	out->bitsPerSample = bitsPerSample;
	out->bytesPerFrame = (bitsPerSample * numChannels) >> 3;
	out->streamCount = streamCount;

	for (i = 0; i < streamCount; i++)
		out->stream[i].volume = 256 / MODDEF_AUDIOOUT_VOLUME_DIVIDER;

#if defined(__APPLE__)
	OSStatus err;
	AudioStreamBasicDescription desc = {0};

	desc.mBitsPerChannel = bitsPerSample;
	desc.mBytesPerFrame = out->bytesPerFrame;
	desc.mBytesPerPacket = desc.mBytesPerFrame;
	desc.mChannelsPerFrame = numChannels;
#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 8
	desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
#elif MODDEF_AUDIOOUT_BITSPERSAMPLE == 16
	desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
#endif
	desc.mFormatID = kAudioFormatLinearPCM;
	desc.mFramesPerPacket = 1;
	desc.mSampleRate = sampleRate;

	pthread_mutex_init(&out->mutex, NULL);

	out->runLoop = CFRunLoopGetCurrent();

	err = AudioQueueNewOutput(&desc, audioQueueCallback, out, NULL, NULL, 0, &out->audioQueue);
	if (noErr != err)
		xsUnknownError("can't create output");

	// 2 buffers, 1/32 of a second each
	for (i = 0; i < kAudioQueueBufferCount; i++)
		AudioQueueAllocateBuffer(out->audioQueue, (((out->bitsPerSample * out->numChannels) >> 3) * out->sampleRate) >> 5, &out->buffer[i]);
#elif ESP32
	out->state = kStateIdle;
	out->mutex = xSemaphoreCreateMutex();

	xTaskCreate(audioOutLoop, "audioOut", 1024, out, 7, &out->task);
#if 32 == MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
	out->buffer32 = heap_caps_malloc((sizeof(out->buffer) / sizeof(uint16_t)) * sizeof(uint32_t), MALLOC_CAP_32BIT);
	if (!out->buffer32)
		xsUnknownError("out of memory");
#endif
#endif
}

void xs_audioout_close(xsMachine *the)
{
	xs_audioout_destructor(xsmcGetHostData(xsThis));
	xsmcSetHostData(xsThis, NULL);
}

void xs_audioout_start(xsMachine *the)
{
	modAudioOut out = xsmcGetHostData(xsThis);
	int i;

#if MODDEF_AUDIOOUT_I2S_PDM
	out->prevSample = 0;
	out->error = 0;
#endif

#if defined(__APPLE__)
	for (i = 0; i < kAudioQueueBufferCount; i++)
		audioQueueCallback(out, out->audioQueue, out->buffer[i]);

	AudioQueueStart(out->audioQueue, NULL);
#elif ESP32
	out->state = kStatePlaying;
	xTaskNotify(out->task, kStatePlaying, eSetValueWithOverwrite);
#elif defined(__ets__)
	#if MODDEF_AUDIOOUT_I2S_PDM
		i2s_begin(doRenderSamples, out, out->sampleRate * (MODDEF_AUDIOOUT_I2S_PDM >> 5));
	#else
		i2s_begin(doRenderSamples, out, out->sampleRate);
	#endif
	out->i2sActive = true;
#endif
}

void xs_audioout_stop(xsMachine *the)
{
	modAudioOut out = xsmcGetHostData(xsThis);

#if defined(__APPLE__)
	AudioQueueStop(out->audioQueue, true);
#elif ESP32
	out->state = kStateIdle;
	xTaskNotify(out->task, kStateIdle, eSetValueWithOverwrite);
#elif defined(__ets__)
	i2s_end();
	out->i2sActive = false;
#endif
}

enum {
	kKindSamples = 1,
	kKindFlush = 2,
	kKindCallback = 3,
	kKindVolume = 4,
	kKindRawSamples = 5,
};

enum {
	kSampleFormatUncompressed = 0,
	kSampleFormatIMA = 1,
};

void xs_audioout_enqueue(xsMachine *the)
{
	modAudioOut out = xsmcGetHostData(xsThis);
	int stream, argc = xsmcArgc;
	int repeat = 1, sampleOffset = 0, samplesToUse = -1, bufferSamples, volume;
	uint8_t kind;
	uint8_t *buffer;
	uint16_t sampleRate;
	uint8_t numChannels;
	uint8_t bitsPerSample;
	uint8_t sampleFormat;
	modAudioQueueElement element;

	xsmcVars(1);

	stream = xsmcToInteger(xsArg(0));
	if ((stream < 0) || (stream >= out->streamCount))
		xsRangeError("invalid stream");

	kind = xsmcToInteger(xsArg(1));
	if (kKindFlush != kind) {
		if (MODDEF_AUDIOOUT_QUEUELENGTH == out->stream[stream].elementCount)
			xsUnknownError("queue full");
	}

	switch (kind) {
		case kKindSamples:
		case kKindRawSamples:
			if (argc > 3) {
				if ((xsNumberType == xsmcTypeOf(xsArg(3))) && (C_INFINITY == xsmcToNumber(xsArg(3))))
					repeat = -1;
				else
					repeat = xsmcToInteger(xsArg(3));
				if (argc > 4) {
					sampleOffset = xsmcToInteger(xsArg(4));
					if (argc > 5)
						samplesToUse = xsmcToInteger(xsArg(5));
				}
			}

			buffer = xsmcGetHostData(xsArg(2));
			if (kind == kKindSamples) {
				if (('m' != c_read8(buffer + 0)) || ('a' != c_read8(buffer + 1)) || (1 != c_read8(buffer + 2)))
					xsUnknownError("bad header");

				bitsPerSample = c_read8(buffer + 3);
				sampleRate = c_read16(buffer + 4);
				numChannels = c_read8(buffer + 6);
				sampleFormat = c_read8(buffer + 7);
				bufferSamples = c_read32(buffer + 8);
				if ((bitsPerSample != out->bitsPerSample) || (sampleRate != out->sampleRate) || (numChannels != out->numChannels))
					xsUnknownError("format doesn't match output");
				if ((kSampleFormatUncompressed != sampleFormat) && (kSampleFormatIMA != sampleFormat))
					xsUnknownError("unsupported compression");

				buffer += 12;

				if (sampleOffset >= bufferSamples)
					xsUnknownError("invalid offset");

				if ((samplesToUse < 0) || ((sampleOffset + samplesToUse) > bufferSamples))
					samplesToUse = bufferSamples - sampleOffset;
			}
			else {
				if (samplesToUse <= 0)
					xsUnknownError("samplesToUse required");

				sampleFormat = kSampleFormatUncompressed;
			}
			
#if defined(__APPLE__)
			pthread_mutex_lock(&out->mutex);
#elif ESP32
			xSemaphoreTake(out->mutex, portMAX_DELAY);
#elif defined(__ets__)
			modCriticalSectionBegin();
#endif
			
			element = &out->stream[stream].element[out->stream[stream].elementCount];
			element->position = 0;
			element->repeat = repeat;
			element->sampleFormat = sampleFormat;
			if (kSampleFormatUncompressed == sampleFormat) {
				element->samples = buffer + (sampleOffset * out->bytesPerFrame);
				element->sampleCount = samplesToUse;
			}
			else {		// must be kSampleFormatIMA
				if (NULL == out->stream[stream].decompressed) {
					out->stream[stream].decompressed = c_malloc(kIMASamplesPerChunk * sizeof(int16_t));
					if (NULL == out->stream[stream].decompressed)
						xsUnknownError("out of memory");
				}
				element->samples = out->stream[stream].decompressed;
				element->sampleCount = kIMASamplesPerChunk;
				// calculations quantize to chunk boundaries
				element->compressedInitial = buffer + ((sampleOffset / kIMASamplesPerChunk) * kIMABytesPerChunk);
				element->compressed = element->compressedInitial;
				element->compressedTotal = samplesToUse / kIMASamplesPerChunk;
				element->compressedRemaining = element->compressedTotal;
			}

		enqueue:
			out->stream[stream].elementCount += 1;

			if (1 == out->stream[stream].elementCount) {
				if (kSampleFormatUncompressed != sampleFormat)
					streamDecompressNext(&out->stream[stream]);
				updateActiveStreams(out);
			}

#if defined(__APPLE__)
			pthread_mutex_unlock(&out->mutex);
#elif ESP32
			xSemaphoreGive(out->mutex);
			xTaskNotify(out->task, 0, eNoAction);		// notify up audio task - will be waiting if no active streams
#elif defined(__ets__)
			modCriticalSectionEnd();
#endif
			break;

		case kKindFlush: {
				int elementCount, i;

#if defined(__APPLE__)
				pthread_mutex_lock(&out->mutex);
#elif ESP32
				xSemaphoreTake(out->mutex, portMAX_DELAY);
#elif defined(__ets__)
				modCriticalSectionBegin();
#endif
				elementCount = out->stream[stream].elementCount;
				out->stream[stream].elementCount = 0;		// flush queue
				updateActiveStreams(out);

				for (i = 0; i < elementCount; i++) {
					modAudioQueueElement element = &out->stream[stream].element[i];
					if ((0 == element->repeat) && (element->position < 0) && (0 == element->sampleCount))
						queueCallback(out, (xsIntegerValue)element->samples);
				}

#if defined(__APPLE__)
				invokeCallbacks(NULL, out);
#elif ESP || defined(__ets__)
				deliverCallbacks(the, out, NULL, 0);
#endif

#if defined(__APPLE__)
				pthread_mutex_unlock(&out->mutex);
#elif ESP32
				xSemaphoreGive(out->mutex);
#elif defined(__ets__)
				modCriticalSectionEnd();
#endif
		} break;

		case kKindCallback:
#if defined(__APPLE__)
			pthread_mutex_lock(&out->mutex);
#elif ESP32
			xSemaphoreTake(out->mutex, portMAX_DELAY);
#elif defined(__ets__)
			modCriticalSectionBegin();
#endif
			element = &out->stream[stream].element[out->stream[stream].elementCount];
			element->samples = (void *)xsmcToInteger(xsArg(2));
			element->sampleCount = 0;
			element->position = -1;
			element->repeat = 0;		//@@
			goto enqueue;

		case kKindVolume:
			volume = xsmcToInteger(xsArg(2));
			if (0 == out->stream[stream].elementCount) {
				setStreamVolume(out, &out->stream[stream], volume);
				break;
			}

#if defined(__APPLE__)
			pthread_mutex_lock(&out->mutex);
#elif ESP32
			xSemaphoreTake(out->mutex, portMAX_DELAY);
#elif defined(__ets__)
			modCriticalSectionBegin();
#endif
			element = &out->stream[stream].element[out->stream[stream].elementCount];
			element->samples = NULL;
			element->sampleCount = 0;
			element->position = volume;
			element->repeat = 0;		//@@
			goto enqueue;

		default:
			xsUnknownError("bad kind");
	}

	xsResult = xsThis;
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

	while (out->pendingCallbackCount) {
		int id;

		pthread_mutex_lock(&out->mutex);
		id = out->pendingCallbacks[0];

		out->pendingCallbackCount -= 1;
		if (out->pendingCallbackCount)
			c_memcpy(out->pendingCallbacks, out->pendingCallbacks + 1, out->pendingCallbackCount * sizeof(xsIntegerValue));
		pthread_mutex_unlock(&out->mutex);

		xsmcSetInteger(xsVar(0), id);
		xsCall1(out->obj, xsID_callback, xsVar(0));		//@@ unsafe to close inside callback
	}

	xsEndHost(out->the);
}

// note: queueCallback relies on caller to lock mutex
void queueCallback(modAudioOut out, xsIntegerValue id)
{
	if (out->pendingCallbackCount < MODDEF_AUDIOOUT_QUEUELENGTH) {
		out->pendingCallbacks[out->pendingCallbackCount++] = id;

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

#elif ESP32
void audioOutLoop(void *pvParameter)
{
	modAudioOut out = pvParameter;
	uint8_t installed = false;

	i2s_config_t i2s_config = {
		.mode = I2S_MODE_MASTER | I2S_MODE_TX,	// Only TX
		.sample_rate = out->sampleRate,
		.bits_per_sample = MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE,
#if MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE == 16
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,	// 2-channels
		.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
#else
		.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT /* I2S_CHANNEL_FMT_RIGHT_LEFT */,	// 2-channels
		.communication_format = I2S_COMM_FORMAT_I2S /* | I2S_COMM_FORMAT_I2S_MSB */,
#endif
		.dma_buf_count = 2,
		.dma_buf_len = sizeof(out->buffer) / out->bytesPerFrame,		// dma_buf_len is in frames, not bytes
		.use_apll = 0,
		.intr_alloc_flags = 0
	};
	i2s_pin_config_t pin_config = {
		.bck_io_num = MODDEF_AUDIOOUT_I2S_BCK_PIN,
		.ws_io_num = MODDEF_AUDIOOUT_I2S_LR_PIN,
		.data_out_num = MODDEF_AUDIOOUT_I2S_DATAOUT_PIN,
		.data_in_num = -1	// unused
	};

	while (kStateTerminated != out->state) {
		if ((kStateIdle == out->state) || (0 == out->activeStreamCount)) {
			uint32_t newState;

			if (installed) {
				i2s_driver_uninstall(MODDEF_AUDIOOUT_I2S_NUM);
				installed = false;
			}

			xTaskNotifyWait(0, 0, &newState, portMAX_DELAY);
			if (kStateTerminated == newState)
				break;

//			if (kStateIdle == newState)
//				i2s_zero_dma_buffer(MODDEF_AUDIOOUT_I2S_NUM);
			continue;
		}

		if (!installed) {
			i2s_driver_install(MODDEF_AUDIOOUT_I2S_NUM, &i2s_config, 0, NULL);
			i2s_set_pin(MODDEF_AUDIOOUT_I2S_NUM, &pin_config);
			installed = true;
		}

		xSemaphoreTake(out->mutex, portMAX_DELAY);
		audioMix(out, sizeof(out->buffer) / out->bytesPerFrame, (OUTPUTSAMPLETYPE *)out->buffer);
		xSemaphoreGive(out->mutex);

#if 16 == MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
		i2s_write_bytes(MODDEF_AUDIOOUT_I2S_NUM, (const char *)out->buffer, sizeof(out->buffer), portMAX_DELAY);
#elif 32 == MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
		int count = sizeof(out->buffer) / out->bytesPerFrame;
		int i = count;
		int16_t *src = (int16_t *)out->buffer;
		int32_t *dst = out->buffer32;

		while (i--)
			*dst++ = *src++ << 16;

		i2s_write_bytes(MODDEF_AUDIOOUT_I2S_NUM, (const char *)out->buffer32, count * out->bytesPerFrame * 2, portMAX_DELAY);
#else
	#error invalid MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE
#endif
	}

	// from here, "out" is invalid
	if (installed)
		i2s_driver_uninstall(MODDEF_AUDIOOUT_I2S_NUM);

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
#endif

#if defined(__ets__) || ESP32

void deliverCallbacks(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	modAudioOut out = refcon;

	xsBeginHost(out->the);
	xsmcVars(1);

	while (out->pendingCallbackCount) {
		int id;

#if ESP32
		xSemaphoreTake(out->mutex, portMAX_DELAY);
#elif defined(__ets__)
		modCriticalSectionBegin();
#endif
		id = out->pendingCallbacks[0];

		out->pendingCallbackCount -= 1;
		if (out->pendingCallbackCount)
			c_memcpy(out->pendingCallbacks, out->pendingCallbacks + 1, out->pendingCallbackCount * sizeof(xsIntegerValue));
#if ESP32
		xSemaphoreGive(out->mutex);
#elif defined(__ets__)
		modCriticalSectionEnd();
#endif

		xsmcSetInteger(xsVar(0), id);
		xsCall1(out->obj, xsID_callback, xsVar(0));		//@@ unsafe to close inside callback
	}

	xsEndHost(out->the);
}

// note: queueCallback relies on caller to lock mutex
void queueCallback(modAudioOut out, xsIntegerValue id)
{
	if (out->pendingCallbackCount < MODDEF_AUDIOOUT_QUEUELENGTH) {
		out->pendingCallbacks[out->pendingCallbackCount++] = id;

		if (1 == out->pendingCallbackCount)
			modMessagePostToMachine(out->the, NULL, 0, deliverCallbacks, out);
	}
	else
		printf("audio callback queue full\n");
}
#endif

#if defined(__ets__)
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
					c_memcpy(output, s0, use * bytesPerFrame);
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
					while (count--)
						*output++ = readSample(s0++) + readSample(s1++);
				}
				else {
					uint16_t v0 = stream0->volume;
					uint16_t v1 = stream1->volume;
					while (count--)
						*output++ = ((readSample(s0++) * v0) + (readSample(s1++) * v1)) >> 8;
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
					while (count--)
						*output++ = readSample(s0++) + readSample(s1++) + readSample(s2++);
				}
				else {
					uint16_t v0 = stream0->volume;
					uint16_t v1 = stream1->volume;
					uint16_t v2 = stream2->volume;
					while (count--)
						*output++ = ((readSample(s0++) * v0) + (readSample(s1++) * v1) + (readSample(s2++) * v2)) >> 8;
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
					while (count--)
						*output++ = readSample(s0++) + readSample(s1++) + readSample(s2++) + readSample(s3++);
				}
				else {
					uint16_t v0 = stream0->volume;
					uint16_t v1 = stream1->volume;
					uint16_t v2 = stream2->volume;
					uint16_t v3 = stream3->volume;
					while (count--)
						*output++ = ((readSample(s0++) * v0) + (readSample(s1++) * v1) + (readSample(s2++) * v2) + (readSample(s3++) * v3)) >> 8;
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

	element->position = 0;
	element->compressedRemaining = element->compressedTotal;
	element->compressed = element->compressedInitial;

	if (element->repeat < 0) {		// infinity... continues until more samples queued
		int i;
		for (i = 1; i < stream->elementCount; i++) {
			if (stream->element[i].sampleCount) {
				element->repeat = 0;
				break;
			}
		}
	}
	else
		element->repeat -= 1;

	while (0 == element->repeat) {
		if (0 == element->sampleCount) {
			if (element->position < 0)
				queueCallback(out, (xsIntegerValue)element->samples);
			else
				setStreamVolume(out, stream, element->position);
		}

		stream->elementCount -= 1;
		if (stream->elementCount)
			c_memcpy(element, element + 1, sizeof(modAudioQueueElementRecord) * stream->elementCount);
		else {
			updateActiveStreams(out);
			break;
		}
	}
}

void setStreamVolume(modAudioOut out, modAudioOutStream stream, int volume)
{
	stream->volume /= MODDEF_AUDIOOUT_VOLUME_DIVIDER;
	if (stream->volume == volume)
		return;

	stream->volume = volume;
	updateActiveStreams(out);
}

int streamDecompressNext(modAudioOutStream stream)
{
	modAudioQueueElement element = stream->element;

	if (0 == element->compressedRemaining)
		return 0;

	dvi_adpcm_decode(element->compressed, kIMABytesPerChunk, stream->decompressed);

	element->compressedRemaining -= 1;
	element->compressed += kIMABytesPerChunk;
	element->position = 0;

	return 1;
}
