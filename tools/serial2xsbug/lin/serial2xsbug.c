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

typedef struct txSerialToolStruct txSerialToolRecord, *txSerialTool;

struct txSerialToolStruct {
	jmp_buf _jmp_buf;
	char* file;
	int line;
	int error;
	
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
	int networkConnection;
	char networkBuffer[mxNetworkBufferSize];
	
	int index;
	int state;
	
	char buffer[mxBufferSize];
};

static void fxCloseNetwork(txSerialTool self);
static void fxCloseSerial(txSerialTool self);
static void fxOpenNetwork(txSerialTool self);
static void fxOpenSerial(txSerialTool self);
static void fxReadNetwork(txSerialTool self);
static void fxReadSerial(txSerialTool self);
static void fxWriteNetwork(txSerialTool self, char* buffer, int size);
static void fxWriteSerial(txSerialTool self, char* buffer, int size);

#define mxThrowElse(_ASSERTION) { if (!(_ASSERTION)) { self->file=__FILE__; self->line=__LINE__; self->error = errno; longjmp(self->_jmp_buf, 1); } }
#define mxThrowError(_ERROR) { self->file=__FILE__; self->line=__LINE__; self->error = _ERROR; longjmp(self->_jmp_buf, 1); } 

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

void fxCloseNetwork(txSerialTool self)
{
	if (self->networkConnection >= 0)
		close(self->networkConnection);
}

void fxCloseSerial(txSerialTool self)
{
	if (self->serialConnection >= 0) {
		close(self->serialConnection);
	}
}

void fxOpenNetwork(txSerialTool self)
{
	struct hostent *host;
	struct sockaddr_in address;
	host = gethostbyname(self->host);
	mxThrowElse(host != NULL);
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	memcpy(&(address.sin_addr), host->h_addr, host->h_length);
	address.sin_port = htons(self->port);
	self->networkConnection = socket(AF_INET, SOCK_STREAM, 0);
	mxThrowElse(self->networkConnection >= 0);
	mxThrowElse(connect(self->networkConnection, (struct sockaddr*)&address, sizeof(address)) >= 0);
}

void fxOpenSerial(txSerialTool self)
{
	speed_t speed;
    int mcs;
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

void fxReadNetwork(txSerialTool self)
{
	int size = read(self->networkConnection, self->networkBuffer, mxNetworkBufferSize - 1);
	mxThrowElse(size > 0);
// 	self->networkBuffer[size] = 0;
// 	fprintf(stderr, "%s\n", self->networkBuffer);
	fxWriteSerial(self, self->networkBuffer, size);
}

void fxReadSerial(txSerialTool self)
{
	int size = read(self->serialConnection, self->serialBuffer, mxSerialBufferSize - 1);
	mxThrowElse(size > 0);
// 	self->serialBuffer[size] = 0;
// 	fprintf(stderr, "%s\n", self->serialBuffer);
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
			if ((offset >= 10) && (dst[-10] == '<') && (dst[-9] == '/') && (dst[-8] == 'x') && (dst[-7] == 's') && (dst[-6] == 'b') && (dst[-5] == 'u') && (dst[-4] == 'g') && (dst[-3] == '>')) {
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
	int count;
	while (size) {
    	count = write(self->networkConnection, buffer, size);
		mxThrowElse(count > 0);
		buffer += count;
		size -= count;
	}
}

void fxWriteSerial(txSerialTool self, char* buffer, int size)
{
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
	fxCloseNetwork(self);
	fxCloseSerial(self);
	exit(0); 
}

int main(int argc, char* argv[]) 
{
	int result = 0;
	txSerialTool self = &tool;
	struct pollfd fds[2];
	struct pollfd *networkPoll, *serialPoll;
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
	self->networkConnection = -1;
	
	signal(SIGINT, handler);

	if (setjmp(self->_jmp_buf) == 0) {
		fxOpenSerial(self);
		fxOpenNetwork(self);
		fds[0].fd = self->serialConnection;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds[1].fd = self->networkConnection;
		fds[1].events = POLLIN;
		fds[1].revents = 0;
		serialPoll = fds;
		networkPoll = fds + 1;
		for (;;) {
			int res;
			res = poll(fds, 2, 1000);
			mxThrowElse(res >= 0);
			if (res) {
				if (networkPoll->revents & (POLLERR | POLLHUP)) 
					mxThrowError(EPIPE);
				if (serialPoll->revents & (POLLERR | POLLHUP))
					mxThrowError(EPIPE);
				if (serialPoll->revents & POLLIN)
					fxReadSerial(self);
				if (networkPoll->revents & POLLIN)
					fxReadNetwork(self);
			}
		}
	}
	else {
		fprintf(stderr, "%s:%d: %s\n", self->file, self->line, strerror(self->error));
		result = 1;
	}
	fxCloseNetwork(self);
	fxCloseSerial(self);
	return result;
}
