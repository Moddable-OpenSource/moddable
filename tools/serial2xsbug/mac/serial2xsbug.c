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

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
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

#define mxBufferSize 32 * 1024
#define mxTagSize 17
#define mxTrace 0

typedef struct txSerialDescriptionStruct txSerialDescriptionRecord, *txSerialDescription;
typedef struct txSerialMachineStruct txSerialMachineRecord, *txSerialMachine;
typedef struct txSerialToolStruct txSerialToolRecord, *txSerialTool;

struct txSerialDescriptionStruct {
	txSerialTool tool;
	io_object_t notification;
	char path[1];
};

struct txSerialMachineStruct {
	txSerialTool tool;
	txSerialMachine nextMachine;
	uint32_t value;
	char tag[mxTagSize + 1];
	CFSocketRef networkSocket;
	CFRunLoopSourceRef networkSource;
};

struct txSerialToolStruct {
	IONotificationPortRef notificationPort;
	io_iterator_t ioIterator;
	
	char* path;
	int baud;
	int data;
	int parity;
	int stop;
	CFSocketRef serialSocket;
	CFRunLoopSourceRef serialSource;
	
	char* host;
	int port;
	txSerialMachine firstMachine;
	txSerialMachine currentMachine;
	
	int index;
	int state;
	char buffer[mxBufferSize + 1];
};

static void fxCloseNetwork(txSerialTool self, uint32_t value);
static void fxCloseSerial(txSerialTool self);
static uint8_t fxMatchProcessingInstruction(char* p, uint8_t* flag, uint32_t* value);
static txSerialMachine fxOpenNetwork(txSerialTool self, uint32_t value);
static void fxOpenSerial(txSerialTool self);
static void fxReadNetwork(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context);
static void fxReadSerial(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context);
static void fxRegisterSerial(void *refcon, io_iterator_t iterator);
static void fxUnregisterSerial(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);
static void fxWriteNetwork(txSerialTool self, void *buffer, int size);
static void fxWriteSerial(txSerialTool self, void *buffer, int size);

