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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define mxNetworkBufferSize 1024
#define mxSerialBufferSize 1024
#define mxBufferSize 32 * 1024

typedef struct txSerialMachineStruct txSerialMachineRecord, *txSerialMachine;
typedef struct txSerialToolStruct txSerialToolRecord, *txSerialTool;

struct txSerialMachineStruct {
	txSerialTool tool;
	int index;
	int networkConnection;
	char networkBuffer[mxNetworkBufferSize];
};

struct txSerialToolStruct {
	jmp_buf _jmp_buf;
	char* file;
	int line;
	int error;
	struct pollfd fds[11];
	
	char* path;
	int baud;
	int data;
	int parity;
	int stop;
	int serialConnection;
	struct termios serialConfiguration;
	char serialBuffer[mxSerialBufferSize];
	
	char* host;
	int port;
	txSerialMachine machines[10];
	txSerialMachine currentMachine;
	
	int index;
	int state;
	
	char buffer[mxBufferSize];
};

static void fxCloseNetwork(txSerialTool self, int index);
static void fxCloseSerial(txSerialTool self);
static txSerialMachine fxOpenNetwork(txSerialTool self, int index);
static void fxOpenSerial(txSerialTool self);
static void fxReadNetwork(txSerialMachine machine);
static void fxReadSerial(txSerialTool self);
static void fxWriteNetwork(txSerialTool self, char* buffer, int size);
static void fxWriteSerial(txSerialTool self, char* buffer, int size);

#define mxThrowElse(_ASSERTION) { if (!(_ASSERTION)) { self->file=__FILE__; self->line=__LINE__; self->error = errno; longjmp(self->_jmp_buf, 1); } }
#define mxThrowError(_ERROR) { self->file=__FILE__; self->line=__LINE__; self->error = _ERROR; longjmp(self->_jmp_buf, 1); } 

static char* gxMachineTags[10] = {
	"?xs0?>\r\n<",
	"?xs1?>\r\n<",
	"?xs2?>\r\n<",
	"?xs3?>\r\n<",
	"?xs4?>\r\n<",
	"?xs5?>\r\n<",
	"?xs6?>\r\n<",
	"?xs7?>\r\n<",
	"?xs8?>\r\n<",
	"?xs9?>\r\n<"
};

#define BOTHER 0010000
struct termios2 {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_line;
	cc_t c_cc[19];
	speed_t c_ispeed;
	speed_t c_ospeed;
};
#define TCGETS2 _IOR('T', 0x2A, struct termios2)
#define TCSETS2 _IOW('T', 0x2B, struct termios2)

void fxCloseNetwork(txSerialTool self, int index)
{
	txSerialMachine machine = self->machines[index];
	if (machine) {
		self->fds[index].fd = -1 - index;
		if (machine->networkConnection >= 0)
			close(machine->networkConnection);
		free(machine);
		self->machines[index] = NULL;
		if (self->currentMachine == machine)
			self->currentMachine = NULL;
	}
}

void fxCloseSerial(txSerialTool self)
{
	if (self->serialConnection >= 0) {
		close(self->serialConnection);
	}
}

txSerialMachine fxOpenNetwork(txSerialTool self, int index)
{
	txSerialMachine machine = self->machines[index];
	if (!machine) {
		struct hostent *host;
		struct sockaddr_in address;
		machine = calloc(sizeof(txSerialMachineRecord), 1);
		mxThrowElse(machine != NULL);
		machine->tool = self;
		machine->index = index;
		self->machines[index] = machine;
		host = gethostbyname(self->host);
		mxThrowElse(host != NULL);
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		memcpy(&(address.sin_addr), host->h_addr, host->h_length);
		address.sin_port = htons(self->port);
		machine->networkConnection = socket(AF_INET, SOCK_STREAM, 0);
		mxThrowElse(machine->networkConnection >= 0);
		mxThrowElse(connect(machine->networkConnection, (struct sockaddr*)&address, sizeof(address)) >= 0);
		self->fds[index].fd = machine->networkConnection;
	}
	return machine;
}

void fxOpenSerial(txSerialTool self)
{
	speed_t speed;
	struct termios term;
  	self->serialConnection = open(self->path, O_RDWR | O_NOCTTY | O_NDELAY);
	mxThrowElse(self->serialConnection >= 0);
	fcntl(self->serialConnection, F_SETFL, 0);
	switch (self->baud) {
	case 460800: speed = B460800; break;
	case 230400: speed = B230400; break;
	case 115200: speed = B115200; break;
	case 57600: speed = B57600; break;
	case 38400: speed = B38400; break;
	case 19200: speed = B19200; break;
	case 9600: speed = B9600; break;
	default: speed = self->baud; break;
	}
	term.c_cflag = speed | CS8 | CLOCAL | CREAD;
	term.c_ispeed = speed;
	term.c_ospeed = speed;
	term.c_iflag = IGNPAR;
	term.c_oflag &= ~ONLCR;
	term.c_lflag = 0;
	term.c_cc[VTIME] = 0;
	term.c_cc[VMIN] = 1;
	mxThrowElse(tcsetattr(self->serialConnection, TCSANOW, &term) == 0);
	tcflush(self->serialConnection, TCIFLUSH);
	if (speed == BOTHER) {
		struct termios2 tio2;
		ioctl(self->serialConnection, TCGETS2, &tio2);
		term.c_cflag &= ~CBAUD;
		term.c_cflag |= BOTHER;
		term.c_ispeed = self->baud;
		term.c_ospeed = self->baud;
		ioctl(self->serialConnection, TCSETS2, &tio2);
	} 
}

