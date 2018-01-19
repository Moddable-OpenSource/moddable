/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>

typedef struct sxScreen txScreen;

typedef void (*txScreenAbortProc)(txScreen* screen);
typedef void (*txScreenBufferChangedProc)(txScreen* screen);
typedef int (*txScreenCreateWorkerProc)(txScreen* screen, char* name);
typedef void (*txScreenFormatChangedProc)(txScreen* screen);
typedef void (*txScreenDeleteWorkerProc)(txScreen* screen, int id);
typedef void (*txScreenIdleProc)(txScreen* screen);
typedef void (*txScreenLaunchProc)(txScreen* screen);
typedef void (*txScreenMessageProc)(txScreen* screen, char* buffer, int size);
typedef void (*txScreenQuitProc)(txScreen* screen);
typedef void (*txScreenStartProc)(txScreen* screen, double interval);
typedef void (*txScreenStopProc)(txScreen* screen);
typedef void (*txScreenTouchProc)(txScreen* screen, int kind, int index, int x, int y, double when);

#define screenBytesPerPixel 4

struct sxScreen {
	void* machine;
	void* view;
	void* archive;
	void* firstWorker;
	txScreenAbortProc abort;
	txScreenBufferChangedProc bufferChanged;
	txScreenCreateWorkerProc createWorker;
	txScreenDeleteWorkerProc deleteWorker;
	txScreenFormatChangedProc formatChanged;
	txScreenIdleProc idle;
	txScreenMessageProc invoke;
	txScreenMessageProc post;
	txScreenQuitProc quit;
	txScreenStartProc start;
	txScreenStopProc stop;
	txScreenTouchProc touch;
	int flags;
	long instrumentTime;
	int pixelFormat;
	int width;
	int height;
	uint16_t *clut;
	uint8_t palette[16 * screenBytesPerPixel];
	void *frameBuffer;		// only used when kPocoFrameBuffer
	uint32_t frameBufferLength;		// only used when kPocoFrameBuffer
	unsigned char *rowAddress;
	int rowCount;
	int rowDelta;
	int rowIndex;
	unsigned char buffer[1];
};

enum {
	rgb565le = 0,
	rgb565be,
	gray8,
	rgb332,
	gray4,
	clut4,
	pixelFormatCount
};

enum {
	touchEventBeganKind = 0,
	touchEventCancelledKind,
	touchEventEndedKind,
	touchEventMovedKind,
};


