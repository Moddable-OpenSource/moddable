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

#include <AudioToolbox/AudioFormat.h>
#include <AudioToolbox/AudioQueue.h>

struct AudioOutputRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onWritable;
	AudioQueueRef queue;
	uint32_t offset;
	uint32_t size;
	uint8_t* data;
	uint8_t calling;
	uint16_t running;
	uint16_t bytesPerFrame;
	uint16_t queueLength;
	AudioQueueBufferRef queueBuffers[1];
};
typedef struct AudioOutputRecord AudioOutputRecord;
typedef struct AudioOutputRecord *AudioOutput;

static void xs_audioout_mark_(xsMachine* the, void* it, xsMarkRoot markRoot);

static void AudioOutputCallback(void *it, AudioQueueRef queue, AudioQueueBufferRef queueBuffer) {
	AudioOutput output = it;
	output->offset = 0;
	output->size = (uint32_t)(queueBuffer->mAudioDataBytesCapacity);
	output->data = (uint8_t*)queueBuffer->mAudioData;
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
			return;
		}
	}
	c_memset(output->data + output->offset, 0, output->size - output->offset);
	queueBuffer->mAudioDataByteSize = output->size;
	AudioQueueEnqueueBuffer(queue, queueBuffer, 0, NULL);
	output->offset = 0;
	output->size = 0;
	output->data = C_NULL;
}

static const xsHostHooks xsAudioOutHooks = {
	xs_audioout_destructor_,
	xs_audioout_mark_,
	NULL
};

void xs_audioout_constructor_(xsMachine *the)
{
	uint8_t format = kIOFormatBuffer;
	int bitsPerSample = 0;
	int numChannels = 0;
	int sampleRate = 0;
	uint16_t queueLength = 8;
	uint16_t bytesPerFrame = 0;
	uint32_t bufferSize = 0;
	AudioOutput output;
	AudioStreamBasicDescription desc;
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
	if (MODDEF_AUDIOOUT_QUEUELENGTH > 8)
		queueLength = MODDEF_AUDIOOUT_QUEUELENGTH;
#endif
	format = builtinInitializeFormat(the, format);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
	if (xsmcHas(xsArg(0), xsID_audioType)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_audioType);
		xsStringValue type = xsmcToString(xsVar(0));
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
	if (queueLength < 8)
		xsRangeError("invalid queue length");
	bytesPerFrame = (bitsPerSample * numChannels) >> 3;
	bufferSize = (bytesPerFrame * sampleRate) >> 5; //??

	output = c_calloc(1, sizeof(AudioOutputRecord) + ((queueLength - 1) * sizeof(AudioQueueBufferRef)));
	if (!output)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, output);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAudioOutHooks);
	
	c_memset(&desc, 0, sizeof(AudioStreamBasicDescription));
	desc.mBitsPerChannel = bitsPerSample;
	desc.mBytesPerFrame = bytesPerFrame;
	desc.mBytesPerPacket = bytesPerFrame;
	desc.mChannelsPerFrame = numChannels;
	desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	if (bitsPerSample == 16)
		desc.mFormatFlags |= kAudioFormatFlagsNativeEndian;
	desc.mFormatID = kAudioFormatLinearPCM;
	desc.mFramesPerPacket = 1;
	desc.mSampleRate = sampleRate;
	if (AudioQueueNewOutput(&desc, AudioOutputCallback, output, CFRunLoopGetCurrent(), NULL, 0, &(output->queue)) != noErr)
		xsUnknownError("cannot create audio queue");
	
	output->bytesPerFrame = bytesPerFrame;
	output->queueLength = queueLength;
	for (i = 0; i < queueLength; i++) {
		if (AudioQueueAllocateBuffer(output->queue, bufferSize, &output->queueBuffers[i]) != noErr)
			xsUnknownError("cannot allocate audio queue buffer");
		c_memset((void *)output->queueBuffers[i]->mAudioData, 0, output->queueBuffers[i]->mAudioDataBytesCapacity);
		output->queueBuffers[i]->mAudioDataByteSize = output->queueBuffers[i]->mAudioDataBytesCapacity;
	   	AudioQueueEnqueueBuffer(output->queue, output->queueBuffers[i], 0, NULL);
	}
	
	output->the = the;
	output->object = xsThis;
	xsRemember(output->object);
	output->onWritable = builtinGetCallback(the, xsID_onWritable);	
	builtinInitializeTarget(the);
	
	AudioQueueStart(output->queue, NULL);
}

void xs_audioout_destructor_(void *it)
{
	if (it) {
		AudioOutput output = it;
		if (output->queue) {
			int i;
			output->onWritable = NULL;
			AudioQueueStop(output->queue, true);
			for (i = 0; i < output->queueLength; i++)
				if (output->queueBuffers[i])
					AudioQueueFreeBuffer(output->queue, output->queueBuffers[i]);
			AudioQueueDispose(output->queue, true);
		}
		c_free(output);
	}
}

void xs_audioout_close_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostData(xsThis);
	if (output && xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks)) {
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

void xs_audioout_start_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	output->running = 1;
}

void xs_audioout_stop_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	output->running = 0;
}

void xs_audioout_write_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	xsUnsignedValue available = output->size - output->offset, length;
	void* buffer;
	xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
	xsmcSetInteger(xsResult, length);		//@@
	if ((length <= 0) || (length > available)) 
		xsUnknownError("invalid size");
	c_memcpy(output->data + output->offset, buffer, length);
	output->offset += length;
}

void xs_audioout_get_format_(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	builtinGetFormat(the, kIOFormatBuffer);
}

void xs_audioout_set_format_(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	uint8_t format = builtinSetFormat(the);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
}

void xs_audioout_get_bitsPerSample_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	AudioStreamBasicDescription desc;
	UInt32 size = sizeof(desc);
	AudioQueueGetProperty(output->queue, kAudioQueueProperty_StreamDescription, &desc, &size);
	xsResult = xsInteger(desc.mBitsPerChannel);
}

void xs_audioout_get_numChannels_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	AudioStreamBasicDescription desc;
	UInt32 size = sizeof(desc);
	AudioQueueGetProperty(output->queue, kAudioQueueProperty_StreamDescription, &desc, &size);
	xsResult = xsInteger(desc.mChannelsPerFrame);
}

void xs_audioout_get_sampleRate_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	AudioStreamBasicDescription desc;
	UInt32 size = sizeof(desc);
	AudioQueueGetProperty(output->queue, kAudioQueueProperty_StreamDescription, &desc, &size);
	xsResult = xsInteger((xsIntegerValue)(desc.mSampleRate));
}

void xs_audioout_get_volume_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	AudioQueueParameterValue volume;
	AudioQueueGetParameter(output->queue, kAudioQueueParam_Volume, &volume);
	xsResult = xsNumber(volume);
}

void xs_audioout_set_volume_(xsMachine *the)
{
	AudioOutput output = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioOutHooks);
	AudioQueueParameterValue volume = xsmcToNumber(xsArg(0));
	if ((volume < 0) || (volume > 1))
		xsRangeError("invalid volume");
	AudioQueueSetParameter(output->queue, kAudioQueueParam_Volume, volume);
}
