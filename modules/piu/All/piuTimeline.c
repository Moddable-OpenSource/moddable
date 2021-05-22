/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
 */

#include "xsPlatform.h"
#include "mc.xs.h"

typedef struct {
	xsNumberValue	from;
	xsNumberValue	range;
	xsIdentifier	id;
} TweenPropertyRecord, *TweenProperty;

void xs_TweenProperty_destructor(void *data)
{
}

void xs_TweenProperty(xsMachine *the)
{
	TweenPropertyRecord tr;

	tr.from = xsToNumber(xsArg(1));
	tr.range = xsToNumber(xsArg(2)) - tr.from;
	tr.id = xsID(xsToString(xsArg(0)));
	xsSetHostChunk(xsThis, &tr, sizeof(tr));
}

void xs_TweenProperty_tween(xsMachine *the)
{
	TweenProperty tr = (TweenProperty)xsGetHostChunk(xsThis);
	xsNumberValue fraction = xsToNumber(xsArg(1));
	xsNumberValue result = tr->from + (fraction * tr->range);

	xsSet(xsArg(0), tr->id, xsNumber(result));
}

typedef struct {
	xsIdentifier	id;
	uint16_t		length;
	xsSlot			*values;
} TweenOnPropertyRecord, *TweenOnProperty;

static void TweenOnPropertyDelete(void* it)
{
}

static void TweenOnPropertyMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	TweenOnProperty tr = it;
	(*markRoot)(the, tr->values);
}

static const xsHostHooks ICACHE_FLASH_ATTR TweenOnPropertyHooks = {
	TweenOnPropertyDelete,
	TweenOnPropertyMark,
	NULL
};


void xs_TweenOnProperty_destructor(void *data)
{
}

void xs_TweenOnProperty(xsMachine *the)
{
	TweenOnPropertyRecord tr;

	tr.length = (uint16_t)xsToInteger(xsGet(xsArg(1), xsID_length)) - 1;
	tr.values = xsToReference(xsArg(1));
	tr.id = xsID(xsToString(xsArg(0)));

	xsSetHostChunk(xsThis, &tr, sizeof(tr));
	xsSetHostHooks(xsThis, &TweenOnPropertyHooks);
}

void xs_TweenOnProperty_tween(xsMachine *the)
{
	TweenOnProperty tr = (TweenOnProperty)xsGetHostChunk(xsThis);
	int fromI, toI;
	xsNumberValue fraction = xsToNumber(xsArg(1));
	xsNumberValue index, from, range, result;

	xsVars(1);
	xsVar(0) = xsReference(tr->values);

	index = tr->length * fraction;
	fromI = (int)c_floor(index);
	toI = ((fromI + 1) < tr->length) ? (fromI + 1) : tr->length;

	from = xsToNumber(xsGetIndex(xsVar(0), fromI));
	range = xsToNumber(xsGetIndex(xsVar(0), toI)) - from;
	result = from + ((index - fromI) * range);
	xsSet(xsArg(0), tr->id, xsNumber(result));
}
