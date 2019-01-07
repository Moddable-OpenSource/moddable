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

#include "xsAll.h"
#include "xsBigIntEx.h"

#ifndef howmany
#define howmany(x, y)	(((x) + (y) - 1) / (y))
#endif
#ifndef MAX
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#endif

/*
 * common functions
 */

void fxBigInt_setBigInt(txSlot *slot, txBigInt *a)
{
	slot->kind = XS_BIGINT_KIND;
	slot->value.bigint = *a;
}

/*
 * modular arithmetics
 */
txBigInt *fxBigInt_mod_add(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m)
{
	txBigInt *c;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	c = fxBigInt_add(the, NULL, a, b);
	fxBigInt_mod(the, r, c, m);
	fxBigInt_free(the, c);
	return r;
}

txBigInt *fxBigInt_mod_sub(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m)
{
	txBigInt *c;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	c = fxBigInt_sub(the, NULL, a, b);
	if (c->sign) {
		while (c->sign)
			fxBigInt_add(the, c, c, m);
	}
	else {
		while (fxBigInt_ucomp(c, m) >= 0)
			fxBigInt_sub(the, c, c, m);
	}
	fxBigInt_copy(r, c);
	fxBigInt_free(the, c);
	return r;
}

txBigInt *fxBigInt_mod_inv(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m)
{
	return fxBigInt_mod_sub(the, r, m, a, m);
}

txBigInt *fxBigInt_mod_mod(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m)
{
	return fxBigInt_mod(the, r, a, m);
}

txBigInt *fxBigInt_mod_mul(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m)
{
	txBigInt *c;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	c = fxBigInt_mul(the, NULL, a, b);
	fxBigInt_mod(the, r, c, m);
	fxBigInt_free(the, c);
	return r;
}

txBigInt *fxBigInt_mod_square(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m)
{
	txBigInt *c;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	c = fxBigInt_sqr(the, NULL, a);
	fxBigInt_mod(the, r, c, m);
	fxBigInt_free(the, c);
	return r;
}

txBigInt *fxBigInt_mod_mulinv_general(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m)
{
	txBigInt *u, *v, *A, *B, *C, *D;
	int sz;
	int m_is_odd = m->data[0] & 1;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	sz = MAX(a->size, m->size) + 1;
	u = fxBigInt_alloc(the, sz);
	fxBigInt_copy(u, m);
	v = fxBigInt_alloc(the, sz);
	fxBigInt_copy(v, a);
	B = fxBigInt_alloc(the, m->size + 1);
	B->size = 1;
	B->data[0] = 0;
	D = fxBigInt_alloc(the, m->size + 1);
	D->size = 1;
	D->data[0] = 1;
	if (!m_is_odd) {
		/* not needed if m is odd */
		A = fxBigInt_alloc(the, a->size + 1);
		A->size = 1;
		A->data[0] = 1;
		C = fxBigInt_alloc(the, a->size + 1);
		C->size = 1;
		C->data[0] = 0;
	}
	else {
		A = C = NULL;
	}
top:
	while (!(u->data[0] & 1)) {
		fxBigInt_ulsr1(the, u, u, 1);
		if (!m_is_odd) {
			if (!(A->data[0] & 1) && !(B->data[0] & 1)) {
				fxBigInt_ulsr1(the, A, A, 1);
				fxBigInt_ulsr1(the, B, B, 1);
			}
			else {
				fxBigInt_add(the, A, A, a);
				fxBigInt_ulsr1(the, A, A, 1);
				fxBigInt_sub(the, B, B, m);
				fxBigInt_ulsr1(the, B, B, 1);
			}
		}
		else {
			if (!(B->data[0] & 1))
				fxBigInt_ulsr1(the, B, B, 1);
			else {
				fxBigInt_sub(the, B, B, m);
				fxBigInt_ulsr1(the, B, B, 1);
			}
		}
	}
	while (!(v->data[0] & 1)) {
		fxBigInt_ulsr1(the, v, v, 1);
		if (!m_is_odd) {
			if (!(C->data[0] & 1) && !(D->data[0] & 1)) {
				fxBigInt_ulsr1(the, C, C, 1);
				fxBigInt_ulsr1(the, D, D, 1);
			}
			else {
				fxBigInt_add(the, C, C, a);
				fxBigInt_ulsr1(the, C, C, 1);
				fxBigInt_sub(the, D, D, m);
				fxBigInt_ulsr1(the, D, D, 1);
			}
		}
		else {
			if (!(D->data[0] & 1))
				fxBigInt_ulsr1(the, D, D, 1);
			else {
				fxBigInt_sub(the, D, D, m);
				fxBigInt_ulsr1(the, D, D, 1);
			}
		}
	}
	if (fxBigInt_ucomp(u, v) >= 0) {
		fxBigInt_sub(the, u, u, v);
		if (!m_is_odd)
			fxBigInt_sub(the, A, A, C);
		fxBigInt_sub(the, B, B, D);
	}
	else {
		fxBigInt_sub(the, v, v, u);
		if (!m_is_odd)
			fxBigInt_sub(the, C, C, A);
		fxBigInt_sub(the, D, D, B);
	}
	if (u->size == 1 && u->data[0] == 0) {
		if (D->sign) {
			while (D->sign)
				fxBigInt_add(the, D, D, m);
		}
		else {
			while (fxBigInt_ucomp(D, m) >= 0)
				fxBigInt_sub(the, D, D, m);
		}
		fxBigInt_copy(r, D);
	}
	else
		goto top;
	if (C != NULL)
		fxBigInt_free(the, C);
	if (A != NULL)
		fxBigInt_free(the, A);
	fxBigInt_free(the, D);
	fxBigInt_free(the, B);
	fxBigInt_free(the, v);
	fxBigInt_free(the, u);
	return r;
}

