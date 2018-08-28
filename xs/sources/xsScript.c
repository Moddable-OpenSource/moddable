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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "xsScript.h"

void fxDisposeParserChunks(txParser* parser)
{
	txParserChunk* block = parser->first;
	while (block) {
		txParserChunk* next = block->next;
		c_free(block);
		block = next;
	}
}

void fxInitializeParser(txParser* parser, void* console, txSize bufferSize, txSize symbolModulo)
{
	c_memset(parser, 0, sizeof(txParser));
	parser->first = C_NULL;
	parser->console = console;

	parser->buffer = fxNewParserChunk(parser, bufferSize);
	parser->bufferSize = bufferSize;
	
	parser->dtoa = fxNew_dtoa(NULL);
	parser->symbolModulo = symbolModulo;
	parser->symbolTable = fxNewParserChunkClear(parser, parser->symbolModulo * sizeof(txSymbol*));

	parser->emptyString = fxNewParserString(parser, "", 0);

	parser->ObjectSymbol = fxNewParserSymbol(parser, "Object");
	parser->__dirnameSymbol = fxNewParserSymbol(parser, "__dirname");
	parser->__filenameSymbol = fxNewParserSymbol(parser, "__filename");
	parser->__jsx__Symbol = fxNewParserSymbol(parser, "__jsx__");
	parser->__proto__Symbol = fxNewParserSymbol(parser, "__proto__");
	parser->allSymbol = fxNewParserSymbol(parser, "*");
	parser->argsSymbol = fxNewParserSymbol(parser, "args");
	parser->argumentsSymbol = fxNewParserSymbol(parser, "arguments");
	parser->arrowSymbol = fxNewParserSymbol(parser, "=>");
	parser->asSymbol = fxNewParserSymbol(parser, "as");
	parser->asyncSymbol = fxNewParserSymbol(parser, "async");
	parser->awaitSymbol = fxNewParserSymbol(parser, "await");
	parser->callerSymbol = fxNewParserSymbol(parser, "caller");
	parser->constructorSymbol = fxNewParserSymbol(parser, "constructor");
	parser->defaultSymbol = fxNewParserSymbol(parser, "default");
	parser->doneSymbol = fxNewParserSymbol(parser, "done");
	parser->evalSymbol = fxNewParserSymbol(parser, "eval");
	parser->exportsSymbol = fxNewParserSymbol(parser, "exports");
	parser->fillSymbol = fxNewParserSymbol(parser, "fill");
	parser->freezeSymbol = fxNewParserSymbol(parser, "freeze");
	parser->fromSymbol = fxNewParserSymbol(parser, "from");
	parser->getSymbol = fxNewParserSymbol(parser, "get");
	parser->idSymbol = fxNewParserSymbol(parser, "id");
	parser->includeSymbol = fxNewParserSymbol(parser, "include");
	parser->InfinitySymbol = fxNewParserSymbol(parser, "Infinity");
	parser->lengthSymbol = fxNewParserSymbol(parser, "length");
	parser->letSymbol = fxNewParserSymbol(parser, "let");
	parser->moduleSymbol = fxNewParserSymbol(parser, "module");
	parser->nameSymbol = fxNewParserSymbol(parser, "name");
	parser->NaNSymbol = fxNewParserSymbol(parser, "NaN");
	parser->nextSymbol = fxNewParserSymbol(parser, "next");
	parser->newTargetSymbol = fxNewParserSymbol(parser, "new.target");
	parser->ofSymbol = fxNewParserSymbol(parser, "of");
	parser->prototypeSymbol = fxNewParserSymbol(parser, "prototype");
	parser->rawSymbol = fxNewParserSymbol(parser, "raw");
	parser->ReferenceErrorSymbol = fxNewParserSymbol(parser, "ReferenceError");
	parser->RegExpSymbol = fxNewParserSymbol(parser, "RegExp");
	parser->returnSymbol = fxNewParserSymbol(parser, "return");
	parser->setSymbol = fxNewParserSymbol(parser, "set");
	parser->sliceSymbol = fxNewParserSymbol(parser, "slice");
	parser->SyntaxErrorSymbol = fxNewParserSymbol(parser, "SyntaxError");
	parser->StringSymbol = fxNewParserSymbol(parser, "String");
	parser->targetSymbol = fxNewParserSymbol(parser, "target");
	parser->thisSymbol = fxNewParserSymbol(parser, "this");
	parser->throwSymbol = fxNewParserSymbol(parser, "throw");
	parser->toStringSymbol = fxNewParserSymbol(parser, "toString");
	parser->undefinedSymbol = fxNewParserSymbol(parser, "undefined");
	parser->uriSymbol = fxNewParserSymbol(parser, "uri");
	parser->valueSymbol = fxNewParserSymbol(parser, "value");
	parser->withSymbol = fxNewParserSymbol(parser, "with");
	parser->yieldSymbol = fxNewParserSymbol(parser, "yield");
	
	parser->errorSymbol = NULL;
	parser->reportError = fxVReportError;
	parser->reportWarning = fxVReportWarning;
}

