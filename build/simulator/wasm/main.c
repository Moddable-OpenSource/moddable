#include <stdlib.h>
#include <string.h>
#include "../screen.h"

extern void fxScreenLaunch(txScreen* screen);

static void fxScreenAbort(txScreen* screen);
static void fxScreenBufferChanged(txScreen* screen);
static void fxScreenFormatChanged(txScreen* screen);
static void fxScreenStart(txScreen* screen, double interval);
static void fxScreenStop(txScreen* screen);

static txScreen* gxScreen = NULL;

void fxScreenAbort(txScreen* screen)
{
}

void fxScreenBufferChanged(txScreen* screen)
{
}

void fxScreenFormatChanged(txScreen* screen)
{
}

int fxScreenIdle()
{
	if (gxScreen->idle) 
		(*gxScreen->idle)(gxScreen);
	return 0;
}

void* fxScreenMain(int width, int height) 
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

void fxScreenStart(txScreen* screen, double interval)
{
}

void fxScreenStop(txScreen* screen)
{
}

