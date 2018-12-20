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

#include "xsl.h"

static txBoolean fxFindScript(txMachine* the, txString path, txID* id);
static txScript* fxLoadScript(txMachine* the, txString path);

int main(int argc, char* argv[]) 
{
	txLinker _linker;
	txLinker* linker = &_linker;
	txLinkerInclude** includeAddress;
	txLinkerPreload** preloadAddress;
	txLinkerResource** resourceAddress;
	txLinkerScript** scriptAddress;
	txLinkerStrip** stripAddress;
	int argi;
	txString base = NULL;
  	txString input = NULL;
  	txString output = NULL;
  	txString separator = NULL;
  	txBoolean stripping = 0;
  	txBoolean archiving = 0;
#if mxWindows
	txString url = "\\";
#else
	txString url = "/";
#endif
	char name[C_PATH_MAX];
	char path[C_PATH_MAX];
	txSize size;
	txLinkerInclude* include;
	txLinkerPreload* preload;
	txLinkerResource* resource;
	txLinkerScript* script;
	txLinkerStrip* strip;
	FILE* file = NULL;
	xsCreation _creation = {
		128 * 1024 * 1024, 	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		4 * 1024 * 1024, 	/* initialHeapCount */
		1 * 1024 * 1024,	/* incrementalHeapCount */
		1024,		/* stack count */
		2048+1024,	/* key count */
		1993,		/* name modulo */
		127,		/* symbol modulo */
	};
	xsCreation* creation = &_creation;
	xsMachine* the = NULL;
	txInteger count;
	txInteger globalCount;
	
	fxInitializeLinker(linker);
	if (c_setjmp(linker->jmp_buf) == 0) {
		c_strcpy(name, "mc");
		includeAddress = &(linker->firstInclude);
		preloadAddress = &(linker->firstPreload);
		resourceAddress = &(linker->firstResource);
		scriptAddress = &(linker->firstScript);
		stripAddress = &(linker->firstStrip);
		for (argi = 1; argi < argc; argi++) {
			if (!c_strcmp(argv[argi], "-a")) {
				archiving = 1;
				linker->symbolModulo = creation->nameModulo;
			}
			else if (!c_strcmp(argv[argi], "-b")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-b: no directory");
				base = fxRealDirectoryPath(linker, argv[argi]);
				if (!base)
					fxReportLinkerError(linker, "-b '%s': directory not found", argv[argi]);
			}
			else if (!c_strcmp(argv[argi], "-c")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-c: no creation");
				sscanf(argv[argi], "%d,%d,%d,%d,%d,%d,%d,%d,%d,%s", 
					&linker->creation.initialChunkSize,
					&linker->creation.incrementalChunkSize,
					&linker->creation.initialHeapCount,
					&linker->creation.incrementalHeapCount,
					&linker->creation.stackCount,
					&linker->creation.keyCount,
					&linker->creation.nameModulo,
					&linker->creation.symbolModulo,
					&linker->creation.staticSize,
					linker->main);
				linker->symbolModulo = linker->creation.nameModulo;
			}		
			else if (!c_strcmp(argv[argi], "-o")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-o: no directory");
				output = fxRealDirectoryPath(linker, argv[argi]);
				if (!output)
					fxReportLinkerError(linker, "-o '%s': directory not found", argv[argi]);
			}
			else if (!c_strcmp(argv[argi], "-p")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-p: no module");
				*preloadAddress = fxNewLinkerPreload(linker, argv[argi]);
				preloadAddress = &((*preloadAddress)->nextPreload);
			}
			else if (!c_strcmp(argv[argi], "-r")) {
				argi++;
				c_strncpy(name, argv[argi], sizeof(name));
			}
			else if (!c_strcmp(argv[argi], "-s")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-s: no symbol");
				if (!c_strcmp(argv[argi], "*"))
					stripping = 1;
				else {
					*stripAddress = fxNewLinkerStrip(linker, argv[argi]);
					stripAddress = &((*stripAddress)->nextStrip);
				}
			}
			else if (!c_strcmp(argv[argi], "-t")) {
				linker->twice = 1;
			}
			else if (!c_strcmp(argv[argi], "-u")) {
				argi++;
				if (argi >= argc)
					fxReportLinkerError(linker, "-u: no url");
				url = argv[argi];
				if (url[c_strlen(url) - 1] != mxSeparator)
					fxReportLinkerError(linker, "-u: invalid url");
			}
			else {
				input = fxRealFilePath(linker, argv[argi]);
				if (!input)
					fxReportLinkerError(linker, "'%s': file not found", argv[argi]);
				separator = c_strrchr(input, mxSeparator);
				if (!separator)
					fxReportLinkerError(linker, "'%s': invalid extension", input);
				separator = c_strrchr(separator, '.');
				if (!separator)
					fxReportLinkerError(linker, "'%s': invalid extension", input);
				if (!c_strcmp(separator, ".xsb")) {
					*scriptAddress = fxNewLinkerScript(linker, input, &file);
					scriptAddress = &((*scriptAddress)->nextScript);
				}
				else if  (!c_strcmp(separator, ".xsi")) {
					*includeAddress = fxNewLinkerInclude(linker, input);
					includeAddress = &((*includeAddress)->nextInclude);
				}
				else {
					*resourceAddress = fxNewLinkerResource(linker, input, &file);
					resourceAddress = &((*resourceAddress)->nextResource);
				}
			}
		}
		if (!output)
			output = fxRealDirectoryPath(linker, ".");
		if (!base)
			base = output;
			
		size = c_strlen(base);
		script = linker->firstScript;
		while (script) {
			fxBaseScript(linker, script, base, size);
			script = script->nextScript;
		}

		linker->symbolTable = fxNewLinkerChunkClear(linker, linker->symbolModulo * sizeof(txLinkerSymbol*));
		if (archiving) {
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
		}
		else {
			c_strcpy(path, base);
			c_strcat(path, name);
			c_strcat(path, ".xsi");
			if (fxRealFilePath(linker, path))
				fxReadSymbols(linker, path, 0, &file);
			else
				fxDefaultSymbols(linker);
			include = linker->firstInclude;
			while (include) {
				fxReadSymbols(linker, include->path, 1, &file);
				include = include->nextInclude;
			}
			script = linker->firstScript;
			while (script) {
				fxMapScript(linker, script);
				script = script->nextScript;
			}
			fxBufferSymbols(linker);
			fxWriteSymbols(linker, path, &file);
	
			linker->base = url;
			linker->baseLength = c_strlen(url);
	
			creation->nameModulo = linker->creation.nameModulo;
			creation->symbolModulo = linker->creation.symbolModulo;
			the = xsCreateMachine(creation, NULL, "xsl", linker);
			mxThrowElse(the);
			xsBeginHost(the);
			{
				xsVars(2);
				{
					xsTry {
						preload = linker->firstPreload;
						while (preload) {
							xsResult = xsCall1(xsGlobal, xsID("require"), xsString(preload->name));
							preload = preload->nextPreload;
						}
						xsCollectGarbage();
					}
					xsCatch {
						xsStringValue message = xsToString(xsException);
						fxReportLinkerError(linker, "%s", message);
					}
				}
			}
			xsEndHost(the);

			if (stripping)
				fxStripCallbacks(linker, the);
			else
				fxUnstripCallbacks(linker);
			strip = linker->firstStrip;
			while (strip) {
				fxStripName(linker, strip->name);
				strip = strip->nextStrip;
			}
	
			count = fxPrepareHeap(the);
			globalCount = the->stackTop[-1].value.reference->next->value.table.length;
	
			c_strcpy(path, output);
			c_strcat(path, name);
			c_strcat(path, ".xs.h");
			file = fopen(path, "w");
			fprintf(file, "/* XS GENERATED FILE; DO NOT EDIT! */\n\n");
			fprintf(file, "#include \"xs.h\"\n\n");
		
			fprintf(file, "#ifdef __cplusplus\n");
			fprintf(file, "extern \"C\" {\n");
			fprintf(file, "#endif\n");
			script = linker->firstScript;
			while (script) {
				fxWriteScriptExterns(script, file);
				script = script->nextScript;
			}
			fprintf(file, "#ifdef __cplusplus\n");
			fprintf(file, "}\n");
			fprintf(file, "#endif\n");
			fprintf(file, "\n");
	
			fxWriteDefines(linker, file);
			fprintf(file, "\n");

			fclose(file);
			file = NULL;
		
			c_strcpy(path, output);
			c_strcat(path, name);
			c_strcat(path, ".xs.c");
			file = fopen(path, "w");
			fprintf(file, "/* XS GENERATED FILE; DO NOT EDIT! */\n\n");
			//fprintf(file, "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"\n");

			fprintf(file, "#include \"xsPlatform.h\"\n");
			fprintf(file, "#define XS_FUNCTION_WEAK\n");
			fprintf(file, "#include \"xsAll.h\"\n");
			fprintf(file, "#include \"%s.xs.h\"\n\n", name);

			fprintf(file, "#define mxAliasCount %d\n", the->aliasCount);
			fprintf(file, "#define mxHeapCount %d\n", (int)count);
			fprintf(file, "static const txSlot gxHeap[mxHeapCount];\n");
			fprintf(file, "#define mxStackCount %d\n", the->stackTop - the->stack);
			fprintf(file, "static const txSlot gxStack[mxStackCount];\n");
			fprintf(file, "#define mxKeysCount %d\n", the->keyIndex);
			fprintf(file, "static const txSlot* gxKeys[mxKeysCount];\n");
			fprintf(file, "#define mxNamesCount %d\n", the->nameModulo);
			fprintf(file, "static const txSlot* gxNames[mxNamesCount];\n");
			fprintf(file, "#define mxSymbolsCount %d\n", the->symbolModulo);
			fprintf(file, "static const txSlot* gxSymbols[mxSymbolsCount];\n");
			fprintf(file, "#define mxGlobalsCount %d\n", (int)globalCount);
			fprintf(file, "static const txSlot* gxGlobals[mxGlobalsCount];\n");
			script = linker->firstScript;
			while (script) {
				if (script->hostsBuffer)
					fprintf(file, "static void xsHostModule%d(xsMachine* the);\n", script->scriptIndex);
				fprintf(file, "static const txU1 gxCode%d[%d];\n", script->scriptIndex, script->codeSize);
				script = script->nextScript;
			}
			fprintf(file, "#define mxScriptsCount %d\n", linker->scriptCount);
			fprintf(file, "static const txScript gxScripts[mxScriptsCount];\n");		
			fprintf(file, "static const txPreparation gxPreparation;\n\n");
			fprintf(file, "mxExport txPreparation* xsPreparation();\n\n");
		
			if (stripping || linker->firstStrip)
				fprintf(file, "static void fxDeadStrip(txMachine* the) { mxUnknownError(\"dead strip\"); }\n\n");
			fxPrintBuilders(the, file);
			script = linker->firstScript;
			while (script) {
				fxWriteScriptHosts(script, file);
				fxWriteScriptCode(script, file);
				script = script->nextScript;
			}
		
			fprintf(file, "static const txScript gxScripts[mxScriptsCount] = {\n");
			script = linker->firstScript;
			while (script) {
				fxWriteScriptRecord(script, file);
				script = script->nextScript;
				if (script)
					fprintf(file, ",\n");
			}
			fprintf(file, "\n};\n\n");
		
			if (linker->twice) {
				fprintf(file, "#if (!defined(linux)) && ((defined(__GNUC__) && defined(__LP64__)) || (defined(_MSC_VER) && defined(_M_X64)))\n");
				fprintf(file, "static const txSlot gxHeap[mxHeapCount] = {\n");
				fxPrintHeap(the, file, count, 1);
				fprintf(file, "};\n\n");
				fprintf(file, "static const txSlot gxStack[mxStackCount] = {\n");
				fxPrintStack(the, file, 1);
				fprintf(file, "};\n\n");
				fprintf(file, "#else\n");
				fprintf(file, "static const txSlot gxHeap[mxHeapCount] = {\n");
				fxPrintHeap(the, file, count, 0);
				fprintf(file, "};\n\n");
				fprintf(file, "static const txSlot gxStack[mxStackCount] = {\n");
				fxPrintStack(the, file, 0);
				fprintf(file, "};\n\n");
				fprintf(file, "#endif\n");
			}
			else {
				fprintf(file, "static const txSlot gxHeap[mxHeapCount] = {\n");
				fxPrintHeap(the, file, count, 0);
				fprintf(file, "};\n\n");
				fprintf(file, "static const txSlot gxStack[mxStackCount] = {\n");
				fxPrintStack(the, file, 0);
				fprintf(file, "};\n\n");
			}
			fprintf(file, "static const txSlot* gxGlobals[mxGlobalsCount] = {\n");
			fxPrintTable(the, file, globalCount, the->stackTop[-1].value.reference->next->value.table.address);
			fprintf(file, "};\n\n");
			fprintf(file, "static const txSlot* gxKeys[mxKeysCount] = {\n");
			fxPrintTable(the, file, the->keyIndex, the->keyArray);
			fprintf(file, "};\n\n");
			fprintf(file, "static const txSlot* gxNames[mxNamesCount] = {\n");
			fxPrintTable(the, file, the->nameModulo, the->nameTable);
			fprintf(file, "};\n\n");
			fprintf(file, "static const txSlot* gxSymbols[mxSymbolsCount] = {\n");
			fxPrintTable(the, file, the->symbolModulo, the->symbolTable);
			fprintf(file, "};\n\n");
			fprintf(file, "static const txPreparation gxPreparation = {\n");
			fprintf(file, "\t{ XS_MAJOR_VERSION, XS_MINOR_VERSION, XS_PATCH_VERSION, 0 },\n");
			fprintf(file, "\tmxAliasCount,\n");
			fprintf(file, "\tmxHeapCount,\n");
			fprintf(file, "\t(txSlot*)gxHeap,\n");
			fprintf(file, "\tmxStackCount,\n");
			fprintf(file, "\t(txSlot*)gxStack,\n");
			fprintf(file, "\tmxKeysCount,\n");
			fprintf(file, "\t(txSlot**)gxKeys,\n");
			fprintf(file, "\tmxNamesCount,\n");
			fprintf(file, "\t(txSlot**)gxNames,\n");
			fprintf(file, "\tmxSymbolsCount,\n");
			fprintf(file, "\t(txSlot**)gxSymbols,\n");
			fprintf(file, "\t%d,\n", (int)linker->baseLength);
			fprintf(file, "\t");
			fxWriteCString(file, linker->base);
			fprintf(file, ",\n");
			fprintf(file, "\tmxScriptsCount,\n");
			fprintf(file, "\t(txScript*)gxScripts,\n");
			fprintf(file, "\t{ %d, %d, %d, %d, %d, %d, %d, %d, %d },\n",
				linker->creation.initialChunkSize,
				linker->creation.incrementalChunkSize,
				linker->creation.initialHeapCount,
				linker->creation.incrementalHeapCount,
				linker->creation.stackCount,
				linker->creation.keyCount,
				linker->creation.nameModulo,
				linker->creation.symbolModulo,
				linker->creation.staticSize
			);
			fprintf(file, "\t\"%s\",\n", linker->main);
			fprintf(file, "\t{ 0x%.2X", linker->symbolsChecksum[0]);
			for (argi = 1; argi < 16; argi++)
				fprintf(file, ", 0x%.2X", linker->symbolsChecksum[argi]);
			fprintf(file, " }\n");
			fprintf(file, "};\n");
			fprintf(file, "txPreparation* xsPreparation() { return (txPreparation*)&gxPreparation; }\n\n");
		
			if (stripping || linker->firstStrip)
				fxStripDefaults(linker, file);
			else
				fprintf(file, "#include \"xsDefaults.c\"\n\n");
		
			fclose(file);
			file = NULL;
		
		}
	}
	else {
		if (linker->error != C_EINVAL) {
		#if mxWindows
			char buffer[2048];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, linker->error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
			fprintf(stderr, "### %s\n", buffer);
		#else
			fprintf(stderr, "### %s\n", strerror(linker->error));
		#endif
		}
	}
	if (file)
		fclose(file);
// 	if (the)
// 		xsDeleteMachine(the);
	fxTerminateLinker(linker);
	return linker->error;
}

