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
// the long loops below feed the watchdog on the ports that have one (xsHost.h is not included here: it is a platform
// header that changes definitions this engine-level file must see as the engine does)
#if ESP32
	#include "sdkconfig.h"
	#if CONFIG_ESP_TASK_WDT_EN
		#include "esp_task_wdt.h"
		#define modWatchDogReset() esp_task_wdt_reset()
	#else
		#define modWatchDogReset()
	#endif
#elif defined(ESP8266)		// not __ets__, which Zephyr ESP32 builds also define
	extern void system_soft_wdt_feed(void);
	#define modWatchDogReset() system_soft_wdt_feed()
#else
	#define modWatchDogReset()
#endif

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
		modWatchDogReset();
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

// the words of a Montgomery result: t has n + 1 words with t[n] in {0, 1}; make it canonical (less than m) and trim it
static txBigInt *mont_finish(txMachine *the, txBigInt *r, txBigInt *t, txBigInt *m)
{
	int n = m->size;
	t->size = (txU2)((t->data[n]) ? (n + 1) : n);
	while ((t->size > 1) && (0 == t->data[t->size - 1]))
		t->size--;
	t->sign = 0;
	if (fxBigInt_ucomp(t, m) >= 0)
		fxBigInt_sub(the, t, t, m);
	fxBigInt_copy(r, t);
	return r;
}

