#include "mc.xs.h"

extern int dvi_adpcm_encode(void *in_buf, int in_size, void *out_buf, int *out_size);

void Tool_compressIMA(xsMachine* the)
{
	size_t inputBufferBytes = xsToInteger(xsArg(1)) << 1;
	void *uncompressedSamples = xsToArrayBuffer(xsArg(0));
	int outputSize;
	unsigned char imaBuffer[256];

	if ((inputBufferBytes >> 1) > sizeof(imaBuffer))		// very conservative check
		xsUnknownError("too many samples");

	dvi_adpcm_encode(uncompressedSamples, inputBufferBytes, imaBuffer, &outputSize);

	xsResult = xsArrayBuffer(imaBuffer, outputSize);
}
