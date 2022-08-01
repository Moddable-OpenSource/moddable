/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
#include "xsmc.h"

extern txBigInt gxBigIntZero;
extern txBigInt gxBigIntOne;

#ifndef howmany
#define howmany(x, y)	(((x) + (y) - 1) / (y))
#endif
#ifndef MAX
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#endif

#define MAX_PELM	30
#define FREE_MASK	((txU2)1 << (sizeof(txU2) * 8 - 1))

typedef struct pool {
	txBigInt *pelm[MAX_PELM];
} pool_t;

static void pool_init(pool_t *pool)
{
	c_memset(pool, 0, sizeof(pool_t));
}

static txBigInt *pool_get(xsMachine *the, txU2 size, pool_t *pool)
{
	// search for a free slot that fits into the size
	for (int i = 0; i < MAX_PELM; i++) {
		txBigInt *e = pool->pelm[i];
		if (e == NULL) {
			e = fxBigInt_alloc(the, size);
			pool->pelm[i] = e;
			return e;
		}
		if (e->size & FREE_MASK && (e->size & ~FREE_MASK) == size) {
			e->size &= ~FREE_MASK;
			return e;
		}
	}
	fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	return NULL;
}

static void pool_put(txBigInt *e, txU2 size)
{
	e->size = FREE_MASK | size;
}

static void pool_dispose(xsMachine *the, pool_t *pool)
{
	for (int i = MAX_PELM; --i >= 0;) {
		if (pool->pelm[i] != NULL)
			fxBigInt_free(the, pool->pelm[i]);
	}
}

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
txBigInt *fxBigInt_mod_add(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m, pool_t *pool)
{
	txBigInt *c = pool_get(the, m->size + 1, pool);
	
	fxBigInt_add(the, c, a, b);
	while (fxBigInt_ucomp(c, m) >= 0)
		fxBigInt_sub(the, c, c, m);
	fxBigInt_copy(r, c);
	pool_put(c, m->size + 1);
	return r;
}

txBigInt *fxBigInt_mod_dbl(txMachine *the, txBigInt *r, txBigInt *a, int k, txBigInt *m, pool_t *pool)
{
	txBigInt *t = pool_get(the, m->size + 1, pool);

	fxBigInt_copy(r, a);
	while (--k >= 0) {
		fxBigInt_ulsl1(the, t, r, 1);
		while (fxBigInt_ucomp(t, m) >= 0)
			fxBigInt_sub(the, t, t, m);
		fxBigInt_copy(r, t);
	}
	pool_put(t, m->size + 1);
	return r;
}

txBigInt *fxBigInt_mod_sub(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m, pool_t *pool)
{
	txBigInt *c = pool_get(the, m->size + 1, pool);

	fxBigInt_sub(the, c, a, b);
	if (c->sign) {
		while (c->sign)
			fxBigInt_add(the, c, c, m);
	}
	else {
		while (fxBigInt_ucomp(c, m) >= 0)
			fxBigInt_sub(the, c, c, m);
	}
	fxBigInt_copy(r, c);
	pool_put(c, m->size + 1);
	return r;
}

txBigInt *fxBigInt_mod_inv(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, pool_t *pool)
{
	return fxBigInt_mod_sub(the, r, m, a, m, pool);
}

txBigInt *fxBigInt_mod_mod(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, pool_t *pool)
{
	return fxBigInt_mod(the, r, a, m);
}

txBigInt *fxBigInt_mod_mul(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m, pool_t *pool)
{
	txU2 mSize = m->size * 2;
	txBigInt *c = pool_get(the, mSize, pool);

	c = fxBigInt_mul(the, c, a, b);
	fxBigInt_mod(the, r, c, m);
	pool_put(c, mSize);
	return r;
}

txBigInt *fxBigInt_mod_square(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, pool_t *pool)
{
	txU2 mSize = m->size * 2;
	txBigInt *c = pool_get(the, mSize, pool);

	fxBigInt_sqr(the, c, a);
	fxBigInt_mod(the, r, c, m);
	pool_put(c, mSize);
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
	pool_t pool;
	
	pool_init(&pool);
	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	t = pool_get(the, m->size, &pool);
	t = mod_exp_init(the, t, m);
	if (fxBigInt_ucomp(b, m) > 0) {
		t2 = pool_get(the, m->size, &pool);
		b = fxBigInt_mod(the, t2, b, m);
	}
	for (i = fxBigInt_bitsize(e); --i >= 0;) {
		fxBigInt_mod_square(the, t, t, m, &pool);
		if (fxBigInt_isset(e, i))
			fxBigInt_mod_mul(the, t, t, b, m,&pool);
	}
	fxBigInt_copy(r, t);
	pool_dispose(the, &pool);
	return r;
}

