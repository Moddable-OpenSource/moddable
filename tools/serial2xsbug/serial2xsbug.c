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

#include "serial2xsbug.h"

#define kInstallInitialFragmentSize (4)
#define kInstallSkipFragmentSize (4)
#define kInstallFragmentSizeMax (4096)

static void fxCommandReceived(txSerialTool self, void *buffer, int size);
static int fxInitializeTarget(txSerialTool self);
static uint8_t fxMatchProcessingInstruction(char* p, uint8_t* flag, uint32_t* value);
static void fxSetTime(txSerialTool self, txSerialMachine machine);
static void fxInstallFragment(txSerialTool self);
static int mapHex(char c);
static void fxSetPref(txSerialTool self);

static uint8_t gRestarting = 0;
static char *gCmd = NULL;
static char *gModuleName = NULL;
static FILE *gInstallFD = 0;
static unsigned int gInstallArchiveSize;
static int gInstallOffset = 0;
static int gInstallFragmentSize = 512;
static uint8_t gBinaryState = 0;
static uint16_t gBinaryLength;

#define ESP_STACK_COUNT 1024
static char* elfPath = NULL;
static char* TOOLS_BIN = NULL;
static char* gStackBuffers[ESP_STACK_COUNT];
static char* gEPC1Buffer;
static int gStackIndex;
static int gExceptionNumber = -1;
static int gdbMode = 0;
static FILE *gCoreDump = 0;
static char gCoreDumpPath[1024];

typedef struct {
	void	*next;
	char	domain[64];
	char	name[64];
	char	value[128];
	char	kind;
} PrefRecord, *Pref;

Pref gPrefs = NULL;

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

