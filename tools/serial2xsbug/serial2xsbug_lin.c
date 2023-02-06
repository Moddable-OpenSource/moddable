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

#include "serial2xsbug.h"

static void fxCountMachines(txSerialTool self);
static void fxProgrammingModeSerial(txSerialTool self);
static void fxReadNetwork(txSerialMachine machine);
static void fxReadSerial(txSerialTool self);

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

void fxCloseNetwork(txSerialTool self, uint32_t value)
{
	txSerialMachine* link = &(self->firstMachine);
	txSerialMachine machine;
	while ((machine = *link)) {
		if ((machine->value == value) || (0 == value)) {
			*link = machine->nextMachine;
			if (self->currentMachine == machine)
				self->currentMachine = NULL;
			if (machine->networkConnection >= 0)
				close(machine->networkConnection);
			free(machine);
		}
		else
			link = &(machine->nextMachine);
	}
	fxCountMachines(self);
}

void fxCloseSerial(txSerialTool self)
{
	if (self->serialConnection >= 0) {
		close(self->serialConnection);
		self->serialConnection = -1;
	}
}

void fxCountMachines(txSerialTool self)
{
	txSerialMachine machine = self->firstMachine;
	self->count = 1;
	while (machine) {
		self->fds[self->count].fd = machine->networkConnection;
		self->fds[self->count].events = POLLIN;
		self->fds[self->count].revents = 0;
		self->count++;
		machine = machine->nextMachine;
	}
}

txSerialMachine fxOpenNetwork(txSerialTool self, uint32_t value)
{
	txSerialMachine* link = &(self->firstMachine);
	txSerialMachine machine;
	while ((machine = *link)) {
		if (machine->value == value)
			break;
		link = &(machine->nextMachine);
	}
	if (!machine) {
		struct hostent *host;
		struct sockaddr_in address;
		
		if (self->count == 1 + mxMachinesCount)
			mxThrowError(ENOMEM);
		
		machine = calloc(sizeof(txSerialMachineRecord), 1);
		mxThrowElse(machine != NULL);
		machine->tool = self;
		machine->value = value;
		sprintf(machine->tag, "?xs.%8.8X?>\r\n<", value);
		*link = machine;
		
		host = gethostbyname(self->host);
		mxThrowElse(host != NULL);
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		memcpy(&(address.sin_addr), host->h_addr, host->h_length);
		address.sin_port = htons(self->port);
		machine->networkConnection = socket(AF_INET, SOCK_STREAM, 0);
		mxThrowElse(machine->networkConnection >= 0);
		mxThrowElse(connect(machine->networkConnection, (struct sockaddr*)&address, sizeof(address)) >= 0);
		
		fxCountMachines(self);
	}
	machine->receiveCount += 1;
	return machine;
}

void fxOpenSerial(txSerialTool self)
{
	speed_t speed;
	struct termios term;

	self->serialConnection = open(self->path, O_RDWR | O_NOCTTY | O_NDELAY);
	if (self->serialConnection < 0) {
		if (!self->reconnecting)
			mxThrowElse(self->serialConnection >= 0);
		return;
	}
	fcntl(self->serialConnection, F_SETFL, 0);
	switch (self->baud) {
	case 921600: speed = B921600; break;
	case 460800: speed = B460800; break;
	case 230400: speed = B230400; break;
	case 115200: speed = B115200; break;
	case 57600: speed = B57600; break;
	case 38400: speed = B38400; break;
	case 19200: speed = B19200; break;
	case 9600: speed = B9600; break;
	default: speed = BOTHER; break;
	}
	// we need a raw terminal, start with zeroed termios and set all fields
	// it's possible that using tcgetattr and then overriding everything is safer, hard to tell
	memset(&term, 0, sizeof(term));
	term.c_cflag = CS8 | CLOCAL | CREAD;
	term.c_iflag = IGNPAR;
	term.c_oflag = 0;
	term.c_lflag = 0;
	term.c_cc[VTIME] = 0;
	term.c_cc[VMIN] = 1;
	cfsetspeed(&term, speed == BOTHER ? B115200 : speed); // adjust below in case of BOTHER
	mxThrowElse(tcsetattr(self->serialConnection, TCSANOW, &term) == 0);
	tcflush(self->serialConnection, TCIFLUSH);

	// prevent interference by ModemManager, et. al.
	ioctl(self->serialConnection, TIOCEXCL, NULL);

	// handle non-standard speeds
	if (speed == BOTHER) {
		struct termios2 tio2;
		ioctl(self->serialConnection, TCGETS2, &tio2);
		tio2.c_cflag &= ~CBAUD;
		tio2.c_cflag |= BOTHER;
		tio2.c_ispeed = self->baud;
		tio2.c_ospeed = self->baud;
		ioctl(self->serialConnection, TCSETS2, &tio2);
	}
	usleep(5000);

	if (self->programming) {
#if mxTraceCommands
		fprintf(stderr, "### programming mode\n");
#endif
		fxProgrammingModeSerial(self);
		exit(0);
	}

	if (self->restartOnConnect) {
		self->restartOnConnect = 0;
		fxRestart(self);
	}
}