/* KEYS */

void fxBuildKeys(txMachine* the)
{
	txLinker* linker = (txLinker*)(the->context);
	txID c = linker->symbolIndex, i;
	for (i = 0; i < XS_SYMBOL_ID_COUNT; i++) {
		txLinkerSymbol* symbol = linker->symbolArray[i];
		txID id = the->keyIndex;
		txSlot* description = fxNewSlot(the);
		fxCopyStringC(the, description, symbol->string);
		the->keyArray[id] = description;
		the->keyIndex++;
	}
	for (; i < c; i++) {
		txLinkerSymbol* symbol = linker->symbolArray[i];
		fxNewNameX(the, symbol->string);
	}
}

/* MODULES */

txID fxFindModule(txMachine* the, txID moduleID, txSlot* slot)
{
	txLinker* linker = (txLinker*)(the->context);
	char name[C_PATH_MAX];
	char path[C_PATH_MAX];
	txBoolean absolute = 0, relative = 0, search = 0;
	txInteger dot = 0;
	txSlot *key;
	txString slash;
	txID id;
		
	fxToStringBuffer(the, slot, name, sizeof(name));
	if (!c_strncmp(name, "/", 1)) {
		absolute = 1;
	}	
	else if (!c_strncmp(name, "./", 2)) {
		dot = 1;
		relative = 1;
	}	
	else if (!c_strncmp(name, "../", 3)) {
		dot = 2;
		relative = 1;
	}
	else {
		relative = 1;
		search = 1;
	}
#if mxWindows
	{
		char c;
		slash = name;
		while ((c = *slash)) {
			if (c == '/')
				*slash = '\\';
			slash++;
		}
	}
#endif
	slash = c_strrchr(name, mxSeparator);
	if (!slash)
		slash = name;
	slash = c_strrchr(slash, '.');
	if (!slash)
		c_strcat(name, ".xsb");
	if (absolute) {
		c_strcpy(path, linker->base);
		c_strcat(path, name + 1);
		if (fxFindScript(the, path, &id))
			return id;
	}
	if (relative && (moduleID != XS_NO_ID)) {
		key = fxGetKey(the, moduleID);
		c_strcpy(path, key->value.key.string);
		slash = c_strrchr(path, mxSeparator);
		if (!slash)
			return XS_NO_ID;
		if (dot == 0)
			slash++;
		else if (dot == 2) {
			*slash = 0;
			slash = c_strrchr(path, mxSeparator);
			if (!slash)
				return XS_NO_ID;
		}
		if (!c_strncmp(path, linker->base, linker->baseLength)) {
			*slash = 0;
			c_strcat(path, name + dot);
			if (fxFindScript(the, path, &id))
				return id;
		}
	}
	if (search) {
		c_strcpy(path, linker->base);
		c_strcat(path, name);
		if (fxFindScript(the, path, &id))
			return id;
	}
	return XS_NO_ID;
}

