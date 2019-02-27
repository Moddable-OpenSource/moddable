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

typedef struct {
	txString strings[5];
	size_t sizes[5];
	size_t offset;
	txInteger index;
} txFunctionStream;

static txString fxCombinePath(txParser* parser, txString theBase, txString theName);
static txString fxRealDirectoryPath(txParser* parser, txString path);
static txString fxRealFilePath(txParser* parser, txString path);
static void fxWriteExterns(txScript* script, FILE* file);
static void fxWriteHosts(txScript* script, FILE* file);
static void fxWriteIDs(txScript* script, FILE* file);

void fxVReport(void* console, txString theFormat, c_va_list theArguments)
{
	vfprintf(stderr, theFormat, theArguments);
}

void fxVReportError(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
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

void fxVReportWarning(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
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

txString fxCombinePath(txParser* parser, txString base, txString name)
{
	txSize baseLength, nameLength;
	txString path;
	txString separator ;
#if mxWindows
	separator = name;
	while (*separator) {
		if (*separator == '/')
			*separator = mxSeparator;
		separator++;
	}
#endif
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
	return fxRealFilePath(parser, path);
}

txString fxRealDirectoryPath(txParser* parser, txString path)
{
#if mxWindows
	char buffer[C_PATH_MAX];
	DWORD attributes;
	if (_fullpath(buffer, path, C_PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
  	 		strcat(buffer, "\\");
			return fxNewParserString(parser, buffer, strlen(buffer));
		}
	}
#else
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
#endif
	parser->error = C_EINVAL; 
	fprintf(stderr, "#  directory not found: %s!\n", path);
	c_longjmp(parser->firstJump->jmp_buf, 1); 
}

txString fxRealFilePath(txParser* parser, txString path)
{
#if mxWindows
	char buffer[C_PATH_MAX];
	DWORD attributes;
	if (_fullpath(buffer, path, C_PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (!(attributes & FILE_ATTRIBUTE_DIRECTORY))) {
			return fxNewParserString(parser, buffer, strlen(buffer));
		}
	}
#else
	char buffer[C_PATH_MAX];
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISREG(a_stat.st_mode)) {
				return fxNewParserString(parser, buffer, strlen(buffer));
			}
		}
	}
#endif
	parser->error = C_EINVAL; 
	fprintf(stderr, "#  file not found: %s!\n", path);
	c_longjmp(parser->firstJump->jmp_buf, 1); 
}

void fxWriteExterns(txScript* script, FILE* file)
{
	txByte* p = script->hostsBuffer;
	txID c, i;
	fprintf(file, "mxExport void xsHostModule(xsMachine* the);\n\n");
	mxDecode2(p, c);
	for (i = 0; i < c; i++) {
		txS1 length = *p++;
		p += 2;
		if (length < 0)
			fprintf(file, "extern void %s(void* data);\n", p);
		else
			fprintf(file, "extern void %s(xsMachine* the);\n", p);
		p += c_strlen((char*)p) + 1;
	}
}

void fxWriteHosts(txScript* script, FILE* file)
{
	txByte* p = script->hostsBuffer;
	txID c, i, id;
	mxDecode2(p, c);
	fprintf(file, "void xsHostModule(xsMachine* the)\n");
	fprintf(file, "{\n");
	fprintf(file, "\tstatic xsHostBuilder builders[%d] = {\n", c);
	for (i = 0; i < c; i++) {
		txS1 length = *p++;
		mxDecode2(p, id);
		if (length < 0)
			fprintf(file, "\t\t{ (xsCallback)%s, -1, -1 },\n", p);
		else
			fprintf(file, "\t\t{ %s, %d, %d },\n", p, length, id);
		p += c_strlen((char*)p) + 1;
	}
	fprintf(file, "\t};\n");
	fprintf(file, "\txsResult = xsBuildHosts(%d, builders);\n", c);
	fprintf(file, "}\n\n");
}

void fxWriteIDs(txScript* script, FILE* file)
{
	txByte* p = script->symbolsBuffer;
	txID c, i;
	mxDecode2(p, c);
	for (i = 0; i < c; i++) {
		if (fxIsIdentifier((txString)p))
			fprintf(file, "#define xsID_%s (the->code[%d])\n", p, i);
		p += c_strlen((char*)p) + 1;
	}
}

