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

typedef struct AudioOutputRecord AudioOutputRecord;
typedef struct AudioOutputRecord *AudioOutput;
struct AudioOutputRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onWritable;
	
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
	int16_t volume;
	
	uint8_t bitsPerSample;
	uint8_t numChannels;
	unsigned int sampleRate;
	uint16_t bytesPerFrame;
	
	uint16_t queueLength;
	uint32_t queueBufferSize;
	AudioBuffer queueBuffers[1];
};

static void xs_audioout_mark_(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks xsAudioOutputHooks = {
	xs_audioout_destructor_,
	xs_audioout_mark_,
	NULL
};

#define alsaIfError(IT) error = IT; if (error < 0) goto bail

AudioBuffer xs_audioout_dequeueBuffer(AudioBuffer* address)
{
	AudioBuffer result = *address;
	if (result) {
		*address = result->nextBuffer;
		result->nextBuffer = C_NULL;
	}
	return result;
}

void xs_audioout_enqueueBuffer(AudioBuffer* address, AudioBuffer buffer)
{
	AudioBuffer former;
	while ((former = *address))
		address = &(former->nextBuffer);
	*address = buffer;
}

gboolean xs_audioout_callback(void *it) {
	AudioOutput output = it;
	AudioBuffer buffer;
	
	g_mutex_lock(&(output->mainMutex));
	buffer = output->mainBuffer;
	output->mainBuffer = C_NULL;
	output->mainSource = C_NULL;
	g_mutex_unlock(&(output->mainMutex));
	
	while (buffer) {
		AudioBuffer nextBuffer = buffer->nextBuffer;
		buffer->nextBuffer = C_NULL;
	
		output->offset = 0;
		output->size = output->queueBufferSize;
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
				return G_SOURCE_REMOVE;
			}
		}
		if ((output->volume < 256) && (output->offset > 0)) {
			if (output->bitsPerSample == 8) {
				int16_t volume = output->volume;
				int8_t* p = (int8_t*)output->data;
				uint32_t c = output->offset;
				while (c) {
					int16_t s = *p;
					s = (s * volume) >> 8;
					s = ((s < -128) ? -128 : ((s > 127) ? 127 : s));
					*p = (int8_t)s;
					p++;
					c--;
				}
			}
			else {
				int32_t volume = output->volume;
				int16_t* p = (int16_t*)output->data;
				uint32_t c = output->offset >> 1;
				while (c) {
					int32_t s = *p;
					s = (s * volume) >> 8;
					s = ((s < -32768) ? -32768 : ((s > 32767) ? 32767 : s));
					*p = (int16_t)s;
					p++;
					c--;
				}
			}
		}
		c_memset(output->data + output->offset, 0, output->size - output->offset);
	
		g_mutex_lock(&(output->threadMutex));
		if (output->threadBuffer == NULL) {
			output->threadBuffer = buffer;
			g_cond_signal(&(output->threadCondition));
		}
		else
			xs_audioout_enqueueBuffer(&(output->threadBuffer), buffer);
		g_mutex_unlock(&(output->threadMutex));
		
		buffer = nextBuffer;
	}
		
	output->offset = 0;
	output->size = 0;
	output->data = C_NULL;
	
	return G_SOURCE_REMOVE;
}