txBigInt *fxBigInt_div2(txMachine* the, txBigInt *q, txBigInt *a, txBigInt *b, txBigInt **r)
{
	q = fxBigInt_udiv(the, q, a, b, r);
	if (a->sign) {
		q->sign = !q->sign;
		if (r != NULL) {
			if (!fxBigInt_iszero(*r))
				(*r)->sign = !(*r)->sign;
		}
	}
	if (b->sign)
		q->sign = !q->sign;
	return(q);
}

txBigInt *fxBigInt_mod_mulinv_euclid(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m)
{
	txBigInt *x, *y, *b, *q, *t, *t1, *m1;
	unsigned int sz;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	sz = MAX(a->size, m->size) + 1;
	x = fxBigInt_alloc(the, sz);
	x->size = 1;
	x->data[0] = 1;
	y = fxBigInt_alloc(the, sz);
	y->size = 1;
	y->data[0] = 0;
	q = fxBigInt_alloc(the, sz);
	t = fxBigInt_alloc(the, sz);
	t1 = fxBigInt_alloc(the, sz);
	b = fxBigInt_alloc(the, sz);
	fxBigInt_copy(b, a);
	m1 = fxBigInt_dup(the, m);
	while (b->size > 1 || (b->size == 1 && b->data[0] > 1)) {
		fxBigInt_div2(the, q, b, m1, &t);
		fxBigInt_copy(b, m1);
		fxBigInt_copy(m1, t);
		fxBigInt_copy(t, y);
		fxBigInt_mul(the, t1, q, y);
		fxBigInt_sub(the, y, x, t1);
		fxBigInt_copy(x, t);
	}
	if (x->sign)
		fxBigInt_add(the, x, x, m);
	fxBigInt_copy(r, x);
	fxBigInt_free(the, m1);
	fxBigInt_free(the, b);
	fxBigInt_free(the, t1);
	fxBigInt_free(the, t);
	fxBigInt_free(the, q);
	fxBigInt_free(the, y);
	fxBigInt_free(the, x);
	return r;
}

static txBigInt *mod_exp_init(txMachine *the, txBigInt *r, txBigInt *m)
{
	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	r->size = 1;
	r->data[0] = 1;
	return r;
}

txBigInt *fxBigInt_mod_exp_LR(txMachine *the, txBigInt *r, txBigInt *b, txBigInt *e, txBigInt *m)
{
	int i;
	txBigInt *t, *t2 = NULL;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	t = mod_exp_init(the, NULL, m);
	if (fxBigInt_ucomp(b, m) > 0)
		t2 = b = fxBigInt_mod(the, NULL, b, m);
	for (i = fxBigInt_bitsize(e); --i >= 0;) {
		fxBigInt_mod_square(the, t, t, m);
		if (fxBigInt_isset(e, i))
			fxBigInt_mod_mul(the, t, t, b, m);
	}
	fxBigInt_copy(r, t);
	if (t2 != NULL)
		fxBigInt_free(the, t2);
	fxBigInt_free(the, t);
	return r;
}

/*
 * Montgomery method
 */
static txU4 mont_init(txBigInt *m)
{
	txU4 a = m->data[0];
	txU4 x, s, d;
	int n;

	x = 1;
	for (n = mxBigIntWordSize - 1, s = 2; --n >= 0; s <<= 1) {
		d = a * x;
		if (d & s)
			x += s;
	}
	return ~x + 1;
}

