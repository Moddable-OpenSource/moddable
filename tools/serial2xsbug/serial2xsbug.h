/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#if mxLinux
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#elif mxMacOSX
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#elif mxWindows
#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <process.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#include <winsock2.h>
#define execvp(COMMAND, ARGV) _execvp(COMMAND, ARGV)
#define popen(COMMAND, MODE) _popen(COMMAND, MODE)
#define pclose(PIPE) _pclose(PIPE)
#define usleep(MICRO) Sleep(MICRO / 1000)
#else
#error unknown platform
#endif

#define mxBufferSize 32 * 1024
#define mxMachinesCount 10
#define mxNetworkBufferSize 1024
#define mxSerialBufferSize 1024
#define mxTagSize 17
#define mxTrace 0
#define mxTraceCommands 0

typedef struct txSerialMachineStruct txSerialMachineRecord, *txSerialMachine;
typedef struct txSerialToolStruct txSerialToolRecord, *txSerialTool;

struct txSerialMachineStruct {
#if mxLinux
	int networkConnection;
	char networkBuffer[mxNetworkBufferSize];
#elif mxMacOSX
	CFSocketRef networkSocket;
	CFRunLoopSourceRef networkSource;
#elif mxWindows
	SOCKET networkConnection;
	HANDLE networkEvent;
	WSAOVERLAPPED networkOverlapped;
	WSABUF networkBuf;
	char networkBuffer[mxNetworkBufferSize];
#endif
	txSerialTool tool;
	txSerialMachine nextMachine;
	uint32_t value;
	uint8_t suppress;
	int receiveCount;
	char tag[mxTagSize + 1];
};

struct txSerialToolStruct {
#if mxLinux
	jmp_buf _jmp_buf;
	char* file;
	int line;
	int error;
	int serialConnection;
	int reconnecting;
	struct termios serialConfiguration;
	char serialBuffer[mxSerialBufferSize];
	int count;
	struct pollfd fds[1 + mxMachinesCount];
#elif mxMacOSX
	IONotificationPortRef notificationPort;
	io_iterator_t ioIterator;
	CFSocketRef serialSocket;
	CFRunLoopSourceRef serialSource;
#elif mxWindows
	jmp_buf _jmp_buf;
	char* file;
	int line;
	DWORD error;
	HANDLE serialConnection;
	HANDLE serialEvent;
	int reconnecting;
	OVERLAPPED serialOverlapped;
	char serialBuffer[mxSerialBufferSize];
	HANDLE signalEvent;
	DWORD count;
	HANDLE events[2 + mxMachinesCount];
#endif
	char* path;
	int vendorID;
	int productID;
	int baud;
	int data;
	int parity;
	int stop;
	char* host;
	int port;
	int dtr;
	int programming;
	int restartOnConnect;
	int showPath;
	int timeout;
	txSerialMachine firstMachine;
	txSerialMachine currentMachine;
	int index;
	int state;
	char buffer[mxBufferSize + 1];
};

extern int fxArguments(txSerialTool self, int argc, char* argv[]);
extern void fxCloseNetwork(txSerialTool self, uint32_t value);
extern void fxCloseSerial(txSerialTool self);
extern txSerialMachine fxOpenNetwork(txSerialTool self, uint32_t value);
extern void fxOpenSerial(txSerialTool self);
extern void fxReadNetworkBuffer(txSerialMachine machine, char* buffer, int size);
extern void fxReadSerialBuffer(txSerialTool self, char* buffer, int size);
extern void fxRestart(txSerialTool self);
extern void fxRestartSerial(txSerialTool self);
extern void fxWriteNetwork(txSerialMachine machine, char* buffer, int size);
extern void fxWriteSerial(txSerialTool self, char* buffer, int size);

