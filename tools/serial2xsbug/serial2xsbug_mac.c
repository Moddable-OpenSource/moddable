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

typedef struct txSerialDescriptionStruct txSerialDescriptionRecord, *txSerialDescription;

struct txSerialDescriptionStruct {
	txSerialTool tool;
	io_object_t notification;
	char path[1];
};

static void fxProgrammingModeSerial(txSerialTool self);
static void fxReadNetwork(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context);
static void fxReadSerial(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context);
static void fxRegisterSerial(void *refcon, io_iterator_t iterator);
static void fxUnregisterSerial(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);

void fxCloseNetwork(txSerialTool self, uint32_t value)
{
	txSerialMachine* link = &(self->firstMachine);
	txSerialMachine machine;
	while ((machine = *link)) {
		if ((machine->value == value) || (0 == value)) {
			*link = machine->nextMachine;
			if (self->currentMachine == machine)
				self->currentMachine = NULL;
			if (machine->networkSource) {
				CFRunLoopRemoveSource(CFRunLoopGetCurrent(), machine->networkSource, kCFRunLoopCommonModes);
				CFRelease(machine->networkSource);
			}
			if (machine->networkSocket) {
				CFSocketInvalidate(machine->networkSocket);
				CFRelease(machine->networkSocket);
			}
			free(machine);
		}
		else
			link = &(machine->nextMachine);
	}
}

void fxCloseSerial(txSerialTool self)
{
	if (self->serialSource) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), self->serialSource, kCFRunLoopCommonModes);
		CFRelease(self->serialSource);
		self->serialSource = NULL;
	}
	if (self->serialSocket) {
		CFSocketInvalidate(self->serialSocket);
		CFRelease(self->serialSocket);
		self->serialSocket = NULL;
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
		CFSocketContext context;
		struct hostent *host;
		struct sockaddr_in address;

		machine = calloc(sizeof(txSerialMachineRecord), 1);
		if (!machine) {
			fprintf(stderr, "Error allocating machine - %s(%d).\n", strerror(errno), errno);
			exit(1);
		}
		machine->tool = self;
		machine->value = value;
		sprintf(machine->tag, "?xs.%8.8X?>\r\n<", value);
		*link = machine;

		memset(&context, 0, sizeof(CFSocketContext));
		context.info = (void*)machine;
		host = gethostbyname(self->host);
		if (!host) {
			fprintf(stderr, "Error getting host - %s(%d).\n", strerror(errno), errno);
			exit(1);
		}
		memcpy(&(address.sin_addr), host->h_addr, host->h_length);
		address.sin_family = AF_INET;
		address.sin_port = htons(self->port);
		machine->networkSocket = CFSocketCreate(kCFAllocatorDefault, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketReadCallBack, fxReadNetwork, &context);
		CFSocketError err = CFSocketConnectToAddress(machine->networkSocket, CFDataCreate(kCFAllocatorDefault, (const UInt8*)&address, sizeof(address)), (CFTimeInterval)10);
		if (err) {
			fprintf(stderr,"Error opening network: %ld.\n",err);
			exit(1);
		}
		machine->networkSource = CFSocketCreateRunLoopSource(NULL, machine->networkSocket, 0);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), machine->networkSource, kCFRunLoopCommonModes);
	}
	machine->receiveCount += 1;
	return machine;
}

