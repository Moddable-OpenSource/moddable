/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
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

struct AudioInputRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onReadable;
	AudioQueueRef queue;
	uint32_t offset;
	uint32_t size;
	void* data;
	uint8_t calling;
	uint16_t running;
	uint16_t bytesPerFrame;
	uint16_t queueLength;
	AudioQueueBufferRef queueBuffers[1];
};
typedef struct AudioInputRecord AudioInputRecord;
typedef struct AudioInputRecord *AudioInput;

static void xs_audioin_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static void AudioInputCallback(void *it, AudioQueueRef queue, AudioQueueBufferRef queueBuffer, const AudioTimeStamp *inStartTime, UInt32 inNumberPacketDescriptions, const AudioStreamPacketDescription *inPacketDescs) {
	AudioInput input = it;
	input->offset = 0;
	input->size = (uint32_t)(queueBuffer->mAudioDataByteSize);
	input->data = queueBuffer->mAudioData;
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
			return;
		}
	}
	input->offset = 0;
	input->size = 0;
	input->data = C_NULL;
	AudioQueueEnqueueBuffer(queue, queueBuffer, 0, NULL);
}

static const xsHostHooks xsAudioInHooks = {
	xs_audioin_destructor,
	xs_audioin_mark,
	NULL
};

void xs_audioin_constructor(xsMachine *the)
{
	uint8_t format = kIOFormatBuffer;
	uint32_t bitsPerSample = 16;
	uint32_t numChannels = 1;
	uint32_t sampleRate = 22050;
	uint16_t queueLength = 4;
	uint16_t bytesPerFrame = 0;
	uint32_t bufferSize = 0;
	AudioInput input;
	AudioStreamBasicDescription desc;
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
		xsStringValue type = xsmcToString(xsVar(0));
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

	input = c_calloc(1, sizeof(AudioInputRecord) + ((queueLength - 1) * sizeof(AudioQueueBufferRef)));
	if (!input)
		xsRangeError("not enough memory");
	xsmcSetHostData(xsThis, input);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsAudioInHooks);
	
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
	if (AudioQueueNewInput(&desc, AudioInputCallback, input, CFRunLoopGetCurrent(), NULL, 0, &(input->queue)) != noErr)
		xsUnknownError("cannot create audio queue");
	   
	input->bytesPerFrame = bytesPerFrame;
	input->queueLength = queueLength;
	for (i = 0; i < queueLength; i++) {
		if (AudioQueueAllocateBuffer(input->queue, bufferSize, &input->queueBuffers[i]) != noErr)
			xsUnknownError("cannot allocate audio queue buffer");
		c_memset((void *)input->queueBuffers[i]->mAudioData, 0, input->queueBuffers[i]->mAudioDataBytesCapacity);
   		AudioQueueEnqueueBuffer(input->queue, input->queueBuffers[i], 0, NULL);
	}
	
	input->the = the;
	input->object = xsThis;
	xsRemember(input->object);
	input->onReadable = builtinGetCallback(the, xsID_onReadable);	
	builtinInitializeTarget(the);
	
	AudioQueueStart(input->queue, NULL);
}

void xs_audioin_destructor(void *it)
{
	if (it) {
		AudioInput input = it;
		if (input->queue) {
			int i;
			input->onReadable = NULL;
			AudioQueueStop(input->queue, true);
			for (i = 0; i < input->queueLength; i++)
				if (input->queueBuffers[i])
					AudioQueueFreeBuffer(input->queue, input->queueBuffers[i]);
			AudioQueueDispose(input->queue, true);
		}
		c_free(input);
	}
}

void xs_audioin_close(xsMachine *the)
{
	AudioInput input = xsmcGetHostData(xsThis);
	if (input && xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks)) {
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
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	xsUnsignedValue available, requested;
	xsBooleanValue allocate = 1;
	void* buffer;
	available = input->size - input->offset;
	if (0 == available)
		return;
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
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	input->running = 1;
}

void xs_audioin_stop(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	input->running = 0;
}

void xs_audioin_get_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	builtinGetFormat(the, kIOFormatBuffer);
}

void xs_audioin_set_format(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	uint8_t format = builtinSetFormat(the);
	if (kIOFormatBuffer != format)
		xsRangeError("invalid format");
}

void xs_audioin_get_bitsPerSample(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	AudioStreamBasicDescription desc;
	UInt32 size = sizeof(desc);
	AudioQueueGetProperty(input->queue, kAudioQueueProperty_StreamDescription, &desc, &size);
	xsResult = xsInteger(desc.mBitsPerChannel);
}

void xs_audioin_get_numChannels(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	AudioStreamBasicDescription desc;
	UInt32 size = sizeof(desc);
	AudioQueueGetProperty(input->queue, kAudioQueueProperty_StreamDescription, &desc, &size);
	xsResult = xsInteger(desc.mChannelsPerFrame);
}

void xs_audioin_get_sampleRate(xsMachine *the)
{
	AudioInput input = xsmcGetHostDataValidate(xsThis, (void *)&xsAudioInHooks);
	AudioStreamBasicDescription desc;
	UInt32 size = sizeof(desc);
	AudioQueueGetProperty(input->queue, kAudioQueueProperty_StreamDescription, &desc, &size);
	xsResult = xsInteger((xsIntegerValue)(desc.mSampleRate));
}