static txBigInt *mont_reduction(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u)
{
	txBigInt *t1, *t2;
	int n;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	n = m->size;
	t2 = fxBigInt_alloc(the, n + 1);
	t1 = fxBigInt_alloc(the, MAX(t2->size, a->size) + 1);
	fxBigInt_copy(t1, a);
	while (--n >= 0) {
		txU4 u1 = u * t1->data[0];	/* mod b */
		fxBigInt_umul1(the, t2, m, u1);
		fxBigInt_uadd(the, t1, t1, t2);
		fxBigInt_ulsr1(the, t1, t1, mxBigIntWordSize);
	}
	if (fxBigInt_ucomp(t1, m) >= 0)
		fxBigInt_sub(the, t1, t1, m);
	fxBigInt_copy(r, t1);
	fxBigInt_free(the, t1);
	fxBigInt_free(the, t2);
	return r;
}

static txBigInt *mont_in(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m)
{
	txBigInt *ta;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	ta = fxBigInt_ulsl1(the, NULL, a, m->size * mxBigIntWordSize);
	fxBigInt_mod(the, r, ta, m);
	fxBigInt_free(the, ta);
	return r;
}

static txBigInt *mont_out(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u)
{
	/* convert x' -> Mont(x', 1) = x'R^{-1} mod m -- same as Montgomery reduction */
	return mont_reduction(the, r, a, m, u);
}

static txBigInt *mont_mul(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m, txU4 u)
{
	int i, n, sz;
	txBigInt *t1, *t2, *t3;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	sz = MAX(b->size + 1, m->size + 1) + 1;
	t1 = fxBigInt_alloc(the, sz);
	t2 = fxBigInt_alloc(the, sz);
	t3 = fxBigInt_alloc(the, sz + 1);
	t3->size = 1;
	t3->data[0] = 0;
	for (i = 0, n = m->size; i < n; i++) {
		txU4 s = i < a->size ? a->data[i] : 0;
		txU4 u1 = (t3->data[0] + s * b->data[0]) * u;
		fxBigInt_umul1(the, t1, b, s);
		fxBigInt_umul1(the, t2, m, u1);
		fxBigInt_uadd(the, t1, t1, t2);
		fxBigInt_uadd(the, t3, t3, t1);
		fxBigInt_ulsr1(the, t3, t3, mxBigIntWordSize);
	}
	if (fxBigInt_ucomp(t3, m) >= 0)
		fxBigInt_sub(the, t3, t3, m);
	fxBigInt_copy(r, t3);
	fxBigInt_free(the, t3);
	fxBigInt_free(the, t2);
	fxBigInt_free(the, t1);
	return r;
}

static txBigInt *mont_square(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u)
{
	txBigInt *t;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	t = fxBigInt_sqr(the, NULL, a);
	mont_reduction(the, r, t, m, u);
	fxBigInt_free(the, t);
	return r;
}

static txBigInt *mont_exp_init(txMachine *the, txBigInt *r, txBigInt *m)
{
	txBigInt *t;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	t = fxBigInt_alloc(the, m->size + 1);
	fxBigInt_fill0(t);
	t->data[t->size - 1] = 1;
	r = fxBigInt_mod(the, r, t, m);
	fxBigInt_free(the, t);
	return r;
}

txBigInt *fxBigInt_mont_exp_LR(txMachine *the, txBigInt *r, txBigInt *b, txBigInt *e, txBigInt *m)
{
	int i;
	txBigInt *t;
	txU4 u;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	u = mont_init(m);
	t = mont_exp_init(the, NULL, m);
	if (fxBigInt_ucomp(b, m) > 0)
		b = fxBigInt_mod(the, NULL, b, m);
	b = mont_in(the, NULL, b, m);
	for (i = fxBigInt_bitsize(e); --i >= 0;) {
		mont_square(the, t, t, m, u);
		if (fxBigInt_isset(e, i))
			mont_mul(the, t, t, b, m, u);
	}
	mont_out(the, r, t, m, u);
	fxBigInt_free(the, t);
	return r;
}

