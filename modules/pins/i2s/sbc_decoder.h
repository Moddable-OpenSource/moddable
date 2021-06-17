//
//  sbc_decoder.h
//  sbccodec
//
//  Created by Peter Barrett on 9/9/14.
//  Copyright (c) 2014 Peter Barrett. All rights reserved.
//

#ifndef sbc_decoder_h
#define sbc_decoder_h

#include <stdint.h>

typedef struct {
    uint8_t inited;
	uint8_t frequency;
	uint8_t blocks;
	uint8_t channels;
    
    uint8_t mode;
    uint8_t allocation;
	uint8_t subbands;
	uint8_t bitpool;
    
	int32_t sb_sample[16][2][8];
    int32_t v[2][160+10];
	uint8_t v_offset[2][16];
} SBC_Decode;

// note: PCM is not interleaved for channels > 1; left block follows right block
// frequencies are 16000,32000,44100,48000
// 32000 mono produces 32 byte packets and 128 pcm samples per

void sbc_init(SBC_Decode* sbc);
int sbc_decoder(SBC_Decode* sbc, const uint8_t *src, int src_len, void *dst, int dst_len, int *decoded);

#endif