static gpointer xs_audioout_loop(gpointer it)
{
	AudioOutput output = it;
	AudioBuffer buffer;
	int error = 0;
	snd_pcm_t *handle = C_NULL;
	snd_pcm_hw_params_t *hw_params = C_NULL;
    snd_pcm_uframes_t period_size = (output->bitsPerSample == 8) ? output->queueBufferSize : output->queueBufferSize >> 1;
    snd_pcm_uframes_t buffer_size = period_size * 2;
	
	alsaIfError(snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0));
		
	alsaIfError(snd_pcm_hw_params_malloc(&hw_params));
	alsaIfError(snd_pcm_hw_params_any(handle, hw_params));
	alsaIfError(snd_pcm_hw_params_set_rate_resample(handle, hw_params, 1));
	alsaIfError(snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
	alsaIfError(snd_pcm_hw_params_set_format(handle, hw_params, (output->bitsPerSample == 8) ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_S16_LE));
	alsaIfError(snd_pcm_hw_params_set_channels(handle, hw_params, output->numChannels));
	alsaIfError(snd_pcm_hw_params_set_rate_near(handle, hw_params, &(output->sampleRate), 0));
	alsaIfError(snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, 0));
	alsaIfError(snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, &buffer_size));
	alsaIfError(snd_pcm_hw_params(handle, hw_params));

	snd_pcm_prepare(handle);
	for (;;) {
		g_mutex_lock(&(output->threadMutex));
		while ((output->threadBuffer == NULL) && (output->destructing == 0))
			g_cond_wait(&(output->threadCondition), &(output->threadMutex));
		buffer = xs_audioout_dequeueBuffer(&(output->threadBuffer));
		g_mutex_unlock(&(output->threadMutex));
		if (output->destructing)
			break;
		
		if (output->bitsPerSample == 8) {
			int8_t* p = (int8_t*)buffer->data;
			snd_pcm_uframes_t c = output->queueBufferSize;
			while (c > 0) {
				snd_pcm_sframes_t result = snd_pcm_writei(handle, p, c);
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
			snd_pcm_uframes_t c = output->queueBufferSize >> 1;
			while (c > 0) {
				snd_pcm_sframes_t result = snd_pcm_writei(handle, p, c);
				if (result < 0) {
					snd_pcm_recover(handle, result, 0);
					continue;
				}
				p += result;
				c -= result;
			}
		}
		
		g_mutex_lock(&(output->mainMutex));
		if (output->mainBuffer == C_NULL) {
			output->mainBuffer = buffer;
			output->mainSource = g_idle_source_new();
			g_source_set_callback(output->mainSource, xs_audioout_callback, output, NULL);
			g_source_set_priority(output->mainSource, G_PRIORITY_DEFAULT_IDLE);
			g_source_attach(output->mainSource, output->mainContext);
			g_source_unref(output->mainSource);
		}
		else
			xs_audioout_enqueueBuffer(&(output->mainBuffer), buffer);
		g_mutex_unlock(&(output->mainMutex));
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
	int i;
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
		xsRangeError("invalid sample rate %d", sampleRate);
	if (queueLength < 2)
		xsRangeError("invalid queue length");
	bytesPerFrame = (bitsPerSample * numChannels) >> 3;
	bufferSize = (bytesPerFrame * sampleRate) >> 5; //??

	output = c_calloc(1, sizeof(AudioOutputRecord) + ((queueLength - 1) * sizeof(AudioBuffer)));
	if (!output)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, output);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAudioOutputHooks);
	
	output->mainContext = g_main_context_get_thread_default();
	g_mutex_init(&(output->mainMutex));
	g_cond_init(&(output->threadCondition));
	g_mutex_init(&(output->threadMutex));
	
	output->volume = 256;
	
	output->bitsPerSample = (uint8_t)bitsPerSample;
	output->numChannels = (uint8_t)numChannels;
	output->sampleRate = (unsigned int)sampleRate;
	output->bytesPerFrame = bytesPerFrame;
	
	output->queueBufferSize = bufferSize;
	output->queueLength = queueLength;
	for (i = 0; i < queueLength; i++) {
		output->queueBuffers[i] = c_calloc(1, sizeof(AudioBufferRecord) + (bufferSize - 1));
		if (!output->queueBuffers[i])
			xsRangeError("not enough memory");
	   	xs_audioout_enqueueBuffer(&(output->threadBuffer), output->queueBuffers[i]);
	}
	
	output->the = the;
	output->object = xsThis;
	xsRemember(output->object);
	output->onWritable = builtinGetCallback(the, xsID_onWritable);	
	builtinInitializeTarget(the);
	
	output->thread = g_thread_new("audioout", xs_audioout_loop, output);
}

void xs_audioout_destructor_(void *it)
{
	if (it) {
		AudioOutput output = it;
		int i;
		g_mutex_lock(&(output->threadMutex));
		output->destructing = 1;
		g_cond_signal(&(output->threadCondition));
		g_mutex_unlock(&(output->threadMutex));
		g_thread_join(output->thread);
		if (output->mainSource)
			g_source_destroy(output->mainSource);
		for (i = 0; i < output->queueLength; i++)
			if (output->queueBuffers[i])
				c_free(output->queueBuffers[i]);
		g_mutex_clear(&(output->threadMutex));
		g_cond_clear(&(output->threadCondition));
		g_mutex_clear(&(output->mainMutex));
		c_free(output);
	}
}

void xs_audioout_close_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostData(xsThis);
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
	AudioOutput output = it;
	if (output->onWritable)
		(*markRoot)(the, output->onWritable);
}

void xs_audioout_start_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	if (output->running)
		xsUnknownError("already started");
	output->running = 1;
}

void xs_audioout_stop_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	output->running = 0;
}

void xs_audioout_write_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
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
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsResult = xsInteger(output->bitsPerSample);
}

void xs_audioout_get_numChannels_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsResult = xsInteger(output->numChannels);
}

void xs_audioout_get_sampleRate_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsResult = xsInteger(output->sampleRate);
}

void xs_audioout_get_volume_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsNumberValue volume = output->volume;
	xsResult = xsNumber(volume / 256);
}

void xs_audioout_set_volume_(xsMachine* the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutputHooks);
	xsNumberValue volume = xsmcToNumber(xsArg(0));
	if ((volume < 0) || (volume > 1))
		xsRangeError("invalid volume");
	output->volume = c_round(volume * 256);
}

	
	
