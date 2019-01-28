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

#ifndef __KCL_ARITH_H__
#define __KCL_ARITH_H__

#include "kcl.h"

/*
 * multi-precision integer
 */

typedef struct kcl_int {
	struct bn *data;	/* variable size */
} kcl_int_t;

extern kcl_err_t kcl_int_alloc(kcl_int_t **);
extern void kcl_int_dispose(kcl_int_t *);
extern int kcl_int_sign(kcl_int_t *);
extern int kcl_int_iszero(kcl_int_t *);
extern void kcl_int_neg(kcl_int_t *);
extern int kcl_int_comp(kcl_int_t *, kcl_int_t *);
extern size_t kcl_int_sizeof(kcl_int_t *);
extern kcl_err_t kcl_int_copy(kcl_int_t *, kcl_int_t *src);
extern kcl_err_t kcl_int_i2os(kcl_int_t *, unsigned char *os, size_t size);
extern kcl_err_t kcl_int_i2os_l(kcl_int_t *, unsigned char *os, size_t size);
extern kcl_err_t kcl_int_os2i(kcl_int_t *, unsigned char *os, size_t size);
extern kcl_err_t kcl_int_os2i_l(kcl_int_t *, unsigned char *os, size_t size, int signess);
extern kcl_err_t kcl_int_i2strx(kcl_int_t *, char *s);
extern kcl_err_t kcl_int_str2ix(kcl_int_t *, char *s);
extern kcl_err_t kcl_int_i2num(kcl_int_t *, long *n);
extern kcl_err_t kcl_int_num2i(kcl_int_t *, long n);
extern kcl_err_t kcl_int_newFrom(kcl_int_t *src, kcl_int_t **dst);
extern void kcl_int_free(kcl_int_t *);
extern kcl_err_t kcl_int_inc(kcl_int_t *, int d);
extern void kcl_int_setNaN(kcl_int_t *);
extern int kcl_int_isNaN(kcl_int_t *);

/*
 * EC point
 */

typedef struct {
	kcl_int_t *x, *y;
	int identity;
} kcl_ecp_t;

extern kcl_err_t kcl_ecp_alloc(kcl_ecp_t **pp);
extern kcl_err_t kcl_ecp_init(kcl_ecp_t *p, kcl_int_t *x, kcl_int_t *y);
extern kcl_err_t kcl_ecp_setX(kcl_ecp_t *, kcl_int_t *x);
extern kcl_err_t kcl_ecp_setY(kcl_ecp_t *, kcl_int_t *y);
extern void kcl_ecp_free(kcl_ecp_t *);
extern void kcl_ecp_dispose(kcl_ecp_t *);
#define kcl_ecp_identity(p)	((p)->identity)
#define kcl_ecp_getX(p)	((p)->x)
#define kcl_ecp_getY(p)	((p)->y)

/*
 * integer arithmetic
 */

typedef struct bn_context kcl_z_t;

