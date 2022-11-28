//
//  sbc_decoder.c
//  sbccodec
//
//  Created by Peter Barrett on 9/9/14.
//  Copyright (c) 2014 Peter Barrett. All rights reserved.
//
//  Modified by Peter Hoddie on 9/15/20 for Moddable SDK
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sbc_decoder.h"

#include "mc.defines.h"

#ifndef MODDEF_AUDIOOUT_BITSPERSAMPLE
	#define MODDEF_AUDIOOUT_BITSPERSAMPLE (16)
#endif
#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 8
	#define OUTPUTSAMPLETYPE int8_t
#elif MODDEF_AUDIOOUT_BITSPERSAMPLE == 16
	#define OUTPUTSAMPLETYPE int16_t
#endif


// about 2300 bytes in thumb2 -Os

//===========================================================================
//===========================================================================
// No joint stereo or 4 subband modes, no crc

uint8_t const SBC_BlockMode[] = {4,8,12,16};

// Appendix B, 12.8 Tables
static const int8_t SBC_offset4[4][4] = {
	{ -1, 0, 0, 0 },
	{ -2, 0, 0, 1 },
	{ -2, 0, 0, 1 },
	{ -2, 0, 0, 1 }
};

// Appendix B, 12.8 Tables
static const int8_t SBC_offset8[4][8] = {
	{ -2, 0, 0, 0, 0, 0, 0, 1 },
	{ -3, 0, 0, 0, 0, 0, 1, 2 },
	{ -4, 0, 0, 0, 0, 0, 1, 2 },
	{ -4, 0, 0, 0, 0, 0, 1, 2 }
};

static const uint32_t SBC_syn_8[] = {
    0x0000B504,0xFFFF4AFB,0xFFFF4AFB,0x0000B504,0x0000B504,0xFFFF4AFB,0xFFFF4AFB,0x0000B504,
    0x00008E39,0xFFFF04EB,0x000031F1,0x0000D4DB,0xFFFF2B24,0xFFFFCE0E,0x0000FB14,0xFFFF71C6,
    0x000061F7,0xFFFF137C,0x0000EC83,0xFFFF9E08,0xFFFF9E08,0x0000EC83,0xFFFF137C,0x000061F7,
    0x000031F1,0xFFFF71C6,0x0000D4DB,0xFFFF04EB,0x0000FB14,0xFFFF2B24,0x00008E39,0xFFFFCE0E,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0xFFFFCE0E,0x00008E39,0xFFFF2B24,0x0000FB14,0xFFFF04EB,0x0000D4DB,0xFFFF71C6,0x000031F1,
    0xFFFF9E08,0x0000EC83,0xFFFF137C,0x000061F7,0x000061F7,0xFFFF137C,0x0000EC83,0xFFFF9E08,
    0xFFFF71C6,0x0000FB14,0xFFFFCE0E,0xFFFF2B24,0x0000D4DB,0x000031F1,0xFFFF04EB,0x00008E39,
    0xFFFF4AFB,0x0000B504,0x0000B504,0xFFFF4AFB,0xFFFF4AFB,0x0000B504,0x0000B504,0xFFFF4AFB,
    0xFFFF2B24,0x000031F1,0x0000FB14,0x00008E39,0xFFFF71C6,0xFFFF04EB,0xFFFFCE0E,0x0000D4DB,
    0xFFFF137C,0xFFFF9E08,0x000061F7,0x0000EC83,0x0000EC83,0x000061F7,0xFFFF9E08,0xFFFF137C,
    0xFFFF04EB,0xFFFF2B24,0xFFFF71C6,0xFFFFCE0E,0x000031F1,0x00008E39,0x0000D4DB,0x0000FB14,
    0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,
    0xFFFF04EB,0xFFFF2B24,0xFFFF71C6,0xFFFFCE0E,0x000031F1,0x00008E39,0x0000D4DB,0x0000FB14,
    0xFFFF137C,0xFFFF9E08,0x000061F7,0x0000EC83,0x0000EC83,0x000061F7,0xFFFF9E08,0xFFFF137C,
    0xFFFF2B24,0x000031F1,0x0000FB14,0x00008E39,0xFFFF71C6,0xFFFF04EB,0xFFFFCE0E,0x0000D4DB,
};

