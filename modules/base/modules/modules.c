/*
 * Copyright (c) 2018-2022  Moddable Tech, Inc.
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

extern void* xsPreparationAndCreation(xsCreation **creation);

void xs_modules_host(xsMachine *the)
{
	txPreparation *preparation = xsPreparationAndCreation(NULL);
	xsResult = xsNewArray(0);
	if (preparation) {
		txInteger scriptCount = preparation->scriptCount, i = 0;
		txScript* script = preparation->scripts;
		while (scriptCount--) {
			xsSetIndex(xsResult, i++, xsString(script->path));
			script++;
		}
	}
}

#define mxArchiveHeaderSize (sizeof(Atom) + sizeof(Atom) + XS_VERSION_SIZE + sizeof(Atom) + XS_DIGEST_SIZE + sizeof(Atom) + XS_DIGEST_SIZE)

void xs_modules_archive(xsMachine *the)
{
	uint8_t *p = the->archive;
	xsResult = xsNewArray(0);
	if (p) {
		uint8_t *q;
		int i = 0;
		p += mxArchiveHeaderSize;
		// NAME
		p += c_read32be(p);
		// SYMB
		p += c_read32be(p);
		// MODS
		q = p + c_read32be(p);
		p += sizeof(Atom);
		while (p < q) {
			int atomSize = c_read32be(p);
			xsSetIndex(xsResult, i++, xsString((txString)(p + sizeof(Atom))));
			p += atomSize;
			p += c_read32be(p);
		}
	}
}

void xs_modules_importNow(xsMachine *the)
{
	char path[C_PATH_MAX];

	xsToStringBuffer(xsArg(0), path, sizeof(path));
	xsResult = xsAwaitImport(path, XS_IMPORT_DEFAULT);
}

void xs_modules_has(xsMachine *the)
{
	char name[C_PATH_MAX];

	xsToStringBuffer(xsArg(0), name, sizeof(name));
	xsResult = xsAwaitImport(name, XS_IMPORT_DEFAULT | XS_IMPORT_PREFLIGHT);
}
