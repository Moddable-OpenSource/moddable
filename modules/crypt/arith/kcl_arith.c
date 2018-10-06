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

#include <stdint.h>
#include <string.h>
#include "bn.h"
#include "kcl_arith.h"
#include "mc.defines.h"

#ifndef howmany
#define howmany(x, y)	(((x) + (y) - 1) / (y))
#endif
#ifndef roundup
#define roundup(x, y)	(howmany(x, y) * (y))
#endif
#ifndef MAX
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#endif

const char gKCLError[] ICACHE_XS6STRING_ATTR = "KCL error: %d";

kcl_err_t
kcl_int_alloc(kcl_int_t **rp)
{
	if ((*rp = kcl_malloc(sizeof(kcl_int_t))) == NULL)
		return KCL_ERR_NOMEM;
	(*rp)->data = NULL;	/* this = NaN */
	return KCL_ERR_NONE;
}

void
kcl_int_dispose(kcl_int_t *ai)
{
	kcl_int_free(ai);
	kcl_free(ai);
}

static kcl_err_t
kcl_int_init(kcl_int_t *ai, bn_size size)
{
	bn_t *bn;

	if ((bn = kcl_malloc(sizeof(bn_t) + ((size - 1) * sizeof(bn_word)))) == NULL)
		return KCL_ERR_NOMEM;
	bn->sign = 0;
	bn->size = size;
	ai->data = bn;
	return KCL_ERR_NONE;
}

static kcl_err_t
kcl_int_reinit(kcl_int_t *ai, bn_size size)
{
	if ((ai->data = kcl_malloc(sizeof(bn_t) + ((size - 1) * sizeof(bn_word)))) == NULL)
		return KCL_ERR_NOMEM;
	return KCL_ERR_NONE;
}

int
kcl_int_isNaN(kcl_int_t *ai)
{
	return ai->data == NULL || bn_isNaN(ai->data);
}

void
kcl_int_setNaN(kcl_int_t *ai)
{
	kcl_int_free(ai);
}

int
kcl_int_sign(kcl_int_t *ai)
{
	return bn_sign(ai->data);
}

int
kcl_int_iszero(kcl_int_t *ai)
{
	return bn_iszero(ai->data);
}

void
kcl_int_neg(kcl_int_t *ai)
{
	bn_negate(ai->data);
}

kcl_err_t
kcl_int_inc(kcl_int_t *ai, int d)
{
	bn_t *bn = ai->data;
	int c = bn_inc(bn, d);

	if (c != 0) {
		kcl_err_t err = kcl_int_reinit(ai, bn->size + 1);
		if (err)
			return err;
		bn = ai->data;
		bn->data[bn->size++] = 1;
	}
	return KCL_ERR_NONE;
}

int
kcl_int_comp(kcl_int_t *ai, kcl_int_t *o)
{
	return bn_comp(ai->data, o->data);
}

size_t
kcl_int_sizeof(kcl_int_t *ai)
{
	int n;

	return (n = bn_bitsize(ai->data)) == 0 ? 1 : howmany(n, 8);	/* bn_bitsize = 0 when the value is zero */
}

