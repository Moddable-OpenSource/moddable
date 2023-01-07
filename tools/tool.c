/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

#include "xsPlatform.h"
#include "xs.h"
#include "mc.xs.h"

#if mxWindows
	#include <direct.h>
	#include <errno.h>
	#include <iphlpapi.h>
	#include <process.h>
	#define mxSeparator '\\'
	#define PATH_MAX 1024
#else
	#include <dirent.h>
	#include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <ifaddrs.h>
    #include <netdb.h>
	#include <unistd.h>
	#include <spawn.h>
	#define mxSeparator '/'
#endif

#ifdef mxDebug
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,(char *)__FILE__,__LINE__,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#else
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,NULL,0,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#endif

static char** then = NULL;

void fxAbort(xsMachine* the, int status)
{
	xsStringValue why = C_NULL;
	switch (status) {
	case xsStackOverflowExit:
		why = "stack overflow";
		break;
	case xsNotEnoughMemoryExit:
		why = "memory full";
		break;
	case xsNoMoreKeysExit:
		why = "not enough keys";
		break;
	case xsDeadStripExit:
		why = "dead strip";
		break;
	case xsDebuggerExit:
		break;
	case xsFatalCheckExit:
		break;
	case xsUnhandledExceptionExit:
		why = "unhandled exception";
		break;
	case xsUnhandledRejectionExit:
		why = "unhandled rejection";
		break;
	case xsTooMuchComputationExit:
		why = "too much computation";
		break;
	default:
		why = "unknown";
		break;
	}
	if (why)
		fprintf(stderr, "Error: %s\n", why);
	exit(status);
}

extern int mainXSA(int argc, char* argv[]) ;
extern int mainXSC(int argc, char* argv[]) ;

int main(int argc, char* argv[]) 
{
	int error = 0;
	if (!strcmp(argv[1], "cp")) {
#ifdef XSTOOLS
		char buffer[1024];
		FILE* src = NULL;
		FILE* dst = NULL;
		size_t srcSize;
		size_t dstSize;
		
		src = fopen(argv[2], "rb");
		if (!src) { error = errno; goto END; };
		dst = fopen(argv[3], "wb");
		if (!dst) { error = errno; goto END; };
		for (;;) {
			srcSize = fread(buffer, 1, sizeof(buffer), src);
			if (!srcSize)
				break;
			dstSize = fwrite(buffer, 1, srcSize, dst);
			if (srcSize != dstSize) { error = errno; goto END; };
		}
	END:
		if (dst)
			fclose(dst);
		if (src)
			fclose(src);
#endif
	}
	else if (!strcmp(argv[1], "xsa")) {
#ifdef XSTOOLS
		error = mainXSA(argc - 1, &argv[1]);
#endif
	}
	else if (!strcmp(argv[1], "xsc")) {
#ifdef XSTOOLS
		error = mainXSC(argc - 1, &argv[1]);
#endif
	}
	else {
		xsMachine* machine = fxPrepareMachine(NULL, xsPreparation(), "tool", NULL, NULL);
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
						xsVar(1) = xsAwaitImport(argv[1], XS_IMPORT_DEFAULT);
						fxPush(xsVar(1));
						fxNew(the);
						fxPush(xsVar(0));
						fxRunCount(the, 1);
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
	}
	return error;
}