void* fxNewParserChunk(txParser* parser, txSize size)
{
	txParserChunk* block = c_malloc(sizeof(txParserChunk) + size);
	if (!block)
		fxThrowMemoryError(parser);
	parser->total += sizeof(txParserChunk) + size;
	block->next = parser->first;
	parser->first = block;
	return block + 1;
}

void* fxNewParserChunkClear(txParser* parser, txSize size)
{
	void* result = fxNewParserChunk(parser, size);
    c_memset(result, 0, size);
	return result;
}

txString fxNewParserString(txParser* parser, txString buffer, txSize size)
{
	txString result = fxNewParserChunk(parser, size + 1);
	c_memcpy(result, buffer, size);
	result[size] = 0;
	return result;
}

txSymbol* fxNewParserSymbol(txParser* parser, txString theString)
{
	txString aString;
	txSize aLength;
	txSize aSum;
	txSize aModulo;
	txSymbol* aSymbol;
	
	aString = theString;
	aLength = 0;
	aSum = 0;
	while(*aString != 0) {
		aLength++;
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	aModulo = aSum % parser->symbolModulo;
	aSymbol = parser->symbolTable[aModulo];
	while (aSymbol != C_NULL) {
		if (aSymbol->sum == aSum)
			if (c_strcmp(aSymbol->string, theString) == 0)
				break;
		aSymbol = aSymbol->next;
	}
	if (aSymbol == C_NULL) {
		aSymbol = fxNewParserChunk(parser, sizeof(txSymbol));
		aSymbol->next = parser->symbolTable[aModulo];
		aSymbol->ID = -1;
		aSymbol->length = aLength + 1;
		aSymbol->string = fxNewParserString(parser, theString, aLength);
		aSymbol->sum = aSum;
		aSymbol->usage = 0;
		parser->symbolTable[aModulo] = aSymbol;
	}
	return aSymbol;
}

void fxReportParserError(txParser* parser, txString theFormat, ...)
{
	c_va_list arguments;
	parser->errorCount++;
	if (!parser->errorSymbol)
		parser->errorSymbol = parser->SyntaxErrorSymbol;
	c_va_start(arguments, theFormat);
	if (!parser->errorMessage) {
		c_vsnprintf(parser->buffer, parser->bufferSize, theFormat, arguments);
		parser->errorMessage = fxNewParserString(parser, parser->buffer, c_strlen(parser->buffer));
	}
    (*parser->reportError)(parser->console, parser->path ? parser->path->string : C_NULL, parser->line, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportReferenceError(txParser* parser, txString theFormat, ...)
{
	c_va_list arguments;
	parser->errorCount++;
	if (!parser->errorSymbol)
		parser->errorSymbol = parser->ReferenceErrorSymbol;
	c_va_start(arguments, theFormat);
	if (!parser->errorMessage) {
		c_vsnprintf(parser->buffer, parser->bufferSize, theFormat, arguments);
		parser->errorMessage = fxNewParserString(parser, parser->buffer, c_strlen(parser->buffer));
	}
    (*parser->reportError)(parser->console, parser->path ? parser->path->string : C_NULL, parser->line, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportParserWarning(txParser* parser, txString theFormat, ...)
{
	c_va_list arguments;
	parser->warningCount++;
	c_va_start(arguments, theFormat);
	(*parser->reportWarning)(parser->console, parser->path ? parser->path->string : C_NULL, parser->line, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportLineReferenceError(txParser* parser, txInteger line, txString theFormat, ...)
{
	c_va_list arguments;
	parser->errorCount++;
	if (!parser->errorSymbol)
		parser->errorSymbol = parser->ReferenceErrorSymbol;
	c_va_start(arguments, theFormat);
	if (!parser->errorMessage) {
		c_vsnprintf(parser->buffer, parser->bufferSize, theFormat, arguments);
		parser->errorMessage = fxNewParserString(parser, parser->buffer, c_strlen(parser->buffer));
	}
	(*parser->reportError)(parser->console, parser->path ? parser->path->string : C_NULL, line, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportLineError(txParser* parser, txInteger line, txString theFormat, ...)
{
	c_va_list arguments;
	parser->errorCount++;
	if (!parser->errorSymbol)
		parser->errorSymbol = parser->SyntaxErrorSymbol;
	c_va_start(arguments, theFormat);
	if (!parser->errorMessage) {
		c_vsnprintf(parser->buffer, parser->bufferSize, theFormat, arguments);
		parser->errorMessage = fxNewParserString(parser, parser->buffer, c_strlen(parser->buffer));
	}
	(*parser->reportError)(parser->console, parser->path ? parser->path->string : C_NULL, line, theFormat, arguments);
	c_va_end(arguments);
}

void fxTerminateParser(txParser* parser)
{
	fxDisposeParserChunks(parser);
	if (parser->dtoa)
		fxDelete_dtoa(parser->dtoa);
}

void fxThrowMemoryError(txParser* parser)
{
	parser->error = C_ENOMEM;
	c_longjmp(parser->firstJump->jmp_buf, 1);
}

void fxThrowParserError(txParser* parser, txInteger count)
{
	parser->error = C_EINVAL;
	c_longjmp(parser->firstJump->jmp_buf, 1);
}













