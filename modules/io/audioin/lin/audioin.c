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

#include <alsa/asoundlib.h>

typedef struct AudioBufferRecord AudioBufferRecord;
typedef struct AudioBufferRecord *AudioBuffer;

struct AudioBufferRecord {
	AudioBuffer nextBuffer;
	int8_t data[1];
};

typedef struct AudioInputRecord AudioInputRecord;
typedef struct AudioInputRecord *AudioInput;
struct AudioInputRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onReadable;
	
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *hw_params;
	
	AudioBuffer mainBuffer;
	GMainContext* mainContext;
	GMutex mainMutex;
	GSource* mainSource;
	
	GThread* thread;
	AudioBuffer threadBuffer;
	GCond threadCondition;
	GMutex threadMutex;
	
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
	uint32_t queueBufferSize;
	AudioBuffer queueBuffers[1];
};

static void xs_audioin_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks xsAudioInputHooks = {
	xs_audioin_destructor,
	xs_audioin_mark,
	NULL
};

#define alsaIfError(IT) error = IT; if (error < 0) goto bail

AudioBuffer xs_audioin_dequeueBuffer(AudioBuffer* address)
{
	AudioBuffer result = *address;
	if (result) {
		*address = result->nextBuffer;
		result->nextBuffer = C_NULL;
	}
	return result;
}

void xs_audioin_enqueueBuffer(AudioBuffer* address, AudioBuffer buffer)
{
	AudioBuffer former;
	while ((former = *address))
		address = &(former->nextBuffer);
	*address = buffer;
}

gboolean xs_audioin_callback(void *it) {
	AudioInput input = it;
	AudioBuffer buffer;
	
	g_mutex_lock(&(input->mainMutex));
	buffer = input->mainBuffer;
	input->mainBuffer = C_NULL;
	g_source_unref(input->mainSource);
	input->mainSource = C_NULL;
	g_mutex_unlock(&(input->mainMutex));
	
	while (buffer) {
		AudioBuffer nextBuffer = buffer->nextBuffer;
		buffer->nextBuffer = C_NULL;
	
		input->offset = 0;
		input->size = input->queueBufferSize;
		input->data = (uint8_t*)(buffer->data);
		if (input->onReadable && input->running) {
			input->calling = 1;
			xsBeginHost(input->the);
			xsResult = xsAccess(input->object);
			xsCallFunction2(xsReference(input->onReadable), xsResult, xsInteger(input->size), xsInteger(input->size / input->bytesPerFrame));
			xsEndHost(input->the);
			if (input->calling)
				input->calling = 0;
			else {
				xs_audioin_destructor(input);
				return G_SOURCE_REMOVE;
			}
		}
	
		g_mutex_lock(&(input->threadMutex));
		if (input->threadBuffer == NULL) {
			input->threadBuffer = buffer;
			g_cond_signal(&(input->threadCondition));
		}
		else
			xs_audioin_enqueueBuffer(&(input->threadBuffer), buffer);
		g_mutex_unlock(&(input->threadMutex));
		
		buffer = nextBuffer;
	}
		
	input->offset = 0;
	input->size = 0;
	input->data = C_NULL;
	
	return G_SOURCE_REMOVE;
}