void FILE_prototype_constructor(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	xsTry {
		char *flags;
		if ((c > 1) && xsTest(xsArg(1)))
			flags = xsToString(xsArg(1));
		else
			flags = "w";
		file = fopen(path, flags);
		xsElseThrow(file);
		xsSetHostData(xsThis, file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void FILE_prototype_destructor(void* data)
{
	if (data)
		fclose(data);
}

void FILE_prototype_close(xsMachine* the)
{
	FILE* file = xsGetHostData(xsThis);
	if (file) {
		fclose(file);
		xsSetHostData(xsThis, NULL);
	}
}

void FILE_prototype_dump(xsMachine* the)
{
	FILE* output = xsGetHostData(xsThis);
	char *path = xsToString(xsArg(0));
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
		if (c) {
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

void FILE_prototype_write(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	FILE* file = xsGetHostData(xsThis);
	size_t total = 0;
	for (i = 0; i < c; i++) {
		char* buffer = xsToString(xsArg(0));
		size_t size = strlen(buffer);
		total += fwrite(buffer, 1, size, file);		
	}
	xsResult = xsInteger(total);
}

void FILE_prototype_writeBuffer(xsMachine* the)
{
	FILE* file = xsGetHostData(xsThis);
	char* buffer = xsToArrayBuffer(xsArg(0));
	size_t size = xsGetArrayBufferLength(xsArg(0));
	size = fwrite(buffer, 1, size, file);		
	xsResult = xsInteger(size);
}

void FILE_prototype_writeByte(xsMachine* the)
{
	FILE* file = xsGetHostData(xsThis);
	unsigned char value = (unsigned char)xsToInteger(xsArg(0));
	size_t size = fwrite(&value, 1, 1, file);		
	xsResult = xsInteger(size);
}

void FILE_prototype_writeString(xsMachine* the)
{
	FILE* file = xsGetHostData(xsThis);
	char* buffer = xsToString(xsArg(0));
	size_t size = strlen(buffer);
	size = fwrite(buffer, 1, size, file);		
	xsResult = xsInteger(size);
}

void Tool_prototype_get_ipAddress(xsMachine* the)
{
#if mxWindows
	#define WORKING_BUFFER_SIZE 15000
	#define MAX_TRIES 3
    DWORD dwRetVal = 0;
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG iterations = 0;
    unsigned int i = 0;
    outBufLen = WORKING_BUFFER_SIZE;
    do {
        pAddresses = (IP_ADAPTER_ADDRESSES *)c_malloc(outBufLen);
        if (pAddresses == NULL)
        	goto bail;
        dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        	c_free(pAddresses);
       	 	pAddresses = NULL;
       	}
        iterations++;
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iterations < MAX_TRIES));
    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL) {
                for (i = 0; pUnicast != NULL; i++) {
 					struct sockaddr_in* address = (struct sockaddr_in*)pUnicast->Address.lpSockaddr;
					int ip = ntohl(address->sin_addr.s_addr);
					char buffer[22];
					snprintf(buffer, 22, "%u.%u.%u.%u", (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16, (ip & 0x0000ff00) >> 8, (ip & 0x000000ff));
					if (c_strcmp(buffer, "127.0.0.1")) {
						xsResult = xsString(buffer);
						break;
					}
					pUnicast = pUnicast->Next;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }
bail:
    if (pAddresses) {
        c_free(pAddresses);
    }
#else
	struct ifaddrs *ifaddr, *ifa;
	if (getifaddrs(&ifaddr) == -1)
		return;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in* address = (struct sockaddr_in*)ifa->ifa_addr;
			int ip = ntohl(address->sin_addr.s_addr);
			char buffer[22];
			snprintf(buffer, 22, "%u.%u.%u.%u", (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16, (ip & 0x0000ff00) >> 8, (ip & 0x000000ff));
			if (c_strcmp(buffer, "127.0.0.1")) {
				xsResult = xsString(buffer);
				break;
			}
		}
	}
	freeifaddrs(ifaddr);
#endif
}

void Tool_prototype_get_currentDirectory(xsMachine* the)
{
	char buffer[PATH_MAX];
#if mxWindows
	xsElseThrow(_getcwd(buffer, PATH_MAX));
#else
	xsElseThrow(getcwd(buffer, PATH_MAX));
#endif
	xsResult = xsString(buffer);
}

void Tool_prototype_set_currentDirectory(xsMachine* the)
{
#if mxWindows
	xsElseThrow(0 == _chdir(xsToString(xsArg(0))));
#else
	xsElseThrow(0 == chdir(xsToString(xsArg(0))));
#endif
}


void Tool_prototype_get_currentPlatform(xsMachine* the)
{
	#if mxWindows
		xsResult = xsString("win");
	#elif mxWasm
		xsResult = xsString("wasm");
	#elif mxMacOSX
		xsResult = xsString("mac");
	#elif mxLinux
		xsResult = xsString("lin");
	#else
		#error("need a platform")
	#endif
}

void Tool_prototype_createDirectory(xsMachine* the)
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


void Tool_prototype_deleteDirectory(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
#if mxWindows
	xsElseThrow(_rmdir(path) == 0);
#else
	xsElseThrow(rmdir(path) == 0);
#endif
}

void Tool_prototype_deleteFile(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
#if mxWindows
	xsElseThrow(_unlink(path) == 0);
#else
	xsElseThrow(unlink(path) == 0);
#endif
}

void Tool_prototype_isDirectoryOrFile(xsMachine* the)
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

void Tool_prototype_enumerateDirectory(xsMachine* the)
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
    xsCall0(xsResult, xsID_sort);
}

void Tool_prototype_execute(xsMachine* the)
{
#if mxWasm
	xsElseThrow(NULL);
#else
	FILE* pipe;
	char buffer[PATH_MAX];
	xsStringValue command = xsToString(xsArg(0));
#if mxWindows
	fflush(NULL);
	pipe = _popen(command, "r");
#else
	pipe = popen(command, "r");
#endif
    xsResult = xsString("");
	if (pipe) {
        xsIntegerValue size;
        for (;;) {
         	size = fread(buffer, sizeof(char), PATH_MAX - 1, pipe);
         	if (size <= 0)
         		break;
        	buffer[size] = 0;
        	xsResult = xsCall1(xsResult, xsID("concat"), xsString(buffer));
        }
#if mxWindows
		_pclose(pipe);
#else
		pclose(pipe);
#endif
	}
#endif
}

void Tool_prototype_getenv(xsMachine* the)
{
	xsStringValue result = getenv(xsToString(xsArg(0)));
	if (result)
		xsResult = xsString(result);
}

void Tool_prototype_getFileSize(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	size_t size;
	xsTry {
		file = fopen(path, "rb");
		xsElseThrow(file);
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fclose(file);
		xsResult = xsInteger(size);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void Tool_prototype_getToolsVersion(xsMachine *the)
{
#ifdef kModdableToolsVersion
	xsResult = xsString(kModdableToolsVersion);
#endif
}

void Tool_prototype_joinPath(xsMachine* the)
{
	char path[PATH_MAX];
	int length;
	strcpy(path, xsToString(xsGet(xsArg(0), xsID("directory"))));
	length = strlen(path);
	path[length] = mxSeparator;
	path[length + 1] = 0;
	strcat(path, xsToString(xsGet(xsArg(0), xsID("name"))));
	if (xsHas(xsArg(0), xsID("extension")))
		strcat(path, xsToString(xsGet(xsArg(0), xsID("extension"))));
	xsResult = xsString(path);
}

void Tool_prototype_readFileString(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	FILE* file = NULL;
	size_t size;
	xsStringValue string;
	xsTry {
		file = fopen(path, "r");
		xsElseThrow(file);
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		xsResult = xsStringBuffer(NULL, size);
		string = xsToString(xsResult);
		size = fread(string, 1, size, file);
		string[size] = 0;
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void Tool_prototype_readFileBuffer(xsMachine* the)
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
		size = fread(xsToArrayBuffer(xsResult), 1, size, file);	
		fclose(file);
	}
	xsCatch {
		if (file)
			fclose(file);
		xsThrow(xsException);
	}
}

void Tool_prototype_report(xsMachine* the)
{
	fprintf(stderr, "%s\n", xsToString(xsArg(0)));
}

void Tool_prototype_reportError(xsMachine* the)
{
	xsIntegerValue c;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		xsStringValue path = xsToString(xsArg(0));
		xsIntegerValue line = xsToInteger(xsArg(1));
	#if mxWindows
		fprintf(stderr, "%s(%d): error: ", path, line);
	#else
		fprintf(stderr, "%s:%d: error: ", path, line);
	#endif
	}
	else
		fprintf(stderr, "# error: ");
	fprintf(stderr, "%s!\n", xsToString(xsArg(2)));
	c = xsToInteger(xsGet(xsThis, xsID("errorCount")));
	xsSet(xsThis, xsID("errorCount"), xsInteger(c + 1));
}

void Tool_prototype_reportWarning(xsMachine* the)
{
	xsIntegerValue c;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		xsStringValue path = xsToString(xsArg(0));
		xsIntegerValue line = xsToInteger(xsArg(1));
	#if mxWindows
		fprintf(stderr, "%s(%d): warning: ", path, line);
	#else
		fprintf(stderr, "%s:%d: warning: ", path, line);
	#endif
	}
	else
		fprintf(stderr, "# warning: ");
	fprintf(stderr, "%s!\n", xsToString(xsArg(2)));
	c = xsToInteger(xsGet(xsThis, xsID("warningCount")));
	xsSet(xsThis, xsID("warningCount"), xsInteger(c + 1));
}

void Tool_prototype_resolveDirectoryPath(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char buffer[PATH_MAX];
#if mxWindows
	DWORD attributes;
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
			xsResult = xsString(buffer);
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISDIR(a_stat.st_mode)) {
				xsResult = xsString(buffer);
			}
		}
	}
