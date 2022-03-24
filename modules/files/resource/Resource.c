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

#include "xs.h"
#include "string.h"
#include "mc.xs.h"

extern const void* mcGetResource(xsMachine* the, char* path, size_t* size);

void Resource_destructor(void *data)
{
	/* nothing to do */
}

void Resource_constructor(xsMachine *the)
{
	xsStringValue path = xsToString(xsArg(0));
	const void *data = NULL;
	size_t size;
	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1)))
		data = fxGetArchiveData(the, xsGetHostData(xsArg(1)), path, &size);
	if (!data)
		data = fxGetArchiveData(the, the->archive, path, &size);
	if (!data)
		data = mcGetResource(the, path, &size);
	if (!data)
		xsURIError("Resource not found: %s", path);
	xsSetHostBuffer(xsThis, (void *)data, size);
	xsPetrifyHostBuffer(xsThis);
}

void Resource_exists(xsMachine *the)
{
	xsStringValue path = xsToString(xsArg(0));
	const void *data = NULL;
	size_t size;
	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1)))
		data = fxGetArchiveData(the, xsGetHostData(xsArg(1)), path, &size);
	if (!data)
		data = fxGetArchiveData(the, the->archive, path, &size);
	if (!data)
		data = mcGetResource(the, path, &size);
	xsResult = data ? xsTrue : xsFalse;
}

void Resource_get_byteLength(xsMachine *the)
{
	int byteLength = xsGetHostBufferLength(xsThis);
	xsResult = xsInteger(byteLength);
}

void Resource_slice(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	unsigned char *data = xsGetHostData(xsThis);
	int start = 0;
	int byteLength = xsGetHostBufferLength(xsThis);
	int end = byteLength;
	xsBooleanValue copy = 1;

	if (argc) {
		start = xsToInteger(xsArg(0));
		if (start < 0) start = 0;
		if (argc > 1) {
			end = xsToInteger(xsArg(1));
			if (end < start) 
				end = start;
			else if (end > byteLength)
				end = byteLength;
			if (argc > 2)
				copy = xsTest(xsArg(2));
		}
	}

	if (copy)
		xsResult = xsArrayBuffer(data + start, end - start);
	else {
		xsResult = xsNewHostObject(NULL);
		xsSetHostBuffer(xsResult, (void *)(data + start), end - start);
		xsPetrifyHostBuffer(xsResult);
		xsDefine(xsResult, xsID_byteLength, xsInteger(end - start), xsDontDelete | xsDontSet);
	}
}
