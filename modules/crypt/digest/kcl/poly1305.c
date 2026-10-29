/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
 */

/*
	Poly1305 one-time authenticator (RFC 8439 section 2.5)

	The accumulator h and the clamped key r are held in five 26-bit limbs, so every partial product fits in 64 bits
	and a block costs 25 multiplies. The reduction modulo 2^130 - 5 folds the carry out of the top limb back into the
	bottom one times 5.
*/

#include <string.h>
#include "poly1305.h"

#define MASK26	0x3ffffff

static uint32_t le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

void poly1305_init(poly1305_t *st, const uint8_t key[POLY1305_KEYSIZE])
{
	// r is clamped: the top four bits of bytes 3, 7, 11, 15 and the bottom two bits of bytes 4, 8, 12 cleared
	st->r[0] = le32(key + 0) & 0x3ffffff;
	st->r[1] = (le32(key + 3) >> 2) & 0x3ffff03;
	st->r[2] = (le32(key + 6) >> 4) & 0x3ffc0ff;
	st->r[3] = (le32(key + 9) >> 6) & 0x3f03fff;
	st->r[4] = (le32(key + 12) >> 8) & 0x00fffff;
	st->h[0] = st->h[1] = st->h[2] = st->h[3] = st->h[4] = 0;
	st->pad[0] = le32(key + 16);
	st->pad[1] = le32(key + 20);
	st->pad[2] = le32(key + 24);
	st->pad[3] = le32(key + 28);
	st->leftover = 0;
}

// h = (h + m) * r mod 2^130 - 5 for each 16-byte block; hibit is 2^128 for a full block, 0 for the padded final block
static void poly1305_blocks(poly1305_t *st, const uint8_t *m, uint32_t bytes, uint32_t hibit)
{
	const uint32_t r0 = st->r[0], r1 = st->r[1], r2 = st->r[2], r3 = st->r[3], r4 = st->r[4];
	const uint32_t s1 = r1 * 5, s2 = r2 * 5, s3 = r3 * 5, s4 = r4 * 5;
	uint32_t h0 = st->h[0], h1 = st->h[1], h2 = st->h[2], h3 = st->h[3], h4 = st->h[4];

	while (bytes >= 16) {
		uint64_t d0, d1, d2, d3, d4;
		uint32_t c;

		h0 += le32(m + 0) & MASK26;
		h1 += (le32(m + 3) >> 2) & MASK26;
		h2 += (le32(m + 6) >> 4) & MASK26;
		h3 += (le32(m + 9) >> 6) & MASK26;
		h4 += (le32(m + 12) >> 8) | hibit;

		d0 = ((uint64_t)h0 * r0) + ((uint64_t)h1 * s4) + ((uint64_t)h2 * s3) + ((uint64_t)h3 * s2) + ((uint64_t)h4 * s1);
		d1 = ((uint64_t)h0 * r1) + ((uint64_t)h1 * r0) + ((uint64_t)h2 * s4) + ((uint64_t)h3 * s3) + ((uint64_t)h4 * s2);
		d2 = ((uint64_t)h0 * r2) + ((uint64_t)h1 * r1) + ((uint64_t)h2 * r0) + ((uint64_t)h3 * s4) + ((uint64_t)h4 * s3);
		d3 = ((uint64_t)h0 * r3) + ((uint64_t)h1 * r2) + ((uint64_t)h2 * r1) + ((uint64_t)h3 * r0) + ((uint64_t)h4 * s4);
		d4 = ((uint64_t)h0 * r4) + ((uint64_t)h1 * r3) + ((uint64_t)h2 * r2) + ((uint64_t)h3 * r1) + ((uint64_t)h4 * r0);

		c = (uint32_t)(d0 >> 26); h0 = (uint32_t)d0 & MASK26;
		d1 += c; c = (uint32_t)(d1 >> 26); h1 = (uint32_t)d1 & MASK26;
		d2 += c; c = (uint32_t)(d2 >> 26); h2 = (uint32_t)d2 & MASK26;
		d3 += c; c = (uint32_t)(d3 >> 26); h3 = (uint32_t)d3 & MASK26;
		d4 += c; c = (uint32_t)(d4 >> 26); h4 = (uint32_t)d4 & MASK26;
		h0 += c * 5; c = h0 >> 26; h0 &= MASK26;
		h1 += c;

		m += 16;
		bytes -= 16;
	}

	st->h[0] = h0; st->h[1] = h1; st->h[2] = h2; st->h[3] = h3; st->h[4] = h4;
}

