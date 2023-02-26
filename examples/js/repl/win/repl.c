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
#include "mc.xs.h"

extern txPreparation* xsPreparation;

void fxAbort(xsMachine* the, int status)
{
	exit(status);
}

void xs_repl_destructor(void* it)
{
}

void xs_repl_receive(xsMachine *the)
{
	int c = getc(stdin);

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

			putc(c, stdout);

			str++;
		} while (1);
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
#ifdef mxInstrument
	fxSampleInstrumentation(the, 0, NULL);
#endif
}

int main(int argc, char* argv[])
{
	static txMachine root;
	int error = 0;
	txMachine* machine = &root;
	txPreparation* preparation = xsPreparation();

	c_memset(machine, 0, sizeof(txMachine));
	machine->preparation = preparation;
	machine->keyArray = preparation->keys;
	machine->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.initialKeyCount;
	machine->keyIndex = (txID)preparation->keyCount;
	machine->nameModulo = preparation->nameModulo;
	machine->nameTable = preparation->names;
	machine->symbolModulo = preparation->symbolModulo;
	machine->symbolTable = preparation->symbols;
	
	machine->stack = &preparation->stack[0];
	machine->stackBottom = &preparation->stack[0];
	machine->stackTop = &preparation->stack[preparation->stackCount];
	
	machine->firstHeap = &preparation->heap[0];
	machine->freeHeap = &preparation->heap[preparation->heapCount - 1];
	machine->aliasCount = (txID)preparation->aliasCount;

	machine = fxCloneMachine(&preparation->creation, machine, "tool", NULL);

#ifdef mxInstrument
	fxDescribeInstrumentation(machine, 0, NULL, NULL);
#endif
	xsBeginHost(machine);
	{
		xsVars(2);
#ifdef mxInstrument
		fxSampleInstrumentation(the, 0, NULL);
#endif
		{
			xsTry {
				xsResult = xsAwaitImport("main", XS_IMPORT_NAMESPACE);
			}
			xsCatch {
				xsStringValue message = xsToString(xsException);
				fprintf(stderr, "### %s\n", message);
				error = 1;
			}
		}
	}
	xsEndHost(the);
	xsDeleteMachine(machine);
	
	return error;
}
