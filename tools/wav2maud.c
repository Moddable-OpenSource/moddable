#include "mc.xs.h"
#include "adpcm-lib.h"

void Tool_compressIMA(xsMachine* the)
{
	static void *context;
	size_t inputBufferBytes = xsToInteger(xsArg(1)) << 1;
	void *uncompressedSamples = xsToArrayBuffer(xsArg(0));
	size_t outputSize = 0;
	uint8_t imaBuffer[256];

	if ((inputBufferBytes >> 1) > sizeof(imaBuffer))		// very conservative check
		xsUnknownError("too many samples");

	if (!context) {
		const int lookahead = 3;
		const int noise_shaping = NOISE_SHAPING_DYNAMIC;
		const int32_t average_deltas [2] = {0, 0};
		context = adpcm_create_context(1, lookahead, noise_shaping, (int32_t *)average_deltas);
	}

	adpcm_encode_block(context, imaBuffer, &outputSize, uncompressedSamples, inputBufferBytes >> 1);

	xsResult = xsArrayBuffer(imaBuffer, outputSize);
}
