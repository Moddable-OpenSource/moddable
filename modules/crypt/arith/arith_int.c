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
#include "kcl_arith.h"
#include "arith_common.h"

#ifndef MAX
#define MAX(a, b)	((a) >= (b) ? (a) : (b))
#endif

static void
__arith_check_nan(xsMachine *the, kcl_int_t *ai)
{
	if (kcl_int_isNaN(ai))
		kcl_throw_error(the, KCL_ERR_NAN);
}
#define arith_check_nan(ai)	__arith_check_nan(the, ai)
/* #define arith_get_integer(slot)	int_arith_get_integer(the, &xsThis) */	/* doesn't work */
#define arith_get_integer(slot)	xsmcGetHostData(slot)	/* for now... */

void
xs_integer_init(xsMachine *the)
{
	kcl_int_t *ai;
	kcl_err_t err;

	if ((err = kcl_int_alloc(&ai)) != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	xsmcSetHostData(xsThis, ai);
}

void
xs_integer_destructor(void *data)
{
	if (data) {
		kcl_int_dispose(data);
	}
}

void
xs_integer_free(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);

	kcl_int_free(ai);
}

extern void resolveBuffer(xsMachine *the, xsSlot *slot, uint8_t **data, uint32_t *count);

void
xs_integer_setChunk(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	unsigned char *data;
	uint32_t size;
	int ac = xsmcArgc;
	xsBooleanValue signess = ac > 1 && xsmcToBoolean(xsArg(1));
	xsBooleanValue lsbFirst = ac > 2 && xsmcToBoolean(xsArg(2));
	int neg = 0;
	kcl_err_t err;

	resolveBuffer(the, &xsArg(0), &data, &size);
	if (size == 0) {
		kcl_int_setNaN(ai);
		return;
	}
	if (lsbFirst)
		err = kcl_int_os2i_l(ai, data, size, signess);
	else {
		if (signess) {
			neg = data[0];
			data++;
			--size;
		}
		err = kcl_int_os2i(ai, data, size);
	}
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	if (neg)
		kcl_int_neg(ai);
}

void
xs_integer_toChunk(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	int ac = xsmcArgc;
	uint32_t minBytes = ac > 0 ? xsmcToInteger(xsArg(0)): 0;
	xsBooleanValue signess = ac > 1 && xsmcToBoolean(xsArg(1));
	xsBooleanValue lsbFirst = ac > 2 && xsmcToBoolean(xsArg(2));
	uint32_t nbytes, chunkSize;
	int32_t n;
	unsigned char *p;
	kcl_err_t err;

	if (kcl_int_isNaN(ai)) {
		xsResult = xsUndefined;
		return;
	}
	nbytes = kcl_int_sizeof(ai);
	chunkSize = MAX(nbytes, minBytes);
	if (signess && !lsbFirst)
		chunkSize++;
	xsResult = xsArrayBuffer(NULL, chunkSize);
	p = xsmcToArrayBuffer(xsResult);
	/* all clear in case i2os won't fill out "nbytes" -- this would happen in libtom when the number is zero */
	c_memset(p, 0, chunkSize);
	if (lsbFirst)
		err = kcl_int_i2os_l(ai, p, nbytes);
	else {
		if (signess)
			*p++ = (unsigned char)kcl_int_sign(ai);
		/* skip the prepended filler */
		if ((n = minBytes - nbytes) > 0)
			p += n;
		err = kcl_int_i2os(ai, p, nbytes);
	}
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
}

void
xs_integer_setHexString(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	char *digits = xsmcToString(xsArg(0));	/* never use the XS functions while accessing this */
	int neg = 0;
	kcl_err_t err;

	if (*digits == '-') {
		neg = 1;
		digits++;
	}
	else if (*digits == '+')
		digits++;
	err = kcl_int_str2ix(ai, digits);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	if (neg)
		kcl_int_neg(ai);
}

void
xs_integer_toHexString(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	char *bp;
	int n;
	kcl_err_t err;

	if (kcl_int_isNaN(ai)) {
		xsResult = xsUndefined;
		return;
	}
	n = kcl_int_sizeof(ai) * 2;
	n += 2;
	if ((bp = crypt_malloc(n)) == NULL)
		kcl_throw_error(the, KCL_ERR_NOMEM);
	if ((err = kcl_int_i2strx(ai, bp)) != KCL_ERR_NONE) {
		crypt_free(bp);
		kcl_throw_error(the, err);
	}
	xsResult = xsString(bp);
	crypt_free(bp);
}

void
xs_integer_setNumber(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	kcl_err_t err;

	switch (xsmcTypeOf(xsArg(0))) {
	case xsNumberType: {
		double d = xsmcToNumber(xsArg(0));
		uint64_t ll;
		if (d < 0)
			ll = -d;
		else
			ll = d;
#if mxLittleEndian
		ll = ((ll << 56) | ((ll << 40) & 0xff000000000000ULL) | ((ll << 24) & 0xff0000000000ULL) | ((ll << 8) & 0xff00000000ULL) |
		      (ll >> 56) | ((ll >> 40) & 0xff00ULL) | ((ll >> 24) & 0xff0000ULL) | ((ll >> 8) & 0xff000000ULL));
#endif
		err = kcl_int_os2i(ai, (unsigned char *)&ll, sizeof(ll));
		if (err != KCL_ERR_NONE)
			kcl_throw_error(the, err);
		break;
	}
	case xsIntegerType: {
		xsIntegerValue v = xsmcToInteger(xsArg(0));
		err = kcl_int_num2i(ai, v);
		if (err != KCL_ERR_NONE)
			kcl_throw_error(the, err);
		break;
	}
	default:
		kcl_throw_error(the, KCL_ERR_TYPE);
		break;
	}
}

void
xs_integer_toNumber(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	long v;
	kcl_err_t err;

	if (kcl_int_isNaN(ai)) {
		xsResult = xsUndefined;	/* how can NaN be returned? */
		return;
	}
	err = kcl_int_i2num(ai, &v);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
	xsResult = xsInteger((xsIntegerValue)v);
}

void
xs_integer_setInteger(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis), *src;
	kcl_err_t err;

	src = arith_get_integer(xsArg(0));
	err = kcl_int_copy(ai, src);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
}

void
xs_integer_setNaN(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);

	kcl_int_setNaN(ai);
}

void
xs_integer_negate(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);

	arith_check_nan(ai);
	kcl_int_neg(ai);
}

void
xs_integer_isZero(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);

	arith_check_nan(ai);
	xsResult = xsBoolean(kcl_int_iszero(ai));
}

void
xs_integer_isNaN(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);

	xsResult = xsBoolean(kcl_int_isNaN(ai));
}

void
xs_integer_comp(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	kcl_int_t *o = arith_get_integer(xsArg(0));

	arith_check_nan(ai);
	xsResult = xsInteger(kcl_int_comp(ai, o));
}

void
xs_integer_sign(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);

	arith_check_nan(ai);
	xsResult = xsBoolean(kcl_int_sign(ai));
}

void
xs_integer_sizeof(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);

	arith_check_nan(ai);
	xsResult = xsInteger(kcl_int_sizeof(ai));
}

void
xs_integer_inc(xsMachine *the)
{
	kcl_int_t *ai = xsmcGetHostData(xsThis);
	int d = 1;
	kcl_err_t err;

	arith_check_nan(ai);
	if (xsmcArgc > 1 && xsmcTypeOf(xsArg(0)) == xsIntegerType)
		d = xsmcToInteger(xsArg(0));
	err = kcl_int_inc(ai, d);
	if (err != KCL_ERR_NONE)
		kcl_throw_error(the, err);
}
