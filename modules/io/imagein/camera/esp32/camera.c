/*
 * Copyright (c) 2024-2025  Moddable Tech, Inc.
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
#include "_i2c.h"

#include "esp_camera.h"
#include "sensor.h"

#include "commodettoBitmapFormat.h"
#include "commodettoPocoBlit.h"

#ifndef MODDEF_CAMERA_POWERDOWN
	#define MODDEF_CAMERA_POWERDOWN -1
#endif
#ifndef MODDEF_CAMERA_RESET
	#define MODDEF_CAMERA_RESET -1
#endif
#ifndef MODDEF_CAMERA_I2C_PORT
	#if CONFIG_SCCB_HARDWARE_I2C_PORT1
		#define MODDEF_CAMERA_I2C_PORT	1
	#else
		#define MODDEF_CAMERA_I2C_PORT	0
	#endif
#endif
#ifndef MODDEF_CAMERA_XCLK_FREQ_HZ
	#define MODDEF_CAMERA_XCLK_FREQ_HZ (24000000)
#endif
#ifndef MODDEF_CAMERA_JPEG_QUALITY
	#define MODDEF_CAMERA_JPEG_QUALITY (12)
#endif

static void xs_camera_mark(xsMachine *the, void *it, xsMarkRoot markRoot);
void xs_camera_destructor(void *data);

static const xsHostHooks ICACHE_RODATA_ATTR xsCameraHooks = {
	xs_camera_destructor,
	xs_camera_mark,
	NULL
};

enum CameraFrameState {
	kCameraStateFree = 0,
	kCameraStatePreparing,
	kCameraStateReady,
	kCameraStateClient,
};
typedef enum CameraFrameState CameraFrameState;

struct CameraFrameRecord {
	void					*data;
	uint32_t				dataLength;
	xsSlot					*hostBuffer;
	camera_fb_t				*fb;
	uint32_t				id;
	CameraFrameState		state;
};
typedef struct CameraFrameRecord CameraFrameRecord;
typedef struct CameraFrameRecord *CameraFrame;

#define kCameraFrameCount (3)

struct CameraRecord {
	xsMachine	*the;
	xsSlot		object;
	xsSlot		*onReadable;

	uint32_t	width;
	uint32_t	height;

	SemaphoreHandle_t	mutex;
	TaskHandle_t		task;
	uint8_t				state;
	uint8_t				calling;

	uint8_t		swap16;
	uint8_t		format;
	uint8_t		isJPEG;
	esp_err_t	initErr;
	int			imageType;

	xsSlot		*hostBufferPrototype;

	uint32_t	frameID;
	CameraFrameRecord	frames[kCameraFrameCount];

};
typedef struct CameraRecord CameraRecord;
typedef struct CameraRecord *Camera;

struct FramesizeRecord {
	uint8_t		id;
	uint16_t	width;
	uint16_t	height;
};
typedef struct FramesizeRecord FramesizeRecord;
typedef struct FramesizeRecord *Framesize;

// ordered by width, then by height
static FramesizeRecord FrameSizes[] = {
    FRAMESIZE_96X96,   96, 96,
    FRAMESIZE_128X128, 128, 128,
    FRAMESIZE_QQVGA,   160, 120,
    FRAMESIZE_QCIF,    176, 144,
    FRAMESIZE_HQVGA,   240, 176,
    FRAMESIZE_240X240, 240, 240,
    FRAMESIZE_QVGA,    320, 240,
    FRAMESIZE_320X320, 320, 320,
    FRAMESIZE_CIF,     400, 296,
    FRAMESIZE_HVGA,    480, 320,
    FRAMESIZE_VGA,     640, 480,
    FRAMESIZE_P_HD,    720, 1280,
    FRAMESIZE_SVGA,    800, 600,
    FRAMESIZE_P_3MP,   864, 1536,
    FRAMESIZE_XGA,     1024, 768,
    FRAMESIZE_P_FHD,   1080, 1920,
    FRAMESIZE_HD,      1280, 720,
    FRAMESIZE_SXGA,    1280, 1024,
    FRAMESIZE_UXGA,    1600, 1200,
    FRAMESIZE_FHD,     1920, 1080,
    FRAMESIZE_QXGA,    2048, 1536,
    FRAMESIZE_QHD,     2560, 1440,
    FRAMESIZE_WQXGA,   2560, 1600,
    FRAMESIZE_QSXGA,   2560, 1920,
    FRAMESIZE_INVALID, 0, 0
};

static camera_config_t camera_config = {
	.pin_pwdn = MODDEF_CAMERA_POWERDOWN,
	.pin_reset = MODDEF_CAMERA_RESET,
	.pin_xclk = MODDEF_CAMERA_XCLK,
	.pin_sccb_sda = MODDEF_CAMERA_SDA,
	.pin_sccb_scl = MODDEF_CAMERA_SCL,

	.pin_d0 = MODDEF_CAMERA_D0,
	.pin_d1 = MODDEF_CAMERA_D1,
	.pin_d2 = MODDEF_CAMERA_D2,
	.pin_d3 = MODDEF_CAMERA_D3,
	.pin_d4 = MODDEF_CAMERA_D4,
	.pin_d5 = MODDEF_CAMERA_D5,
	.pin_d6 = MODDEF_CAMERA_D6,
	.pin_d7 = MODDEF_CAMERA_D7,
	.pin_vsync = MODDEF_CAMERA_VSYNC,
	.pin_href = MODDEF_CAMERA_HREF,
	.pin_pclk = MODDEF_CAMERA_PCLK,

	.xclk_freq_hz = MODDEF_CAMERA_XCLK_FREQ_HZ,
	.ledc_timer = LEDC_TIMER_0,
	.ledc_channel = LEDC_CHANNEL_0,

	.pixel_format = PIXFORMAT_RGB565,
	.frame_size = FRAMESIZE_VGA,

	.jpeg_quality = MODDEF_CAMERA_JPEG_QUALITY,
	.fb_count = 3,
	.fb_location = CAMERA_FB_IN_PSRAM,
	.grab_mode = CAMERA_GRAB_LATEST,		// vs. CAMERA_GRAB_WHEN_EMPTY
	.sccb_i2c_port = MODDEF_CAMERA_I2C_PORT
};

enum {
	kStateInitializing,
	kStateIdle,
	kStateRunning,
	kStateStopping,
	kStateClosing,
	kStateTerminated
};

static void deliverCallbacks(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Camera camera = refcon;

	if (kStateTerminated == camera->state) {
		xs_camera_destructor(camera);		//@@??
		return;
	}

	if (kStateRunning != camera->state)
		return;

	xsBeginHost(the);
		xsCallFunction1(xsReference(camera->onReadable), camera->object, xsInteger(0));
	xsEndHost(the);
	camera->calling = 0;
}

static void cameraLoop(void *pvParameter)
{
	Camera camera = pvParameter;
	uint8_t running = 0;

	camera->initErr = esp_camera_init(&camera_config);
	if (ESP_OK != camera->initErr) {
		xSemaphoreTake(camera->mutex, portMAX_DELAY);
		goto bail;
	}

	while (true) {
		if (kStateClosing == camera->state) {
			xSemaphoreTake(camera->mutex, portMAX_DELAY);
			break;
		}

		if (kStateStopping == camera->state) {
			running = 0;
			camera->state = kStateIdle;
		}

		if (running || (kStateInitializing == camera->state)) {
			camera_fb_t *fb = esp_camera_fb_get();
			CameraFrame frame = NULL;
			int i;

			if (!camera->width) {
				camera->state = kStateIdle;	// was kStateInitializing
				camera->width = fb->width;
				camera->height = fb->height;
			}

			xSemaphoreTake(camera->mutex, portMAX_DELAY);

			for (i = 0; i < kCameraFrameCount; i++) {
				if (kCameraStateFree == camera->frames[i].state) {
					frame = &camera->frames[i];
					frame->id = ++camera->frameID;
					frame->state = kCameraStatePreparing;
					break;
				}
			}

			xSemaphoreGive(camera->mutex);
			if (!frame) {
				esp_camera_fb_return(fb);
				continue;		// keep capturing so that we always have a current frame
			}

			if (camera->swap16) {
				uint32_t *pixels = (uint32_t*)fb->buf;
				const uint32_t mask = 0x00ff00ff;
				uint32_t i, count = fb->len >> 2;
				for (i = 0; i < count; i++) {
					uint32_t t = pixels[i];
					pixels[i] = ((t & mask) << 8) | ((t >> 8) & mask);
				}
			}
			else if (kCommodettoBitmapGray16 == camera->imageType) {	// convert RGB565BE to Gray16
				uint32_t *src = (uint32_t*)fb->buf;
				uint8_t *dst = (uint8_t*)fb->buf;
				uint32_t i, count = fb->len >> 2;
				const uint32_t mask = 0x00ff00ff;
				for (i = 0; i < count; i++) {
					uint32_t t = *src++;
					t = ((t & mask) << 8) | ((t >> 8) & mask);
					uint8_t a = PocoMakePixelGray16((((t >> 27) & 0x1f) << 3), (((t >> 21) & 0x3f) << 2), (((t >> 16) & 0x1f) >> 3));
					uint8_t b = PocoMakePixelGray16((((t >> 11) & 0x1f) << 3), (((t >> 5) & 0x3f) << 2), (((t >> 0) & 0x1f) >> 3));
					*dst++ = (b << 4) | a; 
				}
			}

			frame->fb = fb;
			frame->data = fb->buf;
			frame->dataLength = fb->len;
			frame->state = kCameraStateReady;

			if (camera->onReadable) {
				camera->calling = 1;
				modMessagePostToMachine(camera->the, C_NULL, 0, deliverCallbacks, camera);
			}
		}

		if (kStateIdle == camera->state || !running) {
			uint32_t newState;
			xTaskNotifyWait(0, 0, &newState, portMAX_DELAY);

			if (kStateRunning == newState)
				running = 1;
			camera->state = newState;
		}
	}

bail:
	esp_camera_deinit();

	camera->task = NULL;
	xSemaphoreGive(camera->mutex);
	camera->state = kStateTerminated;
	vTaskDelete(NULL);
}

static int formatToCamFormat(int commodettoFormat)
{
	switch (commodettoFormat) {
		case kCommodettoBitmapRGB565BE:
		case kCommodettoBitmapRGB565LE: return PIXFORMAT_RGB565; 		// 2BPP/RGB565
		case kCommodettoBitmapMonochrome: return PIXFORMAT_GRAYSCALE;	// 1BPP/GRAYSCALE
		case kCommodettoBitmapJPEG: return PIXFORMAT_JPEG;				// JPEG/COMPRESSED
		case kCommodettoBitmap24RGB: return PIXFORMAT_RGB888;			// 3BPP/RGB888
		case kCommodettoBitmapRGB444: return PIXFORMAT_RGB444;			// 3BP2P/RGB444
		case kCommodettoBitmapYUV422: return PIXFORMAT_YUV422;			// 2BPP/YUV422
		case kCommodettoBitmapGray16: return PIXFORMAT_RGB565;			// 4BPP/GRAYSCAPE (post process) (in a perfec world, we would select YUV422 when available)
		// PIXFORMAT_YUV420;    // 1.5BPP/YUV420
		// PIXFORMAT_RAW;       // RAW	(?)
		// PIXFORMAT_RGB555;    // 3BP2P/RGB555
	}
	return -1;
}

static int sizeToFrameSize(int width, int height)
{
	int i;
	for (i = 0; FRAMESIZE_INVALID != FrameSizes[i].id; i++) {
		if (FrameSizes[i].width < width)
			continue;
		if (FrameSizes[i].height < height) {
			if ((FRAMESIZE_INVALID != FrameSizes[i + 1].id) && (FrameSizes[i+1].width == FrameSizes[i].width))
				return i + 1;
		}
		return i;
	}
	return -1;
}


void xs_camera_constructor(xsMachine *the)
{
	uint32_t width = 240;
	uint32_t height = 176;
	uint8_t format = kIOFormatBuffer;
	Camera camera;
	int imageType = kCommodettoBitmapRGB565LE;
	uint8_t isJPEG = 0;

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
			isJPEG = 1;
			imageType = -1;
		}
		else
			imageType = xsmcToInteger(xsVar(0));
	}

	camera = c_calloc(1, sizeof(CameraRecord));
	if (!camera)
		xsUnknownError("not enough memory");
	xsmcSetHostData(xsThis, camera);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsCameraHooks);

	camera->the = the;
	camera->object = xsThis;
	camera->format = format;
	xsRemember(camera->object);
	camera->onReadable = builtinGetCallback(the, xsID_onReadable);
	builtinInitializeTarget(the);

	xsmcGet(xsVar(0), xsArg(0), xsID_prototype);
	camera->hostBufferPrototype = xsmcToReference(xsVar(0));
	int frameSizeIndex = sizeToFrameSize(width, height);
	if (-1 == frameSizeIndex)
		xsUnknownError("unsupported dimensions");

	camera_config.frame_size = FrameSizes[frameSizeIndex].id;

	camera->state = kStateInitializing;
	camera->mutex = xSemaphoreCreateMutex();

	if (isJPEG)
		camera_config.pixel_format = PIXFORMAT_JPEG;
	else {
		camera_config.pixel_format = formatToCamFormat(imageType);
		if (-1 == camera_config.pixel_format)
			xsUnknownError("unsupported pixel format");
	}
	camera->imageType = imageType;
	camera->swap16 = (imageType == kCommodettoBitmapRGB565LE);
	camera->isJPEG = isJPEG;

	xsmcGet(xsVar(0), xsArg(0), xsID_i2cControl);
	xsI2CHostHooks i2c = (xsI2CHostHooks)xsGetHostHooks(xsVar(0));
	if (!i2c || !i2c->hooks.signature || (0 != c_strcmp(i2c->hooks.signature, "i2c")))
		xsUnknownError("invalid i2c");
	void *instanceData = i2c->doValidate(the, &xsVar(0));
	i2c->doDeactivate(instanceData);

	xTaskCreate(cameraLoop, "camera", 8 * 1024 + XT_STACK_EXTRA_CLIB, camera, 10, &camera->task);
	
	while (kStateInitializing == camera->state)
		vTaskDelay(1);

	if (camera->initErr)
		xsUnknownError("camera init failed");
}

void xs_camera_destructor(void *it)
{
	if (it) {
		Camera camera = it;

		if (camera->task) {
			xTaskNotify(camera->task, kStateClosing, eSetValueWithOverwrite);
			while (kStateTerminated != camera->state)
				modDelayMilliseconds(1);

			vSemaphoreDelete(camera->mutex);
		}

		c_free(camera);
	}
}

void xs_camera_close(xsMachine *the)
{
	Camera camera = xsmcGetHostData(xsThis);
	if ((camera) && xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks)) {
	//@@ detach host buffers
	//@@ free frameBuffers

		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(camera->object);
		if (camera->calling)
			camera->calling = 0;
		else
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
	for (i = 0; i < kCameraFrameCount; i++) {
		if (camera->frames[i].hostBuffer)
			(*markRoot)(the, camera->frames[i].hostBuffer);
	}
}

void xs_camera_read(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	void *buffer;
	uint8_t bufferNumber = 0;
	int i;
	CameraFrame frame = NULL;

	xSemaphoreTake(camera->mutex, portMAX_DELAY);

	for (i = 0; i < kCameraFrameCount; i++) {
		if (kCameraStateReady != camera->frames[i].state)
			continue;
		if (NULL == frame)
			frame = &camera->frames[i];
		else if (frame->id > camera->frames[i].id)
			frame = &camera->frames[i];		// take the earliest frame in the queue
	}

	if (NULL == frame) {
		xSemaphoreGive(camera->mutex);
		return;
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
			if (requested < frame->dataLength) {
				xSemaphoreGive(camera->mutex);
				xsRangeError("buffer too small");
			}
			xsmcSetInteger(xsResult, frame->dataLength);

			memcpy(dst, frame->data, frame->dataLength);
		}
		else {
			xsmcSetArrayBuffer(xsResult, frame->data, frame->dataLength);
		}


		esp_camera_fb_return(frame->fb);
		frame->hostBuffer = NULL;
		frame->fb = NULL;
		frame->data = NULL;
		frame->state = kCameraStateFree;
	}

	xSemaphoreGive(camera->mutex);
}

void xs_camera_start(xsMachine *the)
{
    Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);

	xTaskNotify(camera->task, kStateRunning, eSetValueWithOverwrite);
}

void xs_camera_stop(xsMachine *the)
{
    Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xTaskNotify(camera->task, kStateStopping, eSetValueWithOverwrite);

	xSemaphoreTake(camera->mutex, portMAX_DELAY);

	int i;
	for (i = 0; i < kCameraFrameCount; i++) {
		if (camera->frames[i].data) {
			if (camera->frames[i].hostBuffer) {
				xsSlot tmp = xsReference(camera->frames[i].hostBuffer);
				xsmcSetHostBuffer(tmp, NULL, 0);		// detach
			}
			if (camera->frames[i].fb)
				esp_camera_fb_return(camera->frames[i].fb);
			camera->frames[i].fb = NULL;
			camera->frames[i].data = NULL;
		}
		camera->frames[i].state = kCameraStateFree;
	}

	xSemaphoreGive(camera->mutex);
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
	xsmcSetInteger(xsResult, camera->width);
}

void xs_camera_get_height(xsMachine *the)
{
	Camera camera = xsmcGetHostData(xsThis);
	xsmcSetInteger(xsResult, camera->height);
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

	xSemaphoreTake(camera->mutex, portMAX_DELAY);

	int i;
	for (i = 0; i < kCameraFrameCount; i++) {
		if (camera->frames[i].data == buffer) {
			esp_camera_fb_return(camera->frames[i].fb);
			camera->frames[i].state = kCameraStateFree;
			camera->frames[i].fb = NULL;
			camera->frames[i].data = NULL;
			camera->frames[i].hostBuffer = NULL;
			xSemaphoreGive(camera->mutex);
			xsmcSetHostBuffer(xsThis, NULL, 0);
			return;
		}
	}

	xSemaphoreGive(camera->mutex);
	xsUnknownError("unknown buffer");
}
