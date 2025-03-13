/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

#if !mxLittleEndian
	#error big endian support removed. see previous revisions if needed
#endif

static void
xor128(uint128_t *r, const uint128_t *t)
{
	r->_64[0] ^= t->_64[0];
	r->_64[1] ^= t->_64[1];
}

static void
ghash_mul(const uint128_t *x, const uint128_t *y, uint128_t *z)
{
    const uint8_t R = 0xe1;
    uint128_t v = *x;

    z->_64[0] = 0;
    z->_64[1] = 0;

    for (int i = 0; i < 16; i++) {
        uint8_t byte = y->_8[15 - i];

        for (int j = 0x80; j; j >>= 1) {
            uint64_t lo = v._64[0];
            uint64_t hi = v._64[1];

            if (byte & j) {
                z->_64[0] ^= lo;
                z->_64[1] ^= hi;
            }

            v._64[0] = (lo >> 1) | (hi << 63);
            v._64[1] = hi >> 1;

            if (lo & 1)
                v._8[15] ^= R;
        }
    }
}

#if defined(__GNUC__) || defined(__clang__)
    #define SWAP64(v) __builtin_bswap64(v)
#elif defined(_MSC_VER)
    #define SWAP64(v) _byteswap_uint64(v)
#else
    #define SWAP64(v) \
        ((v >> 56) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) | \
        ((v >> 8) & 0xff000000) | ((v << 8) & 0xff00000000) | \
        ((v << 24) & 0xff0000000000) | ((v << 40) & 0xff000000000000) | \
        ((v << 56) & 0xff00000000000000))
#endif

void _ghash_fix128(uint128_t *v)
{
    uint64_t t = SWAP64(v->_64[0]);
    v->_64[0] = SWAP64(v->_64[1]);
    v->_64[1] = t;
}

void _ghash_update(ghash_t *ghash, const void *data, size_t sz)
{
    const uint8_t *p = data;
    uint128_t c;

    ghash->len += sz;

    while (sz >= sizeof(uint128_t)) {
        // Inline memcpy for 16 bytes
        c._64[0] = ((uint64_t *)p)[0];
        c._64[1] = ((uint64_t *)p)[1];

		// inline _ghash_fix128
		uint64_t t = SWAP64(c._64[0]);
		c._64[0] = SWAP64(c._64[1]);
		c._64[1] = t;

		// inline xor128
        ghash->y._64[0] ^= c._64[0];
        ghash->y._64[1] ^= c._64[1];

        ghash_mul(&ghash->y, &ghash->h, &ghash->y);

        p += sizeof(uint128_t);
        sz -= sizeof(uint128_t);
    }

    if (sz > 0) {
        c._64[0] = 0;
        c._64[1] = 0;
        c_memcpy(&c, p, sz);

		// inline _ghash_fix128
		uint64_t t = SWAP64(c._64[0]);
		c._64[0] = SWAP64(c._64[1]);
		c._64[1] = t;

		// inline xor128
        ghash->y._64[0] ^= c._64[0];
        ghash->y._64[1] ^= c._64[1];

        ghash_mul(&ghash->y, &ghash->h, &ghash->y);
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
	l._64[1] = ghash->aad_len * 8;
	l._64[0] = ghash->len * 8;

	xor128(&ghash->y, &l);
	ghash_mul(&ghash->y, &ghash->h, &ghash->y);
	_ghash_fix128(&ghash->y);
	c_memcpy(result, &ghash->y, GHASH_DGSTSIZE);
}