txBigInt *fxBigInt_mont_exp_SW(txMachine *the, txBigInt *r, txBigInt *b, txBigInt *e, txBigInt *m, int param)
{
	unsigned int expLen;
	int i, k, gsize;
	txBigInt *A;
	txBigInt *g[64];
	txU4 u;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	u = mont_init(m);
	A = mont_exp_init(the, NULL, m);
	if (fxBigInt_ucomp(b, m) > 0)
		b = fxBigInt_mod(the, NULL, b, m);
	b = mont_in(the, NULL, b, m);
	expLen = fxBigInt_bitsize(e);
	if (param > 0)
		k = param;
	else
		k = (expLen <= 18 ? 1 : (expLen <= 36 ? 2 : (expLen <= 134 ? 3 : (expLen <= 536 ? 4 : (expLen <= 1224 ? 5 : (expLen <= 1342 ? 6 : 7))))));
	gsize = 1 << (k - 1);
	for (i = 0; i < gsize; i++)
		g[i] = fxBigInt_alloc(the, m->size);
	fxBigInt_copy(g[0], b);
	if (k > 1) {
		txBigInt *t = mont_square(the, NULL, b, m, u);
		for (i = 1; i < gsize; i++)
			g[i] = mont_mul(the, NULL, t, g[i - 1], m, u);
	}
	for (i = expLen - 1; i >= 0;) {
		if (!fxBigInt_isset(e, i)) {
			mont_square(the, A, A, m, u);
			--i;
		}
		else {
			int j, l;
			int power = 0;
			int shift = 0;
			for (j = l = i; j >= 0 && i - j + 1 <= k; --j) {
				if (fxBigInt_isset(e, j)) {
					l = j;
					power = (power << (shift + 1)) | 1;
					shift = 0;
				}
				else
					shift++;
			}
			mont_square(the, A, A, m, u);
			for (j = 1; j < i - l + 1; j++)
				mont_square(the, A, A, m, u);
			mont_mul(the, A, A, g[(power-1)>>1], m, u);
			i = l - 1;
		}
	}
	mont_out(the, r, A, m, u);
	for (i = 0; i < gsize; i++)
		fxBigInt_free(the, g[i]);
	fxBigInt_free(the, A);
	return r;
}

/*
 * EC(p)
 */
#define fxBigInt_mod_mulinv	fxBigInt_mod_mulinv_general

void fxBigInt_ec_double(txMachine *the, txECPoint *r, txECPoint *a, txECParam *ec)
{
	txBigInt *t1, *t2, *t3, *t4, *t5, *t6, *lambda, *rx, *ry;

	if (a->identity || fxBigInt_iszero(a->y)) {
		r->identity = true;
		return;
	}

	r->identity = false;
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);

	/* lambda = (3*x^2 + a) / 2*y */
	lambda = fxBigInt_alloc(the, ec->m->size);
	t1 = fxBigInt_mod_square(the, NULL, a->x, ec->m);		/* t1 = x^2 */
	t2 = fxBigInt_mod_add(the, NULL, t1, t1, ec->m);		/* t2 = t1 + t1 = t1 * 2 */
	t3 = fxBigInt_mod_add(the, NULL, t2, t1, ec->m);		/* t3 = t1 + t2 = t1 * 3 */
	t4 = fxBigInt_mod_add(the, NULL, t3, ec->a, ec->m);
	t5 = fxBigInt_mod_add(the, NULL, a->y, a->y, ec->m);
	t6 = fxBigInt_mod_mulinv(the, NULL, t5, ec->m);
	fxBigInt_mod_mul(the, lambda, t4, t6, ec->m);

	fxBigInt_free(the, t6);
	fxBigInt_free(the, t5);
	fxBigInt_free(the, t4);
	fxBigInt_free(the, t3);
	fxBigInt_free(the, t2);
	fxBigInt_free(the, t1);

	/* rx = lambda^2 - 2*x */
	rx = fxBigInt_alloc(the, ec->m->size);
	t1 = fxBigInt_mod_square(the, NULL, lambda, ec->m);		/* t1 = lambda^2 */
	t2 = fxBigInt_mod_sub(the, NULL, t1, a->x, ec->m);		/* t2 = t1 - x */
	fxBigInt_mod_sub(the, rx, t2, a->x, ec->m);			/* rx = t2 - x = t1 - 2*x */
	fxBigInt_free(the, t2);
	fxBigInt_free(the, t1);

	/* ry = lambda * (x - rx) - y */
	ry = fxBigInt_alloc(the, ec->m->size);
	t1 = fxBigInt_mod_sub(the, NULL, a->x, rx, ec->m);
	t2 = fxBigInt_mod_mul(the, NULL, lambda, t1, ec->m);
	fxBigInt_mod_sub(the, ry, t2, a->y, ec->m);
	fxBigInt_free(the, t2);
	fxBigInt_free(the, t1);

	fxBigInt_copy(r->y, ry);
	fxBigInt_free(the, ry);
	fxBigInt_copy(r->x, rx);
	fxBigInt_free(the, rx);
	fxBigInt_free(the, lambda);
}

