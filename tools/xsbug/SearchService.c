/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "piuCode.h"
extern xsBooleanValue fxCompileRegExp(void* the, xsStringValue pattern, xsStringValue modifier, void** code, void** data, xsUnsignedValue* flags, xsStringValue messageBuffer, xsIntegerValue messageSize);
extern void fxDeleteRegExp(void* the, void* code, void* data);
extern xsIntegerValue fxMatchRegExp(void* the, void* code, void* data, xsUnsignedValue flags, xsStringValue subject, xsIntegerValue offset, xsIntegerValue** offsets, xsIntegerValue* limit);

#if mxLinux
#include <fcntl.h>
#include <gtk/gtk.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#if mxMacOSX
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#if mxWindows
#include "windows.h"
#endif

enum {
	XS_MAP = 0,
	XS_DIRECTORY,
	XS_TITLE,
	XS_FILE,
	XS_NAME,
	XS_PATH,
	XS_RESULT,
	XS_RESULTS,
	XS_URL,
	XS_COUNT
};

typedef struct {
// 	KprMessage message;
	void* code;
	void* data;
	xsUnsignedValue flags;
	char* name;
	int total;
#if mxLinux
	char path[PATH_MAX];
#endif
#if mxWindows
	wchar_t path[4096];
#endif
} KprCodeSearchRecord, *KprCodeSearch;

static void PiuCodeSearchDirectory(xsMachine* the, KprCodeSearch self);
static void PiuCodeSearchData(xsMachine* the, KprCodeSearch self, xsStringValue data, xsIntegerValue size);

void PiuCode_search(xsMachine* the)
{
	KprCodeSearchRecord searchRecord;
	KprCodeSearch self = &searchRecord;
	xsStringValue pattern = xsToString(xsArg(0));
	xsBooleanValue caseless = xsToBoolean(xsArg(1));
	xsStringValue modifier = (caseless) ? "iu" : "u";
	int c, i; 
	xsVars(XS_COUNT);
	xsTry {
		c_memset(self, 0, sizeof(KprCodeSearchRecord));
		xsResult = xsNewArray(0);
		xsVar(XS_MAP) = xsNew0(xsGlobal, xsID_Map);
		xsSet(xsGlobal, xsID("search/results"), xsVar(XS_MAP));
		if (!fxCompileRegExp(NULL, pattern, modifier, &self->code, &self->data, &self->flags, NULL, 0))
			xsUnknownError("pcre_compile error");
		c = xsToInteger(xsGet(xsArg(2), xsID_length));
		for (i = 0; i < c; i++) {
	// 		if (!KprMessageContinue(self->message))
	// 			break;
			xsVar(XS_DIRECTORY) = xsGet(xsArg(2), i);
			xsVar(XS_PATH) = xsGet(xsVar(XS_DIRECTORY), xsID_path);
			xsVar(XS_TITLE) = xsGet(xsVar(XS_DIRECTORY), xsID_name);
		#if mxLinux
			xsToStringBuffer(xsVar(XS_PATH), self->path, sizeof(self->path));
		#endif
		#if mxWindows
			xsToStringBufferW(xsVar(XS_PATH), self->path, sizeof(self->path));
		#endif
			PiuCodeSearchDirectory(the, self);
		}
		if (self->total > 100) {
			c = xsToInteger(xsGet(xsResult, xsID_length));
			for (i = 0; i < c; i++) {
	// 			if (!KprMessageContinue(self->message))
	// 				break;
				xsVar(XS_FILE) = xsGet(xsResult, i);
				xsSet(xsVar(XS_FILE), xsID_expanded, xsFalse);
				xsSet(xsVar(XS_FILE), xsID_items, xsNull);
			}
		}
	}
	xsCatch {
	}
	fxDeleteRegExp(NULL, self->code, self->data);
}

void PiuCode_searchResults(xsMachine* the)
{
	xsVars(1);
	xsVar(0) = xsGet(xsGlobal, xsID("search/results"));
	if (xsTest(xsVar(0)) && xsTest(xsArg(0)))
		xsResult = xsCall1(xsVar(0), xsID_get, xsArg(0));
}

