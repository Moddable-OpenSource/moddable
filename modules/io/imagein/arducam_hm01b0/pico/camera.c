/*
 * Copyright (c) 2024  Moddable Tech, Inc.
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
#include "xsHost.h"

#include "builtinCommon.h"
#include "modTimer.h"
#include "commodettoBitmapFormat.h"

#include "pico/hm01b0.h"

char debugStr[128];

#ifndef MODDEF_CAMERA_POWERDOWN
	#define MODDEF_CAMERA_POWERDOWN -1
#endif
#ifndef MODDEF_CAMERA_RESET
	#define MODDEF_CAMERA_RESET -1
#endif
#ifndef MODDEF_CAMERA_FLIPX
	#define MODDEF_CAMERA_FLIPX	0
#endif
#ifndef MODDEF_CAMERA_FLIPY
	#define MODDEF_CAMERA_FLIPY	0
#endif

static void xs_camera_mark(xsMachine *the, void *it, xsMarkRoot markRoot);
void xs_camera_destructor(void *it);

static const xsHostHooks xsCameraHooks = {
	xs_camera_destructor,
	xs_camera_mark,
	NULL
};

enum {		// CameraFrameState
	kCameraStateUninitialized = 0,
	kCameraStateFree,
	kCameraStatePreparing,
	kCameraStateReady,
	kCameraStateClient,
};

struct CameraFrameRecord {
	void 				*data;
	uint32_t			dataLength;
	xsSlot				*hostBuffer;
	uint32_t			id;
	uint8_t				state;
};
typedef struct CameraFrameRecord CameraFrameRecord;
typedef struct CameraFrameRecord *CameraFrame;

#define kCameraFrameCount (1)

struct CameraRecord {
	xsMachine	*the;
	xsSlot		object;
	xsSlot		*onReadable;

	struct hm01b0_config	config;

	modTimer	timer;
	uint32_t	width;
	uint32_t	height;
	uint8_t		flipx;
	uint8_t		flipy;

	uint8_t		state;
	uint8_t		frameSize;
	uint8_t		format;
	uint8_t		fps;
	uint8_t		isJPEG;
	int			imageType;

	xsSlot		*hostBufferPrototype;

	uint32_t	frameID;
	CameraFrameRecord	frames[kCameraFrameCount];
};
typedef struct CameraRecord CameraRecord;
typedef struct CameraRecord *Camera;

enum {
	kStateIdle,
	kStateRunning,
	kStateStopping,
	kStateClosing,
	kStateTerminated
};

struct FramesizeRecord {
	uint8_t		id;
	uint16_t	width;
	uint16_t	height;
};
typedef struct FramesizeRecord FramesizeRecord;
typedef struct FramesizeRecord *Framesize;

#define MAX_FRAMESIZES (3)
static FramesizeRecord FrameSizes[MAX_FRAMESIZES] = {
    0,   160, 120,
	1,	 320, 240,
	2,	 320, 320
};

static void *tempBuffer;

static int sizeToFrameSize(int width, int height)
{
	int i;

return 0;
/*
	for (i=0; i<MAX_FRAMESIZES; i++) {
		if (FrameSizes[i].width < width)
			continue;
		if (FrameSizes[i].height < height) {
			if (i<MAX_FRAMESIZES-1 && (FrameSizes[i+1].width == FrameSizes[i].width))
				return FrameSizes[i+1].id;
		}
		return FrameSizes[i].id;
	}
modLog("bad size");
	return -1;
*/
}

static void deliverCallbacks(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Camera camera = refcon;

	if (kStateTerminated == camera->state) {
		xs_camera_destructor(camera);
		return;
	}

	xsBeginHost(the);
		xsCallFunction1(xsReference(camera->onReadable), camera->object, xsInteger(0));
	xsEndHost(the);
}

void xs_camera_destructor(void *it)
{
	if (it) {
		Camera camera = it;

		hm01b0_deinit();
		c_free(camera);
		c_free(tempBuffer);
		tempBuffer = NULL;
	}
}

void xs_camera_close(xsMachine *the)
{
	Camera camera = xsmcGetHostData(xsThis);
	if ((camera) && xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks)) {
		//@@ detach host buffers
		//@@ free framebuffers

		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(camera->object);

		xs_camera_destructor(camera);
	}
}

void xs_camera_mark(xsMachine *the, void *it, xsMarkRoot markRoot)
{
	Camera camera = it;

	if (camera->onReadable)
		(*markRoot)(the, camera->onReadable);

	if (camera->hostBufferPrototype)
		(*markRoot)(the, camera->hostBufferPrototype);

	int i;
	for (i=0; i<kCameraFrameCount; i++) {
		if (camera->frames[i].hostBuffer)
			(*markRoot)(the, camera->frames[i].hostBuffer);
	}
}

