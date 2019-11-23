/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
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

/*
	Serial port implementation - similar to Arduino api
*/

#define MAX_TERMINATORS 16

typedef struct {
	int interface;
	int baud;
	int databits;
	char parity[2];
	int stopbits;
	int rxBufferSize;
	int rxPort;
	int rxPin;
	int txBufferSize;
	int txPort;
	int txPin;
} modSerialConfigRecord, *modSerialConfig;

typedef struct modSerialDeviceRecord modSerialDeviceRecord;
typedef modSerialDeviceRecord *modSerialDevice;

modSerialDevice modSerialDevice_setup(modSerialConfig config);
void modSerial_setBaudrate(modSerialDevice serial, int speed);
void modSerial_setTimeout(modSerialDevice serial, int timeoutMS);

void modSerial_puts(modSerialDevice serial, uint8_t *buf, int len);
void modSerial_putc(modSerialDevice serial, int c);
int modSerial_gets(modSerialDevice serial, uint8_t *buf, int len);
int modSerial_getc(modSerialDevice serial);
void modSerial_flush(modSerialDevice serial);
int modSerial_txBusy(modSerialDevice serial);
void modSerial_teardown(modSerialDevice serial);

int modSerial_peek(modSerialDevice serial);
int modSerial_available(modSerialDevice serial);

int modSerial_write(modSerialDevice serial, uint8_t *buf, int len);
int modSerial_read(modSerialDevice serial, uint8_t *buf, int len);

typedef void (*modSerialPollCallbackFn)(uint8_t *data, int len, void *refcon);
void modSerial_poll(modSerialDevice serial, int interval, char *terminators, uint8_t trim, uint8_t *data, int dataLen, modSerialPollCallbackFn callback, void *refcon);
void modSerial_endPoll(modSerialDevice serial);

#define debugout(a, ...) do { char foo[128]; sprintf(foo, a, __VA_ARGS__); modLog_transmit(foo); } while (0)