void PiuCodeSearchDirectory(xsMachine* the, KprCodeSearch self)
{
#if mxLinux
	char* path = self->path;
	char* former = path + strlen(path);
	GDir* iterator = g_dir_open(path, 0, NULL);
	if (iterator) {
		char* name;
		*former = '/';
		while ((name = (xsStringValue)g_dir_read_name(iterator))) {
			struct stat _stat;
			strcpy(former + 1, name);
			if (stat(path, &_stat) == 0) {
				if (S_ISDIR(_stat.st_mode)) {
					PiuCodeSearchDirectory(the, self);
				}
				else {
					char* dot = strrchr(name, '.');
					if (dot && !strcmp(dot, ".js")) {
						int fd = open(path, O_RDONLY);
						if (fd >= 0) {
							xsStringValue string = mmap(NULL, _stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
							if (string != MAP_FAILED) {
								xsVar(XS_FILE) = xsUndefined;
								xsVar(XS_NAME) = xsString(name);
								xsVar(XS_PATH) = xsString(path);
 								PiuCodeSearchData(the, self, string, _stat.st_size);
								munmap(string, _stat.st_size);
							}
							close(fd);
						}
					}
				}
			}
		}
		*former = 0;
	}
#endif
#if mxMacOSX
	CFStringRef basePath = CFStringCreateWithCString(NULL, xsToString(xsVar(XS_PATH)), kCFStringEncodingUTF8);
	CFURLRef baseURL = CFURLCreateWithFileSystemPath(NULL, basePath, kCFURLPOSIXPathStyle, true);
	CFURLEnumeratorRef enumerator = CFURLEnumeratorCreateForDirectoryURL(NULL, baseURL, kCFURLEnumeratorDescendRecursively | kCFURLEnumeratorSkipInvisibles, NULL);
	CFURLRef fileURL;
	while (CFURLEnumeratorGetNextURL(enumerator, &fileURL, NULL) == kCFURLEnumeratorSuccess) {
		CFStringRef path = CFURLCopyFileSystemPath(fileURL, kCFURLPOSIXPathStyle);
		CFStringRef name = CFURLCopyLastPathComponent(fileURL);
		if (CFStringHasSuffix(name, CFSTR(".js"))) {
			int fd = open(CFStringGetCStringPtr(path, kCFStringEncodingUTF8), O_RDONLY);
			if (fd >= 0) {
				struct stat statbuf;
				xsIntegerValue size;
				fstat(fd, &statbuf);
				size = statbuf.st_size;
				if (size > 0) {
					xsStringValue string = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
					if (string != MAP_FAILED) {
						xsVar(XS_FILE) = xsUndefined;
						xsVar(XS_NAME) = xsString(CFStringGetCStringPtr(name, kCFStringEncodingUTF8));
						xsVar(XS_PATH) = xsString(CFStringGetCStringPtr(path, kCFStringEncodingUTF8));
 						PiuCodeSearchData(the, self, string, size);
						munmap(string, size);
					}
				}
				close(fd);
			}
		}
		CFRelease(name);
		CFRelease(path);
	}
	CFRelease(enumerator);
	CFRelease(baseURL);
	CFRelease(basePath);
#endif
#if mxWindows
	wchar_t* path = self->path;
	wchar_t* former = path + wcslen(path);
	WIN32_FIND_DATAW data;
	wcscpy(former, L"\\*");
	HANDLE iterator = FindFirstFileW(path, &data);
	if (iterator != INVALID_HANDLE_VALUE) {
		BOOL result = TRUE;
		wchar_t* current = former + 1;
		while (result) {
			if (((data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0) && wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..")) {
				wcscpy(current, data.cFileName);
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					PiuCodeSearchDirectory(the, self);
				}
				else {
					wchar_t* dot = wcsrchr(data.cFileName, L'.');
					if (dot && (!wcscmp(dot, L".js"))) {
						HANDLE file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (file != INVALID_HANDLE_VALUE) {
							xsIntegerValue size = GetFileSize(file, NULL);
							if (size != INVALID_FILE_SIZE) {
								HANDLE mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, (SIZE_T)size, NULL);
								if (mapping != INVALID_HANDLE_VALUE) {
									xsStringValue string = (xsStringValue)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, (SIZE_T)size);
									if (string != NULL) {
										xsVar(XS_FILE) = xsUndefined;
										xsVar(XS_NAME) = xsStringW(data.cFileName);
										xsVar(XS_PATH) = xsStringW(path);
										PiuCodeSearchData(the, self, string, size);
										UnmapViewOfFile(string);
									}
									CloseHandle(mapping);
								}
							}
							CloseHandle(file);
						}
					}
				}
				*current = 0;
			}
			result = FindNextFileW(iterator, &data);
		}
		FindClose(iterator);
	}
	*former = 0;