kcl_err_t
kcl_int_copy(kcl_int_t *ai, kcl_int_t *src)
{
	if (ai->data == NULL) {
		kcl_err_t err = kcl_int_init(ai, bn_wsizeof(src->data));
		if (err != KCL_ERR_NONE)
			return err;
	}
	bn_copy(ai->data, src->data);
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_i2os(kcl_int_t *ai, unsigned char *os, size_t size)
{
	bn_t *a = ai->data;
	int n;
	bn_word x;
	int blen;

	n = a->size - 1;
	x = a->data[n];
	if (x & 0xff000000)
		blen = 4;
	else if (x & 0x00ff0000)
		blen = 3;
	else if (x & 0x0000ff00)
		blen = 2;
	else
		blen = 1;
	if (size < (size_t)blen + n * 4)
		return KCL_ERR_OUT_OF_RANGE;
	while (--blen >= 0)
		*os++ = (uint8_t)(x >> (blen * 8));
	while (--n >= 0) {
		x = a->data[n];
		*os++ = (uint8_t)(x >> 24);
		*os++ = (uint8_t)(x >> 16);
		*os++ = (uint8_t)(x >> 8);
		*os++ = (uint8_t)(x);
	}
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_i2os_l(kcl_int_t *ai, unsigned char *os, size_t size)
{
	bn_t *a = ai->data;
	int n;
	bn_word x;
	int i;

	n = a->size - 1;
	for (i = 0; i < n; i++) {
		x = a->data[i];
		*os++ = (uint8_t)x;
		*os++ = (uint8_t)(x >> 8);
		*os++ = (uint8_t)(x >> 16);
		*os++ = (uint8_t)(x >> 24);
	}
	x = a->data[i];
	for (; x != 0; x >>= 8)
		*os++ = (uint8_t)x;
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_os2i(kcl_int_t *ai, unsigned char *os, size_t size)
{
	bn_t *bn;
	int i, j;

	if (ai->data == NULL) {
		kcl_err_t err = kcl_int_init(ai, howmany(size, sizeof(bn_word)));
		if (err)
			return err;
	}
	bn = ai->data;

	for (i = size, j = 0; i > 0; i -= 4, j++) {
#define str_data(n)	((n) < 0 ? 0: (bn_word)os[(n)])
		bn->data[j] = (str_data(i-4) << 24) | (str_data(i-3) << 16) | (str_data(i-2) << 8) | str_data(i-1);
	}
	/* adjust size */
	for (i = bn->size; --i >= 1 && bn->data[i] == 0;)
		;
	bn->size = i + 1;
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_os2i_l(kcl_int_t *ai, unsigned char *os, size_t size, int signess)
{
	bn_t *bn;
	unsigned int i, j;

	if (ai->data == NULL) {
		kcl_err_t err = kcl_int_init(ai, howmany(size, sizeof(bn_word)));
		if (err)
			return err;
	}
	bn = ai->data;

	for (i = 0, j = 0; i < size; i += 4, j++) {
#undef str_data
#define str_data(n)	(n >= size ? 0 : (bn_word)os[n])
		bn->data[j] = str_data(i) | (str_data(i + 1) << 8) | (str_data(i + 2) << 16) | (str_data(i + 3) << 24);
	}
	/* adjust size */
	for (i = bn->size; --i >= 1 && bn->data[i] == 0;)
		;
	bn->size = i + 1;
#define MSB	(1 << (sizeof(bn_word)*8 - 1))
	if (signess && (bn->data[bn->size - 1] & MSB)) {
		bn->data[bn->size - 1] &= ~MSB;
		bn->sign = 1;
	}
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_i2strx(kcl_int_t *ai, char *s)
{
	bn_t *bn = ai->data;
	int i = bn->size - 1;
	int n;
	bn_word w;
#define I2X(i)	((i) >= 10 ? 'a' + (i) - 10 : '0' + (i))

	if (kcl_int_sign(ai))
		*s++ = '-';
	/* process the MSW */
	w = bn->data[i];
	if (w == 0) {
		*s++ = '0';
		*s = '\0';
		return KCL_ERR_NONE;
	}
	for (n = sizeof(bn_word) * 8 - 4; n >= 0; n -= 4) {
		if ((w >> n) != 0) {
			for (; n >= 0; n -= 4) {
				uint8_t x = (uint8_t)(w >> n);
				*s++ = I2X(x & 0xf);
			}
			break;
		}
	}
	/* process the rest */
	while (--i >= 0) {
		w = bn->data[i];
		for (n = sizeof(bn_word) * 8 - 4; n >= 0; n -= 4) {
			uint8_t x = (uint8_t)(w >> n);
			*s++ = I2X(x & 0xf);
		}
	}
	*s = '\0';
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_str2ix(kcl_int_t *ai, char *s)
{
	char c;
	unsigned int n;
	char *p;
	int len;
	bn_word digit;
	bn_t *bn;
	kcl_err_t err = KCL_ERR_NONE;

	len = strlen(s);
	if (len <= 0)
		return kcl_int_num2i(ai, 0);
	if (ai->data == NULL) {
		if ((err = kcl_int_init(ai, howmany(len, 8))) != KCL_ERR_NONE)
			return err;
	}
	bn = ai->data;
	c_memset(bn->data, 0, bn->size * sizeof(bn_word));
	n = 0;
	for (p = s + len; --p >= s;) {
		c = *p;
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (c >= 'A' && c <= 'F')
			digit = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			digit = c - 'a' + 10;
		else
			continue;
		bn->data[n / 8] |= digit << ((n % 8) * 4);
		n++;
	}
	if (n <= 0)
		return kcl_int_num2i(ai, 0);
	/* remove leading 0s */
	for (n = howmany(n, 8); --n > 0 && bn->data[n] == 0;)
		;
	bn->size = n + 1;
	return err;
}

kcl_err_t
kcl_int_i2num(kcl_int_t *ai, long *n)
{
	bn_t *bn = ai->data;

	*n = bn->data[0];
	if (bn->sign)
		*n = -*n;
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_num2i(kcl_int_t *ai, long n)
{
	bn_t *bn;

	if (ai->data == NULL) {
		kcl_err_t err = kcl_int_init(ai, 1);
		if (err != KCL_ERR_NONE)
			return err;
	}
	bn = ai->data;
	if (n < 0) {
		bn->data[0] = -n;
		bn->sign = 1;
	}
	else {
		bn->data[0] = n;
		bn->sign = 0;
	}
	bn->size = 1;
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_int_newFrom(kcl_int_t *src, kcl_int_t **dst)
{
	kcl_err_t err;

	if ((err = kcl_int_alloc(dst)) != KCL_ERR_NONE)
		return err;
	return kcl_int_copy(*dst, src);
}

void
kcl_int_free(kcl_int_t *ai)
{
	if (ai->data != NULL) {
		kcl_free(ai->data);
		ai->data = NULL;
	}
}


/*
 * arithmetics on Z
 */
#ifndef MODDEF_KCL_BN_BUFSIZE
	#define BN_BUFSIZE	3200
#else
	#define BN_BUFSIZE MODDEF_KCL_BN_BUFSIZE
#endif

struct bn_context {
	struct bn_buf {
		bn_byte buf[BN_BUFSIZE];
		struct bn_buf *next;
	} *bn_buflist, *bn_cbuf;
	bn_byte *bn_bp;
#define bn_bend	bn_cbuf->buf + BN_BUFSIZE
	void *bn_cb;
};

void
bn_throw(bn_context_t *ctx, bn_err_t code)
{
	kcl_error_callback_t *cb = ctx->bn_cb;
	kcl_err_t err;

	if (cb == NULL)
		return;
	switch(code) {
	case BN_ERR_NOMEM:
		err = KCL_ERR_NOMEM;
		break;
	case BN_ERR_DIVIDE_BY_ZERO:
		err = KCL_ERR_OUT_OF_RANGE;
		break;
	case BN_ERR_MISCALCULATION:
		err = KCL_ERR_BAD_OPERATION;
		break;
	default:
		err = KCL_ERR_UNKNOWN;
		break;
	}
	/* initialize the bn buffers before calling the callback */
	ctx->bn_cbuf = ctx->bn_buflist;
	ctx->bn_bp = ctx->bn_cbuf->buf;
	(*cb->f)(err, cb->closure);
}

void *
bn_allocbuf(bn_context_t *ctx, unsigned int n)
{
	bn_byte *p;

	if (ctx->bn_bp + n > ctx->bn_bend)
		bn_throw(ctx, BN_ERR_NOMEM);
	p = ctx->bn_bp;
	ctx->bn_bp += n;
	return p;
}

void
bn_freebuf(bn_context_t *ctx, void *ptr)
{
	ctx->bn_bp = ptr;
}

kcl_err_t
kcl_z_alloc(kcl_z_t **rp)
{
	if ((*rp = kcl_malloc(sizeof(bn_context_t))) == NULL)
		return KCL_ERR_NOMEM;
	c_memset(*rp, 0, sizeof(bn_context_t));
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_z_init(kcl_z_t *z, kcl_error_callback_t *cb, int *neededBufferSize)
{
	bn_context_t *ctx = z;

	*neededBufferSize = sizeof(struct bn_buf);

	ctx->bn_cb = cb;
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_z_setBuffer(kcl_z_t *z, void *buffer)
{
	bn_context_t *ctx = z;

	ctx->bn_buflist = buffer;
	ctx->bn_buflist->next = NULL;
	ctx->bn_cbuf = ctx->bn_buflist;
	ctx->bn_bp = ctx->bn_cbuf->buf;
	return KCL_ERR_NONE;
}

void
kcl_z_free(kcl_z_t *z)
{
	if (z != NULL) {
		bn_context_t *ctx = z;
		ctx->bn_buflist = ctx->bn_cbuf = NULL;
		ctx->bn_bp = NULL;
	}
}

void
kcl_z_dispose(kcl_z_t *z)
{
	kcl_z_free(z);
	kcl_free(z);
}

static kcl_err_t
z_set_result(kcl_int_t **r, bn_t *bn)
{
	kcl_err_t err;

	if ((err = kcl_int_alloc(r)) != KCL_ERR_NONE)
		return err;
	if ((err = kcl_int_init(*r, bn->size)) != KCL_ERR_NONE) {
		kcl_int_dispose(*r);
		*r = NULL;
		return err;
	}
	bn_copy((*r)->data, bn);
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_z_add(kcl_z_t *z, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_add(z, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_sub(kcl_z_t *z, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_sub(z, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_mul(kcl_z_t *z, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mul(z, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_div(kcl_z_t *z, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r, kcl_int_t **m)
{
	bn_t *bn, *bn_r = NULL;
	kcl_err_t err;

	bn = bn_div(z, NULL, a->data, b->data, m ? &bn_r : NULL);
	err = z_set_result(r, bn);
	if (m && err == KCL_ERR_NONE)
		err = z_set_result(m, bn_r);
	bn_freebuf(z, bn);	/* free all the results */
	return err;
}

kcl_err_t
kcl_z_square(kcl_z_t *z, kcl_int_t *a, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_square(z, NULL, a->data);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_xor(kcl_z_t *z, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_xor(z, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_or(kcl_z_t *z, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_or(z, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_and(kcl_z_t *z, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_and(z, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_lsl(kcl_z_t *z, kcl_int_t *a, int b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_lsl(z, NULL, a->data, b);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_lsr(kcl_z_t *z, kcl_int_t *a, int b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_lsr(z, NULL, a->data, b);
	err = z_set_result(r, bn);
	bn_freebuf(z, bn);
	return err;
}

kcl_err_t
kcl_z_i2str(kcl_z_t *z, kcl_int_t *ai, char *s, size_t size, unsigned int radix)
{
	kcl_int_t *a = NULL, *d = NULL, *q = NULL, *r = NULL;
	char *sp;
	int rem;
	kcl_err_t err;

	if ((err = kcl_int_alloc(&d)) != KCL_ERR_NONE)
		goto bail;
	kcl_int_num2i(d, radix);
	if ((err = kcl_int_alloc(&a)) != KCL_ERR_NONE)
		goto bail;
	if ((err = kcl_int_copy(a, ai)) != KCL_ERR_NONE)
		goto bail;
	sp = s + size;
	*--sp = '\0';
	do {
		if ((err = kcl_z_div(z, a, d, &q, &r)) != KCL_ERR_NONE)
			goto bail;
		rem = ((bn_t *)r->data)->data[0];
		*--sp = (char)(rem >= 10 ? 'a' + rem - 10: '0' + rem);
		kcl_int_dispose(a);
		kcl_int_dispose(r);
		a = q;
	} while (!kcl_int_iszero(a));
	if (kcl_int_sign(ai))
		*--sp = '-';
	if (s != sp)
		memmove(s, sp, strlen(sp) + 1);
bail:
	if (a) kcl_int_dispose(a);
	if (d) kcl_int_dispose(d);
	return err;
}

kcl_err_t
kcl_z_str2i(kcl_z_t *z, kcl_int_t **aip, char *s, unsigned int radix)
{
	kcl_int_t *t = NULL, *r = NULL, *rr = NULL, *cradix = NULL;
	char c;
	kcl_err_t err;

	if ((err = kcl_int_alloc(&cradix)) != KCL_ERR_NONE)
		goto bail;
	kcl_int_num2i(cradix, radix);
	if ((err = kcl_int_alloc(&t)) != KCL_ERR_NONE)
		goto bail;
	kcl_int_num2i(t, 0);	/* just to make a room for 1 digit */
	if ((err = kcl_int_alloc(&r)) != KCL_ERR_NONE)
		goto bail;
	kcl_int_num2i(r, 0);
	while ((c = *s++) != '\0') {
		unsigned int digit;
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (c >= 'A' && c <= 'F')
			digit = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			digit = c - 'a' + 10;
		else
			continue;
		if (digit < radix) {
			((bn_t *)t->data)->data[0] = digit;
			if ((err = kcl_z_mul(z, r, cradix, &rr)) != KCL_ERR_NONE)
				goto bail;
			kcl_int_dispose(r);
			r = rr;
			if ((err = kcl_z_add(z, r, t, &rr)) != KCL_ERR_NONE)
				goto bail;
			kcl_int_dispose(r);
			r = rr;
		}
	}
	*aip = r;
bail:
	if (t) kcl_int_dispose(t);
	if (cradix) kcl_int_dispose(cradix);
	return err;
}


/*
 * Modular arithmetics
 */

kcl_err_t
kcl_mod_alloc(kcl_mod_t **modp)
{
	if ((*modp = kcl_malloc(sizeof(bn_mod_t))) == NULL)
		return KCL_ERR_NOMEM;
	c_memset(*modp, 0, sizeof(bn_mod_t));
	return KCL_ERR_NONE;
}

void
kcl_mod_dispose(kcl_mod_t *mod)
{
	kcl_free(mod);
}

kcl_err_t
kcl_mod_init(kcl_mod_t *mod, kcl_z_t *z, kcl_int_t *m)
{
	bn_mod_init(mod, z, m->data);
	return KCL_ERR_NONE;
}


kcl_err_t
kcl_mod_setBuffer(kcl_mod_t *mod, void *buffer)
{
	return kcl_z_setBuffer((kcl_z_t *)mod->ctx, buffer);
}

kcl_err_t
kcl_mod_add(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_add(mod, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_sub(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_sub(mod, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_inv(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_inv(mod, NULL, a->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_mul(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_mul(mod, NULL, a->data, b->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_square(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_square(mod, NULL, a->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_mulinv(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_mulinv(mod, NULL, a->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_exp(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t *e, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_exp(mod, NULL, a->data, e->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_exp2(kcl_mod_t *mod, kcl_int_t *a1, kcl_int_t *e1, kcl_int_t *a2, kcl_int_t *e2, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_exp2(mod, NULL, a1->data, e1->data, a2->data, e2->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

kcl_err_t
kcl_mod_mod(kcl_mod_t *mod, kcl_int_t *a, kcl_int_t **r)
{
	bn_t *bn;
	kcl_err_t err;

	bn = bn_mod_mod(mod, NULL, a->data);
	err = z_set_result(r, bn);
	bn_mod_freetbuf(mod, bn);
	return err;
}

void
kcl_mod_free(kcl_mod_t *mod)
{
	/* nothing to do */
}

/*
 * Montgomery method
 */
kcl_err_t
kcl_mont_init(kcl_mod_t *mod, kcl_z_t *z, kcl_int_t *m, kcl_mod_method_t method, int options)
{
	bn_mont_init(mod, z, m->data, method == KCL_MOD_METHOD_SW ? BN_MOD_METHOD_SW : BN_MOD_METHOD_LR, options);
	return KCL_ERR_NONE;
}

/*
 * EC point
 */

kcl_err_t
kcl_ecp_alloc(kcl_ecp_t **pp)
{
	kcl_ecp_t *p;

	if ((*pp = kcl_malloc(sizeof(kcl_ecp_t))) == NULL)
		return KCL_ERR_NOMEM;
	p = *pp;
	p->x = p->y = NULL;
	p->identity = 0;
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_ecp_init(kcl_ecp_t *p, kcl_int_t *x, kcl_int_t *y)
{
	kcl_err_t err;

	(void)((err = kcl_int_newFrom(x, &p->x)) || (err = kcl_int_newFrom(y, &p->y)));
	return err;
}

kcl_err_t
kcl_ecp_setX(kcl_ecp_t *p, kcl_int_t *x)
{
	if (p->x != NULL) {
		kcl_int_dispose(p->x);
		p->x = NULL;
	}
	return kcl_int_newFrom(x, &p->x);
}

kcl_err_t
kcl_ecp_setY(kcl_ecp_t *p, kcl_int_t *y)
{
	if (p->y != NULL) {
		kcl_int_dispose(p->y);
		p->y = NULL;
	}
	return kcl_int_newFrom(y, &p->y);
}

kcl_err_t
kcl_ecp_copy(kcl_ecp_t *src, kcl_ecp_t **dstp)
{
	kcl_ecp_t *dst;
	kcl_err_t err;

	err = kcl_ecp_alloc(dstp);
	if (err != KCL_ERR_NONE)
		return err;
	dst = *dstp;
	if (src->identity) {
		dst->identity = 1;
		return KCL_ERR_NONE;
	}
	(void)((err = kcl_int_newFrom(src->x, &dst->x)) || (err = kcl_int_newFrom(src->y, &dst->y)));
	return err;
}

kcl_err_t
kcl_ecp_new_identity(kcl_ecp_t **pp)
{
	kcl_err_t err;

	err = kcl_ecp_alloc(pp);
	if (err == KCL_ERR_NONE) {
		(*pp)->identity = 1;
	}
	return err;
}

void
kcl_ecp_free(kcl_ecp_t *p)
{
	if (p != NULL) {
		if (p->x != NULL) {
			kcl_int_dispose(p->x);
			p->x = NULL;
		}
		if (p->y != NULL) {
			kcl_int_dispose(p->y);
			p->y = NULL;
		}
	}
}

void
kcl_ecp_dispose(kcl_ecp_t *p)
{
	if (p != NULL) {
		kcl_ecp_free(p);
		kcl_free(p);
	}
}


/*
 * EC(p)
 */

kcl_err_t
kcl_ec_alloc(kcl_ec_t **r)
{
	if ((*r = kcl_malloc(sizeof(kcl_ec_t))) == NULL)
		return KCL_ERR_NOMEM;
	c_memset(*r, 0, sizeof(kcl_ec_t));
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_ec_init(kcl_ec_t *ec, kcl_mod_t *mod, kcl_int_t *a, kcl_int_t *b)
{
	ec->mod = mod;
	ec->a = a;
	ec->b = b;
	return KCL_ERR_NONE;
}

void
kcl_ec_dispose(kcl_ec_t *ec)
{
	kcl_free(ec);
}

kcl_err_t
kcl_ec_inv(kcl_ec_t *ec, kcl_ecp_t *a, kcl_ecp_t **rp)
{
	kcl_ecp_t *r;
	kcl_err_t err;

	err = kcl_ecp_alloc(rp);
	if (err != KCL_ERR_NONE)
		return(err);
	r = *rp;
	if (a->identity)
		r->identity = 1;
	else
		(void)((err = kcl_mod_inv(ec->mod, a->y, &r->y)) || (err = kcl_int_newFrom(a->x, &r->x)));
	return err;
}

static inline void
kcl_ec_int_dispose(kcl_int_t *i)
{
	if (i != NULL)
		kcl_int_dispose(i);
}

kcl_err_t
kcl_ec_double(kcl_ec_t *ec, kcl_ecp_t *a, kcl_ecp_t **r)
{
	kcl_int_t *t1 = NULL, *t2 = NULL, *t3 = NULL, *t4 = NULL, *t5 = NULL, *t6 = NULL, *lambda = NULL, *rx = NULL, *ry = NULL;
	int err;

	if (a->identity || kcl_int_iszero(a->y))
		return(kcl_ecp_new_identity(r));

	/* lambda = (3*x^2 + a) / 2*y */
	(void)((err = kcl_mod_square(ec->mod, a->x, &t1)) ||		/* t1 = x^2 */
	       (err = kcl_mod_add(ec->mod, t1, t1, &t2)) ||	/* t2 = t1 + t1 = t1 * 2*/
	       (err = kcl_mod_add(ec->mod, t2, t1, &t3)) ||	/* t3 = t1 + t2 = t1 * 3 */
	       (err = kcl_mod_add(ec->mod, t3, ec->a, &t4)) ||
	       (err = kcl_mod_add(ec->mod, a->y, a->y, &t5)) ||
	       (err = kcl_mod_mulinv(ec->mod, t5, &t6)) ||
	       (err = kcl_mod_mul(ec->mod, t4, t6, &lambda)));
	kcl_ec_int_dispose(t1); t1 = NULL;
	kcl_ec_int_dispose(t2); t2 = NULL;
	kcl_ec_int_dispose(t3); t3 = NULL;
	kcl_ec_int_dispose(t4); t4 = NULL;
	kcl_ec_int_dispose(t5); t5 = NULL;
	kcl_ec_int_dispose(t6); t6 = NULL;
	if (err) goto bail;

	/* rx = lambda^2 - 2*x */
	(void)((err = kcl_mod_square(ec->mod, lambda, &t1)) ||		/* t1 = lambda^2 */
	       (err = kcl_mod_sub(ec->mod, t1, a->x, &t2)) ||	/* t2 = t1 - x */
	       (err = kcl_mod_sub(ec->mod, t2, a->x, &rx)));	/* rx = t2 - x = t1 - 2*x */
	kcl_ec_int_dispose(t1); t1 = NULL;
	kcl_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	/* ry = lambda * (x - rx) - y */
	(void)((err = kcl_mod_sub(ec->mod, a->x, rx, &t1)) ||
	       (err = kcl_mod_mul(ec->mod, lambda, t1, &t2)) ||
	       (err = kcl_mod_sub(ec->mod, t2, a->y, &ry)));
	kcl_ec_int_dispose(t1); t1 = NULL;
	kcl_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	err = kcl_ecp_alloc(r);
	if (err)
		goto bail;
	(*r)->x = rx;
	(*r)->y = ry;

bail:
	kcl_ec_int_dispose(lambda);
	if (err) {
		kcl_ec_int_dispose(rx);
		kcl_ec_int_dispose(ry);
	}
	return err;
}

kcl_err_t
kcl_ec_add(kcl_ec_t *ec, kcl_ecp_t *a, kcl_ecp_t *b, kcl_ecp_t **r)
{
	kcl_int_t *t1 = NULL, *t2 = NULL, *t3 = NULL, *lambda = NULL, *rx = NULL, *ry = NULL;
	kcl_err_t err;

	/* add zero? */
	if (a->identity)
		return(kcl_ecp_copy(b, r));
	if (b->identity)
		return(kcl_ecp_copy(a, r));

	if (kcl_int_comp(a->x, b->x) == 0) {
		/* a == b ? */
		if (kcl_int_comp(a->y, b->y) == 0)
			return(kcl_ec_double(ec, a, r));
		else
			return(kcl_ecp_new_identity(r));
	}

	/* lambda = (y2 - y1) / (x2 - x1) */
	(void)((err = kcl_mod_sub(ec->mod, b->y, a->y, &t1)) ||
	       (err = kcl_mod_sub(ec->mod, b->x, a->x, &t2)) ||
	       (err = kcl_mod_mulinv(ec->mod, t2, &t3)) ||
	       (err = kcl_mod_mul(ec->mod, t1, t3, &lambda)));
	kcl_ec_int_dispose(t1); t1 = NULL;
	kcl_ec_int_dispose(t2); t2 = NULL;
	kcl_ec_int_dispose(t3); t3 = NULL;
	if (err) goto bail;

	/* rx = lambda^2 - x1 - x2 */
	(void)((err = kcl_mod_square(ec->mod, lambda, &t1)) ||
	       (err = kcl_mod_sub(ec->mod, t1, a->x, &t2)) ||
	       (err = kcl_mod_sub(ec->mod, t2, b->x, &rx)));
	kcl_ec_int_dispose(t1); t1 = NULL;
	kcl_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	/* ry = lambda * (x1 - rx) - y1 */
	(void)((err = kcl_mod_sub(ec->mod, a->x, rx, &t1)) ||
	       (err = kcl_mod_mul(ec->mod, lambda, t1, &t2)) ||
	       (err = kcl_mod_sub(ec->mod, t2, a->y, &ry)));
	kcl_ec_int_dispose(t1); t1 = NULL;
	kcl_ec_int_dispose(t2); t2 = NULL;
	if (err) goto bail;

	err = kcl_ecp_alloc(r);
	if (err)
		goto bail;
	(*r)->x = rx;
	(*r)->y = ry;

bail:
	kcl_ec_int_dispose(lambda);
	if (err) {
		kcl_ec_int_dispose(rx);
		kcl_ec_int_dispose(ry);
	}
	return err;
}

static int
kcl_ec_numbits(unsigned char *os, int nbytes)
{
	int i;
	unsigned char m = os[0];

	/* find out the first non-zero bit in the MSB */
	for (i = 0; i < 8 && !(m & (1 << (7 - i))); i++)
		;
	return nbytes * 8 - i;
}

static int
kcl_ec_isset(unsigned char *os, int i, int nbytes)
{
	unsigned char b = os[nbytes - 1 - (i / 8)];
	return (b & (1 << (i % 8))) != 0;
}

kcl_err_t
kcl_ec_mul(kcl_ec_t *ec, kcl_ecp_t *a, kcl_int_t *k, kcl_ecp_t **r)
{
	int i;
	uint32_t nbytes;
	unsigned char *os;
	kcl_ecp_t *t = NULL, *t1 = NULL, *t2 = NULL;
	kcl_err_t err;

	nbytes = kcl_int_sizeof(k);
	if ((os = kcl_malloc(nbytes)) == NULL)
		return KCL_ERR_NOMEM;
	if ((err = kcl_int_i2os(k, os, nbytes)) != KCL_ERR_NONE)
		return err;

	err = kcl_ecp_new_identity(&t);
	if (err) goto bail;
	for (i = kcl_ec_numbits(os, nbytes); --i >= 0;) {
		err = kcl_ec_double(ec, t, &t1);
		if (err) goto bail;
		if (kcl_ec_isset(os, i, nbytes)) {
			err = kcl_ec_add(ec, t1, a, &t2);
			if (err) goto bail;
			kcl_ecp_dispose(t1);
			t1 = t2;
			t2 = NULL;
		}
		kcl_ecp_dispose(t);
		t = t1;
		t1 = NULL;
	}
	*r = t;
	t = NULL;
bail:
	if (t2 != NULL) kcl_ecp_dispose(t2);
	if (t1 != NULL) kcl_ecp_dispose(t1);
	if (t != NULL) kcl_ecp_dispose(t);
	kcl_free(os);
	return err;
}

kcl_err_t
kcl_ec_mul2(kcl_ec_t *ec, kcl_ecp_t *a1, kcl_int_t *k1, kcl_ecp_t *a2, kcl_int_t *k2, kcl_ecp_t **r)
{
	int i, j;
	kcl_ecp_t *G[4];
	int k1size, k2size;
	unsigned char *os1 = NULL, *os2 = NULL;
	uint32_t nbytes1, nbytes2;
	kcl_ecp_t *t = NULL, *t1 = NULL, *t2 = NULL;
	kcl_err_t err;
#define isset_k1(i)	(i < k1size ? kcl_ec_isset(os1, i, nbytes1): 0)
#define isset_k2(i)	(i < k2size ? kcl_ec_isset(os2, i, nbytes2): 0)

	G[1] = a1;
	G[2] = a2;
	err = kcl_ec_add(ec, a1, a2, &G[3]);
	if (err) return(err);

	nbytes1 = kcl_int_sizeof(k1);
	if ((os1 = kcl_malloc(nbytes1)) == NULL) {
		err = KCL_ERR_NOMEM;
		goto bail;
	}
	err = kcl_int_i2os(k1, os1, nbytes1);
	if (err) goto bail;
	k1size = kcl_ec_numbits(os1, nbytes1);

	nbytes2 = kcl_int_sizeof(k2);
	if ((os2 = kcl_malloc(nbytes2)) == NULL) {
		err = KCL_ERR_NOMEM;
		goto bail;
	}
	err = kcl_int_i2os(k2, os2, nbytes2);
	if (err) goto bail;
	k2size = kcl_ec_numbits(os2, nbytes2);

	err = kcl_ecp_new_identity(&t);
	if (err) goto bail;
	for (i = MAX(k1size, k2size); --i >= 0;) {
		err = kcl_ec_double(ec, t, &t1);
		if (err) goto bail;
		j = kcl_ec_isset(os1, i, nbytes1) | (kcl_ec_isset(os2, i, nbytes2) << 1);
		if (j != 0) {
			err = kcl_ec_add(ec, t1, G[j], &t2);
			if (err) goto bail;
			kcl_ecp_dispose(t1);
			t1 = t2;
			t2 = NULL;
		}
		kcl_ecp_dispose(t);
		t = t1;
		t1 = NULL;
	}
	*r = t;
	t = NULL;
bail:
	if (t2 != NULL) kcl_ecp_dispose(t2);
	if (t1 != NULL) kcl_ecp_dispose(t1);
	if (t != NULL) kcl_ecp_dispose(t);
	kcl_ecp_dispose(G[3]);
	if (os1 != NULL) kcl_free(os1);
	if (os2 != NULL) kcl_free(os2);
	return err;
}


/*
 * Edwards Curve
 */

kcl_err_t
kcl_ed_alloc(kcl_ed_t **edp)
{
	if ((*edp = kcl_malloc(sizeof(kcl_ed_t))) == NULL)
		return KCL_ERR_NOMEM;
	c_memset(*edp, 0, sizeof(kcl_ed_t));
	return KCL_ERR_NONE;
}

kcl_err_t
kcl_ed_init(kcl_ed_t *ed, kcl_mod_t *mod, kcl_int_t *d)
{
	ed->mod = mod;
	ed->d = d;
	return KCL_ERR_NONE;
}

void
kcl_ed_dispose(kcl_ed_t *ed)
{
	kcl_free(ed);
}

kcl_err_t
kcl_ed_add(kcl_ed_t *ed, kcl_ecp_t *a, kcl_ecp_t *b, kcl_ecp_t *r)
{
	bn_mod_t *mod = ed->mod;
	bn_point_t *rr;
	bn_point_t p, q;
	kcl_err_t err;

	p.x = a->x->data;
	p.y = a->y->data;
	q.x = b->x->data;
	q.y = b->y->data;
	rr = bn_ed_add(mod, NULL, &p, &q, ed->d->data);
	(void)((err = z_set_result(&r->x, rr->x)) || (err = z_set_result(&r->y, rr->y)));
	bn_freebuf(mod->ctx, rr);
	return err;
}

kcl_err_t
kcl_ed_mul(kcl_ed_t *ed, kcl_ecp_t *p, kcl_int_t *k, kcl_ecp_t *r)
{
	bn_mod_t *mod = ed->mod;
	bn_point_t *rr;
	bn_point_t pp;
	kcl_err_t err;

	pp.x = p->x->data;
	pp.y = p->y->data;
	rr = bn_ed_mul(mod, NULL, &pp, k->data, ed->d->data);
	(void)((err = z_set_result(&r->x, rr->x)) || (err = z_set_result(&r->y, rr->y)));
	bn_freebuf(mod->ctx, rr);
	return err;
}