void fxOpenSerial(txSerialTool self)
{
	int fd = -1;
	struct termios options;
	CFSocketContext context;

	fd = open(self->path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1) {
		usleep(500000);
		fd = open(self->path, O_RDWR | O_NOCTTY | O_NONBLOCK);
		if (fd == -1) {
			fprintf(stderr, "Error opening serial port %s - %s(%d).\n", self->path, strerror(errno), errno);
			return;
		}
	}
	
    if (ioctl(fd, TIOCEXCL) == -1) {
        fprintf(stderr, "Error setting TIOCEXCL on %s - %s(%d).\n", self->path, strerror(errno), errno);
        return;
    }

	tcgetattr(fd, &options);

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;

	// baud rate
    //cfsetspeed(&options, getBaud(baud));
	// bits per character
	options.c_cflag &= ~CSIZE;
	if (self->data == 5)
		options.c_cflag |= CS5;
	else if (self->data == 6)
		options.c_cflag |= CS6;
	else if (self->data == 7)
		options.c_cflag |= CS7;
	else if (self->data == 8)
		options.c_cflag |= CS8;

	// parity
	if (self->parity == 'N')
		options.c_cflag &= ~(PARENB | PARODD);
	else {
		options.c_cflag |= PARENB;
		if (self->parity == 'O')
			options.c_cflag |= PARODD;
		else if (self->parity == 'E')
			options.c_cflag &= ~PARODD;
	}
	// stop bits
	if (self->stop == 1)
		options.c_cflag &= ~CSTOPB;
	else if (self->stop == 2)
		options.c_cflag |= ~CSTOPB;

    // Cause the new options to take effect immediately.
    if (tcsetattr(fd, TCSANOW, &options) == -1) {
        fprintf(stderr, "Error setting tty attributes %s - %s(%d).\n", self->path, strerror(errno), errno);
        return;
    }
    speed_t speed = self->baud;
    if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {
        fprintf(stderr, "Error calling ioctl(..., IOSSIOSPEED, ...) %s - %s(%d).\n", self->path, strerror(errno), errno);
    }
    memset(self->buffer, 0, sizeof(self->buffer));
    self->index = 0;

	memset(&context, 0, sizeof(CFSocketContext));
	context.info = (void*)self;
	self->serialSocket = CFSocketCreateWithNative(kCFAllocatorDefault, fd, kCFSocketReadCallBack, fxReadSerial, &context);
	self->serialSource = CFSocketCreateRunLoopSource(NULL, self->serialSocket, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), self->serialSource, kCFRunLoopCommonModes);

	if (self->programming) {
#if mxTraceCommands
		fprintf(stderr, "### programming mode\n");
#endif
		fxProgrammingModeSerial(self);
		exit(0);
	}

	if (self->restartOnConnect) {
#if mxTraceCommands
		fprintf(stderr, "### restart\n");
#endif
		self->restartOnConnect = 0;
		fxRestart(self);
	}
}