extern kcl_err_t kcl_z_alloc(kcl_z_t **);
extern void kcl_z_dispose(kcl_z_t *);
extern kcl_err_t kcl_z_init(kcl_z_t *, kcl_error_callback_t *cb, int *neededBufferSize);
extern kcl_err_t kcl_z_setBuffer(kcl_z_t *z, void *buffer);
extern kcl_err_t kcl_z_add(kcl_z_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_z_sub(kcl_z_t *, kcl_int_t *a , kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_z_mul(kcl_z_t *, kcl_int_t *a , kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_z_div(kcl_z_t *, kcl_int_t *a , kcl_int_t *b, kcl_int_t **r, kcl_int_t **m);
extern kcl_err_t kcl_z_square(kcl_z_t *, kcl_int_t *a, kcl_int_t **r);
extern kcl_err_t kcl_z_xor(kcl_z_t *, kcl_int_t *a , kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_z_or(kcl_z_t *, kcl_int_t *a , kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_z_and(kcl_z_t *, kcl_int_t *a , kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_z_lsl(kcl_z_t *, kcl_int_t *a, int b, kcl_int_t **r);
extern kcl_err_t kcl_z_lsr(kcl_z_t *, kcl_int_t *a, int b, kcl_int_t **r);
extern void kcl_z_free(kcl_z_t *);
extern kcl_err_t kcl_z_i2str(kcl_z_t *, kcl_int_t *, char *s, size_t size, unsigned int radix);
extern kcl_err_t kcl_z_str2i(kcl_z_t *, kcl_int_t **, char *s, unsigned int radix);

/*
 * modular arithmetic
 */

typedef struct bn_mod kcl_mod_t;

typedef enum {KCL_MOD_METHOD_LR = 0, KCL_MOD_METHOD_SW} kcl_mod_method_t;

extern kcl_err_t kcl_mod_alloc(kcl_mod_t **);
extern void kcl_mod_dispose(kcl_mod_t *);
extern kcl_err_t kcl_mod_init(kcl_mod_t *, kcl_z_t *z, kcl_int_t *m);
extern kcl_err_t kcl_mod_setBuffer(kcl_mod_t *, void *buffer);
extern kcl_err_t kcl_mod_add(kcl_mod_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_mod_inv(kcl_mod_t *, kcl_int_t *a, kcl_int_t **r);
extern kcl_err_t kcl_mod_sub(kcl_mod_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_mod_mul(kcl_mod_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_mod_square(kcl_mod_t *, kcl_int_t *a, kcl_int_t **r);
extern kcl_err_t kcl_mod_mulinv(kcl_mod_t *, kcl_int_t *a, kcl_int_t **r);
extern kcl_err_t kcl_mod_exp(kcl_mod_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r);
extern kcl_err_t kcl_mod_exp2(kcl_mod_t *, kcl_int_t *a1, kcl_int_t *e1, kcl_int_t *b1, kcl_int_t *e2, kcl_int_t **r);
extern kcl_err_t kcl_mod_mod(kcl_mod_t *, kcl_int_t *a, kcl_int_t **r);
extern void kcl_mod_free(kcl_mod_t *);

/*
 * Montgomery method
 */

extern kcl_err_t kcl_mont_init(kcl_mod_t *mod, kcl_z_t *z, kcl_int_t *m, kcl_mod_method_t method, int options);

/*
 * EC arithmetic
 */

typedef struct {
	kcl_mod_t *mod;
	kcl_int_t *a, *b;
} kcl_ec_t;

extern kcl_err_t kcl_ec_alloc(kcl_ec_t **r);
extern kcl_err_t kcl_ec_init(kcl_ec_t *, kcl_mod_t *mod, kcl_int_t *a, kcl_int_t *b);
extern kcl_err_t kcl_ec_inv(kcl_ec_t *, kcl_ecp_t *a, kcl_ecp_t **r);
extern kcl_err_t kcl_ec_double(kcl_ec_t *ec, kcl_ecp_t *a, kcl_ecp_t **r);
extern kcl_err_t kcl_ec_add(kcl_ec_t *, kcl_ecp_t *a, kcl_ecp_t *b, kcl_ecp_t **r);
extern kcl_err_t kcl_ec_mul(kcl_ec_t *, kcl_ecp_t *a, kcl_int_t *k, kcl_ecp_t **r);
extern kcl_err_t kcl_ec_mul2(kcl_ec_t *, kcl_ecp_t *a1, kcl_int_t *k1, kcl_ecp_t *a2, kcl_int_t *k2, kcl_ecp_t **r);
extern void kcl_ec_dispose(kcl_ec_t *);

/*
 * Edwards Curve
 */

typedef struct {
	kcl_mod_t *mod;
	kcl_int_t *d;
} kcl_ed_t;

extern kcl_err_t kcl_ed_alloc(kcl_ed_t **edp);
extern kcl_err_t kcl_ed_init(kcl_ed_t *ed, kcl_mod_t *mod, kcl_int_t *d);
extern kcl_err_t kcl_ed_add(kcl_ed_t *ed, kcl_ecp_t *a, kcl_ecp_t *b, kcl_ecp_t *r);
extern kcl_err_t kcl_ed_mul(kcl_ed_t *ed, kcl_ecp_t *p, kcl_int_t *k, kcl_ecp_t *r);
extern void kcl_ed_dispose(kcl_ed_t *ed);

#endif /* __KCL_ARITH_H__ */