static const uint32_t SBC_proto_8[] = {
    0x00000000,0xFFFFFDF0,0xFFFFFA34,0xFFFFF2C0,0xFFFFBA5E,0xFFFF6984,0x000045A1,0xFFFFF2C0,0x000005CB,0xFFFFFDF0,
    0xFFFFFFD6,0xFFFFFDD8,0xFFFFF7C7,0xFFFFF6EE,0xFFFFAB06,0xFFFF6B1E,0x00003676,0xFFFFF050,0x00000394,0xFFFFFE2C,
    0xFFFFFFA6,0xFFFFFDF5,0xFFFFF54A,0xFFFFFD01,0xFFFF9C15,0xFFFF6FDE,0x00002803,0xFFFFEF63,0x000001B0,0xFFFFFE7C,
    0xFFFFFF6E,0xFFFFFE58,0xFFFFF2F2,0x00000508,0xFFFF8E22,0xFFFF7789,0x00001ABC,0xFFFFEFB6,0x0000002E,0xFFFFFED5,
    0xFFFFFF28,0xFFFFFF13,0xFFFFF0FE,0x00000EFD,0xFFFF81C6,0xFFFF81C6,0x00000EFD,0xFFFFF0FE,0xFFFFFF13,0xFFFFFF28,
    0xFFFFFED5,0x0000002E,0xFFFFEFB6,0x00001ABC,0xFFFF7789,0xFFFF8E22,0x00000508,0xFFFFF2F2,0xFFFFFE58,0xFFFFFF6E,
    0xFFFFFE7C,0x000001B0,0xFFFFEF63,0x00002803,0xFFFF6FDE,0xFFFF9C15,0xFFFFFD01,0xFFFFF54A,0xFFFFFDF5,0xFFFFFFA6,
    0xFFFFFE2C,0x00000394,0xFFFFF050,0x00003676,0xFFFF6B1E,0xFFFFAB06,0xFFFFF6EE,0xFFFFF7C7,0xFFFFFDD8,0xFFFFFFD6,
};

#define UNROLL

// Don't inline otherwise you will run out of arm registers for the snappy unrolled filters
#ifdef _WIN32
__declspec(noinline)
#else
__attribute__ ((noinline))
#endif
static void synthesize8(int32_t* v, uint8_t* offset, int32_t* src, OUTPUTSAMPLETYPE* dst)
{
	int i;
    const int32_t* syn = (const int32_t*)SBC_syn_8;
	for (i = 0; i < 16; i++)    // 128 macs
    {
		if (!offset[i])
        {
			for (int j = 0; j < 9; j++)
				v[j + 160] = v[j];      // Copy to end
            offset[i] = 160;
		}
        int k = --offset[i];
        int s = 0;
#ifdef UNROLL
        s += syn[0] * src[0]; // TODO mla on arm
        s += syn[1] * src[1];
        s += syn[2] * src[2];
        s += syn[3] * src[3];
        s += syn[4] * src[4];
        s += syn[5] * src[5];
        s += syn[6] * src[6];
        s += syn[7] * src[7];
        syn += 8;
#else
        for (int j = 0; j < 8; j++)
            s += *syn++ * src[j];       // Note: Cheeky GCC unrolls these on O3
#endif
		v[k] = s >> 15;
	}
    
    // 8 samples per block
    const int32_t* m = (const int32_t*)SBC_proto_8;
	for (i = 0; i < 8; i++) // 80 macs
    {
        int32_t* p0 = v+offset[i];
        int32_t* p1 = v+offset[(i + 8) & 0xF]+1;
        int s = 0;
#ifdef UNROLL
        s += p0[0] * m[0]; // TODO mla on arm
        s += p1[0] * m[1];
        s += p0[2] * m[2];
        s += p1[2] * m[3];
        s += p0[4] * m[4];
        s += p1[4] * m[5];
        s += p0[6] * m[6];
        s += p1[6] * m[7];
        s += p0[8] * m[8];
        s += p1[8] * m[9];
        m += 10;
#else
        for (int j = 0; j < 10; j += 2)
        {
            s += p0[j] * *m++;  // 10 per sample
            s += p1[j] * *m++;  // TODO MACS, unroll etc Note: Cheeky GCC unrolls these on O3
        }
#endif
        //  Scale and clip
        s >>= 15;
        if (s < -0x7FFF)
            s = -0x7FFF;
        else if (s > 0x7FFF)
            s = 0x7FFF;
#if MODDEF_AUDIOOUT_BITSPERSAMPLE == 8
		s >>= 8;
#endif
		dst[i] = s;
	}
}

