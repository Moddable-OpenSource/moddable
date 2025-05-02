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
#include "commodettoBitmapFormat.h"
#include "builtinCommon.h"
#include "commodettoConvert.h"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

static int xioctl(int fd, int op, void *arg) {
  int r;
  do {
    r = ioctl(fd, op, arg);
  } while (-1 == r && EINTR == errno);
  return r;
}

typedef struct CameraBufferRecord CameraBufferRecord;
typedef struct CameraBufferRecord *CameraBuffer;

struct CameraBufferRecord {
	CameraBuffer nextBuffer;
	uint32_t index;
	uint32_t size;
	uint8_t* data;
};

typedef struct CameraRecord CameraRecord;
typedef struct CameraRecord *Camera;

struct CameraRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onReadable;
	int fd;
	
	CameraBuffer mainBuffer;
	GMainContext* mainContext;
	GMutex mainMutex;
	GSource* mainSource;
	
	GThread* thread;
	CameraBuffer threadBuffer;
	GCond threadCondition;
	GMutex threadMutex;

	uint8_t calling;
	uint8_t destructing;
	uint8_t running;
	
	uint8_t format;
	uint32_t width;
	uint32_t height;
	uint32_t imageType;
	
	CommodettoConverter yuvToRGB;
	
	uint32_t size;
	uint8_t* data;
	
	uint16_t queueLength;
	CameraBufferRecord queueBuffers[1];
};

static void xs_camera_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks xsCameraHooks = {
	xs_camera_destructor,
	xs_camera_mark,
	NULL
	
};

CameraBuffer xs_camera_dequeueBuffer(CameraBuffer* address)
{
	CameraBuffer result = *address;
	if (result) {
		*address = result->nextBuffer;
		result->nextBuffer = C_NULL;
	}
	return result;
}

void xs_camera_enqueueBuffer(CameraBuffer* address, CameraBuffer buffer)
{
	CameraBuffer former;
	while ((former = *address))
		address = &(former->nextBuffer);
	*address = buffer;
}

gboolean xs_camera_releaseBuffer(Camera camera, void* data)
{
	int i;
	CameraBuffer buffer;
	for (i = 0; i < camera->queueLength; i++) {
		buffer = &camera->queueBuffers[i];
		if (buffer->data == data) {
			break;
		}
	}
	if (i == camera->queueLength)
		return 0;
	g_mutex_lock(&(camera->threadMutex));
	if (camera->threadBuffer == NULL) {
		camera->threadBuffer = buffer;
		g_cond_signal(&(camera->threadCondition));
	}
	else
		xs_camera_enqueueBuffer(&(camera->threadBuffer), buffer);
	g_mutex_unlock(&(camera->threadMutex));
	return 1;
}

gboolean xs_camera_callback(void *it)
{
	Camera camera = it;
	CameraBuffer buffer;
	
	g_mutex_lock(&(camera->mainMutex));
	buffer = camera->mainBuffer;
	camera->mainBuffer = C_NULL;
	g_source_unref(camera->mainSource);
	camera->mainSource = C_NULL;
	g_mutex_unlock(&(camera->mainMutex));
	while (buffer) {
		CameraBuffer nextBuffer = buffer->nextBuffer;
		buffer->nextBuffer = C_NULL;
		camera->data = buffer->data;
		camera->size = buffer->size;
		if (camera->onReadable && camera->running && (camera->size > 0)) {
			camera->calling = 1;
			xsBeginHost(camera->the);
			xsResult = xsAccess(camera->object);
			xsCallFunction1(xsReference(camera->onReadable), xsResult, xsInteger(camera->size));
			xsEndHost(camera->the);
			if (camera->calling)
				camera->calling = 0;
			else {
				xs_camera_destructor(camera);
				return G_SOURCE_REMOVE;
			}
		}
		else
			xs_camera_releaseBuffer(camera, camera->data);
		camera->size = 0;
		camera->data = NULL;
		buffer = nextBuffer;
	}
	return G_SOURCE_REMOVE;
}

