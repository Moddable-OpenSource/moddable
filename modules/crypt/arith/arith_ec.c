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

#include "arith_common.h"
#include "kcl_arith.h"
#include "xsmc.h"

typedef struct {
	kcl_ec_t *ctx;
	xsSlot proto_ecp;
	xsSlot proto_int;
} ec_t;

static kcl_ecp_t *
__arith_get_ecp(xsMachine *the, xsSlot *slot, xsSlot *proto)
{
	if (!xsmcIsInstanceOf(*slot, *proto))
		kcl_throw_error(the, KCL_ERR_NAN);
	return xsmcGetHostData(*slot);
}

static void
__arith_set_ecp(xsMachine *the, xsSlot *slot, kcl_ecp_t *p, xsSlot *proto)
{
	*slot = xsmcNewHostInstance(*proto);
	xsmcSetHostData(*slot, p);
}

#define arith_get_integer(ec, slot)	__arith_get_integer(the, &slot, &ec->proto_int)
#define arith_get_ecp(ec, slot)		__arith_get_ecp(the, &slot, &ec->proto_ecp)
#define arith_set_ecp(ec, slot, p)	__arith_set_ecp(the, &slot, p, &ec->proto_ecp)


typedef kcl_err_t (*ec_f1)(kcl_ec_t *ec, kcl_ecp_t *a, kcl_ecp_t **r);
typedef kcl_err_t (*ec_f2)(kcl_ec_t *ec, kcl_ecp_t *a, kcl_ecp_t *b, kcl_ecp_t **r);
typedef kcl_err_t (*ec_f2d)(kcl_ec_t *ec, kcl_ecp_t *a, kcl_int_t *k, kcl_ecp_t **r);
typedef kcl_err_t (*ec_f4d)(kcl_ec_t *ec, kcl_ecp_t *a1, kcl_int_t *k1, kcl_ecp_t *a2, kcl_int_t *k2, kcl_ecp_t **r);

static void
ec_call1(xsMachine *the, ec_t *ec, ec_f1 f)
{
	kcl_ecp_t *a = arith_get_ecp(ec, xsArg(0));
	kcl_ecp_t *r = NULL;
	kcl_err_t err;

	err = (*f)(ec->ctx, a, &r);
	if (err)
		kcl_throw_error(the, err);
	arith_set_ecp(ec, xsResult, r);
}

static void
ec_call2(xsMachine *the, ec_t *ec, ec_f2 f)
{
	kcl_ecp_t *a = arith_get_ecp(ec, xsArg(0));
	kcl_ecp_t *b = arith_get_ecp(ec, xsArg(1));
	kcl_ecp_t *r = NULL;
	kcl_err_t err;

	err = (*f)(ec->ctx, a, b, &r);
	if (err)
		kcl_throw_error(the, err);
	arith_set_ecp(ec, xsResult, r);
}

static void
ec_call2d(xsMachine *the, ec_t *ec, ec_f2d f)
{
	kcl_ecp_t *a = arith_get_ecp(ec, xsArg(0));
	kcl_int_t *k = arith_get_integer(ec, xsArg(1));
	kcl_ecp_t *r = NULL;
	kcl_err_t err;

	err = (*f)(ec->ctx, a, k, &r);
	if (err)
		kcl_throw_error(the, err);
	arith_set_ecp(ec, xsResult, r);
}

static void
ec_call4d(xsMachine *the, ec_t *ec, ec_f4d f)
{
	kcl_ecp_t *a1 = arith_get_ecp(ec, xsArg(0));
	kcl_int_t *k1 = arith_get_integer(ec, xsArg(1));
	kcl_ecp_t *a2 = arith_get_ecp(ec, xsArg(2));
	kcl_int_t *k2 = arith_get_integer(ec, xsArg(3));
	kcl_ecp_t *r = NULL;
	kcl_err_t err;

	err = (*f)(ec->ctx, a1, k1, a2, k2, &r);
	if (err)
		kcl_throw_error(the, err);
	arith_set_ecp(ec, xsResult, r);
}

void
xs_ec_init(xsMachine *the)
{
	ec_t *ec;
	mod_t *mod;
	kcl_int_t *a, *b;
	kcl_err_t err;

	if ((ec = crypt_malloc(sizeof(ec_t))) == NULL)
		kcl_throw_error(the, KCL_ERR_NOMEM);
	if ((err = kcl_ec_alloc(&ec->ctx)) != KCL_ERR_NONE) {
		crypt_free(ec);
		kcl_throw_error(the, err);
	}
	
	xsmcGet(xsResult, xsThis, xsID_m);
	mod = xsmcGetHostData(xsResult);
	xsmcGet(xsResult, xsThis, xsID_a);
	a = xsmcGetHostData(xsResult);
	xsmcGet(xsResult, xsThis, xsID_b);
	b = xsmcGetHostData(xsResult);
	kcl_ec_init(ec->ctx, mod->ctx, a, b);

	xsmcGet(ec->proto_ecp, xsThis, xsID__proto_ecpoint);
	xsmcGet(ec->proto_int, xsThis, xsID__proto_int);
	xsmcSetHostData(xsThis, ec);
}

void
xs_ec_destructor(void *data)
{
	if (data != NULL) {
		ec_t *ec = data;
		kcl_ec_dispose(ec->ctx);
		crypt_free(ec);
	}
}

void
xs_ec_inv(xsMachine *the)
{
	ec_t *ec = xsmcGetHostData(xsThis);

	ec_call1(the, ec, kcl_ec_inv);
}

void
xs_ec_add(xsMachine *the)
{
	ec_t *ec = xsmcGetHostData(xsThis);

	ec_call2(the, ec, kcl_ec_add);
}

void
xs_ec_mul(xsMachine *the)
{
	ec_t *ec = xsmcGetHostData(xsThis);

	ec_call2d(the, ec, kcl_ec_mul);
}

void
xs_ec_mul2(xsMachine *the)
{
	ec_t *ec = xsmcGetHostData(xsThis);

	ec_call4d(the, ec, kcl_ec_mul2);
}