//  Appendix B, 12.6.3 Bit Allocation
static void bit_allocation(SBC_Decode* sbc, uint8_t (*scale_factor)[8], int (*bits)[8])
{
    int sf = sbc->frequency;
    int bp = sbc->bitpool;
    int nrof_subbands = sbc->subbands;
    int bitneed[2][8], loudness, max_bitneed, bitcount, slicecount, bitslice;
    int ch, sb;
    
    uint8_t s;
    for (ch = 0; ch < sbc->channels; ch++)
    {
        max_bitneed = 0;
        if (sbc->allocation) {  // SNR
            for (sb = 0; sb < nrof_subbands; sb++)
            {
                s = scale_factor[ch][sb];
                bitneed[ch][sb] = s;
                if (bitneed[ch][sb] > max_bitneed)
                    max_bitneed = bitneed[ch][sb];
            }
        } else {                // Loudness
            for (sb = 0; sb < nrof_subbands; sb++)
            {
                s = scale_factor[ch][sb];
                if (s == 0)
                    bitneed[ch][sb] = -5;
                else {
                    if (nrof_subbands == 4)
                        loudness = s - SBC_offset4[sf][sb];
                    else
                        loudness = s - SBC_offset8[sf][sb];
                    if (loudness > 0)
                        loudness /= 2;
                    bitneed[ch][sb] = loudness;
                }
                if (bitneed[ch][sb] > max_bitneed)
                    max_bitneed = bitneed[ch][sb];
            }
        }
        
        bitcount = 0;
        slicecount = 0;
        bitslice = max_bitneed + 1;
        do {
            bitslice--;
            bitcount += slicecount;
            slicecount = 0;
            for (sb = 0; sb < nrof_subbands; sb++) {
                if ((bitneed[ch][sb] > bitslice + 1) && (bitneed[ch][sb] < bitslice + 16))
                    slicecount++;
                else if (bitneed[ch][sb] == bitslice + 1)
                    slicecount += 2;
            }
        } while (bitcount + slicecount < bp);
        
        if (bitcount + slicecount == bp)
        {
            bitcount += slicecount;
            bitslice--;
        }
        
        for (sb = 0; sb < nrof_subbands; sb++)
        {
            if (bitneed[ch][sb] < bitslice + 2)
                bits[ch][sb] = 0;
            else {
                bits[ch][sb] = bitneed[ch][sb] - bitslice;
                if (bits[ch][sb] > 16)
                    bits[ch][sb] = 16;
            }
        }
        
        for (sb = 0; bitcount < bp && sb < nrof_subbands; sb++)
        {
            if ((bits[ch][sb] >= 2) && (bits[ch][sb] < 16)) {
                bits[ch][sb]++;
                bitcount++;
            } else if ((bitneed[ch][sb] == bitslice + 1) && (bp > bitcount + 1)) {
                bits[ch][sb] = 2;
                bitcount += 2;
            }
        }
        
        for (sb = 0; bitcount < bp && sb < nrof_subbands; sb++)
        {
            if (bits[ch][sb] < 16) {
                bits[ch][sb]++;
                bitcount++;
            }
        }
    }
}

// Avoid divides on 16mhz m0
const uint32_t divtab[] = {
    0x0,
    0x00010000,
    0x0000AAAA,
    0x00009249,
    0x00008888,
    0x00008421,
    0x00008208,
    0x00008102,
    0x00008080,
    0x00008040,
    0x00008020,
    0x00008010,
    0x00008008,
    0x00008004,
    0x00008002,
    0x00008001,
    0xFFFF8000,
};

