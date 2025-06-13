#include "xsmc.h"

static void encode(uint8_t *out, const int16_t *in, size_t len)
{
    uint8_t alaw = 0;
    int16_t pcm = 0;
    int32_t sign = 0;
    int32_t abcd = 0;
    int32_t eee = 0;
    int32_t mask = 0;
    for (size_t i = 0; i < len; i++)
    {
        pcm = *in++;
        /* 0-7 kinds of quantization level from the table above */
        eee = 7;
        mask = 0x4000; /* 0x4000: '0b0100 0000 0000 0000' */

        /* Get sign bit */
        sign = (pcm & 0x8000) >> 8;

        /* Turn negative pcm to positive */
        /* The absolute value of a negative number may be larger than the size
         * of the corresponding positive number, so here needs `-pcm -1` after
         * taking the opposite number. */
        pcm = sign ? (-pcm - 1) : pcm;

        /* Get eee and abcd bit */
        /* Use mask to locate the first `1` bit and quantization level at the
         * same time */
        while ((pcm & mask) == 0 && eee > 0)
        {
            eee--;
            mask >>= 1;
        }

        /* The location of abcd bits is related with quantization level. Check
         * the table above to determine how many bits to `>>` to get abcd */
        abcd = (pcm >> (eee ? (eee + 3) : 4)) & 0x0f;

        /* Put the quantization level number at right bit location to get eee
         * bits */
        eee <<= 4;

        /* Splice results */
        alaw = (sign | eee | abcd);

        /* The standard specifies that all resulting even bits (LSB
         * is even) are inverted before the octet is transmitted. This is to
         * provide plenty of 0/1 transitions to facilitate the clock recovery
         * process in the PCM receivers. Thus, a silent A-law encoded PCM
         * channel has the 8 bit samples coded 0xD5 instead of 0x80 in the
         * octets. (Reference from wiki above) */
        *out++ = alaw ^ 0xD5;
    }
}

void xs_alaw_encode(xsMachine *the)
{
	const int16_t *src;
	uint8_t *dst;
	xsUnsignedValue srcLength, dstLength;

	xsmcGetBufferReadable(xsArg(0), (void **)&src, &srcLength);
	xsmcGetBufferWritable(xsArg(1), (void **)&dst, &dstLength);
	if ((dstLength < (srcLength >> 1)) || (srcLength & 1))
		xsUnknownError("invalid");

	encode(dst, src, srcLength >> 1);
}