/*
 * Montgomery method
 */
txU4 mont_init(txBigInt *m)
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

txBigInt *mont_reduction(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u, pool_t *pool)
{
	txBigInt *t1, *t2;
	txU2 t1siz, t2siz;
	int n;

	n = m->size;
	t2siz = n + 1;
	t1siz = MAX(t2siz, a->size) + 1;
	t2 = pool_get(the, t2siz, pool);
	t1 = pool_get(the, t1siz, pool);
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
	pool_put(t1, t1siz);
	pool_put(t2, t2siz);
	return r;
}

txBigInt *mont_in(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m)
{
	txBigInt *ta;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	ta = fxBigInt_ulsl1(the, NULL, a, m->size * mxBigIntWordSize);
	fxBigInt_mod(the, r, ta, m);
	fxBigInt_free(the, ta);
	return r;
}

txBigInt *mont_out(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u)
{
	/* convert x' -> Mont(x', 1) = x'R^{-1} mod m -- same as Montgomery reduction */
	pool_t pool;	// must be a small pool
	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	pool_init(&pool);
	mont_reduction(the, r, a, m, u, &pool);
	pool_dispose(the, &pool);
	return r;
}

static txBigInt *mont_mul(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m, txU4 u, pool_t *pool)
{
	int i, n, sz;
	txBigInt *t1, *t2, *t3;

	sz = MAX(b->size + 1, m->size + 1) + 1;
	t1 = pool_get(the, sz, pool);
	t2 = pool_get(the, sz, pool);
	t3 = pool_get(the, sz + 1, pool);
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
	pool_put(t3, sz + 1);
	pool_put(t2, sz);
	pool_put(t1, sz);
	return r;
}

static txBigInt *mont_square(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u, pool_t *pool)
{
	txU2 aSize = a->size * 2;
	txBigInt *t = pool_get(the, aSize, pool);

	fxBigInt_sqr(the, t, a);
	mont_reduction(the, r, t, m, u, pool);
	pool_put(t, aSize);
	return r;
}

static txBigInt *mont_exp_init(txMachine *the, txBigInt *r, txBigInt *m, pool_t *pool)
{
	txBigInt *t = pool_get(the, m->size + 1, pool);

	fxBigInt_fill0(t);
	t->data[t->size - 1] = 1;
	r = fxBigInt_mod(the, r, t, m);
	pool_put(t, m->size + 1);
	return r;
}

txBigInt *fxBigInt_mont_exp_LR(txMachine *the, txBigInt *r, txBigInt *b, txBigInt *e, txBigInt *m)
{
	int i;
	txBigInt *t;
	txU4 u;
	pool_t pool;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	pool_init(&pool);
	u = mont_init(m);
	t = pool_get(the, m->size, &pool);
	mont_exp_init(the, t, m, &pool);
	if (fxBigInt_ucomp(b, m) > 0)
		b = fxBigInt_mod(the, pool_get(the, m->size, &pool), b, m);
	b = mont_in(the, pool_get(the, m->size, &pool), b, m);
	for (i = fxBigInt_bitsize(e); --i >= 0;) {
		mont_square(the, t, t, m, u, &pool);
		if (fxBigInt_isset(e, i))
			mont_mul(the, t, t, b, m, u, &pool);
	}
	mont_out(the, r, t, m, u);
	pool_dispose(the, &pool);
	return r;
}

