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
 */


#include <stdio.h>
#include <stdlib.h>

#include "mc_xs.h"

void destructor(void *data)
{
	if (data)
		free(data);
}

void xs_host_loadFile(xsMachine *the)
{
	const char *path = xsToString(xsArg(0));
	FILE *font;
	int size;
	char *buffer;

	font = fopen(path, "rb");
	if (!font)
		xsErrorPrintf("can't open file");
	fseek(font, 0, SEEK_END);
	size = ftell(font);
	buffer = malloc(size);
	if (!buffer) {
		fclose(font);
		xsErrorPrintf("can't allocate file memory");
	}

	fseek(font, 0, SEEK_SET);
	fread(buffer, 1, size, font);
	fclose(font);

	xsResult = xsNewHostObject(destructor);
	xsSetHostData(xsResult, buffer);
	xsVars(1);
	xsVar(0) = xsInteger(size);
	xsSet(xsResult, xsID("byteLength"), xsVar(0));
}