int main(int argc, char* argv[]) 
{
	int argi;
	char path[C_PATH_MAX];
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;

	txUnsigned flags = 0;
	txString input = NULL;
	txString output = NULL;
	txString rename = NULL;
	txString temporary = NULL;
	txString name = NULL;
	txString map = NULL;
	txString dot = NULL;
	FILE* file = NULL;
	txScript* script = NULL;
	txSize size;
	txByte byte;
	txBoolean embed = 0;
	txBoolean binary = 0;
	txSize length;

	fxInitializeParser(parser, C_NULL, 32*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		for (argi = 1; argi < argc; argi++) {
			if (!strcmp(argv[argi], "-b"))
				binary = 1;
			else if (!strcmp(argv[argi], "-c"))
				flags |= mxCFlag;
			else if (!strcmp(argv[argi], "-d"))
				flags |= mxDebugFlag;
			else if (!strcmp(argv[argi], "-e"))
				embed = 1;
			else if (!strcmp(argv[argi], "-o")) {
				argi++;
				if (argi >= argc)
					fxReportParserError(parser, "no output directory");
				else if (output)
					fxReportParserError(parser, "too many output directories");
				else
					output = fxRealDirectoryPath(parser, argv[argi]);
			}
			else if (!strcmp(argv[argi], "-m"))
				flags |= mxCommonModuleFlag;
			else if (!strcmp(argv[argi], "-p"))
				flags |= mxProgramFlag;
			else if (!strcmp(argv[argi], "-r")) {
				argi++;
				if (argi >= argc)
					fxReportParserError(parser, "no name");
				else if (rename)
					fxReportParserError(parser, "too many names");
				else
					rename = fxNewParserString(parser, argv[argi], strlen(argv[argi]));
			}
			else if (!strcmp(argv[argi], "-t")) {
				argi++;
				if (argi >= argc)
					fxReportParserError(parser, "no temporary directory");
				else if (output)
					fxReportParserError(parser, "too many temporary directories");
				else
					temporary = fxRealDirectoryPath(parser, argv[argi]);
			}
			else {
				if (input)
					fxReportParserError(parser, "too many files");
				else
					input = fxRealFilePath(parser, argv[argi]);
			}
		}
		if (!input)
			fxReportParserError(parser, "no file");
		if (!output)
			output = fxRealDirectoryPath(parser, ".");
		if (!temporary)
			temporary = output;
		if (parser->errorCount)
			fxThrowParserError(parser, parser->errorCount);
			
		name = NULL;
		parser->origin = parser->path = fxNewParserSymbol(parser, input);
		file = fopen(input, "r");
		mxParserThrowElse(file);
		fxParserTree(parser, file, (txGetter)fgetc, flags, &name);
		fclose(file);
		file = NULL;
		if (name) {
			map = fxCombinePath(parser, input, name);
			parser->path = fxNewParserSymbol(parser, map);
			file = fopen(map, "r");
			mxParserThrowElse(file);
			fxParserSourceMap(parser, file, (txGetter)fgetc, flags, &name);
			fclose(file);
			file = NULL;
			map = fxCombinePath(parser, map, name);
			parser->path = fxNewParserSymbol(parser, map);
		}
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
		
		if (rename) {
			name = rename;
		}
		else {
			name = strrchr(input, mxSeparator);
			name++;
			dot = strrchr(name, '.');
			*dot = 0;
 		}
 	 	strcpy(path, output);
 	 	strcat(path, name);
 	 	if (binary)
	 	 	strcat(path, ".jsb");
	 	 else
	 	 	strcat(path, ".xsb");
		file = fopen(path, "wb");
		mxParserThrowElse(file);
		
		size = 8 + 8 + 4 + 8 + script->symbolsSize + 8 + script->codeSize;
		if (script->hostsBuffer && embed)
			size += 8 + script->hostsSize;
		if (rename) {
			length = strlen(rename) + 1;
			size += 8 + length;
		}
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
		byte = (script->hostsBuffer) ? embed ? -1 : 1 : 0;
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
		
		if (script->hostsBuffer && embed) {
			size = 8 + script->hostsSize;
			size = htonl(size);
			mxParserThrowElse(fwrite(&size, 4, 1, file) == 1);
			mxParserThrowElse(fwrite("HOST", 4, 1, file) == 1);
			mxParserThrowElse(fwrite(script->hostsBuffer, script->hostsSize, 1, file) == 1);
		}
		if (rename) {
			size = 8 + length;
			size = htonl(size);
			mxParserThrowElse(fwrite(&size, 4, 1, file) == 1);
			mxParserThrowElse(fwrite("NAME", 4, 1, file) == 1);
			mxParserThrowElse(fwrite(rename, length, 1, file) == 1);
		}

		fclose(file);
		file = NULL;
		
		if (script->hostsBuffer && !embed) {
			strcpy(path, temporary);
			strcat(path, name);
			strcat(path, ".xs.h");
			file = fopen(path, "w");
			mxParserThrowElse(file);
			fprintf(file, "/* XS GENERATED FILE; DO NOT EDIT! */\n\n");
			fprintf(file, "#include <xs.h>\n\n");
			fxWriteExterns(script, file);
			fprintf(file, "\n");
			fxWriteIDs(script, file);
			fprintf(file, "\n");
			fclose(file);
			file = NULL;
		
			strcpy(path, temporary);
			strcat(path, name);
			strcat(path, ".xs.c");
			file = fopen(path, "w");
			mxParserThrowElse(file);
			fprintf(file, "/* XS GENERATED FILE; DO NOT EDIT! */\n\n");
			fprintf(file, "#include \"%s.xs.h\"\n\n", name);
			fxWriteHosts(script, file);
			fclose(file);
			file = NULL;
		}
	}
	else {
		if (parser->error != C_EINVAL) {
		#if mxWindows
			char buffer[2048];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, parser->error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
			fprintf(stderr, "### %s\n", buffer);
		#else
			fprintf(stderr, "### %s\n", strerror(parser->error));
		#endif
		}
	}
	if (parser->errorCount > 0) {
		fprintf(stderr, "### %d error(s)\n", parser->errorCount);
		parser->error = C_EINVAL;
	}
	else if (parser->warningCount > 0)
		fprintf(stderr, "### %d warning(s)\n", parser->warningCount);
	fxDeleteScript(script);
	if (file)
		fclose(file);
	fxTerminateParser(parser);
	return parser->error;
}