txBigInt *fxBigInt_mont_exp_SW(txMachine *the, txBigInt *r, txBigInt *b, txBigInt *e, txBigInt *m, int param)
{
	unsigned int expLen;
	int i, k, gsize;
	txBigInt *A;
	txBigInt *g[64];
	txU4 u;
	pool_t pool;

	if (r == NULL)
		r = fxBigInt_alloc(the, m->size);
	pool_init(&pool);
	u = mont_init(m);
	A = mont_exp_init(the, pool_get(the, m->size, &pool), m, &pool);
	if (fxBigInt_ucomp(b, m) > 0)
		b = fxBigInt_mod(the, pool_get(the, m->size, &pool), b, m);
	b = mont_in(the, pool_get(the, m->size, &pool), b, m);
	expLen = fxBigInt_bitsize(e);
	if (param > 0)
		k = param;
	else
		k = (expLen <= 18 ? 1 : (expLen <= 36 ? 2 : (expLen <= 134 ? 3 : (expLen <= 536 ? 4 : (expLen <= 1224 ? 5 : (expLen <= 1342 ? 6 : 7))))));
	gsize = 1 << (k - 1);
	for (i = 0; i < gsize; i++)
		g[i] = pool_get(the, m->size, &pool);
	fxBigInt_copy(g[0], b);
	if (k > 1) {
		txBigInt *t = mont_square(the, pool_get(the, m->size, &pool), b, m, u, &pool);
		for (i = gsize; --i >= 1;)
			mont_mul(the, g[i], t, g[i - 1], m, u, &pool);
	}
	for (i = expLen - 1; i >= 0;) {
		if (!fxBigInt_isset(e, i)) {
			mont_square(the, A, A, m, u, &pool);
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
			mont_square(the, A, A, m, u, &pool);
			for (j = 1; j < i - l + 1; j++)
				mont_square(the, A, A, m, u, &pool);
			mont_mul(the, A, A, g[(power-1)>>1], m, u, &pool);
			i = l - 1;
		}
	}
	mont_out(the, r, A, m, u);
	pool_dispose(the, &pool);
	return r;
}

/*
 * EC(p)
 */
#define fxBigInt_mod_mulinv	fxBigInt_mod_mulinv_general
#define fxBigInt_mont_square(the, r, a, m, u, pool)	mont_square(the, r, a, m, u, pool)
#define fxBigInt_mont_mul(the, r, a, b, m, u, pool)	mont_mul(the, r, a, b, m, u, pool)

/*
 https://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian.html
 The "dbl-2007-bl" doubling formulas [database entry; Sage verification script; Sage output; three-operand code]:
 Cost: 1M + 8S + 1*a + 10add + 2*2 + 1*3 + 1*8.
 Source: 2007 Bernstein–Lange.
 Explicit formulas:
	   XX = X1^2
	   YY = Y1^2
	   YYYY = YY^2
	   ZZ = Z1^2
	   S = 2*((X1+YY)^2-XX-YYYY)
	   M = 3*XX+a*ZZ^2
	   T = M^2-2*S
	   X3 = T
	   Y3 = M*(S-T)-8*YYYY
	   Z3 = (Y1+Z1)^2-YY-ZZ
 */