void xs_camera_constructor(xsMachine *the)
{
	uint32_t width = 160;
	uint32_t height = 120;
	uint8_t format = kIOFormatBuffer;
	Camera camera;
	int imageType = kCommodettoBitmapGray16;
	uint8_t isJPEG = 0;
	uint8_t fps = 0;

	xsmcVars(1);

	format = builtinInitializeFormat(the, format);
	if ((kIOFormatBuffer != format) && (kIOFormatBufferDisposable != format))
		xsRangeError("invalid format");

	if (xsmcHas(xsArg(0), xsID_width)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_width);
		width = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_height)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_height);
		height = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_imageType)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_imageType);
		if (xsStringType == xsmcTypeOf(xsVar(0))) {
			if (c_strcmp("jpeg", xsmcToString(xsVar(0))))
				xsRangeError("unknown imageType");
			else
				xsRangeError("JPEG unsupported");
		}
		else
			imageType = xsmcToInteger(xsVar(0));
	}
	if (xsmcHas(xsArg(0), xsID_FPS)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_FPS);
		fps = xsmcToInteger(xsVar(0));
	}

	camera = c_calloc(1, sizeof(CameraRecord));
	if (!camera)
		xsUnknownError("not enough memory");
	xsmcSetHostData(xsThis, camera);
	xsSetHostHooks(xsThis, (xsHostHooks*)&xsCameraHooks);

	camera->the = the;
	camera->object = xsThis;
	camera->format = format;
	xsRemember(camera->object);
	camera->onReadable = builtinGetCallback(the, xsID_onReadable);
	builtinInitializeTarget(the);

	xsmcGet(xsVar(0), xsArg(0), xsID_prototype);
	camera->hostBufferPrototype = xsmcToReference(xsVar(0));

	int frameSize = sizeToFrameSize(width, height);
	width = FrameSizes[frameSize].width;
	height = FrameSizes[frameSize].height;

	camera->config.width = width;
	camera->config.height = height;
	camera->config.i2c = i2c0;
	camera->config.sda_pin = MODDEF_CAMERA_SDA;
	camera->config.scl_pin = MODDEF_CAMERA_SCL;
	camera->config.vsync_pin = MODDEF_CAMERA_VSYNC;
	camera->config.hsync_pin = MODDEF_CAMERA_HSYNC;
	camera->config.pclk_pin = MODDEF_CAMERA_PCLK;
	camera->config.data_pin_base = MODDEF_CAMERA_D0;
	camera->config.data_bits = 1;
	camera->config.pio = pio0;
	camera->config.pio_sm = 0;
	camera->config.reset_pin = MODDEF_CAMERA_RESET;
	camera->config.mclk_pin = MODDEF_CAMERA_MCLK;

	camera->config.flipx = MODDEF_CAMERA_FLIPX;
	camera->config.flipy = MODDEF_CAMERA_FLIPY;

	camera->frameSize = frameSize;
	camera->width = width;
	camera->height = height;
	camera->state = kStateIdle;

	camera->imageType = imageType;

	if (camera->onReadable && (0 == fps)) { 
		fps = 30;
	}
	camera->fps = fps;

	if (0 != hm01b0_init(&camera->config)) {
		xsUnknownError("camera init failed");
	}

	tempBuffer = malloc(camera->width * camera->height);
}

void xs_camera_read(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void*)&xsCameraHooks);
	void *buffer;
	uint8_t bufferNumber = 0;
	int i;
	CameraFrame frame = NULL;

	for (i = 0; i < kCameraFrameCount; i++) {
		if (kCameraStateFree != camera->frames[i].state)
			continue;
		if (NULL == frame)
			frame = &camera->frames[i];
		else if (frame->id > camera->frames[i].id)
			frame = &camera->frames[i];		// take the earliest frame in the queue
	}

	if (NULL == frame)
		return;

	frame->state = kCameraStatePreparing;