void poly1305_update(poly1305_t *st, const uint8_t *m, uint32_t bytes)
{
	if (st->leftover) {
		uint32_t want = 16 - st->leftover;
		if (want > bytes)
			want = bytes;
		memcpy(st->buffer + st->leftover, m, want);
		st->leftover += (uint8_t)want;
		m += want;
		bytes -= want;
		if (st->leftover < 16)
			return;
		poly1305_blocks(st, st->buffer, 16, 1 << 24);
		st->leftover = 0;
	}
	if (bytes >= 16) {
		uint32_t whole = bytes & ~15;
		poly1305_blocks(st, m, whole, 1 << 24);
		m += whole;
		bytes -= whole;
	}
	if (bytes) {
		memcpy(st->buffer, m, bytes);
		st->leftover = (uint8_t)bytes;
	}
}

void poly1305_finish(poly1305_t *st, uint8_t tag[POLY1305_TAGSIZE])
{
	uint32_t h0, h1, h2, h3, h4, g0, g1, g2, g3, g4, c, mask;
	uint64_t f;

	if (st->leftover) {		// the final partial block: a 1 byte after the message, then zeros, and no 2^128
		uint32_t i = st->leftover;
		st->buffer[i++] = 1;
		while (i < 16)
			st->buffer[i++] = 0;
		poly1305_blocks(st, st->buffer, 16, 0);
	}

	// carry fully
	h0 = st->h[0]; h1 = st->h[1]; h2 = st->h[2]; h3 = st->h[3]; h4 = st->h[4];
	c = h1 >> 26; h1 &= MASK26;
	h2 += c; c = h2 >> 26; h2 &= MASK26;
	h3 += c; c = h3 >> 26; h3 &= MASK26;
	h4 += c; c = h4 >> 26; h4 &= MASK26;
	h0 += c * 5; c = h0 >> 26; h0 &= MASK26;
	h1 += c;

	// g = h + 5 - 2^130: use it if it does not borrow, that is, if h >= 2^130 - 5
	g0 = h0 + 5; c = g0 >> 26; g0 &= MASK26;
	g1 = h1 + c; c = g1 >> 26; g1 &= MASK26;
	g2 = h2 + c; c = g2 >> 26; g2 &= MASK26;
	g3 = h3 + c; c = g3 >> 26; g3 &= MASK26;
	g4 = h4 + c - (1UL << 26);

	mask = (g4 >> 31) - 1;		// all ones when g4 did not go negative
	g0 &= mask; g1 &= mask; g2 &= mask; g3 &= mask; g4 &= mask;
	mask = ~mask;
	h0 = (h0 & mask) | g0;
	h1 = (h1 & mask) | g1;
	h2 = (h2 & mask) | g2;
	h3 = (h3 & mask) | g3;
	h4 = (h4 & mask) | g4;

	// h mod 2^128 as four 32-bit words, plus the pad s
	h0 = h0 | (h1 << 26);
	h1 = (h1 >> 6) | (h2 << 20);
	h2 = (h2 >> 12) | (h3 << 14);
	h3 = (h3 >> 18) | (h4 << 8);

	f = (uint64_t)h0 + st->pad[0]; h0 = (uint32_t)f;
	f = (uint64_t)h1 + st->pad[1] + (f >> 32); h1 = (uint32_t)f;
	f = (uint64_t)h2 + st->pad[2] + (f >> 32); h2 = (uint32_t)f;
	f = (uint64_t)h3 + st->pad[3] + (f >> 32); h3 = (uint32_t)f;

	put32(tag + 0, h0);
	put32(tag + 4, h1);
	put32(tag + 8, h2);
	put32(tag + 12, h3);

	memset(st, 0, sizeof(*st));
}
