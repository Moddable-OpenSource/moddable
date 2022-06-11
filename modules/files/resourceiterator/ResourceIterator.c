/*
 * Copyright (c) 2020  Moddable Tech, Inc.
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

#include "xsmc.h"
#include "stddef.h"

extern const char *mcGetResourceName(void* it, int i);
extern int mcCountResources(void* it);

void Resource_index(xsMachine *the)
{
	xsIntegerValue archiveCount, i;
	int index = xsmcToInteger(xsArg(0));
	if (index < 0)
		return;

	archiveCount = fxGetArchiveDataCount(the, the->archive);
	if (index < archiveCount)
		xsmcSetString(xsResult, fxGetArchiveDataName(the, the->archive, index));
	else {
		xsIntegerValue count = mcCountResources(NULL);

		index -= archiveCount;
		if (index >= count)
			return;

		if (!archiveCount) {
			xsmcSetString(xsResult, (char *)mcGetResourceName(NULL, index));
			return;
		}

		for (i = 0; i < count; i++) {
			size_t size;
			const char *name = mcGetResourceName(NULL, i);
			if (!name)
				return;

			if (fxGetArchiveData(the, the->archive, (char *)name, &size))
				continue;	// resource with same name in archive and host. only report once.

			if (index) {
				index -= 1;
				continue;
			}

			xsmcSetString(xsResult, (char *)name);
			return;
		}
	}
}
