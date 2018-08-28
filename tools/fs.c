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

static void fs_destructor(void* data);

void fs_closeSync(xsMachine* the)
{
	FILE* file = xsGetHostData(xsArg(0));
	if (file) {
		fclose(file);
		xsSetHostData(xsArg(0), NULL);
	}
}

void fs_copyFileSync(xsMachine* the)
{
	char *fromPath = xsToString(xsArg(0));
	char *toPath = xsToString(xsArg(1));
	FILE* fromFile = NULL;
	FILE* toFile = NULL;
	char buffer[1024];
	size_t count;
	
	xsTry {
		fromFile = fopen(fromPath, "r");
		xsElseThrow(fromFile != NULL);
		toFile = fopen(toPath, "w");
		xsElseThrow(toFile != NULL);
		while ((count = fread(buffer, 1, sizeof(buffer), fromFile)) > 0) {
			xsElseThrow(fwrite(buffer, 1, count, toFile) == count);
		}
		fclose(toFile);
		fclose(fromFile);
	}
	xsCatch {
		if (toFile)
			fclose(toFile);
		if (fromFile)
			fclose(fromFile);
		xsThrow(xsException);
	}
}

void fs_destructor(void* data)
{
	if (data)
		fclose(data);
}

void fs_deleteDirectory(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
#if mxWindows
	xsElseThrow(_rmdir(path) == 0);
#else
	xsElseThrow(rmdir(path) == 0);
#endif
}

void fs_deleteFile(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
#if mxWindows
	xsElseThrow(_unlink(path) == 0);
#else
	xsElseThrow(unlink(path) == 0);
#endif
}

void fs_existsSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
#if mxWindows
	DWORD attributes = GetFileAttributes(path);
	if (attributes != 0xFFFFFFFF) {
		if (attributes & FILE_ATTRIBUTE_DIRECTORY)
			xsResult = xsInteger(-1);
		else 
			xsResult = xsInteger(1);
	}
	else
		xsResult = xsInteger(0);
#else
	struct stat a_stat;
	if (stat(path, &a_stat) == 0) {
		if (S_ISDIR(a_stat.st_mode))
			xsResult = xsInteger(-1);
		else 
			xsResult = xsInteger(1);
	}
	else
		xsResult = xsInteger(0);
#endif
}

void fs_mkdirSync(xsMachine* the)
{
	char* path = xsToString(xsArg(0));
	int result;
#if mxWindows
	result = _mkdir(path);
#else
	result = mkdir(path, 0755);
#endif
	if (result) {
		switch (errno) {
			case EEXIST:
				break;
			default:
				xsElseThrow(NULL);
				break;
		}
	}
	xsResult = xsArg(0);
}

void fs_openSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char *flags = xsToString(xsArg(1));
	FILE* file = NULL;
	xsTry {
		file = fopen(path, flags);
		xsElseThrow(file);
		xsResult = xsNewHostObject(fs_destructor);
		xsSetHostData(xsResult, file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_readDirSync(xsMachine* the)
{
#if mxWindows
	xsStringValue path, name = NULL;
	UINT32 length, index;
	UINT16 *pathW = NULL;
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW findData;

	xsTry {
		xsVars(1);
		xsResult = xsNewArray(0);
	
		path = xsToString(xsArg(0));
		length = strlen(path);
		pathW = malloc((length + 3) * 2);
		xsElseThrow(pathW);
		MultiByteToWideChar(CP_UTF8, 0, path, length + 1, pathW, length + 1);
		for (index = 0; index < length; index++) {
			if (pathW[index] == '/')
				pathW[index] = '\\';
		}
		pathW[length] = '\\';
		pathW[length + 1] = '*';
		pathW[length + 2] = 0;
		findHandle = FindFirstFileW(pathW, &findData);
		if (findHandle != INVALID_HANDLE_VALUE) {
			do {
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ||
					!wcscmp(findData.cFileName, L".") ||
					!wcscmp(findData.cFileName, L".."))
					continue;
				length = wcslen(findData.cFileName);
				name = malloc((length + 1) * 2);
				xsElseThrow(name);
				WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, length + 1, name, length + 1, NULL, NULL);
				xsVar(0) = xsString(name);
				xsCall1(xsResult, xsID_push, xsVar(0));
				free(name);
				name = NULL;
			} while (FindNextFileW(findHandle, &findData));
		}
	}
	xsCatch {
	}
	if (name)
		free(name);
	if (findHandle != INVALID_HANDLE_VALUE)
		FindClose(findHandle);
	if (pathW)
		free(pathW);
#else
    DIR* dir;
	char path[1024];
	int length;

	xsVars(1);
	xsResult = xsNewArray(0);
	dir = opendir(xsToStringBuffer(xsArg(0), path, sizeof(path) - 1));
	length = strlen(path);
	path[length] = '/';
	length++;
	if (dir) {
		struct dirent *ent;
		while ((ent = readdir(dir))) {
			struct stat a_stat;
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
				continue;
			strcpy(path + length, ent->d_name);
			if (!stat(path, &a_stat)) {
				xsVar(0) = xsString(ent->d_name);
				(void)xsCall1(xsResult, xsID_push, xsVar(0));
			}
		}
		closedir(dir);
	}
#endif
}