void fxProgrammingModeSerial(txSerialTool self)
{
	int fd = CFSocketGetNative(self->serialSocket), flags;
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

void fxReadNetwork(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	txSerialMachine machine = context;
	CFSocketNativeHandle handle = CFSocketGetNative(socketRef);
	char buffer[256];
	int size = read(handle, buffer, 256);
	if (size > 0)
		fxReadNetworkBuffer(machine, buffer, size);
	else if ((size < 0) && (errno != EINPROGRESS)) {
        fprintf(stderr, "Error reading network - %s(%d).\n", strerror(errno), errno);
        exit(1);
	}
}

void fxReadSerial(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	txSerialTool self = context;
	CFSocketNativeHandle handle = CFSocketGetNative(socketRef);
	char buffer[1024];
	int size = read(handle, buffer, 1024);
	if (size > 0)
		fxReadSerialBuffer(self, buffer, size);
	else if ((size < 0) && (errno != EINPROGRESS) && (errno != ENXIO)) {
        fprintf(stderr, "Error reading serial - %s(%d).\n", strerror(errno), errno);
        exit(1);
	}
}

void fxRegisterSerial(void *refcon, io_iterator_t iterator)
{
	txSerialTool self = refcon;
    io_service_t usbDevice;
    while ((usbDevice = IOIteratorNext(iterator))) {
		CFTypeRef typeRef, vendorRef, productRef;
		int vendorID = 0, productID = 0;
		int match = 0;

		vendorRef = IORegistryEntrySearchCFProperty(usbDevice, kIOServicePlane, CFSTR("idVendor"),
											kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
		if (vendorRef)
			CFNumberGetValue(vendorRef , kCFNumberIntType, &vendorID);

		productRef = IORegistryEntrySearchCFProperty(usbDevice, kIOServicePlane, CFSTR("idProduct"),
											kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
		if (productRef)
			CFNumberGetValue(productRef , kCFNumberIntType, &productID);

//fprintf(stderr, "checking productID %04x:%04x\n", vendorID, productID);
		match = (!self->productID || (productID == self->productID)) &&
			 		(!self->vendorID || (vendorID == self->vendorID));

		if ((typeRef = IORegistryEntryCreateCFProperty(usbDevice, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0))) {
			CFIndex length = CFStringGetLength(typeRef);
			CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
			txSerialDescription description = malloc(sizeof(txSerialDescriptionRecord) + maxSize);
			if (description) {
				description->tool = self;
				CFStringGetCString(typeRef, &description->path[0], maxSize + 1, kCFStringEncodingUTF8);
				if (!strcmp(self->path, "") && match) {
					self->path = malloc(strlen(description->path) + 1);
					strcpy(self->path, description->path);

					if (self->showPath) {
						fprintf(stderr, "%s\n", description->path);
						exit(0);
					}
					else {
						fprintf(stderr, "product/vendor match: %s\n", description->path);
						fxOpenSerial(self);
					}
				}
				else
				if (!strcmp(self->path, description->path)
						&& IOServiceAddInterestNotification(self->notificationPort, usbDevice, kIOGeneralInterest, fxUnregisterSerial, description, &description->notification) == KERN_SUCCESS) {
					fxOpenSerial(self);
				}
				else
					free(description);
			}
		}

		IOObjectRelease(usbDevice);
	}
}

void fxRestartSerial(txSerialTool self)
{
	int fd = CFSocketGetNative(self->serialSocket), flags;
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

void fxUnregisterSerial(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{
	txSerialDescription description = refCon;
	fxCloseSerial(description->tool);

	free(description);
}

void fxWriteNetwork(txSerialMachine machine, char* buffer, int size)
{
	size = write(CFSocketGetNative(machine->networkSocket), buffer, size);
	if (size < 0) {
		fprintf(stderr, "Error writing network - %s(%d).\n", strerror(errno), errno);
		exit(1);
	}
}

void fxWriteSerial(txSerialTool self, char* buffer, int size)
{
#if mxTrace
	fprintf(stderr, "%.*s", size, buffer);
#endif
	while (size) {
		int result = write(CFSocketGetNative(self->serialSocket), buffer, size);
		if (result < 0) {
			if (EAGAIN == errno)
				continue;

			fprintf(stderr, "Error writing serial - %s(%d).\n", strerror(errno), errno);
			exit(1);
		}
		size -= result;
		buffer += result;
	}
}

static void fxSignalHandler(int s) {
	exit(1);
}

static void timeoutHandler(CFRunLoopTimerRef cfTimer, void *info)
{
	printf("timeout\n");
	exit(1);
}

int main(int argc, char* argv[])
{
	txSerialToolRecord tool;
	txSerialTool self = &tool;
	int result = fxArguments(self, argc, argv);
	if (result)
		return result;
	CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOSerialBSDServiceValue);
	self->notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
	CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(self->notificationPort);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	IOServiceAddMatchingNotification(self->notificationPort, kIOPublishNotification, matchingDict, fxRegisterSerial, self, &self->ioIterator);
	fxRegisterSerial(self, self->ioIterator);

	if (self->showPath) {
		CFRunLoopTimerRef cfTimer;
		CFRunLoopTimerContext context = {0};

		cfTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + (self->timeout / 1000.0), 0, 0, 0, timeoutHandler, &context);
		CFRunLoopAddTimer(CFRunLoopGetCurrent(), cfTimer, kCFRunLoopCommonModes);
	}

	signal(SIGINT, fxSignalHandler);
	
	CFRunLoopRun();
	
}   
