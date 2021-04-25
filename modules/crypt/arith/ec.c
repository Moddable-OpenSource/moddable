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
#include "xsmc.h"
#include "xsBigIntEx.h"
#include "mc.xs.h"

extern void fxBigInt_ec_add(xsMachine *the, txECPoint *r, txECPoint *a, txECPoint *b, txECParam *ec);
extern void fxBigInt_ec_mul(xsMachine *the, txECPoint *r, txECPoint *a, txBigInt *k, txECParam *ec);
extern void fxBigInt_ec_mul2(xsMachine *the, txECPoint *r, txECPoint *a1, txBigInt *k1, txECPoint *a2, txBigInt *k2, txECParam *ec);
extern void fxBigInt_ec_norm(xsMachine *the, txECPoint *r, txECPoint *a, txECParam *ec);
extern txU4 mont_init(txBigInt *m);
extern txBigInt *mont_in(xsMachine *the, txBigInt *r, txBigInt *a, txBigInt *m);
extern txBigInt *mont_out(xsMachine *the, txBigInt *r, txBigInt *a, txBigInt *m, txU4 u);

extern txBigInt gxBigIntOne;

static void get_ec_param(xsMachine *the, txECParam *ec, int *i)
{
	xsmcGet(xsVar(*i), xsThis, xsID_m);
	ec->m = xsmcToBigInt(xsVar(*i));
	(*i)++;
	xsmcGet(xsVar(*i), xsThis, xsID_a);
	ec->a = xsmcToBigInt(xsVar(*i));
	(*i)++;
	xsmcGet(xsVar(*i), xsThis, xsID_b);
	ec->b = xsmcToBigInt(xsVar(*i));
	(*i)++;
	ec->u = mont_init(ec->m);
	ec->a = mont_in(the, NULL, ec->a, ec->m);
	ec->b = mont_in(the, NULL, ec->b, ec->m);
	ec->one = mont_in(the, NULL, &gxBigIntOne, ec->m);
	// RI: (a, b, one) are fxBigInt_alloc'ed. Is it no to free?
}

static void get_ecp(xsMachine *the, txECPoint *p, xsSlot *slot, int *i, txECParam *ec)
{
	xsmcGet(xsVar(*i), *slot, xsID_x);
	p->x = xsmcToBigInt(xsVar(*i));
	(*i)++;
	xsmcGet(xsVar(*i), *slot, xsID_y);
	p->y = xsmcToBigInt(xsVar(*i));
	(*i)++;
	xsmcGet(xsVar(*i), *slot, xsID_z);
	p->z = xsmcToBigInt(xsVar(*i));
	(*i)++;
	p->x = mont_in(the, NULL, p->x, ec->m);
	p->y = mont_in(the, NULL, p->y, ec->m);
	p->z = mont_in(the, NULL, p->z, ec->m);
	// RI: those (x, y, z) are fxBigInt_alloc'ed. Is it ok not to free?
}

static void set_ecp(xsMachine *the, xsSlot *slot, txECPoint *p, txECParam *ec)
{
	mont_out(the, p->x, p->x, ec->m, ec->u);
	mont_out(the, p->y, p->y, ec->m, ec->u);
	mont_out(the, p->z, p->z, ec->m, ec->u);

	xsmcSetBigInt(xsVar(0), p->x);
	xsmcSetBigInt(xsVar(1), p->y);
	xsmcSetBigInt(xsVar(2), p->z);
	xsmcSetNewObject(*slot);
	xsmcSet(*slot, xsID_x, xsVar(0));
	xsmcSet(*slot, xsID_y, xsVar(1));
	xsmcSet(*slot, xsID_z, xsVar(2));
}

void
xs_ec2_add(xsMachine *the)
{
	txECParam ec;
	txECPoint r, p1, p2;
	int i = 0;

	xsmcVars(10);
	get_ec_param(the, &ec, &i);
	get_ecp(the, &p1, &xsArg(0), &i, &ec);
	get_ecp(the, &p2, &xsArg(1), &i, &ec);
	r.x = r.y = r.z = NULL;
	// RI: r(x, y, z) will be fxBigInt_alloc'ed. Is it ok not to free?
	fxBigInt_ec_add(the, &r, &p1, &p2, &ec);
	fxBigInt_ec_norm(the, &r, &r, &ec);
	set_ecp(the, &xsResult, &r, &ec);
}

void
xs_ec2_mul(xsMachine *the)
{
	txECParam ec;
	txECPoint r, p;
	txBigInt *k;
	int i = 0;

	xsmcVars(10);
	get_ec_param(the, &ec, &i);
	get_ecp(the, &p, &xsArg(0), &i, &ec);
	k = xsmcToBigInt(xsArg(1));
	r.x = r.y = r.z = NULL;
	// RI: r(x, y, z) will be fxBigInt_alloc'ed. Is it ok not to free?
	fxBigInt_ec_mul(the, &r, &p, k, &ec);
	fxBigInt_ec_norm(the, &r, &r, &ec);
	set_ecp(the, &xsResult, &r, &ec);
}

void
xs_ec2_mul2(xsMachine *the)
{
	txECParam ec;
	txECPoint r, p1, p2;
	txBigInt *k1, *k2;
	int i = 0;

	xsmcVars(10);
	get_ec_param(the, &ec, &i);
	get_ecp(the, &p1, &xsArg(0), &i, &ec);
	k1 = xsmcToBigInt(xsArg(1));
	get_ecp(the, &p2, &xsArg(2), &i, &ec);
	k2 = xsmcToBigInt(xsArg(3));
	r.x = r.y = r.z = NULL;
	// RI: r(x, y, z) will be fxBigInt_alloc'ed. Is it ok not to free?
	fxBigInt_ec_mul2(the, &r, &p1, k1, &p2, k2, &ec);
	fxBigInt_ec_norm(the, &r, &r, &ec);
	set_ecp(the, &xsResult, &r, &ec);
}
