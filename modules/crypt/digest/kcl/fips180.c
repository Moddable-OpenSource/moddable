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
#include "fips180.h"
#include "xsPlatform.h"

/* the following macros are used in common among all SHA algorithms */
#define Ch(x, y, z)	(z ^ (x & (y ^ z)))
#define Parity(x, y, z)	(x ^ y ^ z)
#define Maj(x, y, z)	((x & y) | (z & (x | y)))

static const uint32_t sha1_k[] ICACHE_XS6RO_ATTR = {
	0x5a827999,
	0x6ed9eba1,
	0x8f1bbcdc,
	0xca62c1d6,
};

static const uint32_t sha1_iv[] ICACHE_XS6RO_ATTR = {
	0x67452301,
	0xefcdab89,
	0x98badcfe,
	0x10325476,
	0xc3d2e1f0,
};

static inline uint32_t rotl(uint32_t n, int k)
{
	return (n << k) | (n >> (32-k));
}
#define ROTL1(n)	rotl(n, 1)
#define ROTL5(n)	rotl(n, 5)
#define ROTL30(n)	rotl(n, 30)

void
sha1_create(struct sha1 *s)
{
	int i;

	s->len = 0;
	for (i = 0; i < SHA1_NUMSTATE; i++)
		s->state[i] = sha1_iv[i];
}

static void
sha1_process(struct sha1 *s, const uint8_t *blk)
{
	uint32_t W[16], a, b, c, d, e, w, T;
	int i;

	for (i = 0; i < 16; i++) {
		w = *blk++ << 24;
		w |= *blk++ << 16;
		w |= *blk++ << 8;
		w |= *blk++;
		W[i] = w;
	}
	a = s->state[0];
	b = s->state[1];
	c = s->state[2];
	d = s->state[3];
	e = s->state[4];
	for (i = 0; i < 80; i++) {
		int s = i & 0xf;
		if (i >= 16)
			W[s] = ROTL1(W[(s+13) & 0xf] ^ W[(s+8) & 0xf] ^ W[(s+2) & 0xf] ^ W[s]);
		if (i < 20)
			T = Ch(b, c, d) + sha1_k[0];
		else if (i < 40)
			T = Parity(b, c, d) + sha1_k[1];
		else if (i < 60)
			T = Maj(b, c, d) + sha1_k[2];
		else
			T = Parity(b, c, d) + sha1_k[3];
		T += ROTL5(a) + e + W[s];
		e = d;
		d = c;
		c = ROTL30(b);
		b = a;
		a = T;
	}
	s->state[0] += a;
	s->state[1] += b;
	s->state[2] += c;
	s->state[3] += d;
	s->state[4] += e;
}

void
sha1_update(struct sha1 *s, const void *data, uint32_t size)
{
	const uint8_t *p = data;
	uint32_t r = s->len % SHA1_BLKSIZE;

	s->len += size;
	if (r > 0) {
		uint32_t n = SHA1_BLKSIZE - r;
		if (size < n) {
			memcpy(s->buf + r, p, size);
			return;
		}
		memcpy(s->buf + r, p, n);
		size -= n;
		p += n;
		sha1_process(s, s->buf);
	}
	for (; size >= SHA1_BLKSIZE; size -= SHA1_BLKSIZE, p += SHA1_BLKSIZE)
		sha1_process(s, p);
	memcpy(s->buf, p, size);
}

void
sha1_fin(struct sha1 *s, uint8_t *dgst)
{
	uint32_t r = s->len % SHA1_BLKSIZE;
	uint64_t l;
	uint8_t *p;
	int i;

	s->buf[r++] = 0x80;
	if (r > SHA1_BLKSIZE - 8) {
		memset(s->buf + r, 0, SHA1_BLKSIZE - r);
		sha1_process(s, s->buf);
		r = 0;
	}
	memset(s->buf + r, 0, SHA1_BLKSIZE - 8 - r);
	l = s->len * 8;
	p = &s->buf[SHA1_BLKSIZE - 8];
	*p++ = l >> 56;
	*p++ = l >> 48;
	*p++ = l >> 40;
	*p++ = l >> 32;
	*p++ = l >> 24;
	*p++ = l >> 16;
	*p++ = l >> 8;
	*p = l;
	sha1_process(s, s->buf);

	for (i = 0; i < SHA1_NUMSTATE; i++) {
		uint32_t w = s->state[i];
		*dgst++ = w >> 24;
		*dgst++ = w >> 16;
		*dgst++ = w >> 8;
		*dgst++ = w;
	}
}

