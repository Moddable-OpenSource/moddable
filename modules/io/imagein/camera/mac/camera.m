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

#include <AVFoundation/AVFoundation.h>
#include <Accelerate/Accelerate.h>

typedef struct CameraRecord CameraRecord;
typedef struct CameraRecord *Camera;

@interface CameraDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property (assign) Camera camera;
- (void) captureOutput: (AVCaptureOutput*) output didOutputSampleBuffer: (CMSampleBufferRef) buffer fromConnection: (AVCaptureConnection*) connection;
- (void) captureOutputMain: (id)object;
@end
@interface CameraDelegate()
{
}
@end

typedef struct CameraBufferRecord CameraBufferRecord;
typedef struct CameraBufferRecord *CameraBuffer;

struct CameraBufferRecord {
	CameraBuffer nextBuffer;
	CVImageBufferRef pixelBuffer;
	uint32_t size;
	uint8_t* data;
};

struct CameraRecord {
	xsMachine *the;
	xsSlot object;
	xsSlot *onReadable;
	
    CameraDelegate *delegate;
    dispatch_queue_t dispatch_queue;
	AVCaptureSession *session;
    
	CameraBuffer mainBuffer;
	pthread_mutex_t mainMutex;
    NSThread *mainThread;
	
	CameraBuffer threadBuffer;
	pthread_cond_t threadCondition;
	pthread_mutex_t threadMutex;
    
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

xsBooleanValue xs_camera_releaseBuffer(Camera camera, void* data)
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
	pthread_mutex_lock(&(camera->threadMutex));
	if (camera->threadBuffer == NULL) {
		camera->threadBuffer = buffer;
		pthread_cond_signal(&(camera->threadCondition));
	}
	else
		xs_camera_enqueueBuffer(&(camera->threadBuffer), buffer);
	pthread_mutex_unlock(&(camera->threadMutex));
	return 1;
}