#endif
}

void Tool_prototype_resolveFilePath(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char buffer[PATH_MAX];
#if mxWindows
	DWORD attributes;
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (!(attributes & FILE_ATTRIBUTE_DIRECTORY))) {
			xsResult = xsString(buffer);
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(path, &a_stat) == 0) {
			if (S_ISREG(a_stat.st_mode)) {
				xsResult = xsString(buffer);
			}
		}
	}
#endif
}

void Tool_prototype_resolvePath(xsMachine* the)
{
	char *path = xsToString(xsArg(0));
	char buffer[PATH_MAX];
#if mxWindows
	DWORD attributes;
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if (attributes != 0xFFFFFFFF) {
			xsResult = xsString(buffer);
		}
	}
#else
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(path, &a_stat) == 0) {
			xsResult = xsString(buffer);
		}
	}
#endif
}

void Tool_prototype_setenv(xsMachine* the)
{
	xsStringValue name = xsToString(xsArg(0));
	xsStringValue value = xsToString(xsArg(1));
	xsIntegerValue c = xsToInteger(xsArgc);
#if mxWindows
	char *buffer = c_malloc(c_strlen(name) + c_strlen(value) + 2);
	if (buffer) {
		c_strcpy(buffer, name);
		c_strcat(buffer, "=");
		c_strcat(buffer, value);
		_putenv(buffer);
		c_free(buffer);
	}
#else
	xsIntegerValue overwrite = 1;
	if (c > 2)
		overwrite = xsToInteger(xsArg(2));
	setenv(name, value, overwrite);
#endif
}

