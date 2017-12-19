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

#include <string.h>
#include "rfc1321.h"
#include "xsPlatform.h"

static const uint32_t md5_k[] ICACHE_XS6RO_ATTR = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint32_t md5_iv[] ICACHE_XS6RO_ATTR = {
	0x67452301,
	0xefcdab89,
	0x98badcfe,
	0x10325476,
};

static const uint32_t md5_rotate[][4] ICACHE_XS6RO_ATTR = {
	{7, 12, 17, 22},
	{5, 9, 14, 20},
	{4, 11, 16, 23},
	{6, 10, 15, 21},
};

static inline uint32_t rotl(uint32_t n, int k)
{
	return (n << k) | (n >> (32-k));
}
#define F(x,y,z) (z ^ (x & (y ^ z)))
#define G(x,y,z) (y ^ (z & (y ^ x)))
#define H(x,y,z) (x ^ y ^ z)
#define I(x,y,z) (y ^ (x | ~z))
#define R1(a, b, c, d, i)	(a = b + rotl(a + F(b, c, d) + W[i] + md5_k[i], md5_rotate[0][i % 4]))
#define R2(a, b, c, d, i)	(a = b + rotl(a + G(b, c, d) + W[(5*i + 1) % 16] + md5_k[i], md5_rotate[1][i % 4]))
#define R3(a, b, c, d, i)	(a = b + rotl(a + H(b, c, d) + W[(3*i + 5) % 16] + md5_k[i], md5_rotate[2][i % 4]))
#define R4(a, b, c, d, i)	(a = b + rotl(a + I(b, c, d) + W[7*i % 16] + md5_k[i], md5_rotate[3][i % 4]))

void
md5_create(struct md5 *s)
{
	int i;

	s->len = 0;
	for (i = 0; i < MD5_NUMSTATE; i++)
		s->state[i] = md5_iv[i];
}

static void
md5_process(struct md5 *s, const uint8_t *blk)
{
	uint32_t W[16], a, b, c, d, w, t;
	int i;

	for (i = 0; i < 16; i++) {
		w = *blk++;
		w |= *blk++ << 8;
		w |= *blk++ << 16;
		w |= *blk++ << 24;
		W[i] = w;
	}
	a = s->state[0];
	b = s->state[1];
	c = s->state[2];
	d = s->state[3];

	i = 0;
	for (; i < 16; i++) {
		R1(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	for (; i < 32; i++) {
		R2(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	for (; i < 48; i++) {
		R3(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	for (; i < 64; i++) {
		R4(a, b, c, d, i);
		t = d; d = c; c = b; b = a; a = t;
	}
	s->state[0] += a;
	s->state[1] += b;
	s->state[2] += c;
	s->state[3] += d;
}

void
md5_update(struct md5 *s, const void *data, uint32_t size)
{
	const uint8_t *p = data;
	uint32_t r = s->len % MD5_BLKSIZE;

	s->len += size;
	if (r > 0) {
		uint32_t n = MD5_BLKSIZE - r;
		if (size < n) {
			memcpy(s->buf + r, p, size);
			return;
		}
		memcpy(s->buf + r, p, n);
		size -= n;
		p += n;
		md5_process(s, s->buf);
	}
	for (; size >= MD5_BLKSIZE; size -= MD5_BLKSIZE, p += MD5_BLKSIZE)
		md5_process(s, p);
	memcpy(s->buf, p, size);
}

void
md5_fin(struct md5 *s, uint8_t *dgst)
{
	uint32_t r = s->len % MD5_BLKSIZE;
	uint64_t l;
	uint8_t *p;
	int i;

	s->buf[r++] = 0x80;
	if (r > MD5_BLKSIZE - 8) {
		memset(s->buf + r, 0, MD5_BLKSIZE - r);
		md5_process(s, s->buf);
		r = 0;
	}
	memset(s->buf + r, 0, MD5_BLKSIZE - 8 - r);
	l = s->len * 8;
	p = &s->buf[MD5_BLKSIZE - 8];
	*p++ = l;
	*p++ = l >> 8;
	*p++ = l >> 16;
	*p++ = l >> 24;
	*p++ = l >> 32;
	*p++ = l >> 40;
	*p++ = l >> 48;
	*p++ = l >> 56;
	md5_process(s, s->buf);

	for (i = 0; i < MD5_NUMSTATE; i++) {
		uint32_t w = s->state[i];
		*dgst++ = w;
		*dgst++ = w >> 8;
		*dgst++ = w >> 16;
		*dgst++ = w >> 24;
	}
}
