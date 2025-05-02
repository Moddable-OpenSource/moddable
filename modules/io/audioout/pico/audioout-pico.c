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

/* Todo:
 * close not supported yet
 */
#include "xsmc.h"
#include "mc.xs.h"
#include "mc.defines.h"

#include "builtinCommon.h"

#include "xsHost.h"
#include "pico/audio_i2s.h"
#include "modTimer.h"

typedef struct AudioOutElementRecord AudioOutElementRecord;
typedef struct AudioOutElementRecord *AudioOutElement;

struct AudioOutElementRecord {
	AudioOutElement		next;

	xsSlot				*completion;
	xsSlot				*buffer;
	void				*data;		// NULL if relocatable
	
	uint32_t			position;
	uint32_t			bytesAvailable;
};

typedef struct AudioOutRecord AudioOutRecord;
typedef struct AudioOutRecord *AudioOut;

struct AudioOutRecord {
	xsSlot				obj;
	uint32_t			bytesWritable;
	uint8_t				useCount;
	uint8_t				started;
	uint8_t				callbackPending;

	uint8_t				numChannels;
	uint8_t				bitsPerSample;
	uint8_t				bytesPerSample;
	uint16_t			sampleRate;

	xsMachine			*the;
	xsSlot				*onWritable;

	double				volumeDouble;
	int16_t				volumeFixed;

	modTimer					timer;
	struct audio_buffer_pool	*ap;
	audio_format_t				format;
	struct audio_buffer_format	producer_format;
	struct audio_buffer_pool	*producer_pool;
	struct audio_i2s_config		config;

	struct audio_buffer 	*bufferPending;
	uint32_t			bufferPendingPos;
	
	AudioOutElement		elements;
};

#define SAMPLES_PER_BUFFER		(882 * 2)
#define NUM_SAMPLES_BUFFERS		3

enum {
	kStateIdle = 0,
	kStatePlaying = 1,
//	kStateClosing = 2,
//	kStateTerminated = 3
};

//static int i2s_enabled = 0;

static void audiooutDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength);
static void picoAudioTimerCallback(modTimer timer, void *refcon, int refconSize);
static int doWrite(AudioOut audioOut, void *buffer, xsUnsignedValue bytes);

static void xs_audioout_mark_(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsAudioOutHooks = {
	xs_audioout_destructor_,
	xs_audioout_mark_,
	C_NULL
};

static void audioOutRelease(AudioOut audioOut)
{
	if (audioOut->timer) {
		modTimerRemove(audioOut->timer);
		audioOut->timer = NULL;
	}

	audio_i2s_set_enabled(false);

	struct audio_buffer *n, *b = audioOut->ap->free_list;
	while (b) {
		n = b ->next;
		free(b->buffer);
		free(b);
		b = n;
	}
	b = audioOut->ap->prepared_list;
	while (b) {
		n = b ->next;
		free(b->buffer);
		free(b);
		b = n;
	}
	if (audioOut->bufferPending) {
		free(audioOut->bufferPending->buffer);
		free(audioOut->bufferPending);
	}

	//@@ do we need to deal with prepared_list_spin_lock and free_list_spin_lock?
	free(audioOut->producer_pool);

	while (audioOut->elements) {
		AudioOutElement element = audioOut->elements, next = element->next;
		c_free(element);
		audioOut->elements = next;
	}
}

void xs_audioout_destructor_(void *data)
{
	AudioOut audioOut = data;
	if (!audioOut) return;

	audioOutRelease(audioOut);
	c_free(audioOut);
}

void xs_audioout_constructor_(xsMachine *the)
{
	AudioOut audioOut;
	xsSlot *onWritable;
	int sampleRate = 0;
	int numChannels = 0;
	int bitsPerSample = 0;

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

	if (xsmcHas(xsArg(0), xsID_channels)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channels);
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

	if (xsmcHas(xsArg(0), xsID_audioType)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_audioType);
		char *type = xsmcToString(xsVar(0));
		if (c_strcmp(type, "LPCM"))
			xsRangeError("invalid audioType");
	}

	onWritable = builtinGetCallback(the, xsID_onWritable);

	builtinInitializeTarget(the);

	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	audioOut = c_calloc(1, sizeof(AudioOutRecord));
	if (!audioOut)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, audioOut);

	audioOut->the = the;
	audioOut->obj = xsThis;

	audioOut->useCount = 1;

	audioOut->sampleRate = (uint16_t)sampleRate;
	audioOut->numChannels = (uint8_t)numChannels;
	audioOut->bitsPerSample = (uint8_t)bitsPerSample;
	audioOut->bytesPerSample = (bitsPerSample == 16) ? 2 : 1;

	audioOut->onWritable = onWritable;
	
	audioOut->volumeDouble = 1.0;
	audioOut->volumeFixed = 256;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAudioOutHooks);
	xsRemember(audioOut->obj);

	audioOut->format.format = AUDIO_BUFFER_FORMAT_PCM_S16;
	audioOut->format.sample_freq = sampleRate;
	audioOut->format.channel_count = numChannels;

	audioOut->producer_format.format = &audioOut->format;	// set up above
