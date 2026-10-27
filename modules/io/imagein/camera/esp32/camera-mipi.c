/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
	MIPI CSI capture through espressif/esp_video (V4L2 open, ioctl, mmap).

	The ESP32-P4X-EYE drives an OV2710: XCLK on GPIO11, reset on GPIO26, and a
	private SCCB that this driver creates. A board that shares the SCCB passes
	an open embedded:io/i2c instance; this driver uses that bus and does not
	close it. The compiled sensor modes are MODDEF_CAMERA_WIDTH/HEIGHT and the
	optional larger pair.
*/

#include "xsmc.h"
#include "mc.xs.h"
#include "mc.defines.h"
#include "xsHost.h"

#include "builtinCommon.h"
#include "commodettoBitmapFormat.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_cache.h"
#include "esp_video_init.h"
#include "esp_video_device.h"
#include "esp_video_ioctl.h"
#include "esp_cam_sensor_xclk.h"
#include "driver/jpeg_encode.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#if !CONFIG_ESP_VIDEO_ENABLE_MIPI_CSI_VIDEO_DEVICE
	#error MIPI CSI video device is required
#endif

#ifndef MODDEF_CAMERA_RESET
	#define MODDEF_CAMERA_RESET (26)
#endif
#ifndef MODDEF_CAMERA_XCLK
	#define MODDEF_CAMERA_XCLK (11)
#endif
#ifndef MODDEF_CAMERA_XCLK_HZ
	#define MODDEF_CAMERA_XCLK_HZ (24000000)
#endif
#ifndef MODDEF_CAMERA_SCL
	#define MODDEF_CAMERA_SCL (13)
#endif
#ifndef MODDEF_CAMERA_SDA
	#define MODDEF_CAMERA_SDA (14)
#endif
#ifndef MODDEF_CAMERA_I2C_PORT
	#define MODDEF_CAMERA_I2C_PORT (1)
#endif
#ifndef MODDEF_CAMERA_I2C_HZ
	#define MODDEF_CAMERA_I2C_HZ (400000)
#endif
#ifndef MODDEF_CAMERA_I2C_OWN
	#define MODDEF_CAMERA_I2C_OWN (1)
#endif
#ifndef MODDEF_CAMERA_WIDTH
	#define MODDEF_CAMERA_WIDTH (1280)
#endif
#ifndef MODDEF_CAMERA_HEIGHT
	#define MODDEF_CAMERA_HEIGHT (720)
#endif
#ifndef MODDEF_CAMERA_WIDTH_MAX
	#define MODDEF_CAMERA_WIDTH_MAX (1920)
#endif
#ifndef MODDEF_CAMERA_HEIGHT_MAX
	#define MODDEF_CAMERA_HEIGHT_MAX (1080)
#endif
#ifndef MODDEF_CAMERA_MODEL
	#define MODDEF_CAMERA_MODEL "OV2710"
#endif

#if MODDEF_CAMERA_XCLK >= 0
	#if !CONFIG_CAMERA_XCLK_USE_ESP_CLOCK_ROUTER
		#error ESP clock router XCLK is required
	#endif
#endif

#if !MODDEF_CAMERA_I2C_OWN
	extern i2c_master_bus_handle_t modI2CMasterBus(xsMachine *the, xsSlot *instance);
#endif

#define kCameraFrameCount (3)
#define kCameraType V4L2_BUF_TYPE_VIDEO_CAPTURE
#define kCameraControlCount (3)
#define kCameraJPEGQuality (80)

enum {
	kCameraBrightness = 0,
	kCameraContrast,
	kCameraSaturation
};

enum {
	kStateInitializing,
	kStateIdle,
	kStateRunning,
	kStateStopping,
	kStateClosing,
	kStateTerminated
};

enum CameraFrameState {
	kCameraStateFree = 0,
	kCameraStateReady,
	kCameraStateClient
};
typedef enum CameraFrameState CameraFrameState;

struct CameraFrameRecord {
	void				*data;
	uint32_t			dataLength;
	xsSlot				*hostBuffer;
	uint32_t			id;
	CameraFrameState	state;
};
typedef struct CameraFrameRecord CameraFrameRecord;
typedef struct CameraFrameRecord *CameraFrame;

struct CameraRecord {
	xsMachine			*the;
	xsSlot				object;
	xsSlot				*onReadable;
	xsSlot				*hostBufferPrototype;

	uint32_t			width;
	uint32_t			height;
	uint8_t				format;
	uint8_t				isJPEG;
	uint8_t				jpegSeen;
	uint8_t				swap16;
	uint8_t				startDone;
	int					imageType;
	const char			*failReason;
	char				reason[16];
	esp_err_t			initErr;

	SemaphoreHandle_t	mutex;
	TaskHandle_t		task;
	uint8_t				state;
	uint8_t				calling;
	uint8_t				inCallback;
	uint8_t				closed;
	uint8_t				posted;
	uint8_t				streaming;
	uint8_t				videoInit;
	uint8_t				bufferCount;