// SAMPLE has LEVEL bits
int IQUANT(int sample, int level, int scale)
{
    sample = ((sample << 1) | 1);
#if 0
    return ((divtab[level]+1)*sample-1) >> (15 + level - scale);// PRETTY CLOSE: POINTS FOR EXACT ACROSS RANGE
#else
    return (sample << scale)/((1 << level) - 1);                // 1..16
#endif
}

#define GETBITS(_n) \
    while (b_count < _n) { \
        b_bits = (b_bits << 8) | *b_data++; \
    } \

// get sample coefficients, return length of packet
static int get_samples(SBC_Decode *sbc, const uint8_t *data, int len)
{
	int bits[2][8];
	uint8_t scale_factor[2][8];
    int ch, sb, blk;

	if (len < 4 || data[0] != 0x9C)
		return -1;
    
	sbc->frequency = (data[1] >> 6) & 0x03;
    sbc->blocks = SBC_BlockMode[(data[1] >> 4) & 0x03];
	sbc->mode = (data[1] >> 2) & 0x03;
    sbc->channels = !sbc->mode ? 1:2;
	sbc->allocation = (data[1] >> 1) & 0x01;
	sbc->subbands = (data[1] & 0x01) ? 8 : 4;
	sbc->bitpool = data[2];
    
    // ignore CRC for now
    if (sbc->mode == 3 || sbc->subbands == 4)
        return -1;
    
    // read scale factors for subbands
    const uint8_t* sf = data+4;
    for (ch = 0; ch < sbc->channels; ch++)
    {
		for (sb = 0; sb < sbc->subbands; sb += 2)
        {
            uint8_t a = *sf++;
			scale_factor[ch][sb] = a >> 4;
			scale_factor[ch][sb+1] = a & 0xF;
		}
	}
    
    // get bit allocation for subbands
	bit_allocation(sbc,scale_factor,bits);
    
    int b_count = 0;
    int b_bits = 0;
    const uint8_t* b_data = data+4+(sbc->channels*sbc->subbands >> 1);
    
    // load samples for subbands
	for (blk = 0; blk < sbc->blocks; blk++)
    {
		for (ch = 0; ch < sbc->channels; ch++)
        {
			for (sb = 0; sb < sbc->subbands; sb++)
            {
                int sample = 0;
                int level = bits[ch][sb];   // 0,2..16
                if (level)
                {
                    while (b_count < level)                     // read level bits
                    {
                        b_bits = (b_bits << 8) | *b_data++;
                        b_count += 8;
                    }
                    b_count -= level;
                    sample = (b_bits >> b_count) & ((1 << level)-1);
                    
                    int scale = scale_factor[ch][sb];           // 4 bits
                    sample = IQUANT(sample,level,scale);
                    sample -= (1 << scale);
				}
                sbc->sb_sample[blk][ch][sb] = sample;
			}
		}
	}
	return (int)(b_data - data);
}

int sbc_decoder(SBC_Decode* sbc, const uint8_t *src, int src_len, void *dst, int dst_len, int *decoded)
{
	int i, ch, blk, framelen;
    
    if (!sbc->inited) {
        sbc->inited = 1;
        for (ch = 0; ch < 2; ch++)
            for (i = 0; i < 16; i++)
                sbc->v_offset[ch][i] = (i+1)*10;
	}
    
    // read subband samples
    framelen = get_samples(sbc, src, src_len);
    if (sbc->subbands == 4)
        return -1; // TODO

    // synthesize to pcm
    OUTPUTSAMPLETYPE* pcm = (OUTPUTSAMPLETYPE*)dst;
    for (ch = 0; ch < sbc->channels; ch++)
    {
        for (blk = 0; blk < sbc->blocks; blk++)
        {
            synthesize8(sbc->v[ch],sbc->v_offset[ch],sbc->sb_sample[blk][ch],pcm);
            pcm += 8;
        }
    }
    
	if (decoded)
		*decoded = sbc->blocks * sbc->subbands * sbc->channels * sizeof(OUTPUTSAMPLETYPE);
	return framelen;
}

void sbc_init(SBC_Decode* sbc)
{
    memset(sbc,0,sizeof(SBC_Decode));   // also clears v
}