static void ec_double(txMachine *the, txECPoint *r, txECPoint *a, txECParam *ec, pool_t *pool)
{
	txBigInt *XX, *YY, *YYYY, *ZZ, *S, *M, *T, *Y3, *Z3;
	txBigInt *t1, *t2;
	txBigInt *m = ec->m;
	
	if (fxBigInt_iszero(a->z)) {
		fxBigInt_copy(r->z, a->z);
		return;
	}

	t1 = pool_get(the, m->size, pool);
	t2 = pool_get(the, m->size, pool);
	XX = pool_get(the, m->size, pool);
	YY = pool_get(the, m->size, pool);
	YYYY = pool_get(the, m->size, pool);
	ZZ = pool_get(the, m->size, pool);
	S = pool_get(the, m->size, pool);
	M = pool_get(the, m->size, pool);
	T = pool_get(the, m->size, pool);
	Y3 = pool_get(the, m->size, pool);
	Z3 = pool_get(the, m->size, pool);
	
	XX = fxBigInt_mont_square(the, XX, a->x, m, ec->u, pool);
	YY = fxBigInt_mont_square(the, YY, a->y, m, ec->u, pool);
	YYYY = fxBigInt_mont_square(the, YYYY, YY, m, ec->u, pool);
	ZZ = fxBigInt_mont_square(the, ZZ, a->z, m, ec->u, pool);
	t1 = fxBigInt_mod_add(the, t1, a->x, YY, m, pool);
	t1 = fxBigInt_mont_square(the, t1, t1, m, ec->u, pool);
	t1 = fxBigInt_mod_sub(the, t1, t1, XX, m, pool);
	t1 = fxBigInt_mod_sub(the, t1, t1, YYYY, m, pool);
	S = fxBigInt_mod_dbl(the, S, t1, 1, m, pool);
	t1 = fxBigInt_mod_dbl(the, t1, XX, 1, m, pool);
	t1 = fxBigInt_mod_add(the, t1, t1, XX, m, pool);
	t2 = fxBigInt_mont_square(the, t2, ZZ, m, ec->u, pool);
	t2 = fxBigInt_mont_mul(the, t2, t2, ec->a, m, ec->u, pool);
	M = fxBigInt_mod_add(the, M, t1, t2, m, pool);
	t1 = fxBigInt_mont_square(the, t1, M, m, ec->u, pool);
	t2 = fxBigInt_mod_dbl(the, t2, S, 1, m, pool);
	T = fxBigInt_mod_sub(the, T, t1, t2, m, pool);
	t1 = fxBigInt_mod_sub(the, t1, S, T, m, pool);
	t1 = fxBigInt_mont_mul(the, t1, t1, M, m, ec->u, pool);
	t2 = fxBigInt_mod_dbl(the, t2, YYYY, 3, m, pool);
	Y3 = fxBigInt_mod_sub(the, Y3, t1, t2, m, pool);
	t1 = fxBigInt_mod_add(the, t1, a->y, a->z, m, pool);
	t1 = fxBigInt_mont_square(the, t1, t1, m, ec->u, pool);
	t1 = fxBigInt_mod_sub(the, t1, t1, YY, m, pool);
	Z3 = fxBigInt_mod_sub(the, Z3, t1, ZZ, m, pool);
	
	fxBigInt_copy(r->z, Z3);
	fxBigInt_copy(r->y, Y3);
	fxBigInt_copy(r->x, T);
	
	pool_put(Z3, m->size);
	pool_put(Y3, m->size);
	pool_put(T, m->size);
	pool_put(M, m->size);
	pool_put(S, m->size);
	pool_put(ZZ, m->size);
	pool_put(YYYY, m->size);
	pool_put(YY, m->size);
	pool_put(XX, m->size);
	pool_put(t2, m->size);
	pool_put(t1, m->size);
}

void fxBigInt_ec_double(txMachine *the, txECPoint *r, txECPoint *a, txECParam *ec)
{
	pool_t pool;
	
	if (r->z == NULL)
		r->z = fxBigInt_alloc(the, ec->m->size);
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);
	pool_init(&pool);
	ec_double(the, r, a, ec, &pool);
	pool_dispose(the, &pool);
}

/*
  https://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian.html
  add-2007-bl:
      Z1Z1 = Z1^2
      Z2Z2 = Z2^2
      U1 = X1*Z2Z2
      U2 = X2*Z1Z1
      S1 = Y1*Z2*Z2Z2
      S2 = Y2*Z1*Z1Z1
      H = U2-U1
      I = (2*H)^2
      J = H*I
      r = 2*(S2-S1)
      V = U1*I
      X3 = r^2-J-2*V
      Y3 = r*(V-X3)-2*S1*J
      Z3 = ((Z1+Z2)^2-Z1Z1-Z2Z2)*H
 */