void fxCloseNetwork(txSerialTool self, uint32_t value)
{
	txSerialMachine* link = &(self->firstMachine);
	txSerialMachine machine;
	while ((machine = *link)) {
		if (machine->value == value)
			break;
		link = &(machine->nextMachine);
	}
	if (machine) {
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

uint8_t fxMatchProcessingInstruction(char* p, uint8_t* flag, uint32_t* value)
{
	char c;
	int i;
	if (*p++ != '<')
		return 0;
	if (*p++ != '?')
		return 0;
	if (*p++ != 'x')
		return 0;
	if (*p++ != 's')
		return 0;
	c = *p++;
	if (c == '.')
		*flag = 1;
	else if (c == '-')
		*flag = 0;
	else
		return 0;
	*value = 0;
	for (i = 0; i < 8; i++) {
		c = *p++;
		if (('0' <= c) && (c <= '9'))
			*value = (*value * 16) + (c - '0');
		else if (('a' <= c) && (c <= 'f'))
			*value = (*value * 16) + (10 + c - 'a');
		else if (('A' <= c) && (c <= 'F'))
			*value = (*value * 16) + (10 + c - 'A');
		else
			return 0;
	}
	if (*p++ != '?')
		return 0;
	if (*p++ != '>')
		return 0;
	return 1;
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
	return machine;
}

void fxOpenSerial(txSerialTool self)
{
	int fd = -1;
	struct termios options;
	CFSocketContext context;

    fd = open(self->path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        fprintf(stderr, "Error opening serial port %s - %s(%d).\n", self->path, strerror(errno), errno);
        return;
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
    speed_t speed = self->baud; // Set 74880 baud
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
}

void fxReadNetwork(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	txSerialMachine machine = context;
	txSerialTool self = machine->tool;
	CFSocketNativeHandle handle = CFSocketGetNative(socketRef);
	char buffer[1024];
	int size = read(handle, buffer, 1024);
	if (size > 0) {
		//fprintf(stderr, "%.*s", size, buffer);
		char* former = buffer;
		char* current = buffer;
		char* limit = buffer + size;
		int offset;
		while (current < limit) {
			offset = current - former;
			if ((offset >= 3) && (current[-3] == 13) && (current[-2] == 10) && (current[-1] == '<')) {
				fxWriteSerial(self, former, offset);
				fxWriteSerial(self, machine->tag, mxTagSize);
				former = current;
			}
			current++;
		}
		offset = limit - former;
		if (offset)
			fxWriteSerial(self, former, offset);
	}
	else if ((size < 0) && (errno != EINPROGRESS)) {
        fprintf(stderr, "Error reading network - %s(%d).\n", strerror(errno), errno);
        exit(1);
	}
}

#define ESP_STACK_COUNT 1024
static char* elfPath = NULL;
static char* TOOLS_BIN = NULL;
static char* gStackBuffers[ESP_STACK_COUNT];
static char* gEPC1Buffer;
static int gStackIndex;
static int gExceptionNumber = -1;

static char* gExceptionList[33] = {
    "Illegal instruction",
    "SYSCALL instruction",
    "InstructionFetchError: Processor internal physical address or data error during instruction fetch",
    "LoadStoreError: Processor internal physical address or data error during load or store",
    "Level1Interrupt: Level-1 interrupt as indicated by set level-1 bits in the INTERRUPT register",
    "Alloca: MOVSP instruction, if caller's registers are not in the register file",
    "IntegerDivideByZero: QUOS, QUOU, REMS, or REMU divisor operand is zero",
    "reserved",
    "Privileged: Attempt to execute a privileged operation when CRING ? 0",
    "LoadStoreAlignmentCause: Load or store to an unaligned address",
    "reserved",
    "reserved",
    "InstrPIFDataError: PIF data error during instruction fetch",
    "LoadStorePIFDataError: Synchronous PIF data error during LoadStore access",
    "InstrPIFAddrError: PIF address error during instruction fetch",
    "LoadStorePIFAddrError: Synchronous PIF address error during LoadStore access",
    "InstTLBMiss: Error during Instruction TLB refill",
    "InstTLBMultiHit: Multiple instruction TLB entries matched",
    "InstFetchPrivilege: An instruction fetch referenced a virtual address at a ring level less than CRING",
    "reserved",
    "InstFetchProhibited: An instruction fetch referenced a page mapped with an attribute that does not permit instruction fetch",
    "reserved",
    "reserved",
    "reserved",
    "LoadStoreTLBMiss: Error during TLB refill for a load or store",
    "LoadStoreTLBMultiHit: Multiple TLB entries matched for a load or store",
    "LoadStorePrivilege: A load or store referenced a virtual address at a ring level less than CRING",
    "reserved",
    "LoadProhibited: A load referenced a page mapped with an attribute that does not permit loads",
    "StoreProhibited: A store referenced a page mapped with an attribute that does not permit stores",
		"reserved",
		"reserved",
		"CoprocessornDisabled: Coprocessor n instruction when cpn disabled. n varies 0..7 as the cause varies 32..39",
  };

static void systemCommand(char* command, char **buffer){
	char line[1024];
	FILE *fp = NULL;
	char *out = calloc(1, 1);

	fp = popen(command, "r");
	if (fp){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			out = realloc(out, strlen(out) + strlen(line));
			strcat(out, line);
		}
		pclose(fp);
	}
		
	*buffer = out;
}

static char* printAddress(char* address){
	char* commandPart = "/xtensa-lx106-elf-addr2line -aipfC -e ";
	char command[2000] = {0};
	char* buffer;
	
	strcat(command, TOOLS_BIN);
	strcat(command, commandPart);
	strcat(command, elfPath);
	strcat(command, " ");
	strcat(command, address);
	systemCommand(command, &buffer);
	if (buffer) {
		buffer[strlen(buffer) - 1] = 0;
		if ((strstr(buffer, "??:0") != NULL) || (strstr(buffer, "??:?") != NULL)) {
			free(buffer);
			buffer = NULL;
		}
	}
	return buffer;
}

void fxReadSerial(CFSocketRef socketRef, CFSocketCallBackType cbType, CFDataRef addr, const void* data, void* context)
{
	txSerialTool self = context;
	CFSocketNativeHandle handle = CFSocketGetNative(socketRef);
	char buffer[1024];
	int size = read(handle, buffer, 1024);
	if (size > 0) {
#if mxTrace
		fprintf(stderr, "%.*s", size, buffer);
#endif
		char* src = buffer;
		char* srcLimit = src + size;
		int offset = self->index;
		char* dst = self->buffer + offset;
		char* dstLimit = self->buffer + mxBufferSize;
		while (src < srcLimit) {
			if (dst == dstLimit) {
				fxWriteNetwork(self, self->buffer, mxBufferSize - mxTagSize);
				memmove(self->buffer, dstLimit - mxTagSize, mxTagSize);
				dst = self->buffer + mxTagSize;
				offset = mxTagSize;
			}
			*dst++ = *src++;
			offset++;
			if ((offset >= 2) && (dst[-2] == 13) && (dst[-1] == 10)) {
				uint8_t flag;
				uint32_t value;
				if ((offset >= mxTagSize) && fxMatchProcessingInstruction(dst - mxTagSize, &flag, &value)) {
					if (flag)
						self->currentMachine = fxOpenNetwork(self, value);
					else
						fxCloseNetwork(self, value);
				}
				else if ((offset >= 10) && (dst[-10] == '<') && (dst[-9] == '/') && (dst[-8] == 'x') && (dst[-7] == 's') && (dst[-6] == 'b') && (dst[-5] == 'u') && (dst[-4] == 'g') && (dst[-3] == '>')) {
					fxWriteNetwork(self, self->buffer, offset);
				}
				else {
					dst[-2] = 0;	
					if (offset > 2) fprintf(stderr, "%s\n", self->buffer);
					
					if (elfPath){
						char* epc;
						char* exception;
						
						//Exception Reason
						exception = strstr(self->buffer, "Exception (");
						if (exception){
							sscanf(self->buffer, "Exception (%d):", &gExceptionNumber);
						}
						
						//Exception Program Counter
						epc = strstr(self->buffer, "epc1=0x");
						if (epc){
								char epc1Address[9];
								memcpy(epc1Address, epc + 7, 8);
								epc1Address[8] = 0;
								gEPC1Buffer = printAddress(epc1Address);
						}
					}	
					
					if (self->state == 0) {
						if (!strcmp(self->buffer, ">>>stack>>>")) {
							gStackIndex = 0;
							self->state = 1;
						}
					}
					else if (self->state == 1) {
						if (!strcmp(self->buffer, "<<<stack<<<")) {
							if (elfPath && gStackIndex) {
								int i;
								
								if (gExceptionNumber != -1){
									int exceptionNum = gExceptionNumber;
									if (exceptionNum >= 32 && exceptionNum <= 39) exceptionNum = 32;
									if (exceptionNum > 39) exceptionNum = 31;
									
									fprintf(stderr, "\n# EXCEPTION DESCRIPTION\n");
									fprintf(stderr, "# Exception %d %s\n", gExceptionNumber, gExceptionList[gExceptionNumber]);
									gExceptionNumber = -1;
								}
								
								if (gEPC1Buffer){
									fprintf(stderr, "\n# EXCEPTION LOCATION\n");
									fprintf(stderr, "# %s\n", gEPC1Buffer);
									free(gEPC1Buffer);
									gEPC1Buffer = NULL;
								}
								
								fprintf(stderr, "\n# CALLS\n");
								for (i = 0; i < gStackIndex; i++) {
									fprintf(stderr, "# %s\n", gStackBuffers[i]);
									free(gStackBuffers[i]);
								}
								fprintf(stderr, "\n");
							}
							self->state = 0;
						}
						else if (elfPath && TOOLS_BIN) {
							char* p = self->buffer + 11;
							char* q = self->buffer + offset - 9;
							while (p < q) {
								p[8] = 0;
								if (gStackIndex < ESP_STACK_COUNT) {
									gStackBuffers[gStackIndex] = printAddress(p);
									if (gStackBuffers[gStackIndex])
										gStackIndex++;
								}
								p += 9;
							}
						}
					}
				}
				dst = self->buffer;
				offset = 0;
			}
		}
		self->index = offset;
	}
	else if ((size < 0) && (errno != EINPROGRESS)) {
        fprintf(stderr, "Error reading serial - %s(%d).\n", strerror(errno), errno);
        exit(1);
	}
	return;
}

void fxRegisterSerial(void *refcon, io_iterator_t iterator)
{
	txSerialTool self = refcon;
    io_service_t usbDevice;
    while ((usbDevice = IOIteratorNext(iterator))) {
		kern_return_t kr;
		CFTypeRef typeRef;

		if ((typeRef = IORegistryEntryCreateCFProperty(usbDevice, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0))) {
			CFIndex length = CFStringGetLength(typeRef);
			CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
			txSerialDescription description = malloc(sizeof(txSerialDescriptionRecord) + maxSize);
			if (description) {
				description->tool = self;
				CFStringGetCString(typeRef, &description->path[0], maxSize + 1, kCFStringEncodingUTF8);
				if (!strcmp(self->path, description->path) 
						&& IOServiceAddInterestNotification(self->notificationPort, usbDevice, kIOGeneralInterest, fxUnregisterSerial, description, &description->notification) == KERN_SUCCESS) {
					fxOpenSerial(self);
				}
				else
					free(description);
			}
		}
		kr = IOObjectRelease(usbDevice);
	}
}

void fxUnregisterSerial(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{
	txSerialDescription description = refCon;
	fxCloseSerial(description->tool);
	free(description);
}

void fxWriteNetwork(txSerialTool self, void *buffer, int size)
{
	txSerialMachine machine = self->currentMachine;
	if (machine) {
		size = write(CFSocketGetNative(machine->networkSocket), buffer, size);
		if (size < 0) {
			fprintf(stderr, "Error writing network - %s(%d).\n", strerror(errno), errno);
			exit(1);
		}
	}
}

void fxWriteSerial(txSerialTool self, void *buffer, int size)
{
#if mxTrace
	fprintf(stderr, "%.*s", size, buffer);
#endif
	size = write(CFSocketGetNative(self->serialSocket), buffer, size);
	if (size < 0) {
        fprintf(stderr, "Error writing serial - %s(%d).\n", strerror(errno), errno);
        exit(1);
	}
}

void fxSignalHandler(int s) {
	   exit(1); 
}

int main(int argc, char* argv[]) 
{
	txSerialToolRecord tool;
	txSerialTool self = &tool;
	memset(&tool, 0, sizeof(tool));
	
	if (argc < 4) {
		fprintf(stderr, "### serial2xsbug <port name> <baud rate> <data bits><parity><stop bits>\n");
		return 1;
	}
	self->path = argv[1];
	self->baud = atoi(argv[2]);
	self->data = argv[3][0] - '0';
	self->parity = argv[3][1];
	self->stop = argv[3][2] - '0';
	self->host = "localhost";
	self->port = 5002;
	if (argc >= 5){
		elfPath = argv[4];
	}else{
		elfPath = NULL;
	}
	if (argc >= 6){
		TOOLS_BIN = argv[5];
	}else{
		TOOLS_BIN = NULL;
	}
	
    CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOSerialBSDServiceValue);
	self->notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
    CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(self->notificationPort);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	IOServiceAddMatchingNotification(self->notificationPort, kIOPublishNotification, matchingDict, fxRegisterSerial, self, &self->ioIterator);
	fxRegisterSerial(self, self->ioIterator);
	
	signal(SIGINT, fxSignalHandler);

	CFRunLoopRun();
	
	return 0;
}
