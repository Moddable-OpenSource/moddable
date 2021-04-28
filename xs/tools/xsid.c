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

#include "xsCommon.h"

typedef struct sxLink txLink;
struct sxLink {
	txLink* next;
	int length;
	char string[1];
};

static txString fxRealDirectoryPath(txString path, txString buffer);
static txString fxRealFilePath(txString path, txString buffer);
static void fxReportError(txString theFormat, ...);

txString fxRealDirectoryPath(txString path, txString buffer)
{
#if mxWindows
	DWORD attributes;
	if (_fullpath(buffer, path, C_PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
  	 		strcat(buffer, "\\");
			return buffer;
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISDIR(a_stat.st_mode)) {
  	 			strcat(buffer, "/");
				return buffer;
			}
		}
	}
#endif
	return NULL;
}

txString fxRealFilePath(txString path, txString buffer)
{
#if mxWindows
	DWORD attributes;
	if (_fullpath(buffer, path, C_PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (!(attributes & FILE_ATTRIBUTE_DIRECTORY))) {
			return buffer;
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISREG(a_stat.st_mode)) {
				return buffer;
			}
		}
	}
#endif
	return NULL;
}

void fxReportError(txString theFormat, ...)
{
	c_va_list arguments;
	fprintf(stderr, "# ");
	c_va_start(arguments, theFormat);
	vfprintf(stderr, theFormat, arguments);
	c_va_end(arguments);
	fprintf(stderr, "\n");
	exit(1);
}

int main(int argc, char* argv[]) 
{
	int argi;
	char inputPath[C_PATH_MAX];
	char outputPath[C_PATH_MAX];

	char* input = NULL;
	char* output = NULL;
	char* rename = NULL;
	FILE* file = NULL;
	size_t size;
	char* buffer = NULL;
	txInteger* code;
	txInteger* data;
	char reason[256];
	int offset;
	int count;
	int total;
	txLink* list = NULL;
	txLink* link;
	txByte byte;
	txID id;
	txByte* p;

	for (argi = 1; argi < argc; argi++) {
		if (!strcmp(argv[argi], "-o")) {
			argi++;
			if (argi >= argc)
				fxReportError("no output directory");
			else if (output)
				fxReportError("too many output directories");
			else
				output = argv[argi];
		}
		else if (!strcmp(argv[argi], "-r")) {
			argi++;
			if (argi >= argc)
				fxReportError("no name");
			else if (rename)
				fxReportError("too many names");
			else
				rename = argv[argi];
		}
		else {
			if (input)
				fxReportError("too many files");
			else
				input = argv[argi];
		}
	}
	if (!input)
		fxReportError("no file");
		
	input = fxRealFilePath(input, inputPath);
	if (!input)
		fxReportError("file not found");
	file = fopen(input, "rb");
	if (!file)
		fxReportError("cannot open input file");
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	buffer = c_malloc(size + 1);
	if (!buffer)
		fxReportError("not enough memory");
	if (fread(buffer, 1, size, file) != size)
		fxReportError("file not read");
	buffer[size] = 0;
	fclose(file);
	file = NULL;

	if (!fxCompileRegExp(NULL, "xsID_[0-9A-Za-z_]+", "u", &code, &data, reason, sizeof(reason)))
		fxReportError("%s", reason);
		
	offset = 0;
	count = 1;
	total = sizeof(txID);
	while (fxMatchRegExp(NULL, code, data, buffer, offset) > 0) {
		char* from = buffer + data[0] + 5;
		char* to = buffer + data[1];
		int length = mxPtrDiff(to - from);
		char tmp = *to;
		txLink** address = &list;
		*to = 0;
		while ((link = *address)) {
			int order = c_strcmp(from, link->string);
			if (order == 0)
				break;
			if (order < 0) {
				link = NULL;
				break;
			}
			address = &(link->next);
		}
		if (!link) {
			count++;
			total += length + 1;
			link = c_malloc(sizeof(txLink) + length);
			if (!link)
				fxReportError("not enough memory");
			link->next = *address;
			link->length = length;
			c_memcpy(link->string, from, length + 1);
			*address = link;
		}
		*to = tmp;
		offset = data[1];
	}
	if (count >= 0x7FFF)
		fxReportError("not enough symbols");
	if (output)
		output = fxRealDirectoryPath(output, outputPath);
	else
		output = fxRealDirectoryPath(".", outputPath);
	if (!output)
		fxReportError("directory not found");
	if (rename)
		c_strcat(output, rename);
	else {
		rename = strrchr(input, mxSeparator);
		if (rename == NULL)
			rename = input;
		else
			rename++;
		strcat(output, rename);
		strcat(output, ".xsi");
	}
	file = fopen(output, "wb");
	if (!file)
		fxReportError("cannot open output file");
	size = 8 + 8 + 4 + 8 + total;
	size = htonl((u_long)size);
	fwrite(&size, 4, 1, file);
	fwrite("XS_I", 4, 1, file);
	size = 8 + 4;
	size = htonl((u_long)size);
	fwrite(&size, 4, 1, file);
	fwrite("VERS", 4, 1, file);
	byte = XS_MAJOR_VERSION;
	fwrite(&byte, 1, 1, file);
	byte = XS_MINOR_VERSION;
	fwrite(&byte, 1, 1, file);
	byte = XS_PATCH_VERSION;
	fwrite(&byte, 1, 1, file);
	byte = 0;
	fwrite(&byte, 1, 1, file);
	size = 8 + total;
	size = htonl((u_long)size);
	fwrite(&size, 4, 1, file);
	fwrite("SYMB", 4, 1, file);
	p = (txByte*)reason;
	id = (txID)count;
	mxEncodeID(p, id);
	fwrite(reason, sizeof(txID), 1, file);
	link = list;
	while (link) {
		fwrite(link->string, link->length + 1, 1, file);
		link = link->next;
	}
	fclose(file);

	return 0;
}
		
		
		
		
		
		
		
		
		
		
		
		