	int					fd;
	int					ispFd;
	jpeg_encoder_handle_t jpegEncoder;
	uint8_t				*jpegOut;
	size_t				jpegOutSize;
	struct {
		uint32_t		id;
		int32_t			minimum;
		int32_t			maximum;
		uint8_t			value;
		uint8_t			supported;
	}					controls[kCameraControlCount];
	esp_cam_sensor_xclk_handle_t xclk;
	i2c_master_bus_handle_t sccbBus;
	void				*mapped[kCameraFrameCount];
	uint32_t			mappedLength[kCameraFrameCount];

	uint32_t			frameID;
	CameraFrameRecord	frames[kCameraFrameCount];
};
typedef struct CameraRecord CameraRecord;
typedef struct CameraRecord *Camera;

static void xs_camera_mark(xsMachine *the, void *it, xsMarkRoot markRoot);
void xs_camera_destructor(void *data);

static const xsHostHooks ICACHE_RODATA_ATTR xsCameraHooks = {
	xs_camera_destructor,
	xs_camera_mark,
	NULL
};

static void cameraShutdown(Camera camera)
{
	int type = kCameraType;

	if (camera->streaming && (camera->fd >= 0)) {
		ioctl(camera->fd, VIDIOC_STREAMOFF, &type);
		camera->streaming = 0;
	}

	if (camera->fd >= 0) {
		int i;
		for (i = 0; i < camera->bufferCount; i++) {
			if (camera->mapped[i]) {
				munmap(camera->mapped[i], camera->mappedLength[i]);
				camera->mapped[i] = NULL;
			}
		}
		close(camera->fd);
		camera->fd = -1;
	}

	if (camera->ispFd >= 0) {
		close(camera->ispFd);
		camera->ispFd = -1;
	}

	if (camera->videoInit) {
		esp_video_deinit();
		camera->videoInit = 0;
	}

	if (camera->xclk) {
		esp_cam_sensor_xclk_stop(camera->xclk);
		esp_cam_sensor_xclk_free(camera->xclk);
		camera->xclk = NULL;
	}

	// jpeg_new_encoder_engine aborts in dma2d_acquire_pool the second time.
	// One engine and one output buffer stay allocated for the process.
	camera->jpegEncoder = NULL;
	camera->jpegOut = NULL;
	camera->jpegOutSize = 0;
}

static int cameraSetFormat(Camera camera, uint32_t width, uint32_t height)
{
	struct v4l2_format format;

	memset(&format, 0, sizeof(format));
	format.type = kCameraType;
	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	if (ioctl(camera->fd, VIDIOC_S_FMT, &format) != 0)
		return 0;
	if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565)
		return 0;
	if ((0 == format.fmt.pix.width) || (0 == format.fmt.pix.height))
		return 0;

	camera->width = format.fmt.pix.width;
	camera->height = format.fmt.pix.height;
	return 1;
}

static const uint32_t gCameraControlID[kCameraControlCount] = {
	V4L2_CID_BRIGHTNESS,
	V4L2_CID_CONTRAST,
	V4L2_CID_SATURATION
};

static int cameraClampPercent(int value)
{
	if (value < 0)
		return 0;
	if (value > 100)
		return 100;
	return value;
}

static int cameraScaleToHardware(int value, int32_t minimum, int32_t maximum)
{
	value = cameraClampPercent(value);
	return (int)(minimum + (((int32_t)value * (maximum - minimum)) / 100));
}

static int cameraScaleFromHardware(int32_t value, int32_t minimum, int32_t maximum)
{
	int32_t span = maximum - minimum;
	int scaled;

	if (span <= 0)
		return 0;
	scaled = (int)((((value - minimum) * 100) + (span / 2)) / span);
	return cameraClampPercent(scaled);
}

static int cameraSetControl(Camera camera, int index, int value)
{
	struct v4l2_ext_controls controls;
	struct v4l2_ext_control control;

	if ((index < 0) || (index >= kCameraControlCount) || !camera->controls[index].supported || (camera->ispFd < 0))
		return 0;

	value = cameraClampPercent(value);
	memset(&controls, 0, sizeof(controls));
	memset(&control, 0, sizeof(control));
	controls.ctrl_class = V4L2_CID_USER_CLASS;
	controls.count = 1;
	controls.controls = &control;
	control.id = camera->controls[index].id;
	control.value = cameraScaleToHardware(value, camera->controls[index].minimum, camera->controls[index].maximum);
	if (ioctl(camera->ispFd, VIDIOC_S_EXT_CTRLS, &controls) != 0)
		return 0;

	camera->controls[index].value = (uint8_t)value;
	return 1;
}

