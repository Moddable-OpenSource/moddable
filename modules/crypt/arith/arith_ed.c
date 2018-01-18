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
	kcl_ed_t *ctx;
	xsSlot proto_int;
	xsSlot proto_ecp;
	xsIndex id_x, id_y;
} ed_t;

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

#define arith_get_ecp(ed, slot)		__arith_get_ecp(the, &slot, &ed->proto_ecp)
#define arith_set_ecp(ed, slot, p)	__arith_set_ecp(the, &slot, p, &ed->proto_ecp)
#define arith_get_integer(ed, slot)	__arith_get_integer(the, &slot, &ed->proto_int)
#define arith_set_integer(ed, slot, x)	__arith_set_integer(the, &slot, x, &ed->proto_int)

void
xs_ed_init(xsMachine *the)
{
	ed_t *ed;
	mod_t *mod;	/* mod_t* */
	void *d;	/* kcl_int_t* */
	kcl_err_t err;

	if ((ed = crypt_malloc(sizeof(ed_t))) == NULL)
		kcl_throw_error(the, KCL_ERR_NOMEM);
	if ((err = kcl_ed_alloc(&ed->ctx)) != KCL_ERR_NONE) {
		crypt_free(ed);
		kcl_throw_error(the, err);
	}
	xsmcGet(xsResult, xsThis, xsID_mod);
	mod = xsmcGetHostData(xsResult);
	xsmcGet(xsResult, xsThis, xsID_d);
	d = xsmcGetHostData(xsResult);
	kcl_ed_init(ed->ctx, mod->ctx, d);

	ed->id_x = xsID_x;
	ed->id_y = xsID_y;
	xsmcGet(ed->proto_int, xsThis, xsID__proto_int);
	xsmcGet(ed->proto_ecp, xsThis, xsID__proto_ecp);
	xsmcSetHostData(xsThis, ed);
}

void
xs_ed_destructor(void *data)
{
	if (data != NULL) {
		ed_t *ed = data;
		kcl_ed_dispose(ed->ctx);
		crypt_free(ed);
	}
}

void
xs_ed_add(xsMachine *the)
{
	ed_t *ed = xsmcGetHostData(xsThis);
	kcl_ecp_t *a, *b, *r;
	kcl_err_t err;

	xsmcVars(1);
	a = arith_get_ecp(ed, xsArg(0));
	b = arith_get_ecp(ed, xsArg(1));
	(void)((err = kcl_ecp_alloc(&r)) || (err = kcl_ed_add(ed->ctx, a, b, r)));
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_ecp(ed, xsResult, r);
}

void
xs_ed_mul(xsMachine *the)
{
	ed_t *ed = xsmcGetHostData(xsThis);
	kcl_ecp_t *p, *r;
	kcl_int_t *k;
	kcl_err_t err;

	xsmcVars(1);
	p = arith_get_ecp(ed, xsArg(0));
	k = arith_get_integer(ed, xsArg(1));
	(void)((err = kcl_ecp_alloc(&r)) || (err = kcl_ed_mul(ed->ctx, p, k, r)));
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	arith_set_ecp(ed, xsResult, r);
}