static gpointer xs_camera_loop(gpointer it)
{
	Camera camera = it;
	int fd = camera->fd;
	enum v4l2_buf_type type;
	CameraBuffer buffer;
 	struct v4l2_buffer buf;
	fd_set fds;
    struct timeval tv;
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		fprintf(stderr, "VIDIOC_STREAMON: %s\n", strerror(errno));
	
	for (;;) {
		g_mutex_lock(&(camera->threadMutex));
		while ((camera->threadBuffer == NULL) && (camera->destructing == 0))
			g_cond_wait(&(camera->threadCondition), &(camera->threadMutex));
		buffer = xs_camera_dequeueBuffer(&(camera->threadBuffer));
		g_mutex_unlock(&(camera->threadMutex));
		if (camera->destructing)
			break;
			
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = buffer->index;
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			fprintf(stderr, "%s\n", strerror(errno));

	again:	
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		int result = select(fd + 1, &fds, NULL, NULL, &tv);
		if (-1 == result) {
			if (EINTR == errno)
				goto again;
 			fprintf(stderr, "select: %s\n", strerror(errno));
			break;
		}
		else if (0 == result) {
 			fprintf(stderr, "select: timeout\n");
			break;
		}
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        	if (EAGAIN == errno)
				goto again;
 			fprintf(stderr, "VIDIOC_DQBUF: %s\n", strerror(errno));
          	break;
        }
        
        buffer = &(camera->queueBuffers[buf.index]);
        if (camera->yuvToRGB)
			(camera->yuvToRGB)(camera->width * camera->height, buffer->data, buffer->data, NULL);
			
		g_mutex_lock(&(camera->mainMutex));
		if (camera->mainBuffer == C_NULL) {
			camera->mainBuffer = buffer;
			camera->mainSource = g_idle_source_new();
			g_source_set_callback(camera->mainSource, xs_camera_callback, camera, NULL);
			g_source_set_priority(camera->mainSource, G_PRIORITY_DEFAULT_IDLE);
			g_source_attach(camera->mainSource, camera->mainContext);
		}
		else {
			xs_camera_enqueueBuffer(&(camera->mainBuffer), buffer);
		}
		g_mutex_unlock(&(camera->mainMutex));
	}
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
		fprintf(stderr, "VIDIOC_STREAMOFF %s\n", strerror(errno));
		
    return 0;
}

void xs_camera_buffer_close(xsMachine *the)
{
	void *data = xsmcGetHostData(xsThis);
	if (!data)
		return;
	xsmcVars(1);
	xsmcGet(xsVar(0), xsThis, xsID_camera);
    Camera camera = (Camera)xsmcGetHostDataValidate(xsVar(0), (void *)&xsCameraHooks);
	if (!xs_camera_releaseBuffer(camera, data))
		xsUnknownError("unknown buffer");
	xsmcSetHostBuffer(xsThis, NULL, 0);
}

void xs_camera_constructor(xsMachine *the)
{
	uint8_t format = kIOFormatBuffer;
	uint32_t width = 320;
	uint32_t height = 240;
	uint32_t imageType = kCommodettoBitmapRGB565LE;
	CommodettoConverter yuvToRGB = NULL;
	uint16_t queueLength = 3;
	
  	int fd;
	struct v4l2_capability caps = {0};
	struct v4l2_frmsizeenum frmsizeenum = {0};
	struct v4l2_format fmt = {0};
	struct v4l2_requestbuffers requestbuffers = {0};
  	struct v4l2_buffer buf;
  	
	Camera camera = NULL;
  	int index;
	xsmcVars(1);

	xsTry {
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
			imageType = xsmcToInteger(xsVar(0));
		}
		if (imageType == kCommodettoBitmapRGB565LE)
			yuvToRGB = CommodettoPixelsConverterGet(kCommodettoBitmapYUV422, kCommodettoBitmapRGB565LE);
		else if (imageType != kCommodettoBitmapYUV422)
			xsRangeError("invalid imageType");
		
		fd = open("/dev/video0", O_RDWR);
		if (fd < 0)
			xsUnknownError("open: %s", strerror(errno));
		if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &caps))
			xsUnknownError("VIDIOC_QUERYCAP: %s", strerror(errno));
	    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
			xsUnknownError("not a video capture device");
	    if (!(caps.capabilities & V4L2_CAP_STREAMING))
			xsUnknownError("not a streaming device");
			
		uint32_t minWidth = 0xFFFFFFFF;
		uint32_t minHeight = 0xFFFFFFFF;
		frmsizeenum.index = 0;
		frmsizeenum.pixel_format = V4L2_PIX_FMT_YUYV;
		while (xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum) != -1) {
			if (frmsizeenum.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				if ((width <= frmsizeenum.discrete.width) && (height <= frmsizeenum.discrete.height)) {
					if ((minWidth > frmsizeenum.discrete.width) || (minHeight > frmsizeenum.discrete.height)) {
						minWidth = frmsizeenum.discrete.width;
						minHeight = frmsizeenum.discrete.height;
					}
				}
			}
			frmsizeenum.index++;
		}
		if (minWidth == 0xFFFFFFFF)
			xsUnknownError("YUYV not found");
		width = minWidth;
		height = minHeight;
		
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.fmt.pix.width = width;
		fmt.fmt.pix.height = height;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		fmt.fmt.pix.field = V4L2_FIELD_NONE;
		if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt))
			xsUnknownError("VIDIOC_S_FMT: %s", strerror(errno));
		width = fmt.fmt.pix.width;
		height = fmt.fmt.pix.height;
		
		requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		requestbuffers.memory = V4L2_MEMORY_MMAP;
		requestbuffers.count = queueLength;
		if (-1 == xioctl(fd, VIDIOC_REQBUFS, &requestbuffers))
			xsUnknownError("VIDIOC_REQBUFS: %s", strerror(errno));
		queueLength = requestbuffers.count;

		camera = (Camera)c_calloc(1, sizeof(CameraRecord) + ((queueLength - 1) * sizeof(CameraBufferRecord)));
		if (!camera)
			xsRangeError("not enough memory");
		camera->fd = fd;
		camera->format = format;
		camera->width = width;
		camera->height = height;
		camera->imageType = imageType;
		camera->yuvToRGB = yuvToRGB;
		
		camera->queueLength = queueLength;
		for (index = 0; index < queueLength; index++) {
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index  = index;
			if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
				xsUnknownError("VIDIOC_QUERYBUF: %s", strerror(errno));
			camera->queueBuffers[index].data = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
			if (MAP_FAILED == camera->queueBuffers[index].data) {
				camera->queueBuffers[index].data = NULL;
				xsUnknownError("mmap: %s", strerror(errno));
			}
			camera->queueBuffers[index].index = index;
			camera->queueBuffers[index].size = buf.length;
			xs_camera_enqueueBuffer(&(camera->threadBuffer), &(camera->queueBuffers[index]));
		}
		
		xsmcSetHostData(xsThis, camera);
		xsSetHostHooks(xsThis, (xsHostHooks *)&xsCameraHooks);
		camera->the = the;
		camera->object = xsThis;
		xsRemember(camera->object);
		camera->onReadable = builtinGetCallback(the, xsID_onReadable);	
		builtinInitializeTarget(the);
	
		camera->mainContext = g_main_context_get_thread_default();
		g_mutex_init(&(camera->mainMutex));
		g_cond_init(&(camera->threadCondition));
		g_mutex_init(&(camera->threadMutex));
			
		camera->thread = g_thread_new("camera", xs_camera_loop, camera);
	}
	xsCatch {
		xs_camera_destructor(camera);
		xsThrow(xsException);
	}
}