txBoolean fxFindScript(txMachine* the, txString path, txID* id)
{
	txLinker* linker = (txLinker*)(the->context);
	txLinkerScript* linkerScript = linker->firstScript;
	path += linker->baseLength;
	while (linkerScript) {
		if (!c_strcmp(path, linkerScript->path)) {
			path -= linker->baseLength;
			*id = fxNewNameC(the, path);
			return 1;
		}
		linkerScript = linkerScript->nextScript;
	}
	*id = XS_NO_ID;
	return 0;
}

void fxLoadModule(txMachine* the, txID moduleID)
{
	txSlot* key = fxGetKey(the, moduleID);
 	char buffer[C_PATH_MAX];
 	txString path = buffer;
 	c_strcpy(path, key->value.key.string);
 	txScript* script = fxLoadScript(the, path);
	fxResolveModule(the, moduleID, script, C_NULL, C_NULL);
}

txScript* fxLoadScript(txMachine* the, txString path)
{
	txLinker* linker = (txLinker*)(the->context);
	txLinkerScript* linkerScript = linker->firstScript;
	path += linker->baseLength;
	while (linkerScript) {
		if (!c_strcmp(path, linkerScript->path)) {
			txScript* script = fxNewLinkerChunk(linker, sizeof(txScript));
			script->callback = fxLinkerScriptCallback;
			script->symbolsBuffer = NULL;
			script->symbolsSize = 0;
			script->codeBuffer = linkerScript->codeBuffer;
			script->codeSize = linkerScript->codeSize;
			script->hostsBuffer = NULL;
			script->hostsSize = 0;
			script->path = linkerScript->path;
			script->version[0] = XS_MAJOR_VERSION;
			script->version[1] = XS_MINOR_VERSION;
			script->version[2] = XS_PATCH_VERSION;
			script->version[3] = 0;
			return script;
		}
		linkerScript = linkerScript->nextScript;
	}
	return C_NULL;
}

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
	return C_NULL;
}


