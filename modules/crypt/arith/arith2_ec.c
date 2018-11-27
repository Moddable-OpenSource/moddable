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
#include "arith2_common.h"
#include "xsBigIntEx.h"
#include "mc.xs.h"

extern void fxBigInt_ec_add(xsMachine *the, txECPoint *r, txECPoint *a, txECPoint *b, txECParam *ec);
extern void fxBigInt_ec_mul(xsMachine *the, txECPoint *r, txECPoint *a, txBigInt *k, txECParam *ec);
extern void fxBigInt_ec_mul2(xsMachine *the, txECPoint *r, txECPoint *a1, txBigInt *k1, txECPoint *a2, txBigInt *k2, txECParam *ec);

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
}

static void get_ecp(xsMachine *the, txECPoint *p, xsSlot *slot, int *i)
{
	xsmcGet(xsVar(*i), *slot, xsID_x);
	p->x = xsmcToBigInt(xsVar(*i));
	(*i)++;
	xsmcGet(xsVar(*i), *slot, xsID_y);
	p->y = xsmcToBigInt(xsVar(*i));
	(*i)++;
	xsmcGet(xsVar(*i), *slot, xsID_identity);
	p->identity = xsmcToBoolean(xsVar(*i));
	(*i)++;
}

static void set_ecp(xsMachine *the, xsSlot *slot, txECPoint *p)
{
	if (p->identity) {
		xsmcSetUndefined(xsVar(0));
		xsmcSetUndefined(xsVar(1));
		xsmcSetTrue(xsVar(2));
	}
	else {
		xsmcSetBigInt(xsVar(0), p->x);
		xsmcSetBigInt(xsVar(1), p->y);
		xsmcSetFalse(xsVar(2));
	}
	xsmcSetNewObject(*slot);
	xsmcSet(*slot, xsID_x, xsVar(0));
	xsmcSet(*slot, xsID_y, xsVar(1));
	xsmcSet(*slot, xsID_identity, xsVar(2));
}

void
xs_ec2_add(xsMachine *the)
{
	txECParam ec;
	txECPoint r, p1, p2;
	int i = 0;

	if (xsmcArgc < 2)
		return;
	xsmcVars(10);
	get_ec_param(the, &ec, &i);
	get_ecp(the, &p1, &xsArg(0), &i);
	get_ecp(the, &p2, &xsArg(1), &i);
	r.x = r.y = NULL;
	fxBigInt_ec_add(the, &r, &p1, &p2, &ec);
	set_ecp(the, &xsResult, &r);
}

void
xs_ec2_mul(xsMachine *the)
{
	txECParam ec;
	txECPoint r, p;
	txBigInt *k;
	int i = 0;

	if (xsmcArgc < 2)
		return;
	xsmcVars(10);
	get_ec_param(the, &ec, &i);
	get_ecp(the, &p, &xsArg(0), &i);
	k = xsmcToBigInt(xsArg(1));
	r.x = r.y = NULL;
	fxBigInt_ec_mul(the, &r, &p, k, &ec);
	set_ecp(the, &xsResult, &r);
}

void
xs_ec2_mul2(xsMachine *the)
{
	txECParam ec;
	txECPoint r, p1, p2;
	txBigInt *k1, *k2;
	int i = 0;

	if (xsmcArgc < 4)
		return;
	xsmcVars(10);
	get_ec_param(the, &ec, &i);
	get_ecp(the, &p1, &xsArg(0), &i);
	k1 = xsmcToBigInt(xsArg(1));
	get_ecp(the, &p2, &xsArg(2), &i);
	k2 = xsmcToBigInt(xsArg(3));
	r.x = r.y = NULL;
	fxBigInt_ec_mul2(the, &r, &p1, k1, &p2, k2, &ec);
	set_ecp(the, &xsResult, &r);
}