void fxProgrammingModeSerial(txSerialTool self)
{
	int fd = self->serialConnection, flags;
	ioctl(fd, TIOCMGET, &flags);

	flags |= TIOCM_RTS | TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);
	usleep(10 * 1000);

	flags &= ~TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);
	usleep(100 * 1000);

	flags &= ~TIOCM_RTS;
	flags |= TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);
	usleep(50 * 1000);

	flags &= ~TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);
}

void fxReadNetwork(txSerialMachine machine)
{
	int size = read(machine->networkConnection, machine->networkBuffer, mxNetworkBufferSize - 1);
	if (size > 0)
		fxReadNetworkBuffer(machine, machine->networkBuffer, size);
}

void fxReadSerial(txSerialTool self)
{
	int size = read(self->serialConnection, self->serialBuffer, mxSerialBufferSize);
	mxThrowElse(size > 0);
	fxReadSerialBuffer(self, self->serialBuffer, size);
}

void fxRestartSerial(txSerialTool self)
{
	int fd = self->serialConnection, flags;
	ioctl(fd, TIOCMGET, &flags);
	flags |= TIOCM_RTS;
	flags &= ~TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);

	usleep(5000);

	flags &= ~TIOCM_RTS;
	if (self->dtr)
		flags |= TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);
}


void fxWriteNetwork(txSerialMachine machine, char* buffer, int size)
{
	txSerialTool self = machine->tool;
	int count;
	while (size) {
		count = write(machine->networkConnection, buffer, size);
		mxThrowElse(count > 0);
		buffer += count;
		size -= count;
	}
}

void fxWriteSerial(txSerialTool self, char* buffer, int size)
{
#if mxTrace
	fprintf(stderr, "%.*s", size, buffer);
#endif
	int count;
	while (size) {
    	count = write(self->serialConnection, buffer, size);
		mxThrowElse(count > 0);
		buffer += count;
		size -= count;
	}
}

static txSerialToolRecord tool;

static void handler(int s) {
	txSerialTool self = &tool;
	fxCloseSerial(self);
	exit(0); 
}

int main(int argc, char* argv[]) 
{
	txSerialTool self = &tool;
	int result = fxArguments(self, argc, argv);
	if (result)
		return result;
	self->serialConnection = -1;
	signal(SIGINT, handler);
	if (setjmp(self->_jmp_buf) == 0) {
		fxOpenSerial(self);
		self->count = 1;
		self->fds[0].fd = self->serialConnection;
		self->fds[0].events = POLLIN;
		self->fds[0].revents = 0;
		for (;;) {
			if (self->reconnecting) {
				fxOpenSerial(self);
				if (self->serialConnection >= 0) {
					self->reconnecting = 0;
					self->fds[0].fd = self->serialConnection;
				}
				else
					usleep(500000);
			}
			else {
				int res;
				res = poll(self->fds, self->count, 1000);
				mxThrowElse(res >= 0);
				if (res) {
					if (self->fds[0].revents & (POLLERR | POLLHUP)) {
						fxCloseSerial(self);
						self->reconnecting = 1;
						usleep(500000);
						continue;
					}
					if (self->fds[0].revents & POLLIN)
						fxReadSerial(self);
					if (self->count > 1) {
						txSerialMachine machine = self->firstMachine;
						int index = 1;
						while (index < self->count) {
							if (self->fds[index].revents & (POLLERR | POLLHUP)) 
								mxThrowError(EPIPE);
							if (self->fds[index].revents & POLLIN)
								fxReadNetwork(machine);
							machine = machine->nextMachine;
							index++;
						}
					}
				}
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