// The ISP image pipeline rewrites brightness, contrast, and saturation. It is left off
// so a configure() value stays put. The sensor keeps its own exposure.
static void cameraProbeControls(Camera camera)
{
	int i;
	int any = 0;

	camera->ispFd = open(ESP_VIDEO_ISP1_DEVICE_NAME, O_RDWR);
	if (camera->ispFd < 0)
		return;

	for (i = 0; i < kCameraControlCount; i++) {
		struct v4l2_query_ext_ctrl query;

		memset(&query, 0, sizeof(query));
		query.id = gCameraControlID[i];
		if (ioctl(camera->ispFd, VIDIOC_QUERY_EXT_CTRL, &query) != 0)
			continue;
		if (V4L2_CTRL_TYPE_INTEGER != query.type)
			continue;
		if (query.flags & (V4L2_CTRL_FLAG_DISABLED | V4L2_CTRL_FLAG_READ_ONLY))
			continue;
		if (query.maximum <= query.minimum)
			continue;

		camera->controls[i].id = gCameraControlID[i];
		camera->controls[i].minimum = (int32_t)query.minimum;
		camera->controls[i].maximum = (int32_t)query.maximum;
		camera->controls[i].value = (uint8_t)cameraScaleFromHardware((int32_t)query.default_value, camera->controls[i].minimum, camera->controls[i].maximum);
		camera->controls[i].supported = 1;
		any = 1;
	}

	if (!any) {
		close(camera->ispFd);
		camera->ispFd = -1;
	}
}

static int cameraQueueAll(Camera camera)
{
	int i;

	for (i = 0; i < camera->bufferCount; i++) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));
		buf.type = kCameraType;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (ioctl(camera->fd, VIDIOC_QBUF, &buf) != 0)
			return 0;
		camera->frames[i].state = kCameraStateFree;
		camera->frames[i].data = NULL;
		camera->frames[i].dataLength = 0;
	}
	return 1;
}

static void cameraNoteErrno(Camera camera, const char *what)
{
	char *out = camera->reason;
	const char *end = camera->reason + sizeof(camera->reason) - 1;
	int err = errno;

	while (*what && (out < end))
		*out++ = *what++;
	if ((err < 0) || (err > 999))
		err = 999;
	if (err >= 100)
		*out++ = '0' + (err / 100);
	if (err >= 10)
		*out++ = '0' + ((err / 10) % 10);
	if (out < end)
		*out++ = '0' + (err % 10);
	*out = 0;
	camera->failReason = camera->reason;
}

static int cameraStreamOn(Camera camera)
{
	int type = kCameraType;

	if (camera->streaming)
		return 1;
	if (!cameraQueueAll(camera)) {
		cameraNoteErrno(camera, "queue ");
		return 0;
	}
	if (ioctl(camera->fd, VIDIOC_STREAMON, &type) != 0) {
		cameraNoteErrno(camera, "stream ");
		return 0;
	}
	camera->streaming = 1;
	return 1;
}

static void cameraStreamOff(Camera camera)
{
	int type = kCameraType;

	if (!camera->streaming)
		return;
	ioctl(camera->fd, VIDIOC_STREAMOFF, &type);
	camera->streaming = 0;
}

static void cameraDetach(xsMachine *the, Camera camera)
{
	int i;

	xSemaphoreTake(camera->mutex, portMAX_DELAY);
	for (i = 0; i < kCameraFrameCount; i++) {
		if (camera->frames[i].hostBuffer) {
			xsSlot tmp = xsReference(camera->frames[i].hostBuffer);
			xsmcSetHostBuffer(tmp, NULL, 0);
			camera->frames[i].hostBuffer = NULL;
		}
		camera->frames[i].data = NULL;
		camera->frames[i].dataLength = 0;
		camera->frames[i].state = kCameraStateFree;
	}
	xSemaphoreGive(camera->mutex);
}

// Hardware JPEG encoder. The capture stays RGB565; imageType "jpeg" compresses one frame.
// The engine is created once. Deleting it and creating it again resets the chip.
static jpeg_encoder_handle_t gJpegEncoder;
static uint8_t *gJpegOut;
static size_t gJpegOutSize;

static int cameraPrepareJPEG(Camera camera)
{
	size_t need;
	jpeg_encode_memory_alloc_cfg_t mem;
	jpeg_encode_engine_cfg_t engine;
	size_t size = 0;
	uint8_t *buffer;
	esp_err_t err;

	if (!camera->isJPEG)
		return 1;

	need = (size_t)camera->width * (size_t)camera->height;
	if (!gJpegOut || (gJpegOutSize < need)) {
		memset(&mem, 0, sizeof(mem));
		mem.buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER;
		buffer = jpeg_alloc_encoder_mem(need, &mem, &size);
		if (!buffer || (size < need)) {
			if (buffer)
				free(buffer);
			camera->failReason = "jpeg failed";
			return 0;
		}
		if (gJpegOut)
			free(gJpegOut);
		gJpegOut = buffer;
		gJpegOutSize = size;
	}

	if (!gJpegEncoder) {
		memset(&engine, 0, sizeof(engine));
		engine.timeout_ms = 500;
		err = jpeg_new_encoder_engine(&engine, &gJpegEncoder);
		if (ESP_OK != err) {
			gJpegEncoder = NULL;
			camera->failReason = "jpeg failed";
			return 0;
		}
	}

	camera->jpegOut = gJpegOut;
	camera->jpegOutSize = gJpegOutSize;
	camera->jpegEncoder = gJpegEncoder;
	return 1;
}

