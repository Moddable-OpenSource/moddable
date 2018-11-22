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
#include "xs.h"
#include "kcl_arith.h"
#include "arith_common.h"
#include "xsmc.h"

#define arith_get_integer(mod, slot)	__arith_get_integer(the, &slot, &mod->proto_int)
#define arith_set_integer(mod, slot, x)	__arith_set_integer(the, &slot, x, &mod->proto_int)

typedef kcl_err_t (*kcl_mod_f1)(kcl_mod_t *, kcl_int_t *a, kcl_int_t **r);
typedef kcl_err_t (*kcl_mod_f2)(kcl_mod_t *, kcl_int_t *a, kcl_int_t *b, kcl_int_t **r);
typedef kcl_err_t (*kcl_mod_f4)(kcl_mod_t *, kcl_int_t *a1, kcl_int_t *e1, kcl_int_t *b1, kcl_int_t *e2, kcl_int_t **r);

static void
mod_call1(xsMachine *the, mod_t *mod, kcl_mod_f1 f)
{
	kcl_int_t *a = arith_get_integer(mod, xsArg(0));
	kcl_int_t *r = NULL;
	kcl_err_t err;

	xsmcGet(xsResult, xsThis, xsID_z);
	xsmcGet(xsResult, xsResult, xsID_buffer);
	kcl_mod_setBuffer(mod->ctx, xsmcToArrayBuffer(xsResult));

	err = (*f)(mod->ctx, a, &r);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(mod, xsResult, r);
}

static void
mod_call2(xsMachine *the, mod_t *mod, kcl_mod_f2 f)
{
	kcl_int_t *a = arith_get_integer(mod, xsArg(0));
	kcl_int_t *b = arith_get_integer(mod, xsArg(1));
	kcl_int_t *r = NULL;
	kcl_err_t err;

	xsmcGet(xsResult, xsThis, xsID_z);
	xsmcGet(xsResult, xsResult, xsID_buffer);
	kcl_mod_setBuffer(mod->ctx, xsmcToArrayBuffer(xsResult));

	err = (*f)(mod->ctx, a, b, &r);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(mod, xsResult, r);
}

static void
mod_call4(xsMachine *the, mod_t *mod, kcl_mod_f4 f)
{
	kcl_int_t *a1 = arith_get_integer(mod, xsArg(0));
	kcl_int_t *e1 = arith_get_integer(mod, xsArg(1));
	kcl_int_t *a2 = arith_get_integer(mod, xsArg(2));
	kcl_int_t *e2 = arith_get_integer(mod, xsArg(3));
	kcl_int_t *r = NULL;
	kcl_err_t err;

	xsmcGet(xsResult, xsThis, xsID_z);
	xsmcGet(xsResult, xsResult, xsID_buffer);
	kcl_mod_setBuffer(mod->ctx, xsmcToArrayBuffer(xsResult));

	err = (*f)(mod->ctx, a1, e1, a2, e2, &r);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_integer(mod, xsResult, r);
}

void
xs_mod_init(xsMachine *the)
{
	mod_t *mod = NULL;
	z_t *z;
	kcl_int_t *m;
	kcl_err_t err;

	if ((mod = crypt_malloc(sizeof(mod_t))) == NULL) {
		err = KCL_ERR_NOMEM;
		goto bail;
	}
	c_memset(mod, 0, sizeof(mod_t));
	if ((err = kcl_mod_alloc(&mod->ctx)) != KCL_ERR_NONE)
		goto bail;

	if ((z = xsmcGetHostData(xsArg(0))) == NULL ||
	    (m = xsmcGetHostData(xsArg(1))) == NULL) {
		err = KCL_ERR_BAD_ARG;
		goto bail;
	}
	if ((err = kcl_mod_init(mod->ctx, z->ctx, m)) != KCL_ERR_NONE)
		goto bail;

	xsmcGet(mod->proto_int, xsThis, xsID__proto_int);
	xsmcSetHostData(xsThis, mod);
	return;

bail:
	if (mod != NULL) {
		if (mod->ctx != NULL)
			kcl_mod_dispose(mod->ctx);
		crypt_free(mod);
	}
	kcl_throw_error(the, err);
}

void
xs_mod_destructor(void *data)
{
	if (data != NULL) {
		mod_t *mod = data;
		if (mod->ctx != NULL)
			kcl_mod_dispose(mod->ctx);
		crypt_free(data);
	}
}

void
xs_mod_add(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call2(the, mod, kcl_mod_add);
}

void
xs_mod_inv(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call1(the, mod, kcl_mod_inv);
}

void
xs_mod_sub(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call2(the, mod, kcl_mod_sub);
}

void
xs_mod_mul(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call2(the, mod, kcl_mod_mul);
}

void
xs_mod_square(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call1(the, mod, kcl_mod_square);
}

void
xs_mod_mulinv(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call1(the, mod, kcl_mod_mulinv);
}

void
xs_mod_exp(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call2(the, mod, kcl_mod_exp);
}

void
xs_mod_exp2(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call4(the, mod, kcl_mod_exp2);
}

void
xs_mod_mod(xsMachine *the)
{
	mod_t *mod = xsmcGetHostData(xsThis);

	mod_call1(the, mod, kcl_mod_mod);
}