static void ec_copy(txMachine *the, txECPoint *r, txECPoint *a)
{
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, a->x->size);
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, a->y->size);
	fxBigInt_copy(r->x, a->x);
	fxBigInt_copy(r->y, a->y);
	r->identity = a->identity;
}

void fxBigInt_ec_add(txMachine *the, txECPoint *r, txECPoint *a, txECPoint *b, txECParam *ec)
{
	txBigInt *t1, *t2, *t3, *lambda, *rx, *ry;

	/* add zero? */
	if (a->identity) {
		ec_copy(the, r, b);
		return;
	}
	if (b->identity) {
		ec_copy(the, r, a);
		return;
	}

	if (fxBigInt_ucomp(a->x, b->x) == 0) {
		/* a == b ? */
		if (fxBigInt_ucomp(a->y, b->y) == 0) {
			fxBigInt_ec_double(the, r, a, ec);
			return;
		}
		else {
			r->identity = true;
			return;
		}
	}

	r->identity = false;
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);

	/* lambda = (y2 - y1) / (x2 - x1) */
	lambda = fxBigInt_alloc(the, ec->m->size);
	t1 = fxBigInt_mod_sub(the, NULL, b->y, a->y, ec->m);
	t2 = fxBigInt_mod_sub(the, NULL, b->x, a->x, ec->m);
	t3 = fxBigInt_mod_mulinv(the, NULL, t2, ec->m);
	fxBigInt_mod_mul(the, lambda, t1, t3, ec->m);
	fxBigInt_free(the, t3);
	fxBigInt_free(the, t2);
	fxBigInt_free(the, t1);

	/* rx = lambda^2 - x1 - x2 */
	rx = fxBigInt_alloc(the, ec->m->size);
	t1 = fxBigInt_mod_square(the, NULL, lambda, ec->m);
	t2 = fxBigInt_mod_sub(the, NULL, t1, a->x, ec->m);
	fxBigInt_mod_sub(the, rx, t2, b->x, ec->m);
	fxBigInt_free(the, t2);
	fxBigInt_free(the, t1);

	/* ry = lambda * (x1 - rx) - y1 */
	ry = fxBigInt_alloc(the, ec->m->size);
	t1 = fxBigInt_mod_sub(the, NULL, a->x, rx, ec->m);
	t2 = fxBigInt_mod_mul(the, NULL, lambda, t1, ec->m);
	fxBigInt_mod_sub(the, ry, t2, a->y, ec->m);
	fxBigInt_free(the, t2);
	fxBigInt_free(the, t1);

	fxBigInt_copy(r->y, ry);
	fxBigInt_free(the, ry);
	fxBigInt_copy(r->x, rx);
	fxBigInt_free(the, rx);
	fxBigInt_free(the, lambda);
}

void fxBigInt_ec_mul(txMachine *the, txECPoint *r, txECPoint *a, txBigInt *k, txECParam *ec)
{
	int i;

	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);
	r->identity = true;
	for (i = fxBigInt_bitsize(k); --i >= 0;) {
		fxBigInt_ec_double(the, r, r, ec);
		if (fxBigInt_isset(k, i))
			fxBigInt_ec_add(the, r, r, a, ec);
	}
}

void fxBigInt_ec_mul2(txMachine *the, txECPoint *r, txECPoint *a1, txBigInt *k1, txECPoint *a2, txBigInt *k2, txECParam *ec)
{
	int i, j;
	txECPoint *G[4], g3;
	int k1size, k2size;
#define isset_k1(i)	(i < k1size ? fxBigInt_isset(k1, i): 0)
#define isset_k2(i)	(i < k2size ? fxBigInt_isset(k2, i): 0)

	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);
	r->identity = true;

	G[1] = a1;
	G[2] = a2;
	g3.x = g3.y = NULL;
	fxBigInt_ec_add(the, &g3, a1, a2, ec);
	G[3] = &g3;
	k1size = fxBigInt_bitsize(k1);
	k2size = fxBigInt_bitsize(k2);
	for (i = MAX(k1size, k2size); --i >= 0;) {
		fxBigInt_ec_double(the, r, r, ec);
		j = isset_k1(i) | (isset_k2(i) << 1);
		if (j != 0)
			fxBigInt_ec_add(the, r, r, G[j], ec);
	}
	fxBigInt_free(the, g3.x);
	fxBigInt_free(the, g3.y);
}