static int cameraJPEGBusy(Camera camera)
{
	int i;

	for (i = 0; i < kCameraFrameCount; i++) {
		if ((kCameraStateFree != camera->frames[i].state) && (camera->jpegOut == camera->frames[i].data))
			return 1;
	}
	return 0;
}

// Drop cache lines the camera DMA just wrote, then sample four pixels.
// A flat near-black frame is what the sensor returns before it is exposing.
static int cameraFrameBlank(void *rgb, uint32_t rgbLength)
{
	size_t line = esp_cache_get_line_size_by_addr(rgb);
	const uint16_t *p;
	uint32_t n;
	uint16_t sample[4];
	int i;

	if (rgbLength < 2)
		return 1;
	if (line && (((uintptr_t)rgb % line) == 0) && ((rgbLength % line) == 0))
		esp_cache_msync(rgb, rgbLength, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
	p = rgb;
	n = rgbLength / 2;
	sample[0] = p[0];
	sample[1] = p[n / 4];
	sample[2] = p[n / 2];
	sample[3] = p[n - 1];
	// The pedestal is about 0x0020, and the next frame only moves to 0x0040.
	// That is still black. Wait until some sample is brighter than that.
	for (i = 0; i < 4; i++) {
		uint16_t pixel = sample[i];
		if (((pixel >> 11) > 2) || (((pixel >> 5) & 0x3f) > 4) || ((pixel & 0x1f) > 2))
			return 0;
	}
	return 1;
}

static int cameraEncodeJPEG(Camera camera, void *rgb, uint32_t rgbLength, uint32_t *encoded)
{
	jpeg_encode_cfg_t cfg;
	esp_err_t err;
	size_t line = esp_cache_get_line_size_by_addr(rgb);

	// The frame was written by the camera DMA. Drop stale cache lines before
	// the encoder writes the cache back over that frame.
	if (line && (((uintptr_t)rgb % line) == 0) && ((rgbLength % line) == 0))
		esp_cache_msync(rgb, rgbLength, ESP_CACHE_MSYNC_FLAG_DIR_M2C);

	memset(&cfg, 0, sizeof(cfg));
	cfg.width = camera->width;
	cfg.height = camera->height;
	cfg.src_type = JPEG_ENCODE_IN_FORMAT_RGB565;
	cfg.sub_sample = JPEG_DOWN_SAMPLING_YUV422;
	cfg.image_quality = kCameraJPEGQuality;

	err = jpeg_encoder_process(camera->jpegEncoder, &cfg, rgb, rgbLength, camera->jpegOut, (uint32_t)camera->jpegOutSize, encoded);
	return (ESP_OK == err) && (*encoded >= 2);
}

static int cameraOpen(Camera camera)
{
	esp_video_init_csi_config_t csi;
	esp_video_init_config_t config;
	struct v4l2_requestbuffers request;
	uint32_t width;
	uint32_t height;
	int i;

	// The eye's GPIO12 is the 3V3_PERI rail, so power-down stays unconnected.
	// Clock the sensor before esp_video_init when the board has an XCLK pin.
	// The CSI config has none. A pin below 0 means the module clocks itself.
#if MODDEF_CAMERA_XCLK >= 0
	{
		esp_cam_sensor_xclk_config_t xclkConfig;

		memset(&xclkConfig, 0, sizeof(xclkConfig));
		xclkConfig.esp_clock_router_cfg.xclk_pin = MODDEF_CAMERA_XCLK;
		xclkConfig.esp_clock_router_cfg.xclk_freq_hz = MODDEF_CAMERA_XCLK_HZ;
		if (ESP_OK != esp_cam_sensor_xclk_allocate(ESP_CAM_SENSOR_XCLK_ESP_CLOCK_ROUTER, &camera->xclk)) {
			camera->failReason = "xclk allocate failed";
			return 0;
		}
		if (ESP_OK != esp_cam_sensor_xclk_start(camera->xclk, &xclkConfig)) {
			camera->failReason = "xclk start failed";
			return 0;
		}
	}
#endif

	memset(&csi, 0, sizeof(csi));
#if MODDEF_CAMERA_I2C_OWN
	csi.sccb_config.init_sccb = true;
	csi.sccb_config.i2c_config.port = MODDEF_CAMERA_I2C_PORT;
	csi.sccb_config.i2c_config.scl_pin = MODDEF_CAMERA_SCL;
	csi.sccb_config.i2c_config.sda_pin = MODDEF_CAMERA_SDA;
#else
	if (!camera->sccbBus) {
		camera->failReason = "i2c required";
		return 0;
	}
	csi.sccb_config.init_sccb = false;
	csi.sccb_config.i2c_handle = camera->sccbBus;
#endif
	csi.sccb_config.freq = MODDEF_CAMERA_I2C_HZ;
	csi.reset_pin = MODDEF_CAMERA_RESET;
	csi.pwdn_pin = (gpio_num_t)-1;
	csi.dont_init_ldo = false;

	memset(&config, 0, sizeof(config));
	config.csi = &csi;
	if (ESP_OK != esp_video_init(&config)) {
		camera->failReason = "video init failed";
		return 0;
	}
	camera->videoInit = 1;

	camera->fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY);
	if (camera->fd < 0) {
		camera->failReason = "video open failed";
		return 0;
	}

	{
		struct timeval timeout;

		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_usec = 20000;
		if (ioctl(camera->fd, VIDIOC_S_DQBUF_TIMEOUT, &timeout) != 0) {
			camera->failReason = "camera timeout failed";
			return 0;
		}
	}

	width = camera->width;
	height = camera->height;
	if ((width > MODDEF_CAMERA_WIDTH) || (height > MODDEF_CAMERA_HEIGHT)) {
		width = MODDEF_CAMERA_WIDTH_MAX;
		height = MODDEF_CAMERA_HEIGHT_MAX;
	}
	else {
		width = MODDEF_CAMERA_WIDTH;
		height = MODDEF_CAMERA_HEIGHT;
	}
	if (!cameraSetFormat(camera, width, height)) {
		if (((width == (uint32_t)MODDEF_CAMERA_WIDTH) && (height == (uint32_t)MODDEF_CAMERA_HEIGHT)) ||
			!cameraSetFormat(camera, MODDEF_CAMERA_WIDTH, MODDEF_CAMERA_HEIGHT)) {
			camera->failReason = "camera format failed";
			return 0;
		}
	}

	memset(&request, 0, sizeof(request));
	// A still stops the stream and encodes one frame, so two buffers are enough.
	request.count = camera->isJPEG ? 2 : kCameraFrameCount;
	request.type = kCameraType;
	request.memory = V4L2_MEMORY_MMAP;
	if (ioctl(camera->fd, VIDIOC_REQBUFS, &request) != 0) {
		camera->failReason = "camera buffers failed";
		return 0;
	}
	if ((request.count < 2) || (request.count > kCameraFrameCount)) {
		camera->failReason = "camera buffers failed";
		return 0;
	}
	camera->bufferCount = request.count;

	for (i = 0; i < camera->bufferCount; i++) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));
		buf.type = kCameraType;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (ioctl(camera->fd, VIDIOC_QUERYBUF, &buf) != 0) {
			camera->failReason = "camera buffers failed";
			return 0;
		}
		camera->mappedLength[i] = buf.length;
		camera->mapped[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, camera->fd, buf.m.offset);
		// esp_video mmap returns NULL on failure. MAP_FAILED is the libc value.
		if (!camera->mapped[i] || (MAP_FAILED == camera->mapped[i])) {
			camera->mapped[i] = NULL;
			camera->failReason = "camera buffers failed";
			return 0;
		}
	}

	cameraProbeControls(camera);
	if (!cameraPrepareJPEG(camera))
		return 0;
	return 1;
}