//	uint16_t *dst = (uint16_t*)frame->data;
	uint8_t *dst = (uint8_t*)frame->data;
	uint8_t b, c, *src = (uint8_t*)tempBuffer;
	int j;

	switch (camera->imageType) {
		case kCommodettoBitmapGray256:
			memcpy(dst, tempBuffer, frame->dataLength);
			break;
		case kCommodettoBitmapGray16:
			for (i=0; i<frame->dataLength; i++) {
				b = (src[i*2] >> 1) & 0xF;
				c = (src[i*2+1] >> 1) & 0xF;
				dst[i] = (b << 4) | c;
			}
			break;
		case kCommodettoBitmapRGB565LE:
			for (i=frame->dataLength/2; i > 0; i--) {
				b = src[i] & 0x1F;
				((uint16_t*)dst)[i] = (b << 11) | (b << 6) | b;
			}
			break;
		default:
			modLog("unknown imageType");
	}


	if (kIOFormatBufferDisposable == camera->format) {
		xsSlot tmp;

		tmp = xsReference(camera->hostBufferPrototype);
		xsmcSetNewHostInstance(xsResult, tmp);
		xsmcSetHostBuffer(xsResult, frame->data, frame->dataLength);
		xsmcDefine(xsResult, xsID_camera, xsThis, xsDontDelete | xsDontSet);
		xsmcSetInteger(tmp, frame->dataLength);
		xsmcDefine(xsResult, xsID_byteLength, tmp, xsDontDelete | xsDontSet);
		xsmcPetrifyHostBuffer(xsResult);

		frame->hostBuffer = xsmcToReference(xsResult);
		frame->state = kCameraStateClient;
	}
	else {
		if ((xsmcArgc > 0) && (xsReferenceType == xsmcTypeOf(xsArg(0)))) {
			void *dst;
			xsUnsignedValue requested;

			xsmcGetBufferWritable(xsArg(0), &dst, &requested);
			if (requested < frame->dataLength)
				xsRangeError("buffer too small");
			xsmcSetInteger(xsResult, frame->dataLength);

			c_memcpy(dst, frame->data, frame->dataLength);
		}
		else {
			xsmcSetArrayBuffer(xsResult, frame->data, frame->dataLength);
		}
		frame->state = kCameraStateFree;
	}

}

void cameraShutter(modTimer timer, void *refcon, int refconSize)
{
	Camera camera = *(Camera *)refcon;
	int l;

	l = camera->width * camera->height;
	hm01b0_read_frame(tempBuffer, l);

	if (camera->onReadable) { 
		modMessagePostToMachine(camera->the, C_NULL, 0, deliverCallbacks, camera);
	}
}

void xs_camera_start(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	int i, l;

	if (camera->state == kStateRunning)
		return;

	l = camera->width * camera->height;

	switch (camera->imageType) {
		case kCommodettoBitmapGray256: 			break;
		case kCommodettoBitmapGray16: l /= 2;	break;
		case kCommodettoBitmapRGB565LE: l *= 2;	break;
	}

	for (i=0; i<kCameraFrameCount; i++) {
		CameraFrame frame = &camera->frames[i];
		frame->dataLength = l;
		frame->data = malloc(frame->dataLength);
		frame->state = kCameraStateFree;
	}

	camera->timer = modTimerAdd(1000, 1000 / camera->fps, cameraShutter, &camera, sizeof(camera));
	camera->state = kStateRunning;
}

void xs_camera_stop(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	int i;

	if (camera->state == kStateRunning) {
		camera->state = kStateStopping;
		modTimerRemove(camera->timer);
		camera->timer = NULL;

		for (i=0; i<kCameraFrameCount; i++) {
			CameraFrame frame = &camera->frames[i];
			frame->state = kCameraStateUninitialized;
			free(frame->data);
		}
	}
}

void xs_camera_get_format(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	builtinGetFormat(the, camera->format);
}

void xs_camera_set_format(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatBuffer != format) && (kIOFormatBufferDisposable != format))
		xsRangeError("invalid format");
	camera->format = format;
}

void xs_camera_get_imageType(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	if (camera->isJPEG)
		xsmcSetString(xsResult, "jpeg");
	else
		xsmcSetInteger(xsResult, camera->imageType);
}

void xs_camera_get_width(xsMachine *the)
{
	Camera camera = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, camera->config.width);
}

void xs_camera_get_height(xsMachine *the)
{
	Camera camera = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, camera->config.height);
}

/*
	Disposable HostBuffer
*/

void _xs_disposable_hostbuffer_destructor(void *data)
{
}

void _xs_disposable_hostbuffer_close(xsMachine *the)
{
	void *buffer = xsmcGetHostData(xsThis);
	if (!buffer)
		return;

	xsmcVars(1);

	xsmcGet(xsVar(0), xsThis, xsID_camera);
	Camera camera = xsmcGetHostDataValidate(xsVar(0), (void *)&xsCameraHooks);

	int i;
   	for (i = 0; i < kCameraFrameCount; i++) {
		if (camera->frames[i].data == buffer) {
			camera->frames[i].state = kCameraStateFree;
			camera->frames[i].hostBuffer = NULL;
			xsmcSetHostBuffer(xsThis, NULL, 0);

			return;
		}
	}
			
	xsUnknownError("unknown buffer");
}

