/*
	WARNING

	EXPERIMENTAL -- ONLY FOR USE WITH WASM
*/

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
#ifdef EMSCRIPTEN
#include "netinet/in.h"
#endif
#include <utime.h>
#define c_stat stat
#define c_utime utime
#define c_utimbuf utimbuf 


typedef struct {
	txString strings[5];
	size_t sizes[5];
	size_t offset;
	txInteger index;
} txFunctionStream;

static txString parser_fxCombinePath(txParser* parser, txString theBase, txString theName);
static txString parser_fxRealDirectoryPath(txParser* parser, txString path);
static txString parser_fxRealFilePath(txParser* parser, txString path);


#include "xsl.h"

void my_fxVReportError(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
	if (thePath) {
		fprintf(stderr, "!!%s:%d: error: ", thePath, (int)theLine);
	}
	else
		fprintf(stderr, "# error: ");
	vfprintf(stderr, theFormat, theArguments);
	fprintf(stderr, "\n");
}

void my_fxVReportWarning(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
	if (thePath) {
		fprintf(stderr, "!!%s:%d: warning: ", thePath, (int)theLine);
	}
	else
		fprintf(stderr, "# warning: ");
	vfprintf(stderr, theFormat, theArguments);
	fprintf(stderr, "\n");
}


txString parser_fxCombinePath(txParser* parser, txString base, txString name)
{
	txSize baseLength, nameLength;
	txString path;
	txString separator ;
	separator = strrchr(base, mxSeparator);
	if (separator) {
		separator++;
		baseLength = separator - base;
	}
	else
		baseLength = 0;
	nameLength = c_strlen(name);
	path = fxNewParserChunk(parser, baseLength + nameLength + 1);
	if (baseLength)
		c_memcpy(path, base, baseLength);
	c_memcpy(path + baseLength, name, nameLength + 1);
	return parser_fxRealFilePath(parser, path);
}


txString parser_fxRealDirectoryPath(txParser* parser, txString path)
{
	char buffer[C_PATH_MAX];
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISDIR(a_stat.st_mode)) {
  	 			strcat(buffer, "/");
				return fxNewParserString(parser, buffer, strlen(buffer));
			}
		}
	}
	parser->error = C_EINVAL; 
	fprintf(stderr, "#  directory not found: %s!\n", path);
	c_longjmp(parser->firstJump->jmp_buf, 1); 
}

txString parser_fxRealFilePath(txParser* parser, txString path)
{
	char buffer[C_PATH_MAX];
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISREG(a_stat.st_mode)) {
				return fxNewParserString(parser, buffer, strlen(buffer));
			}
		}
	}
	parser->error = C_EINVAL; 
	fprintf(stderr, "#  file not found: %s!\n", path);
	c_longjmp(parser->firstJump->jmp_buf, 1); 
}


int compile(char* input) {
	char path[C_PATH_MAX];
	FILE* file = NULL;
	
	txString output = NULL;
	txString dot = NULL;
	txString name = NULL;
  txUnsigned flags = mxDebugFlag;
	txString map = NULL;

	txSize size;
	txByte byte;
	txScript* script = NULL;

	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	
	fxInitializeParser(parser, C_NULL, 32*1024, 1993);
	parser->firstJump = &jump;
	parser->origin = parser->path = fxNewParserSymbol(parser, input);
	parser->reportError = my_fxVReportError;
	parser->reportWarning = my_fxVReportWarning;
	
	if (c_setjmp(jump.jmp_buf) == 0) {
		output = parser_fxRealDirectoryPath(parser, "/build");

		name = NULL;
		file = fopen(input, "r");
		mxParserThrowElse(file);
		fxParserTree(parser, file, (txGetter)fgetc, flags, &name);
		fclose(file);
		file = NULL;

		if (name) {
			map = parser_fxCombinePath(parser, input, name);
			parser->path = fxNewParserSymbol(parser, map);
			file = fopen(map, "r");
			mxParserThrowElse(file);
			fxParserSourceMap(parser, file, (txGetter)fgetc, flags, &name);
			fclose(file);
			file = NULL;
			map = parser_fxCombinePath(parser, map, name);
			parser->path = fxNewParserSymbol(parser, map);
		}

		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
		if (parser->errorCount > 0) {
			fprintf(stderr, "### %s\n", strerror(parser->error));
			fprintf(stderr, "### %d error(s)\n", parser->errorCount);
			parser->error = C_EINVAL;
		} else {
			name = strrchr(input, mxSeparator);
			if (name == NULL) {
				name = input;
			} else {
				name++;
			}

			dot = strrchr(name, '.');
			if (dot != NULL) {
				*dot = 0;
			}

			strcpy(path, output);
			strcat(path, name);
			strcat(path, ".xsb");
			
			file = fopen(path, "wb");
			mxParserThrowElse(file);


			size = 8 + 8 + 4 + 8 + script->symbolsSize + 8 + script->codeSize;
			if (script->hostsBuffer)
				size += 8 + script->hostsSize;
			size = htonl(size);
			mxParserThrowElse(fwrite(&size, 4, 1, file) == 1);
			mxParserThrowElse(fwrite("XS_B", 4, 1, file) == 1);

			size = 8 + 4;
			size = htonl(size);
			mxParserThrowElse(fwrite(&size, 4, 1, file) == 1);
			mxParserThrowElse(fwrite("VERS", 4, 1, file) == 1);
			byte = XS_MAJOR_VERSION;
			mxParserThrowElse(fwrite(&byte, 1, 1, file) == 1);
			byte = XS_MINOR_VERSION;
			mxParserThrowElse(fwrite(&byte, 1, 1, file) == 1);
			byte = XS_PATCH_VERSION;
			mxParserThrowElse(fwrite(&byte, 1, 1, file) == 1);
			byte = (script->hostsBuffer) ? -1 : 0;
			mxParserThrowElse(fwrite(&byte, 1, 1, file) == 1);

			size = 8 + script->symbolsSize;
			size = htonl(size);
			mxParserThrowElse(fwrite(&size, 4, 1, file) == 1);
			mxParserThrowElse(fwrite("SYMB", 4, 1, file) == 1);
			mxParserThrowElse(fwrite(script->symbolsBuffer, script->symbolsSize, 1, file) == 1);

			size = 8 + script->codeSize;
			size = htonl(size);
			mxParserThrowElse(fwrite(&size, 4, 1, file) == 1);
			mxParserThrowElse(fwrite("CODE", 4, 1, file) == 1);
			mxParserThrowElse(fwrite(script->codeBuffer, script->codeSize, 1, file) == 1);

			if (script->hostsBuffer) {
				size = 8 + script->hostsSize;
				size = htonl(size);
				mxParserThrowElse(fwrite(&size, 4, 1, file) == 1);
				mxParserThrowElse(fwrite("HOST", 4, 1, file) == 1);
				mxParserThrowElse(fwrite(script->hostsBuffer, script->hostsSize, 1, file) == 1);
			}
			fclose(file);
			file = NULL;
		}
	} else if (parser->error != C_EINVAL) {
		fprintf(stderr, "### %s\n", strerror(parser->error));
	}
	fxDeleteScript(script);
	if (file)
		fclose(file);
	fxTerminateParser(parser);
	return parser->error;
}