static void cameraJoin(Camera camera)
{
	if (!camera->task)
		return;
	if (kStateTerminated != camera->state) {
		xTaskNotify(camera->task, kStateClosing, eSetValueWithOverwrite);
		while (kStateTerminated != camera->state)
			modDelayMilliseconds(1);
	}
	camera->task = NULL;
}

// Free the record once the device task has exited and every queued callback has run.
static void cameraRelease(Camera camera)
{
	uint8_t freeIt = 1;

	if (!camera)
		return;
	if (camera->mutex) {
		xSemaphoreTake(camera->mutex, portMAX_DELAY);
		freeIt = camera->closed && (0 == camera->posted);
		xSemaphoreGive(camera->mutex);
	}
	if (!freeIt)
		return;

	if (camera->mutex) {
		vSemaphoreDelete(camera->mutex);
		camera->mutex = NULL;
	}
	c_free(camera);
}

static void deliverCallbacks(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	Camera camera = refcon;
	CameraFrame frame = NULL;
	uint32_t byteLength = 0;
	int i;
	uint8_t closed;

	xSemaphoreTake(camera->mutex, portMAX_DELAY);
	if (camera->posted)
		camera->posted--;
	closed = camera->closed;
	xSemaphoreGive(camera->mutex);

	if (closed || (kStateTerminated == camera->state)) {
		cameraRelease(camera);
		return;
	}

	if (kStateRunning != camera->state) {
		camera->calling = 0;
		return;
	}

	xSemaphoreTake(camera->mutex, portMAX_DELAY);
	for (i = 0; i < kCameraFrameCount; i++) {
		if (kCameraStateReady != camera->frames[i].state)
			continue;
		if ((NULL == frame) || (frame->id > camera->frames[i].id))
			frame = &camera->frames[i];
	}
	if (frame)
		byteLength = frame->dataLength;
	xSemaphoreGive(camera->mutex);

	camera->inCallback = 1;
	xsBeginHost(the);
		xsCallFunction1(xsReference(camera->onReadable), camera->object, xsInteger(byteLength));
	xsEndHost(the);
	camera->inCallback = 0;

	if (camera->closed)
		cameraRelease(camera);
	else
		camera->calling = 0;
}