void fs_readFileSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	size_t size;
	xsTry {
		file = fopen(path, "r");
		xsElseThrow(file);
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		xsResult = xsStringBuffer(NULL, size);
		fread(xsToString(xsResult), 1, size, file);	
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_readFileBufferSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	size_t size;
	xsTry {
		file = fopen(path, "rb");
		xsElseThrow(file);
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		xsResult = xsArrayBuffer(NULL, size);
		fread(xsToArrayBuffer(xsResult), 1, size, file);	
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_writeFileSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char* buffer = xsToString(xsArg(1));
	size_t size = strlen(buffer);
	FILE* file = NULL;
	xsTry {
		file = fopen(path, "w");
		xsElseThrow(file);
		fwrite(buffer, 1, size, file);		
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_writeFileBufferSync(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char* buffer = xsToArrayBuffer(xsArg(1));
	size_t size = xsGetArrayBufferLength(xsArg(1));
	FILE* file = NULL;
	xsTry {
		file = fopen(path, "wb");
		xsElseThrow(file);
		fwrite(buffer, 1, size, file);		
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void fs_writeSync(xsMachine* the)
{
	FILE* file = xsGetHostData(xsArg(0));
	char* buffer = xsToString(xsArg(1));
	size_t size = strlen(buffer);
	size = fwrite(buffer, 1, size, file);		
	xsResult = xsInteger(size);
}

void fs_writeBufferSync(xsMachine* the)
{
	FILE* file = xsGetHostData(xsArg(0));
	char* buffer = xsToArrayBuffer(xsArg(1));
	size_t size = xsGetArrayBufferLength(xsArg(1));
	size = fwrite(buffer, 1, size, file);		
	xsResult = xsInteger(size);
}

void fs_writeByteSync(xsMachine* the)
{
	FILE* file = xsGetHostData(xsArg(0));
	unsigned char value = (unsigned char)xsToInteger(xsArg(1));
	size_t size = fwrite(&value, 1, 1, file);		
	xsResult = xsInteger(size);
}

void fs_dumpSync(xsMachine* the)
{
	FILE* output = xsGetHostData(xsArg(0));
	char *path = xsToString(xsArg(1));
	FILE* file = NULL;
	size_t c, i;
	unsigned char byte;
	xsTry {
		file = fopen(path, "rb");
		xsElseThrow(file);
		fseek(file, 0, SEEK_END);
		c = ftell(file);
		fseek(file, 0, SEEK_SET);
		i = 0;
		fprintf(output, "\t");
		for (;;) {
			byte = fgetc(file);
			fprintf(output, "0x%02x", byte);
			i++;
			if (i == c)
				break;
			if (i % 16)
				fprintf(output, ", ");
			else
				fprintf(output, ",\n\t");
		}
		fprintf(output, "\n");
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