#if MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE == 16
	audioOut->producer_format.sample_stride = 2;
#else
	audioOut->producer_format.sample_stride = 1;
#endif

	audioOut->producer_pool = audio_new_producer_pool(&audioOut->producer_format, NUM_SAMPLES_BUFFERS, SAMPLES_PER_BUFFER);

	audioOut->config.data_pin = MODDEF_AUDIOOUT_I2S_DATAOUT_PIN;
	audioOut->config.clock_pin_base = MODDEF_AUDIOOUT_I2S_BCK_PIN;	// BCK and pin + 1 for LR_PIN
#if (MODDEF_AUDIOOUT_I2S_BCK_PIN + 1) != MODDEF_AUDIOOUT_I2S_LR_PIN
	#error LR_PIN must be BCK_PIN + 1
#endif

	audioOut->config.dma_channel = 1;
	audioOut->config.pio_sm = 1;

	const struct audio_format *output_format;
	output_format = audio_i2s_setup(&audioOut->format, &audioOut->config);
	if (!output_format) {
		modLog("can't open audio device");
		return;
	}

	audio_i2s_connect(audioOut->producer_pool);
	audioOut->ap = audioOut->producer_pool;
}
 
void xs_audioout_close_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostData(xsThis);
	if (audioOut && xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks)) {
		xsForget(audioOut->obj);
		
#if defined(_NO_ATOMICS)
		uint8_t useCount = --audioOut->useCount;
#else
		uint8_t useCount = __atomic_sub_fetch(&audioOut->useCount, 1, __ATOMIC_SEQ_CST);
#endif
		audioOutRelease(audioOut);
		if (0 == audioOut->useCount)
			xs_audioout_destructor_(audioOut);
		xsmcSetHostData(xsThis, C_NULL);
		xsmcSetHostDestructor(xsThis, C_NULL);
	}
}
 
void xs_audioout_start_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);

	if (audioOut->started)
		xsUnknownError("already started");

	audioOut->bufferPending = take_audio_buffer(audioOut->ap, false);
	audioOut->bufferPending->sample_count = 0;
	audioOut->bufferPendingPos = 0;
	if (audioOut->bufferPending)
		audioOut->bytesWritable = audioOut->bufferPending->max_sample_count * audioOut->bytesPerSample;

	if (audioOut->bufferPending && audioOut->onWritable) {
		audioOut->useCount += 1;		//@@ atomic
		modMessagePostToMachine(audioOut->the, C_NULL, 0, audiooutDeliver, audioOut);
	}

	audio_i2s_set_enabled(true);

	if (!audioOut->timer) {
		int amt = (SAMPLES_PER_BUFFER * 1000) / audioOut->sampleRate;
		audioOut->timer = modTimerAdd(0, amt, picoAudioTimerCallback, &audioOut, 4);
	}

	audioOut->started = true;
}
 
void xs_audioout_stop_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);

	if (!audioOut->started)
		return;

	modTimerRemove(audioOut->timer);
	audioOut->timer = NULL;
	audioOut->started = false;
	audioOut->bytesWritable = 0;
}
 