void fxReadNetwork(txSerialMachine machine)
{
	txSerialTool self = machine->tool;
	int size = read(machine->networkConnection, machine->networkBuffer, mxNetworkBufferSize - 1);
	if (size > 0) {
		char* former = machine->networkBuffer;
		char* current = former;
		char* limit = former + size;
		int offset;
		while (current < limit) {
			offset = current - former;
			if ((offset >= 3) && (current[-3] == 13) && (current[-2] == 10) && (current[-1] == '<')) {
				fxWriteSerial(self, former, offset);
				fxWriteSerial(self, gxMachineTags[machine->index], 9);
				former = current;
			}
			current++;
		}
		offset = limit - former;
		if (offset)
			fxWriteSerial(self, former, offset);
	}
}

void fxReadSerial(txSerialTool self)
{
	int size = read(self->serialConnection, self->serialBuffer, mxSerialBufferSize);
	mxThrowElse(size > 0);
// 	fprintf(stderr, "%.*s", size, self->serialBuffer);
	char* src = self->serialBuffer;
	char* srcLimit = src + size;
	int offset = self->index;
	char* dst = self->buffer + offset;
	char* dstLimit = self->buffer + mxBufferSize;
	while (src < srcLimit) {
		if (dst == dstLimit) {
			fxWriteNetwork(self, self->buffer, mxBufferSize - 10);
			memmove(self->buffer, dstLimit - 10, 10);
			dst = self->buffer + 10;
			offset = 10;
		}
		*dst++ = *src++;
		offset++;
		if ((offset >= 2) && (dst[-2] == 13) && (dst[-1] == 10)) {
			if ((offset >= 9) && (dst[-9] == '<') && (dst[-8] == '?') && (dst[-7] == 'x') && (dst[-6] == 's') && (dst[-5] >= '0') && (dst[-5] <= '9') && (dst[-4] == '?') && (dst[-3] == '>')) {
				self->currentMachine = fxOpenNetwork(self, dst[-5] - '0');
			}
			else if ((offset >= 10) && (dst[-10] == '<') && (dst[-9] == '?') && (dst[-8] == 'x') && (dst[-7] == 's') && (dst[-6] >= '0') && (dst[-6] <= '9') && (dst[-5] == '-') && (dst[-4] == '?') && (dst[-3] == '>')) {
				fxCloseNetwork(self, dst[-6] - '0');
			}
			else if ((offset >= 10) && (dst[-10] == '<') && (dst[-9] == '/') && (dst[-8] == 'x') && (dst[-7] == 's') && (dst[-6] == 'b') && (dst[-5] == 'u') && (dst[-4] == 'g') && (dst[-3] == '>')) {
				fxWriteNetwork(self, self->buffer, offset);
			}
			else {
				dst[-2] = 0;	
				if (offset > 2) fprintf(stderr, "%s\n", self->buffer);
			}
			dst = self->buffer;
			offset = 0;
		}
	}
	self->index = offset;
}

void fxWriteNetwork(txSerialTool self, char* buffer, int size)
{
	txSerialMachine machine = self->currentMachine;
	if (machine) {
		int count;
		while (size) {
			count = write(machine->networkConnection, buffer, size);
			mxThrowElse(count > 0);
			buffer += count;
			size -= count;
		}
	}
}

void fxWriteSerial(txSerialTool self, char* buffer, int size)
{
// 	fprintf(stderr, "%.*s", size, buffer);
	int count;
	while (size) {
    	count = write(self->serialConnection, buffer, size);
		mxThrowElse(count > 0);
		buffer += count;
		size -= count;
	}
}

static txSerialToolRecord tool;

void handler(int s) {
	txSerialTool self = &tool;
	fxCloseSerial(self);
	exit(0); 
}

int main(int argc, char* argv[]) 
{
	int result = 0;
	txSerialTool self = &tool;
	if (argc < 4) {
		fprintf(stderr, "### serial2xsbug <port name> <baud rate> <data bits><parity><stop bits>\n");
		return 1;
	}
	memset(&tool, 0, sizeof(tool));
	self->path = argv[1];
	self->baud = atoi(argv[2]);
	self->data = argv[3][0] - '0';
	self->parity = argv[3][1];
	self->stop = argv[3][2] - '0';
	self->serialConnection = -1;
	
	self->host = "localhost";
	self->port = 5002;
	
	signal(SIGINT, handler);

	if (setjmp(self->_jmp_buf) == 0) {
		int index;
		for (index = 0; index < 10; index++) {
			self->fds[index].fd = -1 - index;
			self->fds[index].events = POLLIN;
			self->fds[index].revents = 0;
		}
		fxOpenSerial(self);
		self->fds[10].fd = self->serialConnection;
		self->fds[10].events = POLLIN;
		self->fds[10].revents = 0;
		for (;;) {
			int res;
			res = poll(self->fds, 11, 1000);
			mxThrowElse(res >= 0);
			if (res) {
				for (index = 0; index < 10; index++) {
					if (self->fds[index].revents & (POLLERR | POLLHUP)) 
						mxThrowError(EPIPE);
					if (self->fds[index].revents & POLLIN)
						fxReadNetwork(self->machines[index]);
				}
				if (self->fds[10].revents & (POLLERR | POLLHUP))
					mxThrowError(EPIPE);
				if (self->fds[10].revents & POLLIN)
					fxReadSerial(self);
			}
		}
	}
	else {
		fprintf(stderr, "%s:%d: %s\n", self->file, self->line, strerror(self->error));
		result = 1;
	}
	fxCloseSerial(self);
	return result;
}
