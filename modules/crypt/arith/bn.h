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

typedef unsigned long long bn_dword;
typedef unsigned int bn_word;
typedef unsigned char bn_bool;
typedef unsigned short bn_size;
typedef unsigned char bn_byte;

typedef struct bn {
	bn_bool sign;
	bn_size size;		/* data size in word */
	bn_word data[1];
} bn_t;

typedef enum {
	BN_ERR_NONE,
	BN_ERR_NOMEM,
	BN_ERR_DIVIDE_BY_ZERO,
	BN_ERR_MISCALCULATION,
} bn_err_t;

typedef struct bn_context bn_context_t;

extern void *bn_allocbuf(bn_context_t *ctx, unsigned int n);
extern void bn_freebuf(bn_context_t *ctx, void *bufptr);
extern void bn_throw(bn_context_t *ctx, bn_err_t code);

typedef enum {BN_MOD_METHOD_LR, BN_MOD_METHOD_SW} bn_mod_method_t;
typedef struct bn_mod {
	bn_context_t *ctx;
	bn_t *m;
	bn_word u;	/* for the Montgomery method only */
	bn_t *(*square_func)();
	bn_t *(*mul_func)();
	bn_t *(*conv_in_func)();
	bn_t *(*conv_out_func)();
	bn_t *(*exp_init)();
	bn_t *(*exp_func)();
	bn_t *(*exp2_func)();
	int options;
} bn_mod_t;

typedef struct {
	bn_t *x;
	bn_t *y;
} bn_point_t;

extern void bn_freetbuf(bn_context_t *ctx, void *bufptr);

extern int bn_iszero(bn_t *a);
extern int bn_isNaN(bn_t *a);
extern bn_bool bn_sign(bn_t *a);
extern bn_size bn_wsizeof(bn_t *a);
extern void bn_negate(bn_t *a);
extern void bn_copy(bn_t *a, bn_t *b);
extern int bn_bitsize(bn_t *e);
extern int bn_comp(bn_t *a, bn_t *b);

extern bn_t *bn_lsl(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
extern bn_t *bn_lsr(bn_context_t *ctx, bn_t *r, bn_t *a, unsigned int sw);
extern bn_t *bn_xor(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_or(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_and(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);

extern bn_t *bn_add(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
extern bn_t *bn_sub(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
extern bn_t *bn_mul(bn_context_t *ctx, bn_t *rr, bn_t *aa, bn_t *bb);
extern bn_t *bn_square(bn_context_t *ctx, bn_t *r, bn_t *a);
extern bn_t *bn_div(bn_context_t *ctx, bn_t *q, bn_t *a, bn_t *b, bn_t **r);
extern bn_t *bn_mod(bn_context_t *ctx, bn_t *r, bn_t *a, bn_t *b);
extern int bn_inc(bn_t *a, bn_word d);

extern bn_t *bn_mod_add(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_mod_sub(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_mod_inv(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_mod(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_mul(bn_mod_t *mod, bn_t *r, bn_t *a, bn_t *b);
extern bn_t *bn_mod_square(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_mulinv(bn_mod_t *mod, bn_t *r, bn_t *a);
extern bn_t *bn_mod_exp(bn_mod_t *mod, bn_t *r, bn_t *b, bn_t *e);
extern bn_t *bn_mod_exp2(bn_mod_t *mod, bn_t *r, bn_t *b1, bn_t *e1, bn_t *b2, bn_t *e2);
extern void bn_mod_init(bn_mod_t *mod, bn_context_t *ctx, bn_t *m);
extern void bn_mod_freetbuf(bn_mod_t *mod, void *bufptr);
extern void bn_mont_init(bn_mod_t *mod, bn_context_t *ctx, bn_t *m, bn_mod_method_t method, int options);
extern bn_point_t *bn_ed_add(bn_mod_t *mod, bn_point_t *r, bn_point_t *p, bn_point_t *q, bn_t *d);
extern bn_point_t *bn_ed_mul(bn_mod_t *mod, bn_point_t *r, bn_point_t *p, bn_t *k, bn_t *d);
