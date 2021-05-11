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
}

void xs_repl_write(xsMachine *the)
{
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
	xsResult = fxIsConnected(the) ? xsTrue : xsFalse;
#else
	xsResult = xsFalse;
#endif
}
