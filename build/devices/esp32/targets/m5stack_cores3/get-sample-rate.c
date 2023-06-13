#include "piuMC.h"
#include "mc.defines.h"

#ifndef MODDEF_AUDIOOUT_SAMPLERATE
	#define MODDEF_AUDIOOUT_SAMPLERATE (24000)
#endif

void M5CoreS3_getSampleRate(xsMachine* the)
{
	xsResult = xsInteger(MODDEF_AUDIOOUT_SAMPLERATE);
}