#if mxWindows
#else
extern char **environ;
#endif

void Tool_prototype_spawn(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	char **argv = NULL;
	xsTry {
	#if mxWindows
	#else
		pid_t pid;
	#endif
		int status;
		for (i = 0; i < c; i++)
			xsToString(xsArg(i));
		argv = malloc(sizeof(char *)*(c + 1));
		xsElseThrow(argv);
		for (i = 0; i < c; i++)
			argv[i] = xsToString(xsArg(i));
		argv[i] = C_NULL;
	#if mxWindows
		status = _spawnvp(_P_WAIT, argv[0], argv);
	#else
		status = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);
		xsElseThrow(status == 0);
		do {
			xsElseThrow(waitpid(pid, &status, 0) != -1);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		status = WEXITSTATUS(status);
	#endif
		free(argv);
		argv = NULL;
		xsResult = xsInteger(status);
	}
	xsCatch {
		if (argv)
			free(argv);
		xsThrow(xsException);
	}
}

void Tool_prototype_splitPath(xsMachine* the)
{
	char *path;
	char *slash = NULL;
	char *dot = NULL;
	int length;
	char directory[PATH_MAX];
	char name[PATH_MAX];
	char extension[PATH_MAX];
	
	path = xsToString(xsArg(0));
	slash = strrchr(path, mxSeparator);
	if (slash == NULL)
		slash = path;
	else
		slash++;
	dot = strrchr(slash, '.');
	if (dot == NULL)
		dot = slash + strlen(slash);
	length = slash - path;
	strncpy(directory, path, length);
	if (length)
		directory[length - 1] = 0;
	else
		directory[0] = 0;
	length = dot - slash;
	strncpy(name, slash, length);
	name[length] = 0;
	strcpy(extension, dot);
	xsResult = xsNewObject();
	xsSet(xsResult, xsID("directory"), xsString(directory));
	xsSet(xsResult, xsID("name"), xsString(name));
	xsSet(xsResult, xsID("extension"), xsString(extension));
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

void Tool_prototype_writeFileString(xsMachine* the)
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

void Tool_prototype_writeFileBuffer(xsMachine* the)
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

void Tool_prototype_fsvhash(xsMachine* the)
{
	unsigned int d = (unsigned int)xsToInteger(xsArg(0));
	unsigned char *s = (unsigned char *)xsToString(xsArg(1));
	unsigned int c;
	if (d == 0)
		d = 0x811c9dc5;
    while ((c = *s++)) {
		d *= 0x01000193;
		d ^= c;
    }
    xsResult = xsInteger(d & 0x7FFFFFFF);
}

void Tool_prototype_strlen(xsMachine* the)
{
	xsIntegerValue result = strlen(xsToString(xsArg(0)));
	xsResult = xsInteger(result);
}
