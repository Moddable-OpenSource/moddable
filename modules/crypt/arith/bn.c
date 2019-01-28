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

#include "bn.h"
#ifndef NULL
#define NULL	((void *)0)
#endif

#define bn_high_word(x)		((bn_word)((x) >> 32))
#define bn_low_word(x)		((bn_word)(x))
#define bn_wordsize		(sizeof(bn_word) * 8)

#ifndef howmany
#define howmany(x, y)	(((x) + (y) - 1) / (y))
#endif
#ifndef MAX
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#endif

static void
bn_memset(void *data, int c, unsigned int n)
{
	unsigned char *p = data;

	while (n-- != 0)
		*p++ = c;
}

static void
bn_memcpy(void *dst, const void *src, unsigned int n)
{
	unsigned char *p1 = dst;
	const unsigned char *p2 = src;

	while (n-- != 0)
		*p1++ = *p2++;
}

static void
bn_memmove(void *dst, const void *src, unsigned int n)
{
	unsigned char *p1;
	const unsigned char *p2;

	if (src >= dst) {
		p1 = dst;
		p2 = src;
		while (n-- != 0)
			*p1++ = *p2++;
	}
	else {
		p1 = (unsigned char *)dst + n;
		p2 = (unsigned char *)src + n;
		while (n-- != 0)
			*--p1 = *--p2;
	}
}

static bn_t *
bn_alloct(bn_context_t *ctx, unsigned int n)
{
	bn_t *r;
	unsigned int sz = sizeof(bn_t) + ((n-1) * sizeof(bn_word));

	r = (bn_t *)bn_allocbuf(ctx, sz);
	r->sign = 0;
	r->size = n;
	/* do not have to clear the data */
	return(r);
}

int
bn_iszero(bn_t *a)
{
	return(a->size == 1 && a->data[0] == 0);
}

int
bn_isNaN(bn_t *a)
{
	return(a->size == 0);
}

bn_bool
bn_sign(bn_t *a)
{
	return(a->sign);
}

bn_size
bn_wsizeof(bn_t *a)
{
	return(a->size);
}

void
bn_negate(bn_t *a)
{
	a->sign = !a->sign;
}

static void
bn_fill0(bn_t *r)
{
	bn_memset(r->data, 0, r->size * sizeof(bn_word));
}

void
bn_copy(bn_t *a, bn_t *b)
{
	bn_memcpy(a->data, b->data, b->size * sizeof(bn_word));
	a->size = b->size;
	a->sign = b->sign;
}

static bn_t *
bn_dup(bn_context_t *ctx, bn_t *a)
{
	bn_t *r = bn_alloct(ctx, a->size);

	bn_copy(r, a);
	return(r);
}

static int
bn_ffs(bn_t *a)
{
	int i;
	bn_word w = a->data[a->size - 1];

	for (i = 0; i < (int)bn_wordsize && !(w & ((bn_word)1 << (bn_wordsize - 1 - i))); i++)
		;
	return(i);
}

int
bn_bitsize(bn_t *e)
{
	return(e->size * bn_wordsize - bn_ffs(e));
}

static int
bn_isset(bn_t *e, unsigned int i)
{
	bn_word w = e->data[i / bn_wordsize];
	return((w & ((bn_word)1 << (i % bn_wordsize))) != 0);
}

static int
bn_ucomp(bn_t *a, bn_t *b)
{
	int i;

	if (a->size != b->size)
		return(a->size > b->size ? 1: -1);
	for (i = a->size; --i >= 0;) {
		if (a->data[i] != b->data[i])
			return(a->data[i] > b->data[i] ? 1: -1);
	}
	return(0);
}

int
bn_comp(bn_t *a, bn_t *b)
{
	if (a->sign != b->sign)
		return(a->sign ? -1: 1);
	else if (a->sign)
		return(bn_ucomp(b, a));
	else
		return(bn_ucomp(a, b));
}

/*
 * bitwise operations
 */

bn_t *
bn_lsl(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw)
{
	unsigned int wsz, bsz;
	int n;

	/* 'r' can be the same as 'a' */
	/* assume 'r' is large enough if 'r' is present */
	if (r == NULL)
		r = bn_alloct(ctx, a->size + howmany(sw, bn_wordsize));
	wsz = sw / bn_wordsize;
	bsz = sw % bn_wordsize;
	if (bsz == 0) {
		bn_memmove(&r->data[wsz], a->data, a->size * sizeof(bn_word));
		bn_memset(r->data, 0, wsz * sizeof(bn_word));
		r->size = a->size + wsz;
	}
	else {
		r->data[a->size + wsz] = a->data[a->size - 1] >> (bn_wordsize - bsz);
		for (n = a->size; --n >= 1;)
			r->data[n + wsz] = (a->data[n] << bsz) | (a->data[n - 1] >> (bn_wordsize - bsz));
		r->data[wsz] = a->data[0] << bsz;
		/* clear the remaining part */
		for (n = wsz; --n >= 0;)
			r->data[n] = 0;
		/* adjust r */
		r->size = a->size + wsz + (r->data[a->size + wsz] == 0 ? 0: 1);
	}
	return(r);
}

