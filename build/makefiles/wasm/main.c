#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "screen.h"
#include "xsCommon.h"

extern void fxScreenLaunch(txScreen* screen);

static void fxScreenAbort(txScreen* screen, int status);
static void fxScreenBufferChanged(txScreen* screen);
static void fxScreenFormatChanged(txScreen* screen);
static void fxScreenStart(txScreen* screen, double interval);
static void fxScreenStop(txScreen* screen);

static txScreen* gxScreen = NULL;

int fxMainIdle()
{
	if (gxScreen->idle) 
		(*gxScreen->idle)(gxScreen);
	return 0;
}

void* fxMainLaunch(int width, int height, void* archive) 
{
	gxScreen = (txScreen*)malloc(sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
	memset(gxScreen, 0, sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
	gxScreen->abort = fxScreenAbort;
	gxScreen->bufferChanged = fxScreenBufferChanged;
	gxScreen->formatChanged = fxScreenFormatChanged;
	gxScreen->start = fxScreenStart;
	gxScreen->stop = fxScreenStop;
	gxScreen->width = width;
	gxScreen->height = height;
	gxScreen->archive = archive;
	fxScreenLaunch(gxScreen);
	return gxScreen->buffer;
}

int fxMainTouch(int kind, int index, int x, int y, double when)
{
	if (gxScreen->touch) 
		(*gxScreen->touch)(gxScreen, kind, index, x, y, when);
	return 0;
}

int fxMainQuit() 
{
	if (gxScreen->quit) 
		(*gxScreen->quit)(gxScreen);
	if (gxScreen->archive) 
		free(gxScreen->archive);
	free(gxScreen);
	gxScreen = NULL;
	return 0;
}

void fxScreenAbort(txScreen* screen, int status)
{
}

void fxScreenBufferChanged(txScreen* screen)
{
  EM_ASM({
    gxView.onBufferChanged();
  });
}

void fxScreenFormatChanged(txScreen* screen)
{
  EM_ASM({
     gxView.onFormatChanged($0, $1, $2, $3);
  }, screen->pixelFormat, XS_MAJOR_VERSION, XS_MINOR_VERSION, XS_PATCH_VERSION);
}

void fxScreenStart(txScreen* screen, double interval)
{
  EM_ASM({
     gxView.onStart($0);
  }, interval);
}

void fxScreenStop(txScreen* screen)
{
  EM_ASM({
     gxView.onStop();
  });
}