// r = a / R mod m for a < m * R (R = 2^(32n)): operand scanning over a copy of a, each step adding u_i * m and consuming one low word.
// The whole reduction is a single pass of word arithmetic: no intermediate big integers, no shifts of the accumulator.
txBigInt *mont_reduction(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u, pool_t *pool)
{
	int i, j, k, n = m->size, size = 2 * n + 1;
	txU4 *t, *mp, c, ui;
	txBigInt *tt = pool_get(the, size, pool);		// may allocate, which can move the operands: their data pointers are taken after

	t = tt->data;
	mp = m->data;
	for (i = 0; i < size; i++)
		t[i] = (i < a->size) ? a->data[i] : 0;
	for (i = 0; i < n; i++) {
		ui = t[i] * u;
		c = 0;
		for (j = 0; j < n; j++)
			mxBigIntMulAdd(ui, mp[j], t[i + j], c, t[i + j], c);
		for (k = i + n; c && (k < size); k++) {
			t[k] += c;
			c = t[k] < c;
		}
	}
	for (i = 0; i <= n; i++)		// the result is the high half, t[n .. 2n]
		t[i] = t[i + n];
	mont_finish(the, r, tt, m);
	pool_put(tt, size);
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

// r = a * b / R mod m for a, b < m: coarsely integrated operand scanning (CIOS). Each outer step adds a_i * b and u_i * m to the
// accumulator and drops its low word, all in one pass over the n + 2 words of t, so the whole product costs 2n^2 word multiplies
// and nothing else: no temporaries, no copies, no shifts.
static txBigInt *mont_mul(txMachine *the, txBigInt *r, txBigInt *a, txBigInt *b, txBigInt *m, txU4 u, pool_t *pool)
{
	int i, j, n = m->size, an = MIN(a->size, n), bn = MIN(b->size, n);
	txU4 *t, *bp, *mp, c, ai, ui, lo;
	txBigInt *tt = pool_get(the, n + 2, pool);		// may allocate, which can move the operands: their data pointers are taken after

	t = tt->data;
	bp = b->data;
	mp = m->data;
	for (j = 0; j < n + 2; j++)
		t[j] = 0;
	for (i = 0; i < n; i++) {
		ai = (i < an) ? a->data[i] : 0;
		c = 0;
		for (j = 0; j < bn; j++)
			mxBigIntMulAdd(ai, bp[j], t[j], c, t[j], c);
		for (; c && (j < n + 2); j++) {
			t[j] += c;
			c = t[j] < c;
		}
		ui = t[0] * u;
		mxBigIntMulAdd(ui, mp[0], t[0], 0, lo, c);
		for (j = 1; j < n; j++)
			mxBigIntMulAdd(ui, mp[j], t[j], c, t[j - 1], c);
		lo = t[n] + c;
		c = lo < c;
		t[n - 1] = lo;
		t[n] = t[n + 1] + c;
		t[n + 1] = 0;
	}
	mont_finish(the, r, tt, m);
	pool_put(tt, n + 2);
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
		modWatchDogReset();
		mont_square(the, t, t, m, u, &pool);
		if (fxBigInt_isset(e, i))
			mont_mul(the, t, t, b, m, u, &pool);
	}
	mont_out(the, r, t, m, u);
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
	if (ec->aIsMinus3) {		// M = 3(X - Z^2)(X + Z^2) = 3X^2 + aZ^4 for a = -3: one multiply in place of a square and a multiply
		t1 = fxBigInt_mod_sub(the, t1, a->x, ZZ, m, pool);
		t2 = fxBigInt_mod_add(the, t2, a->x, ZZ, m, pool);
		t1 = fxBigInt_mont_mul(the, t1, t1, t2, m, ec->u, pool);
		t2 = fxBigInt_mod_dbl(the, t2, t1, 1, m, pool);
		M = fxBigInt_mod_add(the, M, t1, t2, m, pool);
	}
	else {
		t1 = fxBigInt_mod_dbl(the, t1, XX, 1, m, pool);
		t1 = fxBigInt_mod_add(the, t1, t1, XX, m, pool);
		t2 = fxBigInt_mont_square(the, t2, ZZ, m, ec->u, pool);
		t2 = fxBigInt_mont_mul(the, t2, t2, ec->a, m, ec->u, pool);
		M = fxBigInt_mod_add(the, M, t1, t2, m, pool);
	}
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
	
	// mixed addition: b affine (Z2 = 1, the input point of a scalar multiplication) needs no Z2Z2, U1 or S1
	int mixed = (0 == fxBigInt_ucomp(b->z, ec->one));

	t1 = pool_get(the, m->size, pool);
	t2 = pool_get(the, m->size, pool);
	Z1Z1 = pool_get(the, m->size, pool);
	U2 = pool_get(the, m->size, pool);
	S2 = pool_get(the, m->size, pool);

    Z1Z1 = fxBigInt_mont_square(the, Z1Z1, a->z, m, ec->u, pool);
    U2 = fxBigInt_mont_mul(the, U2, b->x, Z1Z1, m, ec->u, pool);
	t1 = fxBigInt_mont_mul(the, t1, b->y, a->z, m, ec->u, pool);
	S2 = fxBigInt_mont_mul(the, S2, t1, Z1Z1, m, ec->u, pool);
	if (mixed) {
		Z2Z2 = ec->one;
		U1 = a->x;
		S1 = a->y;
	}
	else {
		Z2Z2 = pool_get(the, m->size, pool);
		U1 = pool_get(the, m->size, pool);
		S1 = pool_get(the, m->size, pool);
		Z2Z2 = fxBigInt_mont_square(the, Z2Z2, b->z, m, ec->u, pool);
		U1 = fxBigInt_mont_mul(the, U1, a->x, Z2Z2, m, ec->u, pool);
		t1 = fxBigInt_mont_mul(the, t1, a->y, b->z, m, ec->u, pool);
		S1 = fxBigInt_mont_mul(the, S1, t1, Z2Z2, m, ec->u, pool);
	}

	if (fxBigInt_ucomp(U1, U2) == 0) {
		int eq = fxBigInt_ucomp(S1, S2) == 0;
		if (!mixed) {
			pool_put(S1, m->size);
			pool_put(U1, m->size);
			pool_put(Z2Z2, m->size);
		}
		pool_put(S2, m->size);
		pool_put(U2, m->size);
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
	if (mixed)		// Z3 = 2 Z1 H
		t1 = fxBigInt_mod_dbl(the, t1, a->z, 1, m, pool);
	else {			// Z3 = ((Z1 + Z2)^2 - Z1Z1 - Z2Z2) H
		t1 = fxBigInt_mod_add(the, t1, a->z, b->z, m, pool);
		t1 = fxBigInt_mont_square(the, t1, t1, m, ec->u, pool);
		t1 = fxBigInt_mod_sub(the, t1, t1, Z1Z1, m, pool);
		t1 = fxBigInt_mod_sub(the, t1, t1, Z2Z2, m, pool);
	}
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
	if (!mixed) {
		pool_put(S1, m->size);
		pool_put(U1, m->size);
		pool_put(Z2Z2, m->size);
	}
	pool_put(S2, m->size);
	pool_put(U2, m->size);
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

void fxBigInt_ec_aIsMinus3(txMachine *the, txECParam *ec)		// sets ec->aIsMinus3; call with plain (not Montgomery) a and m
{
	txBigInt *d = fxBigInt_sub(the, NULL, ec->m, ec->a);
	ec->aIsMinus3 = (0 == d->sign) && (1 == d->size) && (3 == d->data[0]);
	fxBigInt_free(the, d);
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
	if (fxBigInt_iszero(k))
		return;

	// addition-subtraction chain (Cohen, "A Course in Computational Algebraic Number Theory", algorithm 7.2.4): with h = 3k,
	// scanning the bits of h and k together from the top yields the non-adjacent form of k without storing it: a 1 in h
	// over a 0 in k adds the point, a 0 in h over a 1 in k subtracts it. About n/3 additions instead of n/2, no table.
	pool_init(&pool);
	{
		txBigInt *h = pool_get(the, k->size + 1, &pool);
		txECPoint neg;
		fxBigInt_umul1(the, h, k, 3);
		neg.x = a->x;
		neg.z = a->z;
		neg.y = pool_get(the, ec->m->size, &pool);
		fxBigInt_mod_sub(the, neg.y, ec->m, a->y, ec->m, &pool);	// -a: (x, m - y, z); y = 0 only for a point of order 2
		fxBigInt_copy(r->x, a->x);
		fxBigInt_copy(r->y, a->y);
		fxBigInt_copy(r->z, a->z);
		for (i = fxBigInt_bitsize(h) - 1; --i >= 1;) {
			int hi = fxBigInt_isset(h, i), ki = (i < (int)(k->size * mxBigIntWordSize)) ? fxBigInt_isset(k, i) : 0;
			modWatchDogReset();
			ec_double(the, r, r, ec, &pool);
			if (hi && !ki)
				ec_add(the, r, r, a, ec, &pool);
			else if (!hi && ki)
				ec_add(the, r, r, &neg, ec, &pool);
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
		modWatchDogReset();
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

/*
 * X25519 (RFC 7748): Montgomery ladder on curve25519 over 32-byte little-endian scalar and u-coordinate
 */

static const txU4 gxData25519[8] = { 0xffffffed, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff };	// 2^255 - 19
static const txBigInt gxBigInt25519 = { .sign = 0, .size = 8, .data = (txU4 *)gxData25519 };
static const txU4 gxDataA24[1] = { 121665 };
static const txBigInt gxBigIntA24 = { .sign = 0, .size = 1, .data = (txU4 *)gxDataA24 };

static void x25519_load(txBigInt *r, const uint8_t *bytes)		// 32 little-endian bytes into limbs, size normalized
{
	c_memcpy(r->data, bytes, 32);
	r->sign = 0;
	r->size = 8;
	while ((r->size > 1) && (0 == r->data[r->size - 1]))
		r->size--;
}

void fxBigInt_x25519(txMachine *the, uint8_t *out, const uint8_t *scalar, const uint8_t *u)
{
	txBigInt *p = (txBigInt *)&gxBigInt25519, *a24 = (txBigInt *)&gxBigIntA24;
	uint8_t k[32], uc[32];
	pool_t pool;
	int i;

	c_memcpy(k, scalar, 32);
	k[0] &= 248; k[31] &= 127; k[31] |= 64;		// clamp
	c_memcpy(uc, u, 32);
	uc[31] &= 127;		// mask the unused high bit of the u-coordinate

	// every temporary is an XS stack slot with a chunk: six scratch values, released before the inverse, keep the peak low.
	// The ladder runs in Montgomery form (mont_mul: no division), as the P-256/P-384 point arithmetic does.
	txU4 mu = mont_init(p);
	txBigInt *one = fxBigInt_alloc(the, 8), *a24m = fxBigInt_alloc(the, 8);
	txBigInt *x1 = fxBigInt_alloc(the, 8), *x2 = fxBigInt_alloc(the, 8), *z2 = fxBigInt_alloc(the, 8);
	txBigInt *x3 = fxBigInt_alloc(the, 8), *z3 = fxBigInt_alloc(the, 8);
	txBigInt *A = fxBigInt_alloc(the, 8), *B = fxBigInt_alloc(the, 8), *C = fxBigInt_alloc(the, 8);
	txBigInt *D = fxBigInt_alloc(the, 8), *E = fxBigInt_alloc(the, 8), *t = fxBigInt_alloc(the, 8);
	txBigInt *sx2 = x2, *sz2 = z2, *sx3 = x3, *sz3 = z3;		// the slots, as allocated
	mont_in(the, one, (txBigInt *)&gxBigIntOne, p);
	mont_in(the, a24m, a24, p);
	x25519_load(x1, uc);
	fxBigInt_mod_mod(the, x1, x1, p, NULL);		// u may exceed p
	mont_in(the, x1, x1, p);
	fxBigInt_copy(x2, one);
	fxBigInt_copy(z2, (txBigInt *)&gxBigIntZero);
	fxBigInt_copy(x3, x1);
	fxBigInt_copy(z3, one);

	pool_init(&pool);
	int swap = 0;
	for (i = 254; i >= 0; i--) {
		modWatchDogReset();
		int kt = (k[i >> 3] >> (i & 7)) & 1;
		swap ^= kt;
		if (swap) {
			txBigInt *s;
			s = x2; x2 = x3; x3 = s;
			s = z2; z2 = z3; z3 = s;
		}
		swap = kt;
		fxBigInt_mod_add(the, A, x2, z2, p, &pool);		// A = x2 + z2
		fxBigInt_mod_sub(the, B, x2, z2, p, &pool);		// B = x2 - z2
		fxBigInt_mod_add(the, C, x3, z3, p, &pool);		// C = x3 + z3
		fxBigInt_mod_sub(the, D, x3, z3, p, &pool);		// D = x3 - z3
		fxBigInt_mont_mul(the, E, D, A, p, mu, &pool);		// E = DA
		fxBigInt_mont_mul(the, t, C, B, p, mu, &pool);		// t = CB
		fxBigInt_mod_add(the, C, E, t, p, &pool);
		fxBigInt_mont_square(the, x3, C, p, mu, &pool);		// x3 = (DA + CB)^2
		fxBigInt_mod_sub(the, C, E, t, p, &pool);
		fxBigInt_mont_square(the, C, C, p, mu, &pool);
		fxBigInt_mont_mul(the, z3, x1, C, p, mu, &pool);		// z3 = x1 * (DA - CB)^2
		fxBigInt_mont_square(the, A, A, p, mu, &pool);		// A = AA
		fxBigInt_mont_square(the, B, B, p, mu, &pool);		// B = BB
		fxBigInt_mod_sub(the, E, A, B, p, &pool);		// E = AA - BB
		fxBigInt_mont_mul(the, x2, A, B, p, mu, &pool);		// x2 = AA * BB
		fxBigInt_mont_mul(the, t, a24m, E, p, mu, &pool);
		fxBigInt_mod_add(the, t, A, t, p, &pool);
		fxBigInt_mont_mul(the, z2, E, t, p, mu, &pool);		// z2 = E * (AA + a24 * E)
	}
	if (swap) {
		txBigInt *s;
		s = x2; x2 = x3; x3 = s;
		s = z2; z2 = z3; z3 = s;
	}
	pool_dispose(the, &pool);
	fxBigInt_free(the, t); fxBigInt_free(the, E); fxBigInt_free(the, D);
	fxBigInt_free(the, C); fxBigInt_free(the, B); fxBigInt_free(the, A);
	if (x2 == sx3) {		// the result landed in the later pair of slots: move it down so the later pair can be released
		fxBigInt_copy(sx2, sx3);
		fxBigInt_copy(sz2, sz3);
		x2 = sx2;
		z2 = sz2;
	}
	fxBigInt_free(the, sz3); fxBigInt_free(the, sx3);
	mont_out(the, x2, x2, p, mu);
	mont_out(the, z2, z2, p, mu);

	c_memset(out, 0, 32);
	if (fxBigInt_iszero(z2))		// a small-order point: the result is all zero (RFC 7748 section 6.1); the inverse of zero would not terminate
		return;
	txBigInt *inv = fxBigInt_mod_mulinv_general(the, NULL, z2, p);		// not the euclid variant: it returns wrong inverses for large arguments
	txBigInt *r = fxBigInt_alloc(the, 8);
	pool_init(&pool);
	fxBigInt_mod_mul(the, r, x2, inv, p, &pool);		// x2 / z2
	pool_dispose(the, &pool);
	c_memcpy(out, r->data, r->size * sizeof(txU4));
	// x1, x2, z2, inv and r are stack slots; the caller's frame releases them
}