int fxArguments(txSerialTool self, int argc, char* argv[])
{
	int argi;
	if (argc < 3) {
		fprintf(stderr, "### serial2xsbug <port name> <baud rate> <data bits><parity><stop bits>\n");
		fprintf(stderr, "### serial2xsbug <vid:pid> -showPath [-timeout <ms>]\n");
		return 1;
	}
	memset(self, 0, sizeof(txSerialToolRecord));
	self->path = argv[1];
	if (argc > 2) {
		self->baud = atoi(argv[2]);
		if (argc > 3) {
			self->data = argv[3][0] - '0';
			self->parity = argv[3][1];
			self->stop = argv[3][2] - '0';
		}
	}
	self->host = "localhost";
	if (getenv("XSBUG_HOST"))
		self->host = getenv("XSBUG_HOST");
	self->port = 5002;
	if (getenv("XSBUG_PORT"))
		self->port = atoi(getenv("XSBUG_PORT"));
	self->restartOnConnect = 1;
	self->showPath = 0;
	self->timeout = 5000;	// for showpath
	
	if (9 == strlen(self->path) && (':' == self->path[4])) {
			self->vendorID = (mapHex(self->path[0]) << 12) | (mapHex(self->path[1]) << 8) | (mapHex(self->path[2]) << 4) | mapHex(self->path[3]);
			self->productID = (mapHex(self->path[5]) << 12) | (mapHex(self->path[6]) << 8) | (mapHex(self->path[7]) << 4) | mapHex(self->path[8]);
			self->path = "";
	}

	TOOLS_BIN = NULL;
	elfPath = NULL;
	gCmd = NULL;

	for (argi = 2; argi < argc; argi++) {
		if (argv[argi][0] != '-')
			continue;
		if (!strcmp(argv[argi], "-showPath"))
			self->showPath = 1;
		else if (!strcmp(argv[argi], "-timeout") && ((argi + 1) < argc)) {
			self->timeout = atoi(argv[++argi]);
		}
		else if (!strcmp(argv[argi], "-elf") && ((argi + 1) < argc)) {
			elfPath = argv[++argi];
		}
		else if (!strcmp(argv[argi], "-bin") && ((argi + 1) < argc)) {
			TOOLS_BIN = argv[++argi];
		}
		else if (!strcmp(argv[argi], "-uninstall") && !gCmd) {
			gCmd = argv[argi] + 1;
		}
		else if (!strcmp(argv[argi], "-install") && !gCmd && ((argi + 1) < argc)) {
			gCmd = argv[argi++] + 1;
			gInstallFD = fopen(argv[argi], "rb");
			if (!gInstallFD) {
				fprintf(stderr, "### can't open '%s'\n", argv[argi]);
				return 1;
			}

			// validate archive header
			uint32_t header[4];
			fread(header, 1, sizeof(header), gInstallFD);
			fseek(gInstallFD, gInstallOffset, SEEK_END);
			gInstallArchiveSize = ftell(gInstallFD);

			if ((gInstallArchiveSize != ntohl(header[0])) ||
				strncmp((char *)&header[1], "XS_A", 4) ||
				(12 != ntohl(header[2])) ||
				strncmp((char *)&header[3], "VERS", 4)) {
				fprintf(stderr, "### invalid archive '%s'\n", argv[argi]);
				return 1;
			}
			fseek(gInstallFD, 0, SEEK_SET);
		}
		else if (!strcmp(argv[argi], "-pref") && ((argi + 1) < argc)) {
			PrefRecord pr;
			Pref pref;
			Pref walker = gPrefs;
			char *p = argv[++argi];
			char *equal = strstr(p, "="), *dot = strstr(p, ".");
			if (!equal || (p == equal) || !dot || (dot > equal)) {
				fprintf(stderr, "### invalid pref - domain.name=value: %s\n", argv[argi]);
				return 1;
			}
			if ((dot - p + 1) > (int)sizeof(pr.domain)) {
				fprintf(stderr, "### invalid pref - domain too long: %s\n", argv[argi]);
				return 1;
			}
			if ((equal - dot + 1) > (int)sizeof(pr.name)) {
				fprintf(stderr, "### invalid pref - name too long: %s\n", argv[argi]);
				return 1;
			}
			if ((p + strlen(p) - equal + 1) > (int)sizeof(pr.value)) {
				fprintf(stderr, "### invalid pref - value too long: %s\n", argv[argi]);
				return 1;
			}

			memset(&pr, 0, sizeof(pr));
			strncpy(pr.domain, p, dot - p);
			strncpy(pr.name, dot + 1, equal - dot - 1);
			strncpy(pr.value, equal + 1, p + strlen(p) - equal);
			if (('"' == pr.value[0]) && ('"' == pr.value[strlen(pr.value) - 1]))
				strncpy(pr.value, equal + 1 + 1, p + strlen(p) - equal - 2);		// strip quotesso

			pr.next = NULL;
			pr.kind = 3;		// string
			pref = malloc(sizeof(pr));
			memcpy(pref, &pr, sizeof(pr));

			if (!walker)
				gPrefs = pref;
			else {
				while (walker->next)
					walker = walker->next;
				walker->next = pref;
			}
		}
		else if (!strcmp(argv[argi], "-load") && !gModuleName && ((argi + 1) < argc)) {
			gModuleName = argv[++argi];
		}
		else if (!strcmp(argv[argi], "-dtr")) {
			self->dtr = 1;
		}
		else if (!strcmp(argv[argi], "-programming")) {
			self->programming = 1;
		}
		else if (!strcmp(argv[argi], "-norestart")) {
			self->restartOnConnect = 0;
		}
		else {
			fprintf(stderr, "### unexpected option '%s'\n", argv[argi]);
			return 1;
		}
	}
	return 0;
}

