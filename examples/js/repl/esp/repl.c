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

#include "xsAll.h"
#include "xs.h"

void xs_repl_destructor(void)
{
}

void xs_repl_receive(xsMachine *the)
{
	int c = ESP_getc();

	if (-1 == c)
		return;

	xsResult = xsInteger(c);
}

void xs_repl_write(xsMachine *the)
{
	int argc = xsToInteger(xsArgc), i;

	for (i = 0; i < argc; i++) {
		char *str = xsToString(xsArg(i));

		do {
			uint8_t c = c_read8(str);
			if (!c) break;

			ESP_putc(c);

			str++;
		} while (true);
	}
}

void xs_repl_eval(txMachine* the)
{
	txStringStream aStream;
	txSlot* realm = mxProgram.value.reference->next->value.module.realm;
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = c_strlen(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag), mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, mxProgram.value.reference);
	mxPullSlot(mxResult);
}

void xs_repl_isDebug(xsMachine *the)
{
#ifdef mxDebug
	xsResult = xsTrue;
#else
	xsResult = xsFalse;
#endif
}

