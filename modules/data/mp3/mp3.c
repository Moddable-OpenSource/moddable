/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

#include "xsHost.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "./libmad/config.h"
#include "./libmad/mad.h"

#define kMP3SamplesPerFrame (1152)

struct xsMP3Record {
	struct mad_stream stream;
	struct mad_frame frame;
	struct mad_synth synth;

	uint8_t *outputBuffer;
	int32_t outputBufferPosition;
};
typedef struct xsMP3Record xsMP3Record;
typedef struct xsMP3Record *xsMP3;

void xs_mp3_destructor(void *data)
{
	xsMP3 mp3 = data;
	if (!mp3) return;

	mad_synth_finish(&mp3->synth);
	mad_frame_finish(&mp3->frame);
	mad_stream_finish(&mp3->stream);
	c_free(mp3);
}

void xs_mp3(xsMachine *the)
{
	xsMP3 mp3;
	
	mp3 = c_calloc(1, sizeof(xsMP3Record));
	if (!mp3) xsUnknownError("no memory");

	xsmcSetHostData(xsThis, mp3);

	mad_stream_init(&mp3->stream);
	mad_frame_init(&mp3->frame);
	mad_synth_init(&mp3->synth);
}

void xs_mp3_close(xsMachine *the)
{
	xsMP3 mp3 = xsmcGetHostData(xsThis);
	xs_mp3_destructor(mp3);
	xsmcSetHostData(xsThis, NULL);
}

static enum mad_flow output(void *s, struct mad_header const *header, struct mad_pcm *pcm)
{
	xsMP3 mp3 = s;
	if (MAD_MODE_SINGLE_CHANNEL == header->mode)
		c_memcpy(mp3->outputBuffer + mp3->outputBufferPosition, pcm->samples[0], pcm->length * 2);
	else {
		int16_t *left = (int16_t *)pcm->samples[0];
		int16_t *right = (int16_t *)pcm->samples[1];
		int16_t *out = (int16_t *)(mp3->outputBuffer + mp3->outputBufferPosition);
		int count = pcm->length;
		while (count--)
			*out++ = (*left++ + *right++) >> 1;
	}
	mp3->outputBufferPosition += pcm->length * 2;

	return MAD_FLOW_CONTINUE;
}

void xs_mp3_decode(xsMachine *the)
{
	xsMP3 mp3 = xsmcGetHostData(xsThis);
	void *data;
	xsUnsignedValue count;
	
	if (!mp3)
		xsUnknownError("closed");

	xsmcGetBufferWritable(xsArg(1), (void *)&mp3->outputBuffer, &count);
	if ((kMP3SamplesPerFrame * sizeof(int16_t)) > count)
		xsUnknownError("output too small");
	mp3->outputBufferPosition = 0;

	xsmcGetBufferReadable(xsArg(0), &data, &count);
	mad_stream_buffer(&mp3->stream, data, count);		// count includes MAD_BUFFER_GUARD

	mad_frame_decode(&mp3->frame, &mp3->stream);

	enum mad_flow result = mad_synth_frame(&mp3->synth, &mp3->frame, output, mp3);
	if (MAD_FLOW_BREAK == result)
		xsUnknownError("break");

	if (!mp3->stream.this_frame || !mp3->stream.next_frame)
		return;

	xsmcSetInteger(xsResult, mp3->outputBufferPosition / sizeof(int16_t));
	xsmcSet(xsArg(1), xsID_samples, xsResult);
	xsmcSetInteger(xsResult, mp3->stream.next_frame - mp3->stream.this_frame);
}