int fxInitializeTarget(txSerialTool self)
{
	char out[64];

#if 0
	{
		#define BAUD (115200 * 4)
		char machine[32];
		sprintf(machine, "\r\n<?xs#%8.8X?>", self->currentMachine->value);

		fxWriteSerial(self, machine, strlen(machine));
		// 2 bytes of  length followed by 7 bytes of payload
		uint8_t command[] = {0, 7, 8, 0, 0, (BAUD >> 24) & 255, (BAUD >> 16) & 255, (BAUD >> 8) & 255, BAUD & 255};
		fxWriteSerial(self, command, sizeof(command));
		tcdrain(CFSocketGetNative(self->serialSocket));
		usleep(200000);		//@@ why?
		speed_t speed = BAUD;
		if (-1 == ioctl(CFSocketGetNative(self->serialSocket), IOSSIOSPEED, &speed))
			fprintf(stderr, "set speed failed\n");
	}
#endif

	fxSetTime(self, self->currentMachine);

	if (gPrefs) {
		fxSetPref(self);
		return 0;
	}

	if (!gCmd) {
		if (!gModuleName)
			return 0;
		gCmd = "load";
	}

	// run command
	if (!strcmp("uninstall", gCmd)) {
#if mxTraceCommands
		fprintf(stderr, "### uninstall\n");
#endif

		sprintf(out, "\r\n<?xs#%8.8X?>", self->currentMachine->value);
		fxWriteSerial(self, out, strlen(out));

		out[0] = 0;
		out[1] = 3;		// length
		out[2] = 2;		// uninstall cmd
		out[3] = 0xff;
		out[4] = 2;
		fxWriteSerial(self, out, out[1] + 2);
	}
	else if (!strcmp("install", gCmd)) {
#if mxTraceCommands
		fprintf(stderr, "### install\n");
#else
		fprintf(stderr, "Installing mod.");
#endif
		gInstallOffset = 0;

		sprintf(out, "\r\n<?xs#%8.8X?>", self->currentMachine->value);
		fxWriteSerial(self, out, strlen(out));

		out[0] = 0;
		out[1] = 3;		// length
		out[2] = 0x0f;	// get install space cmd
		out[3] = 0xff;
		out[4] = 0x0f;
		fxWriteSerial(self, out, out[1] + 2);

		gCmd = NULL;
		return 1;
	}
	else if (!strcmp("load", gCmd)) {
#if mxTraceCommands
		fprintf(stderr, "### load '%s'\n", gModuleName);
#endif

		sprintf(out, "\r\n<?xs#%8.8X?>", self->currentMachine->value);
		fxWriteSerial(self, out, strlen(out));

		memset(out, 0, sizeof(out));
		out[0] = 0;
		out[1] = 3 + (char)strlen(gModuleName) + 1;		// length
		out[2] = 10;		// run cmd
		out[3] = 0;
		out[4] = 0;
		strcpy(out + 5, gModuleName);
		fxWriteSerial(self, out, out[1] + 2);
	}

	gCmd = NULL;
	return 0;
}