static void cameraSwap16(void *data, uint32_t length)
{
	uint32_t *pixels = data;
	const uint32_t mask = 0x00ff00ff;
	uint32_t count = length >> 2;
	uint32_t i;

	for (i = 0; i < count; i++) {
		uint32_t value = pixels[i];
		pixels[i] = ((value & mask) << 8) | ((value >> 8) & mask);
	}
}

static void cameraDequeue(Camera camera)
{
	struct v4l2_buffer buf;
	CameraFrame frame;
	void *data;
	uint32_t length;

	memset(&buf, 0, sizeof(buf));
	buf.type = kCameraType;
	buf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(camera->fd, VIDIOC_DQBUF, &buf) != 0) {
		vTaskDelay(pdMS_TO_TICKS(20));
		return;
	}
	if ((buf.index >= camera->bufferCount) || (buf.flags & V4L2_BUF_FLAG_ERROR) || (0 == buf.bytesused) || !camera->mapped[buf.index]) {
		ioctl(camera->fd, VIDIOC_QBUF, &buf);
		vTaskDelay(pdMS_TO_TICKS(20));
		return;
	}

	data = camera->mapped[buf.index];
	length = buf.bytesused;
	if (length > camera->mappedLength[buf.index])
		length = camera->mappedLength[buf.index];

	if (camera->isJPEG) {
		int busy;
		uint32_t encoded = 0;
		uint32_t expect = camera->width * camera->height * 2;

		xSemaphoreTake(camera->mutex, portMAX_DELAY);
		busy = cameraJPEGBusy(camera);
		xSemaphoreGive(camera->mutex);
		if (busy || (length < expect)) {
			ioctl(camera->fd, VIDIOC_QBUF, &buf);
			vTaskDelay(pdMS_TO_TICKS(20));
			return;
		}
		// The first frames after open sit at the black pedestal, with a little
		// noise. Keep streaming until a sample is brighter, or 30 frames have passed.
		if (cameraFrameBlank(data, expect) && (camera->jpegSeen < 30)) {
			camera->jpegSeen++;
			ioctl(camera->fd, VIDIOC_QBUF, &buf);
			return;
		}
		// Stop the sensor before the encode. A stalled encode with no spare
		// buffer resets the chip. The mapped frame stays valid after STREAMOFF.
		cameraStreamOff(camera);
		if (cameraEncodeJPEG(camera, data, expect, &encoded)) {
			data = camera->jpegOut;
			length = encoded;
		}
		else {
			data = camera->jpegOut;
			length = 0;
		}
	}
	else if (camera->swap16)
		cameraSwap16(data, length);

	xSemaphoreTake(camera->mutex, portMAX_DELAY);
	frame = &camera->frames[buf.index];
	if ((kStateRunning != camera->state) || (kCameraStateFree != frame->state)) {
		ioctl(camera->fd, VIDIOC_QBUF, &buf);
		xSemaphoreGive(camera->mutex);
		return;
	}
	frame->data = data;
	frame->dataLength = length;
	frame->id = ++camera->frameID;
	frame->state = kCameraStateReady;
	xSemaphoreGive(camera->mutex);

	if (camera->onReadable && (kStateRunning == camera->state)) {
		uint8_t post = 0;

		xSemaphoreTake(camera->mutex, portMAX_DELAY);
		if (camera->onReadable && (kStateRunning == camera->state) && !camera->closed) {
			camera->posted++;
			camera->calling = 1;
			post = 1;
		}
		xSemaphoreGive(camera->mutex);
		if (post && modMessagePostToMachine(camera->the, NULL, 0, deliverCallbacks, camera)) {
			xSemaphoreTake(camera->mutex, portMAX_DELAY);
			if (camera->posted)
				camera->posted--;
			if ((0 == camera->posted) && !camera->closed)
				camera->calling = 0;
			xSemaphoreGive(camera->mutex);
		}
	}
}

static void cameraLoop(void *pvParameter)
{
	Camera camera = pvParameter;
	uint8_t running = 0;

	if (!cameraOpen(camera)) {
		if (!camera->failReason)
			camera->failReason = "camera init failed";
		if (!camera->initErr)
			camera->initErr = ESP_FAIL;
		cameraShutdown(camera);
		camera->state = kStateTerminated;
		vTaskDelete(NULL);
		return;
	}

	camera->state = kStateIdle;

	while (true) {
		if (kStateClosing == camera->state)
			break;

		if (kStateStopping == camera->state) {
			cameraStreamOff(camera);
			running = 0;
			camera->state = kStateIdle;
		}

		if (running) {
			uint32_t notified = 0;

			if (pdTRUE == xTaskNotifyWait(0, 0, &notified, 0)) {
				if ((kStateClosing == notified) || (kStateStopping == notified)) {
					camera->state = notified;
					continue;
				}
			}

			cameraDequeue(camera);
			// The still streams off once it has a frame. Wait for close instead of
			// dequeuing a stopped camera.
			if (camera->isJPEG && !camera->streaming)
				running = 0;
			continue;
		}

		{
			uint32_t newState = 0;

			xTaskNotifyWait(0, 0, &newState, portMAX_DELAY);
			if (kStateRunning == newState) {
				if (!cameraStreamOn(camera)) {
					newState = kStateIdle;
					camera->startDone = 1;
				}
				else
					running = 1;
			}
			else
				camera->startDone = 1;
			camera->state = newState;
		}
	}

	cameraShutdown(camera);
	camera->state = kStateTerminated;
	vTaskDelete(NULL);
}

