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
#include "xs.h"		/* for mxLittleEndian */
#include "ghash.h"

#define SWAP64(v)	((v >> 56) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) | ((v >> 8) & 0xff000000) | ((v << 8) & 0xff00000000) | ((v << 24) & 0xff0000000000) | ((v << 40) & 0xff000000000000) | ((v << 56) & 0xff00000000000000))

void
_ghash_fix128(uint128_t *v)
{
#if mxLittleEndian
	uint64_t t;

	t = v->_64[1];
	v->_64[1] = SWAP64(v->_64[0]);
	v->_64[0] = SWAP64(t);
#endif
}

static void
rightshift(uint128_t *v)
{
#if mxLittleEndian
	v->_64[0] >>= 1;
	v->_64[0] |= (v->_64[1] & 1) << 63;
	v->_64[1] >>= 1;
#else
	v->_64[1] >>= 1;
	v->_64[1] |= (v->_64[0] & 1) << 63;
	v->_64[0] >>= 1;
#endif
}

static int
isset128(const uint128_t *v, int i)
{
#if mxLittleEndian
	return (v->_8[15 - (i / 8)] >> (7 - (i % 8))) & 1;
#else
	return (v->_8[i / 8] >> (7 - (i % 8))) & 1;
#endif
}

static void
xor128(uint128_t *r, const uint128_t *t)
{
	r->_64[0] ^= t->_64[0];
	r->_64[1] ^= t->_64[1];
}

static void
ghash_mul(const uint128_t *x, const uint128_t *y, uint128_t *z)
{
	uint128_t v;
	int i;
#define R	0xe1

	/* take into consideration x = z */
	c_memcpy(&v, x, sizeof(uint128_t));
	c_memset(z, 0, sizeof(uint128_t));
	for (i = 0; i < 128; i++) {
		if (isset128(y, i))
			xor128(z, &v);
		if (isset128(&v, 127)) {
			rightshift(&v);
#if mxLittleEndian
			v._8[15] ^= R;
#else
			v._8[0] ^= R;
#endif
		}
		else
			rightshift(&v);
	}
}

void
_ghash_update(ghash_t *ghash, const void *data, size_t sz)
{
	const uint8_t *p = data;
	uint128_t c;

	ghash->len += sz;
	while (sz != 0) {
		size_t len = sz;
		if (len > sizeof(uint128_t))
			len = sizeof(uint128_t);
		else if (len < sizeof(uint128_t))
			c_memset(&c, 0, sizeof(uint128_t));
		c_memcpy(&c, p, len);
		_ghash_fix128(&c);
		xor128(&ghash->y, &c);
		ghash_mul(&ghash->y, &ghash->h, &ghash->y);
		p += len;
		sz -= len;
	}
}

void
_ghash_create(ghash_t *ghash)
{
	c_memcpy(&ghash->y, &ghash->y0, sizeof(ghash->y));
	ghash->len = 0;
}

void
_ghash_fin(ghash_t *ghash, uint8_t *result)
{
	uint128_t l;

	/* len(A) || len(C) */
#if mxLittleEndian
	l._64[1] = ghash->aad_len * 8;
	l._64[0] = ghash->len * 8;
#else
	l._64[0] = ghash->aad_len * 8;
	l._64[1] = ghash->len * 8;
#endif
	xor128(&ghash->y, &l);
	ghash_mul(&ghash->y, &ghash->h, &ghash->y);
	_ghash_fix128(&ghash->y);
	c_memcpy(result, &ghash->y, GHASH_DGSTSIZE);
}