static void ec_add(txMachine *the, txECPoint *r, txECPoint *a, txECPoint *b, txECParam *ec, pool_t *pool)
{
    txBigInt *Z1Z1, *Z2Z2, *U1, *U2, *S1, *S2, *H, *I, *J, *R, *V, *X3, *Y3, *Z3;
    txBigInt *t1, *t2;
    txBigInt *m = ec->m;

	if (fxBigInt_iszero(a->z)) {
		fxBigInt_copy(r->x, b->x);
		fxBigInt_copy(r->y, b->y);
		fxBigInt_copy(r->z, b->z);
		return;
	}
	else if (fxBigInt_iszero(b->z)) {
		fxBigInt_copy(r->x, a->x);
		fxBigInt_copy(r->y, a->y);
		fxBigInt_copy(r->z, a->z);
		return;
	}
	
	t1 = pool_get(the, m->size, pool);
	t2 = pool_get(the, m->size, pool);
	Z1Z1 = pool_get(the, m->size, pool);
	Z2Z2 = pool_get(the, m->size, pool);
	U1 = pool_get(the, m->size, pool);
	U2 = pool_get(the, m->size, pool);
	S1 = pool_get(the, m->size, pool);
	S2 = pool_get(the, m->size, pool);

    Z1Z1 = fxBigInt_mont_square(the, Z1Z1, a->z, m, ec->u, pool);
    Z2Z2 = fxBigInt_mont_square(the, Z2Z2, b->z, m, ec->u, pool);
    U1 = fxBigInt_mont_mul(the, U1, a->x, Z2Z2, m, ec->u, pool);
    U2 = fxBigInt_mont_mul(the, U2, b->x, Z1Z1, m, ec->u, pool);
	t1 = fxBigInt_mont_mul(the, t1, a->y, b->z, m, ec->u, pool);
	S1 = fxBigInt_mont_mul(the, S1, t1, Z2Z2, m, ec->u, pool);
	t1 = fxBigInt_mont_mul(the, t1, b->y, a->z, m, ec->u, pool);
	S2 = fxBigInt_mont_mul(the, S2, t1, Z1Z1, m, ec->u, pool);

	if (fxBigInt_ucomp(U1, U2) == 0) {
		int eq = fxBigInt_ucomp(S1, S2) == 0;
		pool_put(S2, m->size);
		pool_put(S1, m->size);
		pool_put(U2, m->size);
		pool_put(U1, m->size);
		pool_put(Z2Z2, m->size);
		pool_put(Z1Z1, m->size);
		pool_put(t2, m->size);
		pool_put(t1, m->size);
		if (eq)
			ec_double(the, r, a, ec, pool);
		else {
			// zero?
			fxBigInt_copy(r->z, &gxBigIntZero);
		}
		return;
	}

	H = pool_get(the, m->size, pool);
	I = pool_get(the, m->size, pool);
	J = pool_get(the, m->size, pool);
	R = pool_get(the, m->size, pool);
	V = pool_get(the, m->size, pool);
	X3 = pool_get(the, m->size, pool);
	Y3 = pool_get(the, m->size, pool);
	Z3 = pool_get(the, m->size, pool);

    H = fxBigInt_mod_sub(the, H, U2, U1, m, pool);
	t1 = fxBigInt_mod_dbl(the, t1, H, 1, m, pool);
	I = fxBigInt_mont_square(the, I, t1, m, ec->u, pool);
	J = fxBigInt_mont_mul(the, J, H, I, m, ec->u, pool);
	t1 = fxBigInt_mod_sub(the, t1, S2, S1, m, pool);
	R = fxBigInt_mod_dbl(the, R, t1, 1, m, pool);
	V = fxBigInt_mont_mul(the, V, U1, I, m, ec->u, pool);
	t1 = fxBigInt_mont_square(the, t1, R, m, ec->u, pool);
	t1 = fxBigInt_mod_sub(the, t1, t1, J, m, pool);
	t2 = fxBigInt_mod_dbl(the, t2, V, 1, m, pool);
	X3 = fxBigInt_mod_sub(the, X3, t1, t2, m, pool);
	t1 = fxBigInt_mod_sub(the, t1, V, X3, m, pool);
	t1 = fxBigInt_mont_mul(the, t1, t1, R, m, ec->u, pool);
	t2 = fxBigInt_mont_mul(the, t2, S1, J, m, ec->u, pool);
	t2 = fxBigInt_mod_dbl(the, t2, t2, 1, m, pool);
	Y3 = fxBigInt_mod_sub(the, Y3, t1, t2, m, pool);
	t1 = fxBigInt_mod_add(the, t1, a->z, b->z, m, pool);
	t1 = fxBigInt_mont_square(the, t1, t1, m, ec->u, pool);
	t1 = fxBigInt_mod_sub(the, t1, t1, Z1Z1, m, pool);
	t1 = fxBigInt_mod_sub(the, t1, t1, Z2Z2, m, pool);
	Z3 = fxBigInt_mont_mul(the, Z3, t1, H, m, ec->u, pool);

	fxBigInt_copy(r->z, Z3);
	fxBigInt_copy(r->y, Y3);
	fxBigInt_copy(r->x, X3);

	pool_put(Z3, m->size);
	pool_put(Y3, m->size);
	pool_put(X3, m->size);
	pool_put(V, m->size);
	pool_put(R, m->size);
	pool_put(J, m->size);
	pool_put(I, m->size);
	pool_put(H, m->size);
	pool_put(S2, m->size);
	pool_put(S1, m->size);
	pool_put(U2, m->size);
	pool_put(U1, m->size);
	pool_put(Z2Z2, m->size);
	pool_put(Z1Z1, m->size);
	pool_put(t2, m->size);
	pool_put(t1, m->size);
}

