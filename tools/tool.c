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
						return;
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
				return;
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
	#elif mxMacOSX
		xsResult = xsString("mac");
	#elif mxLinux
		xsResult = xsString("lin");
	#else
		#error("need a platform")
	#endif
}

void Tool_prototype_execute(xsMachine* the)
{
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
}

void Tool_prototype_getenv(xsMachine* the)
{
	xsStringValue result = getenv(xsToString(xsArg(0)));
	if (result)
		xsResult = xsString(result);
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
