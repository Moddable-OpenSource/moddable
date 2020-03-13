#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "../screen.h"

extern void fxScreenLaunch(txScreen* screen);

static void fxScreenAbort(txScreen* screen);
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

void* fxMainLaunch(int width, int height) 
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
	free(gxScreen);
	gxScreen = NULL;
	return 0;
}

void fxScreenAbort(txScreen* screen)
{
}

void fxScreenBufferChanged(txScreen* screen)
{
  EM_ASM({
    onBufferChanged();
  });
}

void fxScreenFormatChanged(txScreen* screen)
{
  EM_ASM({
    onFormatChanged($0);
  }, screen->pixelFormat);
}

void fxScreenStart(txScreen* screen, double interval)
{
  EM_ASM({
    onStart($0);
  }, interval);
}

void fxScreenStop(txScreen* screen)
{
  EM_ASM({
    onStop();
  });
}
