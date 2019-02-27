/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "xsPlatform.h"
#include "chacha.h"

#define ROTL32(v, n)	((v) << (n)) | ((v) >> (32 - (n)))
#define U32TO8_LITTLE(p, v)	((p)[0] = (v), (p)[1] = (v) >> 8, (p)[2] = (v) >> 16, (p)[3] = (v) >> 24)
//#define U8TO32_LITTLE(p)	(((uint32_t)(p)[0]) | ((uint32_t)(p)[1] << 8) | ((uint32_t)(p)[2] << 16) | ((uint32_t)(p)[3] << 24))
#define U8TO32_LITTLE(p) c_read32(p)

#define QUARTERROUND(x, a, b, c, d) \
	(x[a] += x[b], x[d] = ROTL32(x[d] ^ x[a], 16), \
	 x[c] += x[d], x[b] = ROTL32(x[b] ^ x[c], 12), \
	 x[a] += x[b], x[d] = ROTL32(x[d] ^ x[a], 8), \
	 x[c] += x[d], x[b] = ROTL32(x[b] ^ x[c], 7))

static const char sigma[16] ICACHE_XS6RO_ATTR = "expand 32-byte k";
static const char tau[16] ICACHE_XS6RO_ATTR = "expand 16-byte k";

#ifdef CHACHA_DEBUG
#include <stdio.h>

static void
chacha_dump(uint32_t *s, const char *prompt)
{
	int i, j;

	if (prompt) printf("%s:\n", prompt);
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++)
			printf("%08x ", *s++);
		printf("\n");
	}
}
#endif

int
chacha_keysetup(chacha_ctx *x, const uint8_t *k, uint32_t kbytes)
{
	const char *constants;

	if (kbytes < 16)
		return 0;
	x->state[4] = U8TO32_LITTLE(k + 0);
	x->state[5] = U8TO32_LITTLE(k + 4);
	x->state[6] = U8TO32_LITTLE(k + 8);
	x->state[7] = U8TO32_LITTLE(k + 12);
	if (kbytes >= 32) { /* recommended */
		k += 16;
		constants = sigma;
	} else { /* kbits == 128 */
		constants = tau;
	}
	x->state[8] = U8TO32_LITTLE(k + 0);
	x->state[9] = U8TO32_LITTLE(k + 4);
	x->state[10] = U8TO32_LITTLE(k + 8);
	x->state[11] = U8TO32_LITTLE(k + 12);
	x->state[0] = U8TO32_LITTLE(constants + 0);
	x->state[1] = U8TO32_LITTLE(constants + 4);
	x->state[2] = U8TO32_LITTLE(constants + 8);
	x->state[3] = U8TO32_LITTLE(constants + 12);

	x->state[12] = x->state[13] = x->state[14] = x->state[15] = 0;
	x->npad = 0;

	return 1;
}

void
chacha_ivsetup(chacha_ctx *x, const uint8_t *iv, uint32_t ivSize, uint64_t counter)
{
	x->state[12] = counter;
	x->state[13] = counter >> 32;
	if (ivSize >= 12) {
		x->state[13] |= U8TO32_LITTLE(iv);	/* ?? */
		iv += 4;
	}
	x->state[14] = ivSize >= 4 ? U8TO32_LITTLE(iv + 0) : 0;
	x->state[15] = ivSize >= 8 ? U8TO32_LITTLE(iv + 4) : 0;
	x->npad = 0;
#ifdef CHACHA_DEBUG
	chacha_dump(x->state, "initial");
#endif
}

static void
chacha_block(chacha_ctx *ctx)
{
	int i;
	uint32_t *input = ctx->state;
	uint8_t *output = ctx->pad;
	uint32_t x[16];

	for (i = 0; i < CHACHA_NSTATE ; i++)
		x[i] = input[i];
	for (i = 10; --i >= 0;) {
		QUARTERROUND(x, 0, 4, 8, 12);
		QUARTERROUND(x, 1, 5, 9, 13);
		QUARTERROUND(x, 2, 6, 10, 14);
		QUARTERROUND(x, 3, 7, 11, 15);
		QUARTERROUND(x, 0, 5, 10, 15);
		QUARTERROUND(x, 1, 6, 11, 12);
		QUARTERROUND(x, 2, 7, 8, 13);
		QUARTERROUND(x, 3, 4, 9, 14);
	}
	for (i = 0; i < CHACHA_NSTATE; i++) {
		x[i] += input[i];
		U32TO8_LITTLE(&output[4 * i], x[i]);
	}
	/* increment the counter */
	input[12]++;
	if (input[12] == 0) {
		input[13]++;
		/* stopping at 2^70 bytes per nonce is user's responsibility */
	}
	ctx->npad = sizeof(ctx->pad);
#ifdef CHACHA_DEBUG
	chacha_dump((uint32_t *)ctx->pad, "output");
#endif
}

static uint32_t
chacha_xor(chacha_ctx *ctx, const uint8_t *src, uint8_t *dst, uint32_t n)
{
	const uint8_t *pad = ctx->pad + sizeof(ctx->pad) - ctx->npad;
	uint32_t i;

	if (n > ctx->npad)
		n = ctx->npad;
	for (i = 0; i < n; i++)
		dst[i] = src[i] ^ pad[i];
	ctx->npad -= n;
	return n;
}

void
chacha_process(const void *indata, void *outdata, uint32_t nbytes, chacha_ctx *ctx)
{
	const uint8_t *p1 = indata;
	uint8_t *p2 = outdata;
	uint32_t n;

	while (nbytes != 0) {
		if (ctx->npad == 0)
			chacha_block(ctx);
		n = chacha_xor(ctx, p1, p2, nbytes);
		nbytes -= n;
		p1 += n;
		p2 += n;
	}
}