@implementation CameraDelegate
- (id) init {
	self = [super init];
	return self;
}
- (void) dealloc {
	[super dealloc];
}
- (void) captureOutputMain: (id)object
{
	Camera camera = self.camera;
	CameraBuffer buffer;
	
	pthread_mutex_lock(&(camera->mainMutex));
	buffer = camera->mainBuffer;
	camera->mainBuffer = C_NULL;
	pthread_mutex_unlock(&(camera->mainMutex));
	while (buffer) {
		CameraBuffer nextBuffer = buffer->nextBuffer;
		buffer->nextBuffer = C_NULL;
		camera->data = buffer->data;
		camera->size = buffer->size;
		if (camera->onReadable && camera->running) {
			camera->calling = 1;
			xsBeginHost(camera->the);
			xsResult = xsAccess(camera->object);
			xsCallFunction1(xsReference(camera->onReadable), xsResult, xsInteger(camera->size));
			xsEndHost(camera->the);
			if (camera->calling)
				camera->calling = 0;
			else
				xs_camera_destructor(camera);
		}
		else
			xs_camera_releaseBuffer(camera, camera->data);
		camera->size = 0;
		camera->data = NULL;
		buffer = nextBuffer;
	}
}
- (void) captureOutput: (AVCaptureOutput*) output didOutputSampleBuffer: (CMSampleBufferRef)sampleBuffer fromConnection: (AVCaptureConnection*) connection 
{
#pragma unused (output)
#pragma unused (connection)
	Camera camera = self.camera;
	CameraBuffer buffer;
	
	pthread_mutex_lock(&(camera->threadMutex));
	while ((camera->threadBuffer == NULL) && (camera->destructing == 0))
		pthread_cond_wait(&(camera->threadCondition), &(camera->threadMutex));
	buffer = xs_camera_dequeueBuffer(&(camera->threadBuffer));
	pthread_mutex_unlock(&(camera->threadMutex));
	if (camera->destructing)
		return;
		
	if (buffer->pixelBuffer) {
		CVPixelBufferUnlockBaseAddress(buffer->pixelBuffer, 0);
		CVBufferRelease(buffer->pixelBuffer);
		buffer->pixelBuffer = NULL;
		buffer->data = NULL;
		buffer->size = 0;
	}
	
	CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
	CVBufferRetain(pixelBuffer);
	CVPixelBufferLockBaseAddress(pixelBuffer, 0);
	buffer->pixelBuffer = pixelBuffer;
	buffer->data = CVPixelBufferGetBaseAddress(pixelBuffer);
	buffer->size = CVPixelBufferGetDataSize(pixelBuffer);
	if (camera->yuvToRGB)
		(camera->yuvToRGB)(camera->width * camera->height, buffer->data, buffer->data, NULL);
		
	pthread_mutex_lock(&(camera->mainMutex));
	if (camera->mainBuffer == C_NULL) {
		camera->mainBuffer = buffer;
		[self performSelector:@selector(captureOutputMain:) onThread:camera->mainThread withObject:NULL waitUntilDone:NO];
	}
	else
		xs_camera_enqueueBuffer(&(camera->mainBuffer), buffer);
	pthread_mutex_unlock(&(camera->mainMutex));
}
@end

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
	int32_t width = 320;
	int32_t height = 240;
	uint8_t format = kIOFormatBuffer;
	uint8_t imageType = kCommodettoBitmapRGB565LE;
	CommodettoConverter yuvToRGB = NULL;
	uint16_t queueLength = 3;
	
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
	
		AVCaptureDevice *device = [[AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo] autorelease];
		if (!device)
			xsUnknownError("no camera");
		
		int32_t minWidth = 0x7FFFFFFF;
		int32_t minHeight = 0x7FFFFFFF;
		AVCaptureDeviceFormat* minFormat = NULL;
		NSArray<AVCaptureDeviceFormat *> *formats = device.formats;
		NSEnumerator *enumerator = [formats objectEnumerator];
		AVCaptureDeviceFormat* deviceFormat;
		while (deviceFormat = [enumerator nextObject]) {
			CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(deviceFormat.formatDescription);
			if ((width <= dimensions.width) && (height <= dimensions.height)) {
				if ((minWidth > dimensions.width) || (minHeight > dimensions.height)) {
					minWidth = dimensions.width;
					minHeight = dimensions.height;
					minFormat = deviceFormat;
				}
			}
		}
		if (minFormat) {
			if ([device lockForConfiguration:NULL]) {
				device.activeFormat = minFormat;
				[device unlockForConfiguration];
			}
		}
		CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(device.activeFormat.formatDescription);
		width = dimensions.width;	
		height = dimensions.height;
		
		AVCaptureSession *session = [[[AVCaptureSession alloc] init] autorelease];
		session.sessionPreset = AVCaptureSessionPresetMedium;
		NSError *error = nil;
		AVCaptureDeviceInput *input = [[AVCaptureDeviceInput deviceInputWithDevice:device error:&error] autorelease];
		if (!input)
			xsUnknownError("cannot create device input");
		[session addInput:input];
		AVCaptureVideoDataOutput *output = [[[AVCaptureVideoDataOutput alloc] init] autorelease];
		if (!output)
			xsUnknownError("cannot create device output");
		output.videoSettings = @{ 
			(NSString *)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_422YpCbCr8_yuvs),
			(NSString *)kCVPixelBufferWidthKey : @(width),
			(NSString *)kCVPixelBufferHeightKey : @(height)
		};
		[session addOutput:output];
	
		camera = (Camera)c_calloc(1, sizeof(CameraRecord) + ((queueLength - 1) * sizeof(CameraBufferRecord)));
		if (!camera)
			xsRangeError("not enough memory");
			
		camera->format = format;
		camera->width = width;
		camera->height = height;
		camera->imageType = imageType;
		camera->yuvToRGB = yuvToRGB;

		camera->queueLength = queueLength;
		for (index = 0; index < queueLength; index++)
			xs_camera_enqueueBuffer(&(camera->threadBuffer), &(camera->queueBuffers[index]));
		 
		camera->delegate = [[CameraDelegate alloc] init];
		if (!camera->delegate)
			xsUnknownError("cannot create delegate");
		camera->delegate.camera = camera;
	
		camera->dispatch_queue = dispatch_queue_create("com.moddable.camera.capture",  DISPATCH_QUEUE_SERIAL);
		if (!camera->dispatch_queue)
			xsUnknownError("cannot create dispatch queue");
		 
		xsmcSetHostData(xsThis, camera);
		xsSetHostHooks(xsThis, (xsHostHooks *)&xsCameraHooks);
		camera->the = the;
		camera->object = xsThis;
		xsRemember(camera->object);
		camera->onReadable = builtinGetCallback(the, xsID_onReadable);	
		builtinInitializeTarget(the);
		
		camera->mainThread = [NSThread currentThread];
		pthread_mutex_init(&(camera->mainMutex), NULL);
		pthread_cond_init(&(camera->threadCondition), NULL);
		pthread_mutex_init(&(camera->threadMutex), NULL);
		
		camera->session = [session retain];
		[output setSampleBufferDelegate:camera->delegate queue:camera->dispatch_queue];
		[session startRunning];
	}
	xsCatch {
		xs_camera_destructor(camera);
		xsThrow(xsException);
	}
}

void xs_camera_destructor(void *it)
{
	if (it) {
		Camera camera = it;
		pthread_mutex_lock(&(camera->threadMutex));
		camera->destructing = 1;
		pthread_cond_signal(&(camera->threadCondition));
		pthread_mutex_unlock(&(camera->threadMutex));
		
      	if (camera->session) {
			[camera->session stopRunning];
   			[camera->session release];
     	}
    	if (camera->dispatch_queue)
			dispatch_release(camera->dispatch_queue);
    	if (camera->delegate)
			[camera->delegate release];
		
		pthread_cond_destroy(&(camera->threadCondition));
		pthread_mutex_destroy(&(camera->threadMutex));
		pthread_mutex_destroy(&(camera->mainMutex));
	
		c_free(camera);
	}
}

void xs_camera_close(xsMachine *the)
{
	Camera camera = xsmcGetHostData(xsThis);
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
	Camera camera = it;
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
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	camera->running = 1;
}

void xs_camera_stop(xsMachine *the)
{
	Camera camera = xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	camera->running = 0;
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

void xs_camera_get_imageType(xsMachine *the)
{
	Camera camera = (Camera)xsmcGetHostDataValidate(xsThis, (void *)&xsCameraHooks);
	xsmcSetInteger(xsResult, camera->imageType);
}