bn_t *
bn_lsr(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw)
{
	int wsz, bsz;
	int i, n;

	wsz = sw / bn_wordsize;
	bsz = sw % bn_wordsize;
	n = a->size - wsz;
	if (n <= 0) {
		if (r == NULL) r = bn_alloct(ctx, 1);
		r->size = 1;
		r->data[0] = 0;
		return(r);
	}
	/* 'r' can be the same as 'a' */
	if (r == NULL)
		r = bn_alloct(ctx, n);
	if (bsz == 0) {
		bn_memmove(r->data, &a->data[wsz], n * sizeof(bn_word));
		r->size = n;
	}
	else {
		for (i = 0; i < n - 1; i++)
			r->data[i] = (a->data[i + wsz] >> bsz) | (a->data[i + wsz + 1] << (bn_wordsize - bsz));
		r->data[i] = a->data[i + wsz] >> bsz;
		r->size = (n > 1 && r->data[n - 1] == 0) ? n - 1: n;
	}
	return(r);
}

bn_t *
bn_xor(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	int i;

	if (a->size < b->size) {
		bn_t *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = bn_alloct(ctx, a->size);
	for (i = 0; i < b->size; i++)
		r->data[i] = a->data[i] ^ b->data[i];
	for (; i < a->size; i++)
		r->data[i] = a->data[i];
	return(r);
}

bn_t *
bn_or(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	int i;

	if (a->size < b->size) {
		bn_t *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = bn_alloct(ctx, a->size);
	for (i = 0; i < b->size; i++)
		r->data[i] = a->data[i] | b->data[i];
	for (; i < a->size; i++)
		r->data[i] = a->data[i];
	return(r);
}

bn_t *
bn_and(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	int i;

	if (a->size > b->size) {
		bn_t *t = b;
		b = a;
		a = t;
	}
	if (r == NULL)
		r = bn_alloct(ctx, a->size);
	for (i = 0; i < a->size; i++)
		r->data[i] = a->data[i] & b->data[i];
	return(r);
}

/*
 * arith on Z
 */

static int
bn_uadd_prim(bn_word *rp, bn_word *ap, bn_word *bp, int an, int bn)
{
	bn_word a, b, t, r, c = 0;
	int i;

	for (i = 0; i < an; i++) {
		a = ap[i];
		b = bp[i];
		t = a + b;
		r = t + c;
		rp[i] = r;
		c = t < a || r < t;
	}
	for (; i < bn; i++) {
		r = bp[i] + c;
		rp[i] = r;
		c = r < c;
	}
	return(c);
}

static bn_t *
bn_uadd(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	bn_t *x, *y;
	int c;

	if (aa->size < bb->size) {
		x = aa;
		y = bb;
	}
	else {
		x = bb;
		y = aa;
	}
	if (rr == NULL)
		rr = bn_alloct(ctx, y->size + 1);
	c = bn_uadd_prim(rr->data, x->data, y->data, x->size, y->size);
	/* CAUTION: rr may equals aa or bb. do not touch until here */
	rr->size = y->size;
	rr->sign = 0;
	if (c != 0)
		rr->data[rr->size++] = 1;
	return(rr);
}

static bn_t *
bn_usub(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	int i, n;
	bn_word a, b, r, t;
	bn_word *ap, *bp, *rp;
	unsigned int c = 0;

	if (rr == NULL)
		rr = bn_alloct(ctx, MAX(aa->size, bb->size));
	rr->sign = (aa->size < bb->size ||
		    (aa->size == bb->size && bn_ucomp(aa, bb) < 0));
	if (rr->sign) {
		bn_t *tt = aa;
		aa = bb;
		bb = tt;
	}
	ap = aa->data;
	bp = bb->data;
	rp = rr->data;
	n = MIN(aa->size, bb->size);
	for (i = 0; i < n; i++) {
		a = ap[i];
		b = bp[i];
		t = a - b;
		r = t - c;
		rp[i] = r;
		c = a < b || r > t;
	}
	if (aa->size >= bb->size) {
		n = aa->size;
		for (; i < n; i++) {
			t = ap[i];
			r = t - c;
			rp[i] = r;
			c = r > t;
		}
	}
	else {
		n = bb->size;
		for (; i < n; i++) {
			t = -bp[i];
			r = t - c;
			rp[i] = r;
			c = r > t;
		}
	}
	/* remove leading 0s */
	while (--i > 0 && rp[i] == 0)
		;
	rr->size = i + 1;
	return(rr);
}

bn_t *
bn_add(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	if ((aa->sign ^ bb->sign) == 0) {
		rr = bn_uadd(ctx, rr, aa, bb);
		if (aa->sign)
			rr->sign = 1;
	}
	else {
		if (!aa->sign)
			rr = bn_usub(ctx, rr, aa, bb);
		else
			rr = bn_usub(ctx, rr, bb, aa);
	}
	return(rr);
}

bn_t *
bn_sub(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	if ((aa->sign ^ bb->sign) == 0) {
		if (!aa->sign)
			rr = bn_usub(ctx, rr, aa, bb);
		else
			rr = bn_usub(ctx, rr, bb, aa);
	}
	else {
		bn_bool sign = aa->sign;	/* could be replaced if rr=aa */
		rr = bn_uadd(ctx, rr, aa, bb);
		rr->sign = sign;
	}
	return(rr);
}

static bn_t *
bn_umul(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	bn_dword a, b, r;
	bn_word *ap, *bp, *rp;
	bn_word c = 0;
	int i, j, n, m;

	if (rr == NULL)
		rr = bn_alloct(ctx, aa->size + bb->size);
	bn_fill0(rr);
	ap = aa->data;
	bp = bb->data;
	rp = rr->data;
	n = bb->size;
	for (i = 0, j = 0; i < n; i++) {
		b = (bn_dword)bp[i];
		c = 0;
		m = aa->size;
		for (j = 0; j < m; j++) {
			a = (bn_dword)ap[j];
			r = a * b + c;
			r += (bn_dword)rp[i + j];
			rp[i + j] = bn_low_word(r);
			c = bn_high_word(r);
		}
		rp[i + j] = c;
	}
	/* remove leading 0s */
	for (n = i + j; --n > 0 && rp[n] == 0;)
		;
	rr->size = n + 1;
	return(rr);
}

bn_t *
bn_mul(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb)
{
	rr = bn_umul(ctx, rr, aa, bb);
	if (aa->sign != bb->sign)
		rr->sign = 1;
	return(rr);
}

static bn_t *
bn_umul1(bn_context_t *ctx, bn_t *r, bn_t *a, bn_word b)
{
	int i, n;
	bn_word c = 0;
	bn_word *ap, *rp;
	bn_dword wa, wr;

	if (r == NULL)
		r = bn_alloct(ctx, a->size + 1);
	ap = a->data;
	rp = r->data;
	n = a->size;
	for (i = 0; i < n; i++) {
		wa = (bn_dword)ap[i];
		wr = wa * b + c;
		c = bn_high_word(wr);
		rp[i] = bn_low_word(wr);
	}
	if (c != 0)
		rp[i] = c;
	else {
		/* remove leading 0s */
		while (--i > 0 && rp[i] == 0)
			;
	}
	r->size = i + 1;
	return(r);
}

bn_t *
bn_square(bn_context_t *ctx, bn_t *r, bn_t *a)
{
	int i, j, t;
	bn_word *ap, *rp;
	bn_dword uv, t1, t2, t3, ai;
	bn_word c, cc;
	bn_word overflow = 0;	/* overflow flag of 'u' */

	if (r == NULL)
		r = bn_alloct(ctx, a->size * 2);
	bn_fill0(r);
	t = a->size;
	ap = a->data;
	rp = r->data;

	for (i = 0; i < t - 1; i++) {
		uv = (bn_dword)ap[i] * ap[i] + rp[i * 2];
		rp[i * 2] = bn_low_word(uv);
		c = bn_high_word(uv);
		cc = 0;
		ai = ap[i];
		for (j = i + 1; j < t; j++) {
			int k = i + j;
			t1 = ai * ap[j];
			t2 = t1 + c + ((bn_dword)cc << bn_wordsize);	/* 'cc:c' must be <= 2(b-1) so no overflow here */
			t3 = t1 + t2;
			uv = t3 + rp[k];
			cc = t3 < t1 || uv < t3;
			c = (bn_word)bn_high_word(uv);
			rp[k] = bn_low_word(uv);
		}
		c += overflow;
		rp[i + t] = c;		/* c = u */
		overflow = cc || c < overflow;
	}
	/* the last loop */
	uv = (bn_dword)ap[i] * ap[i] + rp[i * 2];
	rp[i * 2] = bn_low_word(uv);
	rp[i + t] = bn_high_word(uv) + overflow;

	/* remove leading 0s */
	for (i = 2*t; --i > 0 && rp[i] == 0;)
		;
	r->size = i + 1;
	return(r);
}

static void
bn_makepoly(bn_t *r, bn_t *a, int t)
{
	int n;
	bn_word *rp, *ap;

	/* make up a polynomial a_t*b^n + a_{t-1}*b^{n-1} + ... */
	n = r->size;
	rp = &r->data[n];
	ap = a->data;
	while (--n >= 0) {
		*--rp = t < 0 ? 0: ap[t];
		--t;
	}
}

#if BN_NO_ULDIVMOD
static bn_dword
div64_32(bn_dword a, bn_word b)
{
	bn_word high = (bn_word)(a >> 32);
	bn_dword r = 0, bb = b, d = 1;

	if (high >= b) {
		high /= b;
		r = (bn_dword)high << 32;
		a -= (bn_dword)(high * b) << 32;
	}
	while ((long long)bb > 0 && bb < a) {
		bb += bb;
		d += d;
	}
	do {
		if (a >= bb) {
			a -= bb;
			r += d;
		}
		bb >>= 1;
		d >>= 1;
	} while (d != 0);
	return r;
}
#endif

static bn_t *
bn_udiv(bn_context_t *ctx, bn_t *q, bn_t *a, bn_t *b, bn_t **r)
{
	int sw;
	bn_t *nb, *na, *tb, *a2, *b2, *tb2, *tb3;
	int i, n, t;
	bn_word *qp, *ap, *bp;
#define mk_dword(p, i)	(((bn_dword)(p)[i] << bn_wordsize) | (p)[i - 1])

	if (bn_ucomp(a, b) < 0) {
		if (q == NULL) {
			q = bn_alloct(ctx, 1);
		}
		else {
			q->sign = 0;
			q->size = 1;
		}
		q->data[0] = 0;
		if (r != NULL) {
			if (*r == NULL)
				*r = bn_dup(ctx, a);
			else
				bn_copy(*r, a);
			(*r)->sign = 0;
		}
		return(q);
	}

	/* CAUTION: if q is present, it must take account of normalization */
	if (q == NULL)
		q = bn_alloct(ctx, a->size - b->size + 2);
	if (r != NULL && *r == NULL)
		*r = bn_alloct(ctx, b->size);

	/* normalize */
	sw = bn_ffs(b);
	nb = bn_lsl(ctx, NULL, b, sw);
	na = bn_lsl(ctx, NULL, a, sw);
	t = nb->size - 1;	/* the size must not change from 'b' */
	n = na->size - 1;

	/* adjust size of q */
	q->size = na->size - nb->size + 1;
	bn_fill0(q);	/* set 0 to quotient */

	/* process the most significant word */
	qp = &q->data[q->size - 1];
	tb = bn_lsl(ctx, NULL, nb, (n - t) * bn_wordsize);	/* y*b^n */
	if (bn_ucomp(na, tb) >= 0) {
		(*qp)++;
		bn_sub(ctx, na, na, tb);
		/* since nomalization done, must be na < tb here */
	}

	/* prepare the constant for the adjustment: y_t*b + y_{t-1} */
	b2 = bn_alloct(ctx, 2);
	bn_makepoly(b2, nb, t);
	/* and allocate for temporary buffer */
	a2 = bn_alloct(ctx, 3);
	tb2 = bn_alloct(ctx, 3);
	tb3 = bn_alloct(ctx, tb->size + 1);

	ap = na->data;
	bp = nb->data;
	for (i = n; i >= t + 1; --i) {
		bn_word tq;

		/* the first estimate */
#if BN_NO_ULDIVMOD
		tq = (ap[i] == bp[t]) ? ~(bn_word)0: (bn_word)div64_32(mk_dword(ap, i), bp[t]);
#else
		tq = (ap[i] == bp[t]) ? ~(bn_word)0: (bn_word)(mk_dword(ap, i) / bp[t]);
#endif

		/* adjust */
		bn_makepoly(a2, na, i);
		while (bn_ucomp(bn_umul1(ctx, tb2, b2, tq), a2) > 0)
			--tq;

		/* next x */
		bn_lsr(ctx, tb, tb, bn_wordsize);
		bn_usub(ctx, na, na, bn_umul1(ctx, tb3, tb, tq));
		if (na->sign) {
			bn_add(ctx, na, na, tb);
			--tq;
		}
		*--qp = tq;
	}
	if (r != NULL)
		*r = bn_lsr(ctx, *r, na, sw);
	/* remove leading 0s from q */
	for (i = q->size; --i > 0 && q->data[i] == 0;)
		;
	q->size = i + 1;
	bn_freebuf(ctx, nb);
	return(q);
}

bn_t *
bn_div(bn_context_t *ctx, bn_t *q, bn_t *a, bn_t *b, bn_t **r)
{
	q = bn_udiv(ctx, q, a, b, r);
	if (a->sign) {
		q->sign = !q->sign;
		if (r) {
			/* (*r)->sign = !(*r)->sign;*/
			bn_sub(ctx, *r, b, *r);
		}
	}
	if (b->sign)
		q->sign = !q->sign;
	return(q);
}

bn_t *
bn_mod(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b)
{
	(void)bn_div(ctx, NULL, a, b, &r);
	/* cannot deallocate 'q' here */
	return(r);
}

int
bn_inc(bn_t *a, bn_word d)
{
	bn_word t[1];

	t[0] = d;
	return bn_uadd_prim(a->data, t, a->data, 1, a->size);
}


/*
 * modular arithmetics
 */

/* modular inverse of 2^{sizeof(bn_word)} */
static bn_word
modinv1(bn_word a)
{
	bn_word x, s, d;
	int n;

	x = 1;
	for (n = bn_wordsize - 1, s = 2; --n >= 0; s <<= 1) {
		d = a * x;
		if (d & s)
			x += s;
	}
	return(x);
}

/*
 * Montgomery method
 */
static bn_t *
mont_reduction(bn_mod_t *mod, bn_t *r, bn_t *a)
{
	bn_t *t1, *t2;
	int n;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	n = mod->m->size;
	t2 = bn_alloct(mod->ctx, n + 1);
	t1 = bn_alloct(mod->ctx, MAX(t2->size, a->size) + 1);
	bn_copy(t1, a);
	while (--n >= 0) {
		bn_word u = mod->u * t1->data[0];	/* mod b */
		bn_umul1(mod->ctx, t2, mod->m, u);
		bn_uadd(mod->ctx, t1, t1, t2);
		bn_lsr(mod->ctx, t1, t1, bn_wordsize);
	}
	if (bn_comp(t1, mod->m) >= 0)
		bn_sub(mod->ctx, t1, t1, mod->m);
	bn_copy(r, t1);
	bn_freebuf(mod->ctx, t2);
	return(r);
}

static bn_t *
mont_in(bn_mod_t *mod, bn_t *r, bn_t *a)
{
	bn_t *ta;

	/* convert x -> Mont(x, R^2 mod m) = xR mod m */
	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	ta = bn_lsl(mod->ctx, NULL, a, mod->m->size * bn_wordsize);
	bn_mod(mod->ctx, r, ta, mod->m);
	bn_freebuf(mod->ctx, ta);
	return(r);
}

static bn_t *
mont_out(bn_mod_t *mod, bn_t *r, bn_t *a)
{
	/* convert x' -> Mont(x', 1) = x'R^{-1} mod m -- same as Montgomery reduction */
	return(mont_reduction(mod, r, a));
}

static bn_t *
mont_mul(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b)
{
	int i, n, sz;
	bn_t *t1, *t2, *t3;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	sz = MAX(b->size + 1, mod->m->size + 1) + 1;
	t1 = bn_alloct(mod->ctx, sz);
	t2 = bn_alloct(mod->ctx, sz);
	t3 = bn_alloct(mod->ctx, sz + 1);
	t3->size = 1;
	t3->data[0] = 0;
	for (i = 0, n = mod->m->size; i < n; i++) {
		bn_word s = i < a->size ? a->data[i] : 0;
		bn_word u = (t3->data[0] + s * b->data[0]) * mod->u;
		bn_umul1(mod->ctx, t1, b, s);
		bn_umul1(mod->ctx, t2, mod->m, u);
		bn_uadd(mod->ctx, t1, t1, t2);
		bn_uadd(mod->ctx, t3, t3, t1);
		bn_lsr(mod->ctx, t3, t3, bn_wordsize);
	}
	if (bn_comp(t3, mod->m) >= 0)
		bn_sub(mod->ctx, t3, t3, mod->m);
	bn_copy(r, t3);
	bn_freebuf(mod->ctx, t1);
	return(r);
}

static bn_t *
mont_square(bn_mod_t *mod, bn_t *r, bn_t *a)
{
#if 1
	bn_t *t;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	t = bn_square(mod->ctx, NULL, a);
	mont_reduction(mod, r, t);
	bn_freebuf(mod->ctx, t);
	return(r);
#else
	return mont_mul(mod, r, a, a);
#endif
}

static bn_t *
mont_exp_init(bn_mod_t *mod, bn_t *r)
{
	bn_t *t;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	t = bn_alloct(mod->ctx, mod->m->size + 1);
	bn_fill0(t);
	t->data[t->size - 1] = 1;
	r = bn_mod(mod->ctx, r, t, mod->m);
	bn_freebuf(mod->ctx, t);
	return r;
}

/*
 * modular arithmetics
 */
bn_t *
bn_mod_add(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b)
{
	bn_t *c;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	c = bn_add(mod->ctx, NULL, a, b);
	/*
	while (bn_comp(c, mod->m) >= 0)
		bn_sub(mod->ctx, c, c, mod->m);
	bn_copy(r, c);
	*/
	bn_mod(mod->ctx, r, c, mod->m);
	bn_freebuf(mod->ctx, c);
	return(r);
}

bn_t *
bn_mod_sub(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b)
{
	bn_t *c;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	c = bn_sub(mod->ctx, NULL, a, b);
	if (c->sign) {
		while (c->sign)
			bn_add(mod->ctx, c, c, mod->m);
	}
	else {
		while (bn_comp(c, mod->m) >= 0)
			bn_sub(mod->ctx, c, c, mod->m);
	}
	bn_copy(r, c);
	bn_freebuf(mod->ctx, c);
	return(r);
}

bn_t *
bn_mod_inv(bn_mod_t *mod, bn_t *r, bn_t *a)
{
	return(bn_mod_sub(mod, r, mod->m, a));
}

bn_t *
bn_mod_mod(bn_mod_t *mod, bn_t *r, bn_t *a)
{
	return(bn_mod(mod->ctx, r, a, mod->m));
}

bn_t *
bn_mod_mul(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b)
{
	bn_t *c;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	c = bn_mul(mod->ctx, NULL, a, b);
	bn_mod(mod->ctx, r, c, mod->m);
	bn_freebuf(mod->ctx, c);
	return(r);
}

bn_t *
bn_mod_square(bn_mod_t *mod, bn_t *r, bn_t *a)
{
	bn_t *c;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	c = bn_square(mod->ctx, NULL, a);
	bn_mod(mod->ctx, r, c, mod->m);
	bn_freebuf(mod->ctx, c);
	return(r);
}

bn_t *
bn_mod_mulinv(bn_mod_t *mod, bn_t *r, bn_t *a)
{
	bn_t *u, *v, *A, *B, *C, *D;
	int sz;
	int m_is_odd = mod->m->data[0] & 1;

	if (bn_iszero(a))
		bn_throw(mod->ctx, BN_ERR_DIVIDE_BY_ZERO);
	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	sz = MAX(a->size, mod->m->size) + 1;
	u = bn_alloct(mod->ctx, sz);
	bn_copy(u, mod->m);
	v = bn_alloct(mod->ctx, sz);
	bn_copy(v, a);
	B = bn_alloct(mod->ctx, mod->m->size + 1);
	B->size = 1;
	B->data[0] = 0;
	D = bn_alloct(mod->ctx, mod->m->size + 1);
	D->size = 1;
	D->data[0] = 1;
	if (!m_is_odd) {
		/* not needed if m is odd */
		A = bn_alloct(mod->ctx, a->size + 1);
		A->size = 1;
		A->data[0] = 1;
		C = bn_alloct(mod->ctx, a->size + 1);
		C->size = 1;
		C->data[0] = 0;
	}
top:
	while (!(u->data[0] & 1)) {
		bn_lsr(mod->ctx, u, u, 1);
		if (!m_is_odd) {
			if (!(A->data[0] & 1) && !(B->data[0] & 1)) {
				bn_lsr(mod->ctx, A, A, 1);
				bn_lsr(mod->ctx, B, B, 1);
			}
			else {
				bn_add(mod->ctx, A, A, a);
				bn_lsr(mod->ctx, A, A, 1);
				bn_sub(mod->ctx, B, B, mod->m);
				bn_lsr(mod->ctx, B, B, 1);
			}
		}
		else {
			if (!(B->data[0] & 1))
				bn_lsr(mod->ctx, B, B, 1);
			else {
				bn_sub(mod->ctx, B, B, mod->m);
				bn_lsr(mod->ctx, B, B, 1);
			}
		}
	}
	while (!(v->data[0] & 1)) {
		bn_lsr(mod->ctx, v, v, 1);
		if (!m_is_odd) {
			if (!(C->data[0] & 1) && !(D->data[0] & 1)) {
				bn_lsr(mod->ctx, C, C, 1);
				bn_lsr(mod->ctx, D, D, 1);
			}
			else {
				bn_add(mod->ctx, C, C, a);
				bn_lsr(mod->ctx, C, C, 1);
				bn_sub(mod->ctx, D, D, mod->m);
				bn_lsr(mod->ctx, D, D, 1);
			}
		}
		else {
			if (!(D->data[0] & 1))
				bn_lsr(mod->ctx, D, D, 1);
			else {
				bn_sub(mod->ctx, D, D, mod->m);
				bn_lsr(mod->ctx, D, D, 1);
			}
		}
	}
	if (bn_comp(u, v) >= 0) {
		bn_sub(mod->ctx, u, u, v);
		if (!m_is_odd)
			bn_sub(mod->ctx, A, A, C);
		bn_sub(mod->ctx, B, B, D);
	}
	else {
		bn_sub(mod->ctx, v, v, u);
		if (!m_is_odd)
			bn_sub(mod->ctx, C, C, A);
		bn_sub(mod->ctx, D, D, B);
	}
	if (u->size == 1 && u->data[0] == 0) {
		if (!(v->size == 1 && v->data[0] == 1))
			bn_throw(mod->ctx, BN_ERR_MISCALCULATION);
		if (D->sign) {
			while (D->sign)
				bn_add(mod->ctx, D, D, mod->m);
		}
		else {
			while (bn_comp(D, mod->m) >= 0)
				bn_sub(mod->ctx, D, D, mod->m);
		}
		bn_copy(r, D);
	}
	else
		goto top;
	bn_freebuf(mod->ctx, u);
	return(r);
}

#if BN_PREPROP
static void
bn_mod_preprocess(bn_mod_t *mod, bn_t **bp, bn_t **ep)
{
	bn_t *re;

	if ((*ep)->size <= mod->m->size)
		return;
	re = bn_lsr(mod->ctx, NULL, *ep, 8);
	*ep = bn_mod_mulinv(mod, re, re);
}
#endif

static bn_t *
bn_mod_exp_init(bn_mod_t *mod, bn_t *r)
{
	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	r->size = 1;
	r->data[0] = 1;
	return(r);
}

static bn_t *
bn_mod_exp_LR(bn_mod_t *mod, bn_t *r, bn_t *b, bn_t *e)
{
	int i;
	bn_t *t;

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	t = (*mod->exp_init)(mod, NULL);
#if BN_PREPROP
	bn_mod_preprocess(mod, &b, &e);
#endif
	if (bn_comp(b, mod->m) > 0)
		b = bn_mod(mod->ctx, NULL, b, mod->m);
	if (mod->conv_in_func != NULL)
		b = (*mod->conv_in_func)(mod, NULL, b);
	for (i = bn_bitsize(e); --i >= 0;) {
		(*mod->square_func)(mod, t, t);
		if (bn_isset(e, i))
			(*mod->mul_func)(mod, t, t, b);
	}
	if (mod->conv_out_func != NULL)
		(*mod->conv_out_func)(mod, r, t);
	else
		bn_copy(r, t);
	bn_freebuf(mod->ctx, t);
	return(r);
}

static bn_t *
bn_mod_exp2_LR(bn_mod_t *mod, bn_t *r, bn_t *b1, bn_t *e1, bn_t *b2, bn_t *e2)
{
	int i, j;
	bn_t *A, *G[4];
	int e1size, e2size;
#define isset_e1(i)	(i < e1size ? bn_isset(e1, i): 0)
#define isset_e2(i)	(i < e2size ? bn_isset(e2, i): 0)

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	A = (*mod->exp_init)(mod, NULL);
	if (bn_comp(b1, mod->m) > 0)
		b1 = bn_mod(mod->ctx, NULL, b1, mod->m);
	if (bn_comp(b2, mod->m) > 0)
		b2 = bn_mod(mod->ctx, NULL, b2, mod->m);
	if (mod->conv_in_func != NULL) {
		b1 = (*mod->conv_in_func)(mod, NULL, b1);
		b2 = (*mod->conv_in_func)(mod, NULL, b2);
	}
	/* G[0] is never used */
	G[1] = b1;
	G[2] = b2;
	G[3] = (*mod->mul_func)(mod, NULL, b1, b2);
	e1size = bn_bitsize(e1);
	e2size = bn_bitsize(e2);
	for (i = MAX(e1size, e2size); --i >= 0;) {
		(*mod->square_func)(mod, A, A);
		j = bn_isset(e1, i) | (bn_isset(e2, i) << 1);
		if (j != 0)
			(*mod->mul_func)(mod, A, A, G[j]);
	}
	if (mod->conv_out_func != NULL)
		(*mod->conv_out_func)(mod, r, A);
	else
		bn_copy(r, A);
	bn_freebuf(mod->ctx, A);
	return(r);
}

static bn_t *
bn_mod_exp_sliding_window(bn_mod_t *mod, bn_t *r, bn_t *b, bn_t *e)
{
	unsigned int expLen;
	int i, k, gsize;
	bn_t *A;
	bn_t *g[64];

	if (r == NULL)
		r = bn_alloct(mod->ctx, mod->m->size);
	A = (*mod->exp_init)(mod, NULL);
#if BN_PREPROP
	bn_mod_preprocess(mod, &b, &e);
#endif
	if (bn_comp(b, mod->m) > 0)
		b = bn_mod(mod->ctx, NULL, b, mod->m);
	if (mod->conv_in_func != NULL)
		b = (*mod->conv_in_func)(mod, NULL, b);
	expLen = bn_bitsize(e);
	if (mod->options > 0)
		k = mod->options;
	else
		k = (expLen <= 18 ? 1 : (expLen <= 36 ? 2 : (expLen <= 134 ? 3 : (expLen <= 536 ? 4 : (expLen <= 1224 ? 5 : (expLen <= 1342 ? 6 : 7))))));
	gsize = 1 << (k - 1);
	for (i = 0; i < gsize; i++)
		g[i] = bn_alloct(mod->ctx, mod->m->size);
	bn_copy(g[0], b);
	if (k > 1) {
		bn_t *t = (*mod->square_func)(mod, NULL, b);
		for (i = 1; i < gsize; i++)
			g[i] = (*mod->mul_func)(mod, NULL, t, g[i - 1]);
	}
	for (i = expLen - 1; i >= 0;) {
		if (!bn_isset(e, i)) {
			(*mod->square_func)(mod, A, A);
			--i;
		}
		else {
			int j, l;
			int power = 0;
			int shift = 0;
			for (j = l = i; j >= 0 && i - j + 1 <= k; --j) {
				if (bn_isset(e, j)) {
					l = j;
					power = (power << (shift + 1)) | 1;
					shift = 0;
				}
				else
					shift++;
			}
			(*mod->square_func)(mod, A, A);
			for (j = 1; j < i - l + 1; j++)
				(*mod->square_func)(mod, A, A);
			(*mod->mul_func)(mod, A, A, g[(power-1)>>1]);
			i = l - 1;
		}
	}
	if (mod->conv_out_func != NULL)
		(*mod->conv_out_func)(mod, r, A);
	else
		bn_copy(r, A);
	bn_freebuf(mod->ctx, A);
	return(r);
}

bn_t *
bn_mod_exp(bn_mod_t *mod, bn_t *r, bn_t *b, bn_t *e)
{
	return((*mod->exp_func)(mod, r, b, e));
}

bn_t *
bn_mod_exp2(bn_mod_t *mod, bn_t *r, bn_t *b1, bn_t *e1, bn_t *b2, bn_t *e2)
{
	return((*mod->exp2_func)(mod, r, b1, e1, b2, e2));
}

void
bn_mod_init(bn_mod_t *mod, bn_context_t *ctx, bn_t *m)
{
	mod->ctx = ctx;
	mod->m = m;
	mod->square_func = bn_mod_square;
	mod->mul_func = bn_mod_mul;
	mod->conv_in_func = NULL;
	mod->conv_out_func = NULL;
	mod->exp_init = bn_mod_exp_init;
	mod->exp_func = bn_mod_exp_LR;
	mod->exp2_func = bn_mod_exp2_LR;
}

void
bn_mod_freetbuf(bn_mod_t *mod, void *bufptr)
{
	bn_freebuf(mod->ctx, bufptr);
}

void
bn_mont_init(bn_mod_t *mod, bn_context_t *ctx, bn_t *m, bn_mod_method_t method, int options)
{
	if (!(m->data[0] & 1))
		return;
	/* pre-compute u = -m^{-1} mod b */
	mod->u = ~modinv1(mod->m->data[0]) + 1;
	/* overwrite the methods */
	mod->square_func = mont_square;
	mod->mul_func = mont_mul;
	mod->conv_in_func = mont_in;
	mod->conv_out_func = mont_out;
	mod->exp_init = mont_exp_init;
	mod->exp_func = method == BN_MOD_METHOD_SW ? bn_mod_exp_sliding_window : bn_mod_exp_LR;
	mod->exp2_func = bn_mod_exp2_LR;
	mod->options = options;
}

/*
 * Edwards curve
 */
static void
ed_add(bn_mod_t *mod, bn_point_t *r, bn_point_t *p, bn_point_t *q, bn_t *d)
{
	bn_context_t *ctx = mod->ctx;
	int size = mod->m->size;
	bn_t *x1, *y1, *x2, *y2, *t, *t1, *t2, *rx, *one;
	bn_t *(*mulf)() = mod->mul_func;

	t = bn_alloct(ctx, size);
	t1 = bn_alloct(ctx, size);
	t2 = bn_alloct(ctx, size);
	rx = bn_alloct(ctx, size);
	one = bn_alloct(ctx, 1);
	one->data[0] = 1;
	x1 = p->x;
	y1 = p->y;
	x2 = q->x;
	y2 = q->y;
	/* t = d * x1 * x2 * y1 * y2 */
	(*mulf)(mod, t, d, x1);
	(*mulf)(mod, t, t, x2);
	(*mulf)(mod, t, t, y1);
	(*mulf)(mod, t, t, y2);
	/* Q.x = (x1 * y2 + x2 * y1) * inv(1 + t) */
	(*mulf)(mod, t1, x1, y2);
	(*mulf)(mod, t2, x2, y1);
	bn_mod_add(mod, t1, t1, t2);
	bn_mod_add(mod, t2, one, t);
	bn_mod_mulinv(mod, t2, t2);
	(*mulf)(mod, rx, t1, t2);
	/* Q.y = (y1 * y2 + x1 * x2) * inv(1 - t) */
	(*mulf)(mod, t1, y1, y2);
	(*mulf)(mod, t2, x1, x2);
	bn_mod_add(mod, t1, t1, t2);
	bn_mod_sub(mod, t2, one, t);
	bn_mod_mulinv(mod, t2, t2);
	(*mulf)(mod, r->y, t1, t2);
	bn_copy(r->x, rx);	/* r can be p or q so don't overwrite it until the end */

	bn_freebuf(ctx, t);
}

bn_point_t *
bn_ed_add(bn_mod_t *mod, bn_point_t *r, bn_point_t *p, bn_point_t *q, bn_t *d)
{
	bn_context_t *ctx = mod->ctx;
	int size = mod->m->size;
	bn_point_t pp, qq;

	/* addition on the edward curve */
	if (r == NULL) {
		r = bn_allocbuf(ctx, sizeof(bn_point_t));
		r->x = bn_alloct(ctx, size);
		r->y = bn_alloct(ctx, size);
	}
	if (mod->conv_in_func != NULL) {
		pp.x = (*mod->conv_in_func)(mod, NULL, p->x);
		pp.y = (*mod->conv_in_func)(mod, NULL, p->y);
		qq.x = (*mod->conv_in_func)(mod, NULL, q->x);
		qq.y = (*mod->conv_in_func)(mod, NULL, q->y);
		p = &pp;
		q = &qq;
	}
	ed_add(mod, r, p, q, d);
	if (mod->conv_out_func != NULL) {
		(*mod->conv_out_func)(mod, r->x, r->x);
		(*mod->conv_out_func)(mod, r->y, r->y);
		bn_freebuf(ctx, pp.x);
	}
	return r;
}


static bn_point_t *
ed_zero(bn_mod_t *mod)
{
	bn_context_t *ctx = mod->ctx;
	int size = mod->m->size;
	bn_point_t *r;

	/* P(0) = (0, 1) */
	r = bn_allocbuf(ctx, sizeof(bn_point_t));
	r->x = bn_alloct(ctx, size);
	r->y = bn_alloct(ctx, size);
	r->x->data[0] = 0;
	r->x->size = 1;
	(void)(*mod->exp_init)(mod, r->y);
	return r;
}

bn_point_t *
bn_ed_mul(bn_mod_t *mod, bn_point_t *r, bn_point_t *p, bn_t *k, bn_t *d)
{
	bn_context_t *ctx = mod->ctx;
	int size = mod->m->size;
	bn_point_t *t;
	bn_point_t pp;
	int i;

	if (r == NULL) {
		r = bn_allocbuf(ctx, sizeof(bn_point_t));
		r->x = bn_alloct(ctx, size);
		r->y = bn_alloct(ctx, size);
	}
	t = ed_zero(mod);
	if (mod->conv_in_func != NULL) {
		pp.x = (*mod->conv_in_func)(mod, NULL, p->x);
		pp.y = (*mod->conv_in_func)(mod, NULL, p->y);
		p = &pp;
	}
	for (i = bn_bitsize(k); --i >= 0;) {
		ed_add(mod, t, t, t, d);
		if (bn_isset(k, i))
			ed_add(mod, t, t, p, d);
	}
	if (mod->conv_out_func != NULL) {
		(*mod->conv_out_func)(mod, r->x, t->x);
		(*mod->conv_out_func)(mod, r->y, t->y);
	}
	else {
		bn_copy(r->x, t->x);
		bn_copy(r->y, t->y);
	}
	bn_freebuf(ctx, t);
	return r;
}
