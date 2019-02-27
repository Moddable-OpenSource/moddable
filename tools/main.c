/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "tool.h"

static char** then = NULL;

void fxAbort(xsMachine* the)
{
	exit(1);
}

int main(int argc, char* argv[]) 
{
	static txMachine root;
	int error = 0;
	txMachine* machine = &root;
	txPreparation* preparation = xsPreparation();
	
	c_memset(machine, 0, sizeof(txMachine));
	machine->archive = preparation;
	machine->keyArray = preparation->keys;
	machine->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.keyCount;
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
	
	xsBeginHost(machine);
	{
		xsVars(2);
		{
			xsTry {
				if (argc > 1) {
					int argi;
					xsVar(0) = xsNewArray(0);
					for (argi = 1; argi < argc; argi++) {
						xsSetAt(xsVar(0), xsInteger(argi - 1), xsString(argv[argi]));
					}
					xsVar(1) = xsCall1(xsGlobal, xsID_require, xsString(argv[1]));
					fxPush(xsVar(0));
					fxPushCount(the, 1);
					fxPush(xsVar(1));
					fxNew(the);
					xsResult = fxPop();
					xsCall0(xsResult, xsID_run);
				}
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
	if (!error && then) {
	#if mxWindows
		error =_spawnvp(_P_WAIT, then[0], then);
		if (error < 0)
			fprintf(stderr, "### Cannot execute %s!\n", then[0]);
	#else
		execvp(then[0], then);
	#endif
	}
	return error;
}



void Tool_prototype_then(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	then = malloc(sizeof(char *)*(c + 1));
	for (i = 0; i < c; i++) {
		xsStringValue string = xsToString(xsArg(i));
		xsIntegerValue length = strlen(string) + 1;
		then[i] = malloc(length);
		c_memcpy(then[i], string, length);
	}
	then[c] = NULL;
}