#endif
}

void PiuCodeSearchData(xsMachine* the, KprCodeSearch self, xsStringValue data, xsIntegerValue size)
{
	int offset, count, total = 0;
	char *fromFile, *toFile, *fromLine, *toLine, *from, *to;
	xsIntegerValue* offsets;
	
	offset = 0;
	fromFile = data;
	toFile = data + size;
	for (;;) {
// 		if (!KprMessageContinue(self->message))
// 			break;
		count = fxMatchRegExp(NULL, self->code, self->data, self->flags, fromFile, offset, &offsets, NULL);
		if (count <= 0) {
			break;
		}
		from = fromLine = fromFile + offsets[0];
		while (fromLine >= fromFile) {
			char c = *fromLine;
			if ((c == 10) || (c == 13))
				break;
			fromLine--;
		}
		fromLine++;
		while (fromLine < from) {
			char c = *fromLine;
			if ((c != 9) && (c != 32))
				break;
			fromLine++;
		}
		to = toLine = fromFile + offsets[1];
		while (toLine < toFile) {
			char c = *toLine;
			if ((c == 10) || (c == 13))
				break;
			toLine++;
		}
		toLine--;
		while (toLine > to) {
			char c = *toLine;
			if ((c != 9) && (c != 32))
				break;
			toLine--;
		}
		toLine++;
		offset = offsets[1];
		if (!xsTest(xsVar(XS_FILE))) {
			xsVar(XS_RESULTS) = xsNewArray(0);
			xsVar(XS_FILE) = xsNewObject();
			xsDefine(xsVar(XS_FILE), xsID_name, xsVar(XS_NAME), xsDefault);
			xsDefine(xsVar(XS_FILE), xsID_path, xsVar(XS_PATH), xsDefault);
			xsDefine(xsVar(XS_FILE), xsID_title, xsVar(XS_TITLE), xsDefault);
			xsDefine(xsVar(XS_FILE), xsID_items, xsVar(XS_RESULTS), xsDefault);
			xsDefine(xsVar(XS_FILE), xsID_expanded, xsTrue, xsDefault);
			(void)xsCall1(xsResult, xsID_push, xsVar(XS_FILE));
			(void)xsCall2(xsVar(XS_MAP), xsID_set, xsVar(XS_PATH), xsVar(XS_RESULTS));
		}
		xsVar(XS_RESULT) = xsNewObject();
		xsDefine(xsVar(XS_RESULT), xsID_string, xsStringBuffer(fromLine, toLine - fromLine), xsDefault);
		xsDefine(xsVar(XS_RESULT), xsID_offset, xsInteger(fxUTF8ToUnicodeOffset(fromFile, offsets[0])), xsDefault);
		xsDefine(xsVar(XS_RESULT), xsID_length, xsInteger(fxUTF8ToUnicodeOffset(from, to - from)), xsDefault);
		xsDefine(xsVar(XS_RESULT), xsID_delta, xsInteger(fxUTF8ToUnicodeOffset(fromLine, from - fromLine)), xsDefault);
		(void)xsCall1(xsVar(XS_RESULTS), xsID_push, xsVar(XS_RESULT));
		total++;
	}
	if (xsTest(xsVar(XS_FILE))) {
		xsDefine(xsVar(XS_FILE), xsID_count, xsInteger(total), xsDefault);
		self->total += 1 + total;
	}
}