static const uint32_t sha256_k[] ICACHE_XS6RO_ATTR = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static const uint32_t sha256_iv[] ICACHE_XS6RO_ATTR = {
	0x6a09e667,
	0xbb67ae85,
	0x3c6ef372,
	0xa54ff53a,
	0x510e527f,
	0x9b05688c,
	0x1f83d9ab,
	0x5be0cd19,
};

#define rotr(n, k)	((n >> k) | (n << (32-k)))
#define shr(n, k)	(n >> k)
#define Sigma256_0(x)	(rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22))
#define Sigma256_1(x)	(rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25))
#define sigma256_0(x)	(rotr(x, 7) ^ rotr(x, 18) ^ shr(x, 3))
#define sigma256_1(x)	(rotr(x, 17) ^ rotr(x, 19) ^ shr(x, 10))

void
sha256_create(struct sha256 *s)
{
	int i;

	s->len = 0;
	for (i = 0; i < SHA256_NUMSTATE; i++)
		s->state[i] = sha256_iv[i];
}

static void
sha256_process(struct sha256 *s, const uint8_t *blk)
{
	uint32_t W[64], a, b, c, d, e, f, g, h, w, T1, T2;
	int i;

	for (i = 0; i < 16; i++) {
		w = *blk++ << 24;
		w |= *blk++ << 16;
		w |= *blk++ << 8;
		w |= *blk++;
		W[i] = w;
	}
	for (; i < 64; i++)
		W[i] = sigma256_1(W[i-2]) + W[i-7] + sigma256_0(W[i-15]) + W[i-16];
	a = s->state[0];
	b = s->state[1];
	c = s->state[2];
	d = s->state[3];
	e = s->state[4];
	f = s->state[5];
	g = s->state[6];
	h = s->state[7];
	for (i = 0; i < 64; i++) {
		T1 = h + Sigma256_1(e) + Ch(e, f, g) + sha256_k[i] + W[i];
		T2 = Sigma256_0(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;
	}
	s->state[0] += a;
	s->state[1] += b;
	s->state[2] += c;
	s->state[3] += d;
	s->state[4] += e;
	s->state[5] += f;
	s->state[6] += g;
	s->state[7] += h;
}

void
sha256_update(struct sha256 *s, const void *data, uint32_t size)
{
	const uint8_t *p = data;
	uint32_t r = s->len % SHA256_BLKSIZE;

	s->len += size;
	if (r > 0) {
		uint32_t n = SHA256_BLKSIZE - r;
		if (size < n) {
			memcpy(s->buf + r, p, size);
			return;
		}
		memcpy(s->buf + r, p, n);
		size -= n;
		p += n;
		sha256_process(s, s->buf);
	}
	for (; size >= SHA256_BLKSIZE; size -= SHA256_BLKSIZE, p += SHA256_BLKSIZE)
		sha256_process(s, p);
	memcpy(s->buf, p, size);
}

void
sha256_fin(struct sha256 *s, uint8_t *dgst)
{
	uint32_t r = s->len % SHA256_BLKSIZE;
	uint64_t l;
	uint8_t *p;
	int i;

	s->buf[r++] = 0x80;
	if (r > SHA256_BLKSIZE - 8) {
		memset(s->buf + r, 0, SHA256_BLKSIZE - r);
		sha256_process(s, s->buf);
		r = 0;
	}
	memset(s->buf + r, 0, SHA256_BLKSIZE - 8 - r);
	l = s->len * 8;
	p = &s->buf[SHA256_BLKSIZE - 8];
	*p++ = l >> 56;
	*p++ = l >> 48;
	*p++ = l >> 40;
	*p++ = l >> 32;
	*p++ = l >> 24;
	*p++ = l >> 16;
	*p++ = l >> 8;
	*p = l;
	sha256_process(s, s->buf);

	for (i = 0; i < SHA256_NUMSTATE; i++) {
		uint32_t w = s->state[i];
		*dgst++ = w >> 24;
		*dgst++ = w >> 16;
		*dgst++ = w >> 8;
		*dgst++ = w;
	}
}

static const uint32_t sha224_iv[] ICACHE_XS6RO_ATTR = {
	0xc1059ed8,
	0x367cd507,
	0x3070dd17,
	0xf70e5939,
	0xffc00b31,
	0x68581511,
	0x64f98fa7,
	0xbefa4fa4,
};

void
sha224_create(struct sha256 *s)
{
	int i;

	s->len = 0;
	for (i = 0; i < SHA256_NUMSTATE; i++)
		s->state[i] = sha224_iv[i];
}

void
sha224_update(struct sha256 *s, const void *data, uint32_t size)
{
	sha256_update(s, data, size);
}

void
sha224_fin(struct sha256 *s, uint8_t *dgst)
{
	uint8_t dgst256[SHA256_DGSTSIZE];

	sha256_fin(s, dgst256);
	memcpy(dgst, dgst, SHA224_DGSTSIZE);
}

static const uint64_t sha512_k[] ICACHE_XS6RO_ATTR = {
	0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
	0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
	0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
	0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
	0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
	0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
	0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
	0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
	0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
	0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
	0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
	0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
	0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
	0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
	0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
	0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
	0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
	0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
	0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
	0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

static const uint64_t sha512_iv[] ICACHE_XS6RO_ATTR = {
	0x6a09e667f3bcc908ULL,
	0xbb67ae8584caa73bULL,
	0x3c6ef372fe94f82bULL,
	0xa54ff53a5f1d36f1ULL,
	0x510e527fade682d1ULL,
	0x9b05688c2b3e6c1fULL,
	0x1f83d9abfb41bd6bULL,
	0x5be0cd19137e2179ULL,
};

#define rotr64(n, k)	((n >> k) | (n << (64-k)))
#define shr64(n, k)	(n >> k)
#define Sigma512_0(x)	(rotr64(x, 28) ^ rotr64(x, 34) ^ rotr64(x, 39))
#define Sigma512_1(x)	(rotr64(x, 14) ^ rotr64(x, 18) ^ rotr64(x, 41))
#define sigma512_0(x)	(rotr64(x, 1) ^ rotr64(x, 8) ^ shr64(x, 7))
#define sigma512_1(x)	(rotr64(x, 19) ^ rotr64(x, 61) ^ shr64(x, 6))

void
sha512_create(struct sha512 *s)
{
	int i;

	s->len[0] = s->len[1] = 0;
	for (i = 0; i < SHA512_NUMSTATE; i++)
		s->state[i] = sha512_iv[i];
}

static void
sha512_process(struct sha512 *s, const uint8_t *blk)
{
	uint64_t W[80], a, b, c, d, e, f, g, h, w, T1, T2;
	int i;

	for (i = 0; i < 16; i++) {
		w = (uint64_t)*blk++ << 56;
		w |= (uint64_t)*blk++ << 48;
		w |= (uint64_t)*blk++ << 40;
		w |= (uint64_t)*blk++ << 32;
		w |= (uint64_t)*blk++ << 24;
		w |= (uint64_t)*blk++ << 16;
		w |= (uint64_t)*blk++ << 8;
		w |= (uint64_t)*blk++;
		W[i] = w;
	}
	for (; i < 80; i++)
		W[i] = sigma512_1(W[i-2]) + W[i-7] + sigma512_0(W[i-15]) + W[i-16];
	a = s->state[0];
	b = s->state[1];
	c = s->state[2];
	d = s->state[3];
	e = s->state[4];
	f = s->state[5];
	g = s->state[6];
	h = s->state[7];
	for (i = 0; i < 80; i++) {
		T1 = h + Sigma512_1(e) + Ch(e, f, g) + sha512_k[i] + W[i];
		T2 = Sigma512_0(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;
	}
	s->state[0] += a;
	s->state[1] += b;
	s->state[2] += c;
	s->state[3] += d;
	s->state[4] += e;
	s->state[5] += f;
	s->state[6] += g;
	s->state[7] += h;
}

void
sha512_update(struct sha512 *s, const void *data, uint32_t size)
{
	const uint8_t *p = data;
	uint64_t l = s->len[0];
	uint32_t r = l % SHA512_BLKSIZE;

	s->len[0] += size;
	if (s->len[0] < l)
		s->len[1]++;
	if (r > 0) {
		uint32_t n = SHA512_BLKSIZE - r;
		if (size < n) {
			memcpy(s->buf + r, p, size);
			return;
		}
		memcpy(s->buf + r, p, n);
		size -= n;
		p += n;
		sha512_process(s, s->buf);
	}
	for (; size >= SHA512_BLKSIZE; size -= SHA512_BLKSIZE, p += SHA512_BLKSIZE)
		sha512_process(s, p);
	memcpy(s->buf, p, size);
}

void
sha512_fin(struct sha512 *s, uint8_t *dgst)
{
	uint32_t r = s->len[0] % SHA512_BLKSIZE;
	uint64_t l1, l2;
	uint8_t *p;
	int i;

	s->buf[r++] = 0x80;
	if (r > SHA512_BLKSIZE - 16) {
		memset(s->buf + r, 0, SHA512_BLKSIZE - r);
		sha512_process(s, s->buf);
		r = 0;
	}
	memset(s->buf + r, 0, SHA512_BLKSIZE - 16 - r);
	l1 = (s->len[1] << 3) | (s->len[0] >> 61);
	l2 = s->len[0] << 3;
	p = &s->buf[SHA512_BLKSIZE - 16];
	*p++ = l1 >> 56;
	*p++ = l1 >> 48;
	*p++ = l1 >> 40;
	*p++ = l1 >> 32;
	*p++ = l1 >> 24;
	*p++ = l1 >> 16;
	*p++ = l1 >> 8;
	*p++ = l1;
	*p++ = l2 >> 56;
	*p++ = l2 >> 48;
	*p++ = l2 >> 40;
	*p++ = l2 >> 32;
	*p++ = l2 >> 24;
	*p++ = l2 >> 16;
	*p++ = l2 >> 8;
	*p++ = l2;
	sha512_process(s, s->buf);

	for (i = 0; i < SHA512_NUMSTATE; i++) {
		uint64_t w = s->state[i];
		*dgst++ = w >> 56;
		*dgst++ = w >> 48;
		*dgst++ = w >> 40;
		*dgst++ = w >> 32;
		*dgst++ = w >> 24;
		*dgst++ = w >> 16;
		*dgst++ = w >> 8;
		*dgst++ = w;
	}
}

static const uint64_t sha384_iv[] ICACHE_XS6RO_ATTR = {
	0xcbbb9d5dc1059ed8ULL,
	0x629a292a367cd507ULL,
	0x9159015a3070dd17ULL,
	0x152fecd8f70e5939ULL,
	0x67332667ffc00b31ULL,
	0x8eb44a8768581511ULL,
	0xdb0c2e0d64f98fa7ULL,
	0x47b5481dbefa4fa4ULL,
};

void
sha384_create(struct sha512 *s)
{
	int i;

	s->len[0] = s->len[1] = 0;
	for (i = 0; i < SHA512_NUMSTATE; i++)
		s->state[i] = sha384_iv[i];
}

void
sha384_update(struct sha512 *s, const void *data, uint32_t size)
{
	sha512_update(s, data, size);
}

void
sha384_fin(struct sha512 *s, uint8_t *dgst)
{
	uint8_t dgst512[SHA512_DGSTSIZE];

	sha512_fin(s, dgst512);
	memcpy(dgst, dgst512, SHA384_DGSTSIZE);
}