void xs_camera_destructor(void *it)
{
	if (it) {
		Camera camera = (Camera)it;
		int i;
		g_mutex_lock(&(camera->threadMutex));
		camera->destructing = 1;
		g_cond_signal(&(camera->threadCondition));
		g_mutex_unlock(&(camera->threadMutex));
		g_thread_join(camera->thread);
		if (camera->mainSource)
			g_source_destroy(camera->mainSource);
		for (i = 0; i < camera->queueLength; i++) {
			if (camera->queueBuffers[i].data)
				munmap(camera->queueBuffers[i].data, camera->queueBuffers[i].size);
		}
		g_mutex_clear(&(camera->threadMutex));
		g_cond_clear(&(camera->threadCondition));
		g_mutex_clear(&(camera->mainMutex));
		c_free(camera);
	}
}

void xs_camera_close(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostData(xsThis);
	if (camera && xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks)) {
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
		xsForget(camera->object);
		if (camera->calling)
			camera->calling = 0;
		else
			xs_camera_destructor(camera);
	}
}

void xs_camera_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	Camera camera = (Camera)it;
	if (camera->onReadable)
		(*markRoot)(the, camera->onReadable);
}

void xs_camera_read(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	if (camera->data == NULL)
		return;
	if (camera->format == kIOFormatBuffer) {
		if ((xsmcArgc > 0) && (xsReferenceType == xsmcTypeOf(xsArg(0)))) {
			void *data;
			xsUnsignedValue size;
			xsmcGetBufferWritable(xsArg(0), &data, &size);
			if (size < camera->size) {
				xsRangeError("buffer too small");
			}
			xsmcSetInteger(xsResult, camera->size);
			memcpy(data, camera->data, camera->size);
		}
		else {
			xsmcSetArrayBuffer(xsResult, camera->data, camera->size);
		}
		xs_camera_releaseBuffer(camera, camera->data);
	}
	else {
		xsmcVars(1);
		xsResult = xsNewHostObject(C_NULL);
		xsVar(0) = xsNewHostFunction(xs_camera_buffer_close, 0);
		xsmcDefine(xsResult, xsID_close, xsVar(0), xsDefault);
		xsmcDefine(xsResult, xsID_camera, xsThis, xsDefault);
		xsmcSetHostBuffer(xsResult, camera->data, camera->size);
	}
}

void xs_camera_start(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	camera->running = 1;
}

void xs_camera_stop(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	camera->running = 0;
}

void xs_camera_get_format(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	builtinGetFormat(the, camera->format);
}

void xs_camera_set_format(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	uint8_t format = builtinSetFormat(the);
	if ((kIOFormatBuffer != format) && (kIOFormatBufferDisposable != format))
		xsRangeError("invalid format");
	camera->format = format;
}

void xs_camera_get_width(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->width);
}

void xs_camera_get_height(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->height);
}

void xs_camera_get_imageType(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->imageType);
}