void xs_audioout_writeSync_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	void *buffer;
	xsUnsignedValue requested;

	xsmcGetBufferReadable(xsArg(0), &buffer, &requested);

	if (requested > audioOut->bytesWritable)
		xsUnknownError("insufficient space");

	if (requested & 1)							//@@ broken for stereo & 8 bit
		xsUnknownError("full samples only");

	doWrite(audioOut, buffer, requested);
	if (audioOut->bytesWritable && !audioOut->callbackPending) {
		audioOut->callbackPending = true;
		audioOut->useCount += 1;		//@@ atomic
		modMessagePostToMachine(audioOut->the, C_NULL, 0, audiooutDeliver, audioOut);
	}
}
 
 void xs_audioout_writeAsync_(xsMachine *the)
 {
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	AudioOutElement element;
	xsSlot *buffer, *completion = C_NULL;
	void *data;
	xsUnsignedValue bytesAvailable;

	if (xsBufferRelocatable == xsmcGetBufferReadable(xsArg(0), &data, &bytesAvailable))
		data = C_NULL;

	buffer = xsmcToReference(xsArg(0));
	if (xsmcArgc > 1)
		completion = xsmcToReference(xsArg(1));

	element = c_calloc(1, sizeof(AudioOutElementRecord));
	if (!element)
		xsUnknownError("no memory");
	
	element->completion = completion;
	element->buffer = buffer;
	element->data = data;
	element->bytesAvailable = bytesAvailable;

//@@ lock around queue manipulation

	if (audioOut->elements) {
		AudioOutElement walker = audioOut->elements;
		while (walker->next)
			walker = walker->next;
		walker->next = element;
	}
	else
		audioOut->elements = element;

//@@ write to start playing if there's space?
}

void xs_audioout_get_format_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	builtinGetFormat(the, kIOFormatBuffer);
}
 
void xs_audioout_set_format_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	if (kIOFormatBuffer != builtinSetFormat(the))
		xsRangeError("invalid format");
}
 
void xs_audioout_get_bitsPerSample_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetInteger(xsResult, audioOut->bitsPerSample);
}
 
void xs_audioout_get_numChannels_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetInteger(xsResult, audioOut->numChannels);
}
 
void xs_audioout_get_sampleRate_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetInteger(xsResult, audioOut->sampleRate);
}
 
void xs_audioout_get_volume_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsmcSetNumber(xsResult, audioOut->volumeDouble);
}
 
void xs_audioout_set_volume_(xsMachine *the)
{
	AudioOut audioOut = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	double volume = xsmcToNumber(xsArg(0));
	if ((volume < 0) || (volume > 1))
		xsRangeError("invalid volume");
	audioOut->volumeDouble = volume;
	audioOut->volumeFixed = (int16_t)(volume * 256);
}
 
void xs_audioout_mark_(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	AudioOut audioOut = it;
	AudioOutElement walker;

	if (audioOut->onWritable)
		(*markRoot)(the, audioOut->onWritable);

	for (walker = audioOut->elements; C_NULL != walker; walker = walker->next) {
		if (walker->buffer)
			(*markRoot)(the, walker->buffer);
		if (walker->completion)
			(*markRoot)(the, walker->completion);
	}
}

static void picoAudioTimerCallback(modTimer timer, void *refcon, int refconSize)
{
	AudioOut audioOut = *(AudioOut*)refcon;

	if (audioOut->started) {
		int j;
		if (!audioOut->bufferPending) {
			audioOut->bufferPending = take_audio_buffer(audioOut->ap, false);
			audioOut->bufferPendingPos = 0;
			if (audioOut->bufferPending) {
				audioOut->bufferPending->sample_count = 0;
				audioOut->bytesWritable = audioOut->bufferPending->max_sample_count * audioOut->bytesPerSample;
			}
		}

		if ((audioOut->bufferPending) &&
			(audioOut->bytesWritable && !audioOut->callbackPending)) {
			audioOut->callbackPending = true;
			audioOut->useCount += 1;		//@@ atomic
			modMessagePostToMachine(audioOut->the, C_NULL, 0, audiooutDeliver, audioOut);
		}

	}
}

