/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

#include "xsAll.h"
#include "xsScript.h"

static void Tool_prototype_listSpecifiersError(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
	if (thePath) {
		#if mxWindows
			fprintf(stderr, "%s(%d): error: ", thePath, (int)theLine);
		#else
			fprintf(stderr, "%s:%d: error: ", thePath, (int)theLine);
		#endif
	}
	else
		fprintf(stderr, "# error: ");
	vfprintf(stderr, theFormat, theArguments);
	fprintf(stderr, "!\n");
}

static void Tool_prototype_listSpecifiersWarning(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
	if (thePath) {
		#if mxWindows
			fprintf(stderr, "%s(%d): warning: ", thePath, (int)theLine);
		#else
			fprintf(stderr, "%s:%d: warning: ", thePath, (int)theLine);
		#endif
	}
	else
		fprintf(stderr, "# warning: ");
	vfprintf(stderr, theFormat, theArguments);
	fprintf(stderr, "!\n");
}

void Tool_prototype_listSpecifiers(txMachine* the)
{
	char *path = fxToString(the, mxArgv(0));
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	FILE* file = NULL;
	txString name = NULL;
	fxInitializeParser(parser, NULL, the->parserBufferSize, the->parserTableModulo);
	parser->firstJump = &jump;
	parser->reportError = Tool_prototype_listSpecifiersError;
	parser->reportWarning = Tool_prototype_listSpecifiersWarning;
	if (c_setjmp(jump.jmp_buf) == 0) {
		file = fopen(path, "r");
		mxParserThrowElse(file);
		parser->path = fxNewParserSymbol(parser, path);
		fxParserTree(parser, file, (txGetter)fgetc, mxDebugFlag | mxStrictFlag, &name);
		fclose(file);
		file = NULL;
		fxParserHoist(parser);
		fxParserBind(parser);
		if (parser->errorCount == 0) {
			txModuleNode* self = (txModuleNode*)(parser->root);
			txDeclareNode* node = self->scope->firstDeclareNode;
			txBoolean hasDefault = 0;
			txBoolean isAsync = (self->flags & mxAwaitingFlag) ? 1 : 0;
			txSize modulo;
			
			fxNewObject(the);
			mxPullSlot(mxResult);

			mxPush(mxArrayPrototype);
			fxNewArrayInstance(the);
			fxArrayCacheBegin(the, the->stack);
			while (node) {
				if (node->flags & mxDeclareNodeUseClosureFlag) {
					txSpecifierNode* specifier = node->importSpecifier;
					if (specifier) {
						txSlot* slot = fxNewObject(the);
						slot = fxNextStringProperty(the, slot, ((txStringNode*)(specifier->from))->value, mxID(_from), XS_NO_FLAG);
						slot = fxNextIntegerProperty(the, slot, node->line, mxID(_line), XS_NO_FLAG);
						fxArrayCacheItem(the, the->stack + 1, the->stack);
						mxPop();
					}
					specifier = node->firstExportSpecifier;
					while (specifier) {
						if (specifier->asSymbol) {
							if (!c_strcmp(specifier->asSymbol->string, "default"))
								hasDefault = 1;
						}
						else if (specifier->symbol) {
							if (!c_strcmp(specifier->symbol->string, "default"))
								hasDefault = 1;
						}
						specifier = specifier->nextSpecifier;
					}
				}
				node = node->nextDeclareNode;
			}
			fxArrayCacheEnd(the, the->stack);
			mxPushSlot(mxResult);
			mxSetID(mxID(_from));
			mxPop();
			
			mxPush(mxArrayPrototype);
			fxNewArrayInstance(the);
			fxArrayCacheBegin(the, the->stack);
			for (modulo = 0; modulo < parser->symbolModulo; modulo++) {
				txSymbol* symbol = parser->symbolTable[modulo];
				while (symbol) {
					if (symbol->usage & 2) {
						mxPushStringC(symbol->string);
						fxArrayCacheItem(the, the->stack + 1, the->stack);
						mxPop();
					}
					symbol = symbol->next;
				}
			}
			fxArrayCacheEnd(the, the->stack);
			mxPushSlot(mxResult);
			mxSetID(mxID(_global));
			mxPop();
			
			mxPushBoolean(hasDefault);
			mxPushSlot(mxResult);
			mxSetID(mxID(_default));
			mxPop();
			
			mxPushBoolean(isAsync);
			mxPushSlot(mxResult);
			mxSetID(fxID(the, "async"));
			mxPop();
		}
		else {
			txID id = fxID(the, "errorCount");
			txInteger c;
			mxPushSlot(mxThis);
			mxGetID(id);
			c = fxToInteger(the, the->stack);
			mxPop();
			c += parser->errorCount;
			mxPushInteger(c);
			mxPushSlot(mxThis);
			mxSetID(id);
			mxPop();
		}
	}
	if (file)
		fclose(file);
	fxTerminateParser(parser);
}