void fxBigInt_ec_add(txMachine *the, txECPoint *r, txECPoint *a, txECPoint *b, txECParam *ec)
{
	pool_t pool;
	
	if (r->z == NULL)
		r->z = fxBigInt_alloc(the, ec->m->size);
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);
	pool_init(&pool);
	ec_add(the, r, a, b, ec, &pool);
	pool_dispose(the, &pool);
}

void fxBigInt_ec_mul(txMachine *the, txECPoint *r, txECPoint *a, txBigInt *k, txECParam *ec)
{
	int i;
	pool_t pool;

	if (r->z == NULL)
		r->z = fxBigInt_alloc(the, ec->m->size);
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);
	fxBigInt_copy(r->z, &gxBigIntZero);
	
	pool_init(&pool);
	for (i = fxBigInt_bitsize(k); --i >= 0;) {
		ec_double(the, r, r, ec, &pool);
		if (fxBigInt_isset(k, i)) {
			ec_add(the, r, r, a, ec, &pool);
		}
	}
	pool_dispose(the, &pool);
}

void fxBigInt_ec_mul2(txMachine *the, txECPoint *r, txECPoint *a1, txBigInt *k1, txECPoint *a2, txBigInt *k2, txECParam *ec)
{
	int i, j;
	txECPoint *G[4], g3;
	int k1size, k2size;
	pool_t pool;
#define isset_k1(i)	(i < k1size ? fxBigInt_isset(k1, i): 0)
#define isset_k2(i)	(i < k2size ? fxBigInt_isset(k2, i): 0)

	if (r->z == NULL)
		r->z = fxBigInt_alloc(the, ec->m->size);
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);
	fxBigInt_copy(r->z, &gxBigIntZero);

	pool_init(&pool);
	G[1] = a1;
	G[2] = a2;
	g3.x = pool_get(the, ec->m->size, &pool);
	g3.y = pool_get(the, ec->m->size, &pool);
	g3.z = pool_get(the, ec->m->size, &pool);
	ec_add(the, &g3, a1, a2, ec, &pool);
	G[3] = &g3;
	k1size = fxBigInt_bitsize(k1);
	k2size = fxBigInt_bitsize(k2);
	for (i = MAX(k1size, k2size); --i >= 0;) {
		ec_double(the, r, r, ec, &pool);
		j = isset_k1(i) | (isset_k2(i) << 1);
		if (j != 0)
			ec_add(the, r, r, G[j], ec, &pool);
	}
	pool_dispose(the, &pool);
}

void fxBigInt_ec_norm(xsMachine *the, txECPoint *r, txECPoint *a, txECParam *ec)
{
	txBigInt *inv, *inv2, *inv3;
	pool_t pool;
	
	if (r->z == NULL)
		r->z = fxBigInt_alloc(the, ec->m->size);
	if (r->y == NULL)
		r->y = fxBigInt_alloc(the, ec->m->size);
	if (r->x == NULL)
		r->x = fxBigInt_alloc(the, ec->m->size);

	if (fxBigInt_iszero(a->z) || fxBigInt_ucomp(a->z, ec->one) == 0) {
		fxBigInt_copy(r->x, a->x);
		fxBigInt_copy(r->y, a->y);
		fxBigInt_copy(r->z, a->z);
		return;
	}
	
	pool_init(&pool);
	inv = pool_get(the, ec->m->size, &pool);
	inv2 = pool_get(the, ec->m->size, &pool);
	inv3 = pool_get(the, ec->m->size, &pool);
	inv = mont_out(the, inv, a->z, ec->m, ec->u);
	inv = fxBigInt_mod_mulinv(the, inv, inv, ec->m);
	inv = mont_in(the, inv, inv, ec->m);
	inv2 = fxBigInt_mont_square(the, inv2, inv, ec->m, ec->u, &pool);
	inv3 = fxBigInt_mont_mul(the, inv3, inv2, inv, ec->m, ec->u, &pool);
	
	fxBigInt_mont_mul(the, r->x, a->x, inv2, ec->m, ec->u, &pool);
	fxBigInt_mont_mul(the, r->y, a->y, inv3, ec->m, ec->u, &pool);
	fxBigInt_copy(r->z, ec->one);
	pool_dispose(the, &pool);
}