void xs_camera_constructor(xsMachine *the)
{
	uint32_t width = 240;
	uint32_t height = 176;
	uint8_t format = kIOFormatBuffer;
	Camera camera;
	int imageType = kCommodettoBitmapRGB565LE;
	uint8_t isJPEG = 0;
#if !MODDEF_CAMERA_I2C_OWN
	i2c_master_bus_handle_t sccbBus = C_NULL;
#endif

	xsmcVars(1);

#if !MODDEF_CAMERA_I2C_OWN
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_i2c))
		xsUnknownError("i2c required");
	sccbBus = modI2CMasterBus(the, &xsVar(0));
	if (!sccbBus)
		xsUnknownError("i2c required");
#endif

	format = builtinInitializeFormat(the, format);
	if ((kIOFormatBuffer != format) && (kIOFormatBufferDisposable != format))
		xsRangeError("invalid format");

	if (xsmcGet(xsVar(0), xsArg(0), xsID_width))
		width = xsmcToInteger(xsVar(0));
	if (xsmcGet(xsVar(0), xsArg(0), xsID_height))
		height = xsmcToInteger(xsVar(0));
	if (xsmcGet(xsVar(0), xsArg(0), xsID_imageType)) {
		if (xsStringType == xsmcTypeOf(xsVar(0))) {
			if (c_strcmp("jpeg", xsmcToString(xsVar(0))))
				xsRangeError("unknown imageType");
			isJPEG = 1;
			imageType = -1;
		}
		else
			imageType = xsmcToInteger(xsVar(0));
	}
	if (!isJPEG && (kCommodettoBitmapRGB565LE != imageType) && (kCommodettoBitmapRGB565BE != imageType))
		xsUnknownError("unsupported pixel format");

	camera = c_calloc(1, sizeof(CameraRecord));
	if (!camera)
		xsUnknownError("not enough memory");
	xsmcSetHostData(xsThis, camera);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsCameraHooks);

	camera->the = the;
	camera->object = xsThis;
	camera->format = format;
	camera->width = width;
	camera->height = height;
	camera->isJPEG = isJPEG;
	camera->imageType = isJPEG ? kCommodettoBitmapJPEG : imageType;
	camera->swap16 = (!isJPEG && (imageType == kCommodettoBitmapRGB565BE));
	camera->fd = -1;
	camera->ispFd = -1;
#if !MODDEF_CAMERA_I2C_OWN
	camera->sccbBus = sccbBus;
#endif
	camera->state = kStateInitializing;
	xsRemember(camera->object);
	camera->onReadable = builtinGetCallback(the, xsID_onReadable);
	builtinInitializeTarget(the);

	xsmcGet(xsVar(0), xsArg(0), xsID_prototype);
	camera->hostBufferPrototype = xsmcToReference(xsVar(0));

	camera->mutex = xSemaphoreCreateMutex();
	if (!camera->mutex)
		xsUnknownError("not enough memory");

	xTaskCreate(cameraLoop, "camera", 12 * 1024 + XT_STACK_EXTRA_CLIB, camera, 10, &camera->task);
	if (!camera->task)
		xsUnknownError("not enough memory");

	while (kStateInitializing == camera->state)
		vTaskDelay(1);

	if (camera->initErr)
		xsUnknownError((char *)(camera->failReason ? camera->failReason : "camera init failed"));
}

void xs_camera_destructor(void *it)
{
	Camera camera = it;

	if (!camera)
		return;

	camera->closed = 1;
	cameraJoin(camera);
	if (camera->mutex) {
		vSemaphoreDelete(camera->mutex);
		camera->mutex = NULL;
	}
	c_free(camera);
}

void xs_camera_close(xsMachine *the)
{
	Camera camera = xsmcGetHostData(xsThis);
	uint8_t inCallback;

	if (!camera || !xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks))
		return;
	if (camera->closed)
		return;

	// Release the sensor before returning so the caller can open another camera.
	// A frame callback already in the queue frees this record when it runs.
	xSemaphoreTake(camera->mutex, portMAX_DELAY);
	camera->closed = 1;
	camera->state = kStateClosing;
	inCallback = camera->inCallback;
	xSemaphoreGive(camera->mutex);

	cameraDetach(the, camera);
	cameraJoin(camera);

	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
	xsForget(camera->object);

	if (!inCallback)
		cameraRelease(camera);
}