void fxCommandReceived(txSerialTool self, void *bufferIn, int size)
{
	uint8_t *buffer = bufferIn;
	uint16_t resultId = (buffer[1] << 8) | buffer[2];
	int16_t resultCode = (buffer[3] << 8) | buffer[4];

	if (resultCode) {
		fprintf(stderr, "### fxCommandReceived: remote operation id %#04x failed with resultCode %d\n", (int)resultId, (int)resultCode);
		exit(-1);
	}
#if mxTraceCommands
	else
		fprintf(stderr, "### fxCommandReceived: remote operation id %#04x SUCCESS with resultCode %d\n", (int)resultId, (int)resultCode);
#endif

	if (0xff02 == resultId) {	// uninstall
#if mxTraceCommands
		fprintf(stderr, "### uninstalled\n");
#endif
		fxRestart(self);
		usleep(50000);
		return;
	}

	if (0xff0f == resultId) {	// get install space
		uint32_t installSpace;
		
		if (0 != resultCode) {
			fprintf(stderr, "getInstallSpace failed %d\n", (int)resultCode);
			return;
		}

		installSpace = (buffer[5] << 24) | (buffer[6] << 16) | (buffer[7] << 8) | buffer[8];
		
		gInstallFragmentSize = 512;
		if (size >= 13) {
			gInstallFragmentSize = (buffer[9] << 24) | (buffer[10] << 16) | (buffer[11] << 8) | buffer[12];
			if (gInstallFragmentSize > kInstallFragmentSizeMax)
				gInstallFragmentSize = kInstallFragmentSizeMax;
		}

#if mxTraceCommands
		fprintf(stderr, "### installSpace: %d, fragment size %d\n", (int)installSpace, (int)gInstallFragmentSize);
#endif
		if (installSpace < gInstallArchiveSize) {
			fprintf(stderr, "install failed. mod needs %d bytes, only %d available\n", (int)gInstallArchiveSize, (int)installSpace);
			return;
		} 
	
		fxInstallFragment(self);
		return;
	}

	if (0xe0e0 == resultId) {	// installed fragment
		if ((gInstallOffset / 4096) != ((gInstallOffset - gInstallFragmentSize) / 4096))
			fprintf(stderr, ".");
		fxInstallFragment(self);
		return;
	}
	if (0xe8e8 == resultId) {	// install complete
#if mxTraceCommands
		fprintf(stderr, "### install complete\n");
#endif
		fprintf(stderr, "..complete\n");
		fclose(gInstallFD);
		gInstallFD = NULL;
		fxRestart(self);
		usleep(10000);
		return;
	}

	if (0xff03 == resultId) {	// set preference
		if (gPrefs)
			fxSetPref(self);
		else {
			fxRestart(self);
			usleep(10000);
		}
		return;
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
	else if (c == '#')
		*flag = 2;
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

void fxReadNetworkBuffer(txSerialMachine machine, char* buffer, int size)
{
	if (!machine->suppress) {
		txSerialTool self = machine->tool;
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
}

void fxReadSerialBuffer(txSerialTool self, char* buffer, int size)
{
	char* src = buffer;
	char* srcLimit = src + size;
	int offset = self->index;
	char* dst = self->buffer + offset;
	char* dstLimit = self->buffer + mxBufferSize;
#if mxTrace
	fprintf(stderr, "%.*s", size, buffer);
#endif
	while (src < srcLimit) {
		if (dst == dstLimit) {
			txSerialMachine machine = self->currentMachine;
			if (machine && !machine->suppress)
				fxWriteNetwork(machine, self->buffer, mxBufferSize - mxTagSize);
			memmove(self->buffer, dstLimit - mxTagSize, mxTagSize);
			dst = self->buffer + mxTagSize;
			offset = mxTagSize;
		}
		*dst++ = *src++;
		offset++;
		if (gBinaryState) {
			if (1 == gBinaryState) {
				gBinaryLength = dst[-1] << 8;
				gBinaryState = 2;
			}
			else if (2 == gBinaryState) {
				gBinaryLength |= dst[-1];
				gBinaryState = 3;
			}
			if ((3 == gBinaryState) && ((2 + gBinaryLength) == (dst - self->buffer))) {
				fxCommandReceived(self, self->buffer + 2, gBinaryLength);

				dst = self->buffer;
				offset = 0;
				gBinaryState = 0;
			}
		}
		else
		if ((offset >= 2) && (dst[-2] == 13) && (dst[-1] == 10)) {
			uint8_t flag;
			uint32_t value;
			if ((offset >= mxTagSize) && fxMatchProcessingInstruction(dst - mxTagSize, &flag, &value)) {
				if (flag) {
					self->currentMachine = fxOpenNetwork(self, value);
					gBinaryState = 2 == flag;

					if (gRestarting)
						fxRestart(self);
				}
				else {
					fxCloseNetwork(self, value);
					gRestarting = 0;
				}
			}
			else if ((offset >= 10) && (dst[-10] == '<') && (dst[-9] == '/') && (dst[-8] == 'x') && (dst[-7] == 's') && (dst[-6] == 'b') && (dst[-5] == 'u') && (dst[-4] == 'g') && (dst[-3] == '>')) {
				txSerialMachine machine = self->currentMachine;
				if ((1 == machine->receiveCount) && !self->firstMachine->nextMachine) {	// first command when after transitioning from 0 to 1 machines
					if (fxInitializeTarget(self))
						self->currentMachine->suppress = 1;
				}
				if (machine && !machine->suppress)
					fxWriteNetwork(machine, self->buffer, offset);
			}
			else {
				dst[-2] = 0;

				// esp32 core dump - capture to temporary file then run espcoredump.py 
				if (TOOLS_BIN && elfPath && !gCoreDump && strstr(self->buffer, "================= CORE DUMP START =================")) {
					char *msg = "<xsbug><log># ESP32 Core Dump!!\n</log></xsbug>\r\n";
					fxWriteNetwork(self->currentMachine, msg, strlen(msg));

					strcpy(gCoreDumpPath, elfPath);
					strcpy(strrchr(gCoreDumpPath, '/') + 1, "coredump.txt");
					fprintf(stderr, "Writing CORE DUMP to %s\n", gCoreDumpPath);
					gCoreDump = fopen(gCoreDumpPath, "wb");
					dst = self->buffer;
					offset = 0;
					continue;
				}
				if (gCoreDump) {
					if (strstr(self->buffer, "================= CORE DUMP END =================")) {
						fclose(gCoreDump);
						gCoreDump = NULL;

						char *output;
						char command[2048];
						sprintf(command, "espcoredump.py info_corefile -t b64 -c %s %s", gCoreDumpPath, elfPath);
						fprintf(stderr, "%s\n", command);
						systemCommand(command, &output);
						fprintf(stderr, "%s\n", output);
						free(output);
					}
					else {
						fwrite(self->buffer, 1, strlen(self->buffer), gCoreDump);
						fwrite("\n", 1, 1, gCoreDump);
					}
					dst = self->buffer;
					offset = 0;
					continue;
				}

				if (offset > 2) fprintf(stderr, "%s\n", self->buffer);				

				if (TOOLS_BIN && elfPath && strstr(self->buffer, "gdb stub")) {
					gdbMode = 1;
				}

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
		else if (gdbMode && (offset == 7)) {
			char a, b;
			int match, check, sum;
			*dst = 0;
			fprintf(stderr, "%s", self->buffer);
			match = sscanf(self->buffer, "$T%c%c#%2x", &a, &b, &check);
			sum = ((int)'T' + (int)a + (int)b) & 255;
			if ((match == 3) && (check == sum)) {
				char* args[9];
				char baud[256];
				char target[256];
				sprintf(baud, "set serial baud %d", 115200);
				sprintf(target, "target remote %s", self->path);
				args[0] = TOOLS_BIN;
				args[1] = "-ex";
				args[2] = baud;
				args[3] = "-ex";
				args[4] = target;
				args[5] = "-ex";
				args[6] = "interrupt";
				args[7] = elfPath;
				args[8] = NULL;

				fxCloseNetwork(self, 0);
				fxCloseSerial(self);
				execvp(args[0], args);
				exit(0);
			}
		}
	}
	self->index = offset;
}

void fxRestart(txSerialTool self)
{
#if mxTraceCommands
	fprintf(stderr, "### fxRestart\n");
#endif

	if (self->currentMachine) {	// send a software restart request for boards with no RTS to toggle
		char out[32];

		sprintf(out, "\r\n<?xs#%8.8X?>", self->currentMachine->value);
		fxWriteSerial(self, out, strlen(out));

		out[0] = 0;
		out[1] = 1;	// length
		out[2] = 1;		// restart
		fxWriteSerial(self, out, 3);
	}

	fxRestartSerial(self);
	gRestarting = 1;
}

void fxSetTime(txSerialTool self, txSerialMachine machine)
{
	time_t time;
	int gmt = 0;
	int dst = 0;
	char out[32];

#if mxTraceCommands
	fprintf(stderr, "### set time\n");
#endif

#if mxWindows
	{
		struct _timeb tb;
		_ftime(&tb);
		time = (long)tb.time;
		gmt = -tb.timezone * 60;
		if (tb.dstflag) {
			dst = 60 * 60;
		}
	}
#else
	{
		struct timeval tv;
		struct tm *tm;
		gettimeofday(&tv, NULL);
		time = tv.tv_sec;
		tm = localtime(&time);
		gmt = tm->tm_gmtoff;
		if (tm->tm_isdst) {
			dst = 60 * 60;
			gmt -= dst;
		}
	}
#endif

	sprintf(out, "\r\n<?xs#%8.8X?>", machine->value);
	fxWriteSerial(self, out, strlen(out));

	out[0] = 0;
	out[1] = 15;	// length
	out[2] = 9;		// set time
	out[3] = 0;
	out[4] = 0;
	out[5] = (time >> 24) & 255;
	out[6] = (time >> 16) & 255;
	out[7] = (time >> 8) & 255;
	out[8] = time & 255;
	out[9] = (gmt >> 24) & 255;
	out[10] = (gmt >> 16) & 255;
	out[11] = (gmt >> 8) & 255;
	out[12] = gmt & 255;
	out[13] = (dst >> 24) & 255;
	out[14] = (dst >> 16) & 255;
	out[15] = (dst >> 8) & 255;
	out[16] = dst & 255;
	fxWriteSerial(self, out, 17);
}

// erases first block and writes gInstallFragmentSize bytes of header
// skips kInstallSkipFragmentSize bytes
// writes remaining for data
// backs up to offset 0 and writes the kInstallSkipFragmentSize bytes skipped
// this ensures that the mod header is only valid if all bytes are received
//@@ add GET VERSION
void fxInstallFragment(txSerialTool self)
{
	char preamble[32];
	char out[kInstallFragmentSizeMax + 16];
	int use = (0 == gInstallOffset) ? kInstallInitialFragmentSize : gInstallFragmentSize;
	uint8_t id = 0xe0;

	fseek(gInstallFD, gInstallOffset, SEEK_SET);
	use = fread(out + 9, 1, use, gInstallFD);
	if (0 == use) {
#if mxTraceCommands
		fprintf(stderr, "### update install header\n");
#endif
		gInstallOffset = kInstallInitialFragmentSize;
		fseek(gInstallFD, gInstallOffset, SEEK_SET);
		use = fread(out + 9, 1, kInstallSkipFragmentSize, gInstallFD);
		id = 0xe8;
	}

#if mxTraceCommands
	fprintf(stderr, "### install fragment @ %d size %d\n", gInstallOffset, use);
#endif

	sprintf(preamble, "\r\n<?xs#%8.8X?>", self->currentMachine->value);
	fxWriteSerial(self, preamble, strlen(preamble));

	out[0] = ((use + 7) >> 8) & 0xff;		// length high
	out[1] = (use + 7) & 0xff;		// length low
	out[2] = 3;		// install cmd
	out[3] = id;	// id high
	out[4] = id;	// id low
	out[5] = (gInstallOffset >> 24) & 255;
	out[6] = (gInstallOffset >> 16) & 255;
	out[7] = (gInstallOffset >> 8) & 255;
	out[8] = gInstallOffset & 255;

	fxWriteSerial(self, out, use + 4 + 5);

	gInstallOffset += use;
}

int mapHex(char c)
{
	if (('a' <= c) && (c <= 'f'))
		return 10 + c - 'a';
	if (('A' <= c) && (c <= 'F'))
		return 10 + c - 'A';
	if (('0' <= c) && (c <= '9'))
		return c - '0';

	fprintf(stderr, "bad hex digit %c\n", c);
	exit(1);
}

void fxSetPref(txSerialTool self)
{
	char preamble[32];
	char out[5];
	Pref p = gPrefs;
	if (!p) return;

	gPrefs = p->next;

#if mxTraceCommands
	fprintf(stderr, "### set preference %s.%s=%s\n", p->domain, p->name, p->value);
#endif

	sprintf(preamble, "\r\n<?xs#%8.8X?>", self->currentMachine->value);
	fxWriteSerial(self, preamble, strlen(preamble));

	int size = strlen(p->domain) + 1 + strlen(p->name) + 1 + 1 + strlen(p->value);
	size += 3;
	out[0] = (size >> 8) & 0xff;	// length high
	out[1] = size & 0xff;			// length low
	out[2] = 4;						// set preference cmd
	out[3] = 0xff;					// id high
	out[4] = 0x03;					// id low
	fxWriteSerial(self, out, 5);
	fxWriteSerial(self, p->domain, strlen(p->domain) + 1);
	fxWriteSerial(self, p->name, strlen(p->name) + 1);
	fxWriteSerial(self, &p->kind, 1);
	fxWriteSerial(self, p->value, strlen(p->value));
}