int fxLink(char* inputFiles, char* name) {
	txLinker _linker;
	txLinker* linker = &_linker;
	txLinkerInclude** includeAddress;
	txLinkerPreload** preloadAddress;
	txLinkerResource** resourceAddress;
	txLinkerScript** scriptAddress;
	txLinkerStrip** stripAddress;
	txString base = NULL;
  	txString output = NULL;
	txString url = "/";
	char path[C_PATH_MAX];
	txSize size;
	txLinkerResource* resource;
	txLinkerScript* script;
	FILE* file = NULL;
	xsCreation _creation = {
		128 * 1024 * 1024, 	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		4 * 1024 * 1024, 	/* initialHeapCount */
		1 * 1024 * 1024,	/* incrementalHeapCount */
		1024,				/* stackCount */
		2048+2048,			/* keyCount */
		1993,				/* nameModulo */
		127,				/* symbolModulo */
		32 * 1024,			/* parserBufferSize */
		1993,				/* parserTableModulo */
	};
	xsCreation* creation = &_creation;
	


	fxInitializeLinker(linker);

	if (c_setjmp(linker->jmp_buf) == 0) {
		includeAddress = &(linker->firstInclude);
		preloadAddress = &(linker->firstPreload);
		resourceAddress = &(linker->firstResource);
		scriptAddress = &(linker->firstScript);
		stripAddress = &(linker->firstStrip);
		linker->symbolModulo = creation->nameModulo;

		base = output = fxRealDirectoryPath(linker, "/build");

		char * xsbFilePath;
		xsbFilePath = strtok (inputFiles, ",");
		while (xsbFilePath != NULL){
			*scriptAddress = fxNewLinkerScript(linker, xsbFilePath, &file);
			scriptAddress = &((*scriptAddress)->nextScript);
			xsbFilePath = strtok (NULL, ",");
		}

		size = c_strlen(base);
		script = linker->firstScript;
		while (script) {
			fxBaseScript(linker, script, base, size);
			fxSlashPath(script->path, mxSeparator, url[0]);
			script = script->nextScript;
		}

		linker->symbolTable = fxNewLinkerChunkClear(linker, linker->symbolModulo * sizeof(txLinkerSymbol*));
		resource = linker->firstResource;
		while (resource) {
			fxBaseResource(linker, resource, base, size);
			resource = resource->nextResource;
		}
		script = linker->firstScript;
		while (script) {
			fxMapScript(linker, script);
			script = script->nextScript;
		}
		fxBufferSymbols(linker);
		
		c_strcpy(path, output);
		c_strcat(path, name);
		c_strcat(path, ".xsa");
		fxWriteArchive(linker, path, &file);


	}	else {
		if (linker->error != C_EINVAL) {
			fprintf(stderr, "### %s\n", strerror(linker->error));
		}
	}
	if (file)
		fclose(file);
	fxTerminateLinker(linker);
	return linker->error;
}

// Dummy impl
txBoolean fxIsConnected(txMachine* the) {return 0; }
txBoolean fxIsReadable(txMachine* the) { return 0; }
void fxReceive(txMachine* the){}
void fxDisconnect(txMachine* the) {} 
void fxSend(txMachine* the, txBoolean more) {}
txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
	return C_NULL;
}
void fxAbort(txMachine* the){ c_exit(0); }
