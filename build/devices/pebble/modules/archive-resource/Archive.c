/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

 #include "xsAll.h"
#include "xs.h"
#include "mc.xs.h"
#include "mc.defines.h"

void Archive_constructor(xsMachine *the)
{
	xsTypeError("new Archive");
}

void Archive_destructor(void* data)
{
}

void  Archive_get_modulePaths(xsMachine *the)
{
	void* archive = xsGetHostData(xsThis);
	xsIntegerValue c = fxGetArchiveCodeCount(the, archive), i;
	xsResult = xsNewArray(c);
	for (i = 0; i < c; i++) {
		xsStringValue name = fxGetArchiveCodeName(the, archive, i);
		xsSetIndex(xsResult, i, xsString(name));
	}
}

void Archive_get_name(xsMachine *the)
{
	void* archive = xsGetHostData(xsThis);
	xsStringValue name = fxGetArchiveName(the, archive);
	xsResult = xsString(name);;
}

void Archive_get_resourcePaths(xsMachine *the)
{
	void* archive = xsGetHostData(xsThis);
	xsIntegerValue c = fxGetArchiveDataCount(the, archive), i;
	xsResult = xsNewArray(c);
	for (i = 0; i < c; i++) {
		xsStringValue name = fxGetArchiveDataName(the, archive, i);
		xsSetIndex(xsResult, i, xsString(name));
	}
}