void xs_camera_mark(xsMachine *the, void *it, xsMarkRoot markRoot)
{
	Camera camera = it;
	int i;

	if (camera->onReadable)
		(*markRoot)(the, camera->onReadable);
	if (camera->hostBufferPrototype)
		(*markRoot)(the, camera->hostBufferPrototype);

	for (i = 0; i < kCameraFrameCount; i++) {
		if (camera->frames[i].hostBuffer)
			(*markRoot)(the, camera->frames[i].hostBuffer);
	}
}

static void cameraRequeue(Camera camera, int index)
{
	struct v4l2_buffer buf;

	if (!camera->streaming)
		return;

	memset(&buf, 0, sizeof(buf));
	buf.type = kCameraType;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;
	ioctl(camera->fd, VIDIOC_QBUF, &buf);
}

void xs_camera_read(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	CameraFrame frame = NULL;
	int index = -1;
	int i;

	xSemaphoreTake(camera->mutex, portMAX_DELAY);

	for (i = 0; i < kCameraFrameCount; i++) {
		if (kCameraStateReady != camera->frames[i].state)
			continue;
		if ((NULL == frame) || (frame->id > camera->frames[i].id)) {
			frame = &camera->frames[i];
			index = i;
		}
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
		xSemaphoreGive(camera->mutex);
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
		else
			xsmcSetArrayBuffer(xsResult, frame->data, frame->dataLength);

		frame->hostBuffer = NULL;
		frame->data = NULL;
		frame->state = kCameraStateFree;
		cameraRequeue(camera, index);
		xSemaphoreGive(camera->mutex);
	}
}

void xs_camera_start(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	int i;

	camera->failReason = NULL;
	camera->startDone = 0;
	xTaskNotify(camera->task, kStateRunning, eSetValueWithOverwrite);
	for (i = 0; i < 10; i++) {
		if (camera->streaming || camera->startDone)
			break;
		modDelayMilliseconds(10);
	}
	if (!camera->streaming)
		xsUnknownError((char *)(camera->failReason ? camera->failReason : "stream timeout"));
}

void xs_camera_stop(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);

	xTaskNotify(camera->task, kStateStopping, eSetValueWithOverwrite);
	while ((kStateIdle != camera->state) && (kStateTerminated != camera->state))
		modDelayMilliseconds(1);

	cameraDetach(the, camera);
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
		xsmcSetStringX(xsResult, "jpeg");
	else
		xsmcSetInteger(xsResult, camera->imageType);
}

void xs_camera_get_width(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->width);
}

void xs_camera_get_height(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->height);
}

void xs_camera_get_identification(xsMachine *the)
{
	xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcVars(1);
	xsmcSetNewObject(xsResult);
	xsmcSetString(xsVar(0), MODDEF_CAMERA_MODEL);
	xsmcSet(xsResult, xsID_model, xsVar(0));
}

static void cameraSetOne(xsMachine *the, Camera camera, int index, xsIdentifier id)
{
	xsSlot slot;
	int value;

	if (!camera->controls[index].supported)
		return;
	if (!xsmcGet(slot, xsArg(0), id))
		return;

	value = cameraClampPercent(xsmcToInteger(slot));
	cameraSetControl(camera, index, value);
}

void xs_camera_get_configuration(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsSlot slot;

	xsmcSetNewObject(xsResult);
	if (camera->controls[kCameraBrightness].supported) {
		xsmcSetInteger(slot, camera->controls[kCameraBrightness].value);
		xsmcSet(xsResult, xsID_brightness, slot);
	}
	if (camera->controls[kCameraContrast].supported) {
		xsmcSetInteger(slot, camera->controls[kCameraContrast].value);
		xsmcSet(xsResult, xsID_contrast, slot);
	}
	if (camera->controls[kCameraSaturation].supported) {
		xsmcSetInteger(slot, camera->controls[kCameraSaturation].value);
		xsmcSet(xsResult, xsID_saturation, slot);
	}
}

void xs_camera_configure(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);

	cameraSetOne(the, camera, kCameraBrightness, xsID_brightness);
	cameraSetOne(the, camera, kCameraContrast, xsID_contrast);
	cameraSetOne(the, camera, kCameraSaturation, xsID_saturation);
}

void _xs_disposable_hostbuffer_destructor(void *data)
{
}

void _xs_disposable_hostbuffer_close(xsMachine *the)
{
	void *buffer = xsmcGetHostData(xsThis);
	Camera camera;
	int i;

	if (!buffer)
		return;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsThis, xsID_camera);
	camera = xsmcGetHostDataValidate(xsVar(0), (void *)&xsCameraHooks);

	xSemaphoreTake(camera->mutex, portMAX_DELAY);
	for (i = 0; i < kCameraFrameCount; i++) {
		if (camera->frames[i].data != buffer)
			continue;
		camera->frames[i].state = kCameraStateFree;
		camera->frames[i].data = NULL;
		camera->frames[i].dataLength = 0;
		camera->frames[i].hostBuffer = NULL;
		cameraRequeue(camera, i);
		xSemaphoreGive(camera->mutex);
		xsmcSetHostBuffer(xsThis, NULL, 0);
		return;
	}

	xSemaphoreGive(camera->mutex);
	xsUnknownError("unknown buffer");
}
