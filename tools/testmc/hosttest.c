/*
 * Copyright (c) 2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsmc.h"
#include "mc.xs.h"
#include "xsHost.h"

void xs_hostobject_destructor(void *data)
{
}

void xs_hostobject(xsMachine *the)
{
	xsmcSetHostData(xsThis, (void *)-1);
}


void xs_hostbuffer_destructor(void *data)
{
	if (data)
		c_free(data);
}

void xs_hostobjectchunk(xsMachine *the)
{
	xsmcSetHostChunk(xsThis, NULL, 16);
}

void xs_hostobjectchunk_destructor(void *data)
{
}

void xs_hostbuffer(xsMachine *the)
{
	int byteLength = xsmcToInteger(xsArg(0));
	void *data = c_calloc(byteLength, 1);
	if (!data)
		xsUnknownError("no memory");
	
	xsmcSetHostBuffer(xsThis, data, byteLength);

	xsSlot tmp;
	xsmcSetInteger(tmp, byteLength);
	xsmcDefine(xsThis, xsID_byteLength, tmp, xsDontDelete | xsDontSet);
}