static gpointer xs_audioin_loop(gpointer it)
{
	AudioInput input = it;
	AudioBuffer buffer;
	int error = 0;
	snd_pcm_t *handle = C_NULL;
	snd_pcm_hw_params_t *hw_params = C_NULL;
    snd_pcm_uframes_t period_size = (input->bitsPerSample == 8) ? input->queueBufferSize : input->queueBufferSize >> 1;
    snd_pcm_uframes_t buffer_size = period_size * 2;
	
	alsaIfError(snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0));
		
	alsaIfError(snd_pcm_hw_params_malloc(&hw_params));
	alsaIfError(snd_pcm_hw_params_any(handle, hw_params));
	alsaIfError(snd_pcm_hw_params_set_rate_resample(handle, hw_params, 1));
	alsaIfError(snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
	alsaIfError(snd_pcm_hw_params_set_format(handle, hw_params, (input->bitsPerSample == 8) ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_S16_LE));
	alsaIfError(snd_pcm_hw_params_set_channels(handle, hw_params, input->numChannels));
	alsaIfError(snd_pcm_hw_params_set_rate_near(handle, hw_params, &(input->sampleRate), 0));
	alsaIfError(snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, 0));
	alsaIfError(snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, &buffer_size));
	alsaIfError(snd_pcm_hw_params(handle, hw_params));

	snd_pcm_prepare(handle);
	for (;;) {
		g_mutex_lock(&(input->threadMutex));
		while ((input->threadBuffer == NULL) && (input->destructing == 0))
			g_cond_wait(&(input->threadCondition), &(input->threadMutex));
		buffer = xs_audioin_dequeueBuffer(&(input->threadBuffer));
		g_mutex_unlock(&(input->threadMutex));
		if (input->destructing)
			break;
		
		if (input->bitsPerSample == 8) {
			int8_t* p = (int8_t*)buffer->data;
			snd_pcm_uframes_t c = input->queueBufferSize;
			while (c > 0) {
				snd_pcm_sframes_t result = snd_pcm_readi(handle, p, c);
				if (result < 0) {
					snd_pcm_recover(handle, result, 0);
					continue;
				}
				p += result;
				c -= result;
			}
		}
		else {
			int16_t* p = (int16_t*)buffer->data;
			snd_pcm_uframes_t c = input->queueBufferSize >> 1;
			while (c > 0) {
				snd_pcm_sframes_t result = snd_pcm_readi(handle, p, c);
				if (result < 0) {
					snd_pcm_recover(handle, result, 0);
					continue;
				}
				p += result;
				c -= result;
			}
		}
		
		g_mutex_lock(&(input->mainMutex));
		if (input->mainBuffer == C_NULL) {
			input->mainBuffer = buffer;
			input->mainSource = g_idle_source_new();
			g_source_set_callback(input->mainSource, xs_audioin_callback, input, NULL);
			g_source_set_priority(input->mainSource, G_PRIORITY_DEFAULT_IDLE);
			g_source_attach(input->mainSource, input->mainContext);
		}
		else
			xs_audioin_enqueueBuffer(&(input->mainBuffer), buffer);
		g_mutex_unlock(&(input->mainMutex));
	}
    if (snd_pcm_state(handle) == SND_PCM_STATE_RUNNING)
        snd_pcm_drop(handle);

	snd_pcm_reset(handle);
	
bail:
	if (hw_params)
		snd_pcm_hw_params_free(hw_params);
	if (handle)
		snd_pcm_close(handle);
	if (error < 0)
		fprintf(stderr, "%s\n", snd_strerror(error));
	return NULL;
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
	int i;
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
	if (xsmcHas(xsArg(0), xsID_audioType)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_audioType);
		type = xsmcToString(xsVar(0));
		if (c_strcmp(type, "LPCM"))
			xsRangeError("invalid audioType");
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

	input = c_calloc(1, sizeof(AudioInputRecord) + ((queueLength - 1) * sizeof(AudioBuffer)));
	if (!input)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, input);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAudioInputHooks);
	
	input->mainContext = g_main_context_get_thread_default();
	g_mutex_init(&(input->mainMutex));
	g_cond_init(&(input->threadCondition));
	g_mutex_init(&(input->threadMutex));
	
	input->bitsPerSample = bitsPerSample;
	input->numChannels = numChannels;
	input->sampleRate = sampleRate;
	input->bytesPerFrame = bytesPerFrame;
	
	input->queueBufferSize = bufferSize;
	input->queueLength = queueLength;
	for (i = 0; i < queueLength; i++) {
		input->queueBuffers[i] = c_calloc(1, sizeof(AudioBufferRecord) + (bufferSize - 1));
		if (!input->queueBuffers[i])
			xsRangeError("not enough memory");
	   	xs_audioin_enqueueBuffer(&(input->threadBuffer), input->queueBuffers[i]);
	}
	
	input->the = the;
	input->object = xsThis;
	xsRemember(input->object);
	input->onReadable = builtinGetCallback(the, xsID_onReadable);	
	builtinInitializeTarget(the);
	
	input->thread = g_thread_new("audioin", xs_audioin_loop, input);
}

void xs_audioin_destructor(void *it)
{
	if (it) {
		AudioInput input = it;
		int i;
		g_mutex_lock(&(input->threadMutex));
		input->destructing = 1;
		g_cond_signal(&(input->threadCondition));
		g_mutex_unlock(&(input->threadMutex));
		g_thread_join(input->thread);
		if (input->mainSource)
			g_source_destroy(input->mainSource);
		for (i = 0; i < input->queueLength; i++)
			if (input->queueBuffers[i])
				c_free(input->queueBuffers[i]);
		g_mutex_clear(&(input->threadMutex));
		g_cond_clear(&(input->threadCondition));
		g_mutex_clear(&(input->mainMutex));
		c_free(input);
	}
}

void xs_audioin_close(xsMachine *the)
{
	AudioInput input = xsmcGetHostData(xsThis);
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
	AudioInput input = it;
	if (input->onReadable)
		(*markRoot)(the, input->onReadable);
}

void xs_audioin_read(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
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
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	input->running = 1;
}

void xs_audioin_stop(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
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
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	xsResult = xsInteger(input->bitsPerSample);
}

void xs_audioin_get_numChannels(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	xsResult = xsInteger(input->numChannels);
}

void xs_audioin_get_sampleRate(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInputHooks);
	xsResult = xsInteger(input->sampleRate);
}
	