static void audiooutDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = (xsMachine *)theIn;
	AudioOut audioOut = (AudioOut)refcon;

	if (1 == audioOut->useCount)
		goto done;

	audioOut->callbackPending = false;
	xsBeginHost(the);

	if (audioOut->bytesWritable) {
		AudioOutElement element;
		if (audioOut->onWritable) {
			uint32_t bytesWritable = audioOut->bytesWritable;

			for (element = audioOut->elements; C_NULL != element; element = element->next) {
				if (element->bytesAvailable >= bytesWritable) {
					bytesWritable = 0;
					break;
				}
				bytesWritable -= element->bytesAvailable;
			}

			if (bytesWritable) {
				xsTry {
					xsmcSetInteger(xsResult, audioOut->bytesWritable);
					xsCallFunction1(xsReference(audioOut->onWritable), audioOut->obj, xsResult);
				}
				xsCatch {
				}
				if (audioOut->useCount == 0) goto abort;
			}
		}
		
		if (audioOut->bytesWritable && audioOut->elements) {
			for (element = audioOut->elements; (C_NULL != element) && audioOut->bytesWritable; element = element->next) {
				int use = audioOut->bytesWritable;
				if (use > element->bytesAvailable)
					use = element->bytesAvailable;
				if (!use)
					continue;

				void *data = element->data;
				if (!data) {
					xsUnsignedValue currentSize;
					xsResult = xsReference(element->buffer);
					xsmcGetBufferReadable(xsResult, &data, &currentSize);
					
					if ((element->position + use) > currentSize) {		// buffer resized to smaller than original request
						element->bytesAvailable = 0;
						continue;
					}
				}

				use = doWrite(audioOut, element->position + (uint8_t *)data, use);
				element->position += use;
				element->bytesAvailable -= use;
			}
		}
	}

	while (audioOut->elements && (0 == audioOut->elements->bytesAvailable)) { 
		AudioOutElement element = audioOut->elements;
		audioOut->elements = element->next;

		if (element->completion) {
			xsTry {
				xsCallFunction1(xsReference(element->completion), audioOut->obj, xsResult);
			}
			xsCatch {
			}
		}
		c_free(element);
		if (audioOut->useCount == 0) goto abort;
	}

abort:
	xsEndHost(the);
	
done:
#if defined(_NO_ATOMICS)
	if (0 == --audioOut->useCount)
#else
	if (0 == __atomic_sub_fetch(&audioOut->useCount, 1, __ATOMIC_SEQ_CST))
#endif
	{
		c_free(audioOut);
		return;
	}
}

int doWrite(AudioOut audioOut, void *audioData, xsUnsignedValue requested)
{
	size_t bytes_written;
	int amt, chunk;
	struct audio_buffer *buffer = audioOut->bufferPending;

	if (!buffer)
		goto bail;
	if (!audioOut->started)
		modLog("not started - queue?");

	chunk = buffer->max_sample_count * audioOut->bytesPerSample - audioOut->bufferPendingPos;
	amt = requested > chunk ? chunk : requested;

	if (256 == audioOut->volumeFixed) {
		c_memcpy(buffer->buffer->bytes + audioOut->bufferPendingPos, audioData, amt);
	}
	else {
		int16_t volumeFixed = audioOut->volumeFixed;
		int requestedSamples = amt >> 1;		//@@ broken for stereo & 8 bit
		int16_t *src = (int16_t *)audioData;
		int16_t *samples = buffer->buffer->bytes + audioOut->bufferPendingPos;
		int i;
		for (i=0; i<requestedSamples; i++)
			samples[i] = (*src++ * volumeFixed) >> 8;
	}

	buffer->sample_count += amt >> 1;
	audioOut->bytesWritable -= amt;
	if (audioOut->bytesWritable == 0) {
		give_audio_buffer(audioOut->ap, buffer);
		audioOut->bufferPending = NULL;
	}
	else {
		audioOut->bufferPendingPos += amt;
	}

bail:
	return amt;
}
