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
#include "xsmc.h"

#ifndef howmany
#define howmany(x, y)	((x + (y) - 1) / (y))
#endif

size_t mc_encode64(char *out, const unsigned char *in, size_t sz);
size_t mc_decode64(unsigned char *out, const char *in, size_t sz);

static const char gNoMem[] ICACHE_RODATA_ATTR = "no mem";

void
xs_bin_encode(xsMachine *the)
{
	void *src = xsmcToArrayBuffer(xsArg(0));
	size_t len = xsGetArrayBufferLength(xsArg(0));
	char *dst;
	size_t n;

	if ((dst = c_malloc(howmany(len, 3) * 4 + 1)) == NULL)
		xsUnknownError((char *)gNoMem);
	n = mc_encode64(dst, src, len);
	dst[n] = '\0';
	xsmcSetString(xsResult, dst);
	c_free(dst);
}

void
xs_bin_decode(xsMachine *the)
{
	void *src = xsmcToString(xsArg(0));
	size_t len = c_strlen(src);
	size_t n = howmany(len, 4) * 3;

	xsResult = xsArrayBuffer(NULL, n);
	n = mc_decode64(xsmcToArrayBuffer(xsResult), src, n);
	xsSetArrayBufferLength(xsResult, n);
}

/*
 * base64
 */
static const int8_t b64tab[] ICACHE_FLASH_ATTR = {
	/* start with '+' = 0x2b */
	62, -1, -1, -1, 63,				/* + , - . / */
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,		/* 0 1 2 ... */
	-1, -1, -1,					/* : ; < */
	0,						/* = */
	-1, -1, -1,					/* > ? @ */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,			/* A B C ... */
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25,
	-1, -1, -1, -1, -1, -1,				/* [ \ ] ^ _ ` */
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35,		/* a b c ... */
	36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
	46, 47, 48, 49, 50, 51,				/* ... z */
};

size_t
mc_decode64(unsigned char *out, const char *in, size_t sz)
{
	unsigned int c1, c2, c3, c4;
	unsigned char *op = out;
#define C2I(c)	(c >= '+' && c <= 'z' ? c_read8(b64tab +  (c - '+')) : 0)
#define OUTC(c)	if (sz-- != 0) *op++ = (c); else break

	while (*in != '\0') {
		c1 = *in != '\0' ? *in++ : 0;
		c2 = *in != '\0' ? *in++ : 0;
		c3 = *in != '\0' ? *in++ : 0;
		c4 = *in != '\0' ? *in++ : 0;
		c1 = C2I(c1);
		c2 = C2I(c2);
		OUTC((c1 << 2) | (c2 >> 4));
		if (c3 != '=') {
			c3 = C2I(c3);
			OUTC((c2 << 4) | (c3 >> 2));
			if (c4 != '=') {
				c4 = C2I(c4);
				OUTC((c3 << 6) | c4);
			}
		}
	}
	return op - out;
}

static const char b64str[] ICACHE_FLASH_ATTR = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t
mc_encode64(char *out, const unsigned char *in, size_t sz)
{
	unsigned long l;
	long n = (long)sz;
	char *op = out;
#define ENC(x)	c_read8(b64str + ((x) & 0x3f))

	while (n > 0) {
		l = --n >= 0 ? *in++ : 0;
		l = (l << 8) | (--n >= 0 ? *in++ : 0);
		l = (l << 8) | (--n >= 0 ? *in++ : 0);
		*op++ = ENC(l >> 18);
		*op++ = ENC(l >> 12);
		*op++ = n < -1 ? '=' : ENC(l >> 6);
		*op++ = n < 0 ? '=' : ENC(l);
	}
	return op - out;
}
