/*
 * Copyright (c) 2019-2026 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
	I2C - uing ESP-IDF
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_* values
#include "xsHost.h"			// esp platform support
#include "_i2c.h"			// I2C host hooks

#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "builtinCommon.h"

#ifndef MODDEF_ECMA419_I2C_PINS_COMPATIBLE
	#define MODDEF_ECMA419_I2C_PINS_COMPATIBLE 1
#endif

struct I2CRecord {
	uint32_t					hz;
	uint32_t					data;
	uint32_t					clock;
	uint32_t					timeout;
	uint8_t						address;		// 7-bit
	uint8_t						pullup;
	uint8_t						stop;			// SMBus
	i2c_port_num_t				port;
	xsSlot						obj;
	struct I2CRecord			*next;

	i2c_master_bus_handle_t 	bus;
	i2c_master_dev_handle_t		device;
};
typedef struct I2CRecord I2CRecord;
typedef struct I2CRecord *I2C;

static I2C gI2C;

static uint8_t usingPins(uint32_t data, uint32_t clock);

static void _xs_i2c_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static SemaphoreHandle_t gI2CMutex;

static void *modI2CValidate(xsMachine *the, xsSlot *instance);
static uint8_t modI2CDeactivate(void *instanceData);

#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
	__attribute__((weak)) void modI2CUninit(void *);	// pins I2C
	uint8_t i2cActivate(struct I2CRecord *i2c);			// called by pins I2C with NULL
#endif

static const xsI2CHostHooksRecord ICACHE_RODATA_ATTR xsI2CHooks = {
	.hooks = {
		_xs_i2c_destructor,
		_xs_i2c_mark,
		"i2c"
	},
	.doValidate = modI2CValidate,
	.doDeactivate = modI2CDeactivate,
};

static const xsHostHooks ICACHE_RODATA_ATTR xsSMBusHooks = {
	_xs_i2c_destructor,
	_xs_i2c_mark,
};

void _xs_i2c_constructor(xsMachine *the)
{
	I2C i2c;
	int data, clock, hz, address;
	int timeout = 400;		// ms (-1 = wait forever)
	uint8_t pullup = 1;
	i2c_port_num_t port = -1;			// none requested
	uint8_t busIsNew = 0;
	i2c_master_bus_handle_t bus = C_NULL;
	i2c_master_dev_handle_t device = C_NULL;

	if (NULL == gI2CMutex)
		gI2CMutex = xSemaphoreCreateMutex();

	xsmcVars(1);

	if (!xsmcHas(xsArg(0), xsID_data))
		xsRangeError("data required");
	if (!xsmcHas(xsArg(0), xsID_clock))
		xsRangeError("clock required");
	if (!xsmcHas(xsArg(0), xsID_address))
		xsRangeError("address required");

	xsmcGet(xsVar(0), xsArg(0), xsID_data);
	data = builtinGetPin(the, &xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_clock);
	clock = builtinGetPin(the, &xsVar(0));

	if (usingPins(data, clock))
		;
	else if (!builtinIsPinFree(data) || !builtinIsPinFree(clock))
		xsRangeError("inUse");

	xsmcGet(xsVar(0), xsArg(0), xsID_address);
	address = builtinGetPin(the, &xsVar(0));
	if ((address < 0) || (address > 127))
		xsRangeError("invalid address");

	for (i2c = gI2C; i2c; i2c = i2c->next) {
		if ((i2c->data == data) && (i2c->clock == clock) && (i2c->address == address))
			xsRangeError("duplicate address");
	}

	if (!GPIO_IS_VALID_OUTPUT_GPIO(clock) || !GPIO_IS_VALID_OUTPUT_GPIO(data))
		xsRangeError("unusable pins");

	xsmcGet(xsVar(0), xsArg(0), xsID_hz);
	hz = xsmcToInteger(xsVar(0));
	if ((hz <= 0) || (hz > 20000000))
		xsRangeError("invalid hz");

	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0));
		if (timeout < -1)
			xsRangeError("invalid timeout");
	}

	if (xsmcHas(xsArg(0), xsID_pullup)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pullup);
		pullup = xsmcToBoolean(xsVar(0));
	}

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
		if ((port < 0) || (port >= I2C_NUM_MAX))
			xsRangeError("invalid port");
	}

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	xSemaphoreTake(gI2CMutex, portMAX_DELAY);

	for (i2c = gI2C; i2c; i2c = i2c->next) {
		if (i2c->bus && (i2c->data == data) && (i2c->clock == clock) && (i2c->port == port) && (i2c->pullup == pullup)) {
			bus = i2c->bus;
			break;
		}
	}

	xSemaphoreGive(gI2CMutex);

	if (!bus) {
#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
		if (modI2CUninit)
			modI2CUninit(C_NULL);		// make pins release these pins
#endif
		i2c_master_bus_config_t busC = {
			.i2c_port = port,
			.sda_io_num = data,
			.scl_io_num = clock,
			.clk_source = I2C_CLK_SRC_DEFAULT,
			.flags.enable_internal_pullup = pullup,
		};
		if (ESP_OK != i2c_new_master_bus(&busC, &bus))
			xsUnknownError("can't create bus");
		busIsNew = 1;
	}

	i2c_device_config_t deviceC = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = address,
		.scl_speed_hz = hz,
	};
	if (ESP_OK != i2c_master_bus_add_device(bus, &deviceC, &device)) {
		if (busIsNew)
			i2c_del_master_bus(bus);
		xsUnknownError("can't create device");
	}

	i2c = c_malloc(sizeof(I2CRecord));
	if (!i2c) {
		i2c_master_bus_rm_device(device);
		if (busIsNew)
			i2c_del_master_bus(bus);
		xsRangeError("no memory");
	}

	xsmcSetHostData(xsThis, i2c);
	i2c->obj = xsThis;
	xsRemember(i2c->obj);
	i2c->clock = clock;
	i2c->data = data;
	i2c->hz = hz;
	i2c->address = address;
	i2c->timeout = timeout;
	i2c->pullup = pullup;
	i2c->port = port;
	i2c->bus = bus;
	i2c->device = device;

	i2c->next = gI2C;
	gI2C = i2c;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsI2CHooks);

	builtinUsePin(data);
	builtinUsePin(clock);
}

void _xs_i2c_destructor(void *data)
{
	I2C i2c = data;
	if (!i2c)
		return;

	if (i2c->device) {
		i2c_master_bus_rm_device(i2c->device);
		i2c->device = C_NULL;
	}

	xSemaphoreTake(gI2CMutex, portMAX_DELAY);
	I2C walker;
	if (i2c->bus) {
		int busCount = 0;
		for (walker = gI2C; walker; walker = walker->next) {
			if (walker->bus == i2c->bus)
				busCount++;
		}
		if (1 == busCount) {
			i2c_del_master_bus(i2c->bus);
			i2c->bus = C_NULL;
		}
	}
	xSemaphoreGive(gI2CMutex);

	if (gI2C == i2c)
		gI2C = i2c->next;
	else {
		for (walker = gI2C; walker; walker = walker->next) {
			if (walker->next == i2c) {
				walker->next = i2c->next;
				break;
			}
		}
	}

	if (!usingPins(i2c->data, i2c->clock)) {
		builtinFreePin(i2c->data);
		builtinFreePin(i2c->clock);
	}

	c_free(i2c);
}

void _xs_i2c_mark(xsMachine*, void *, xsMarkRoot)
{
}

void _xs_i2c_close(xsMachine *the)
{
	I2C i2c = xsmcGetHostData(xsThis);
	if (i2c && xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsI2CHooks)) {
		xsForget(i2c->obj);
		_xs_i2c_destructor(i2c);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void _xs_i2c_read(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsI2CHooks);
	xsUnsignedValue length;
	int err;
	uint8_t stop = true;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));

	if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		xsResult = xsArg(0);
		xsmcGetBufferWritable(xsResult, &buffer, &length);
		xsmcSetInteger(xsResult, length);
	}
	else {
 		length = xsmcToInteger(xsArg(0));
		buffer = xsmcSetArrayBuffer(xsResult, NULL, length);
	}

#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");
	err = i2c_master_receive(i2c->device, buffer, length, i2c->timeout);
	xSemaphoreGive(gI2CMutex);
#else
	err = i2c_master_receive(i2c->device, buffer, length, i2c->timeout);
#endif
	if (ESP_OK != err)
		xsUnknownError("read failed");
}

void _xs_i2c_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsI2CHooks);
	int err;
	xsUnsignedValue length;
	uint8_t stop = true;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));

	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");
#endif
	if (length)
		err = i2c_master_transmit(i2c->device, buffer, length, i2c->timeout);
	else //@@ write quick unsupported through transmit, but probe seems similar-ish
		err = i2c_master_probe(i2c->bus, i2c->address, 1000);
#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
	xSemaphoreGive(gI2CMutex);
#endif

	if (length == 0) {
		if (ESP_OK != err)
			xsmcSetInteger(xsResult, 1);
	}
	else if (ESP_OK != err)
		xsUnknownError("write failed");
}

void _xs_i2c_writeRead(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsI2CHooks);
	int err;
	xsUnsignedValue lengthWrite, lengthRead;
	void *bufferWrite, *bufferRead;
	uint8_t stop = true;

	if (xsmcArgc > 2)
		stop = xsmcToBoolean(xsArg(2));

	if (xsReferenceType == xsmcTypeOf(xsArg(1))) {
		xsResult = xsArg(1);
		xsmcGetBufferWritable(xsResult, &bufferRead, &lengthRead);
		xsmcSetInteger(xsResult, lengthRead);
	}
	else {
 		lengthRead = xsmcToInteger(xsArg(1));
		bufferRead = xsmcSetArrayBuffer(xsResult, NULL, lengthRead);
	}

	xsmcGetBufferReadable(xsArg(0), &bufferWrite, &lengthWrite);

#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");
	err = i2c_master_transmit_receive(i2c->device, bufferWrite, lengthWrite, bufferRead, lengthRead, i2c->timeout);
	xSemaphoreGive(gI2CMutex);
#else
	err = i2c_master_transmit_receive(i2c->device, bufferWrite, lengthWrite, bufferRead, lengthRead, i2c->timeout);
#endif
	if (ESP_OK != err)
		xsUnknownError("writeRead failed");
}

uint8_t usingPins(uint32_t data, uint32_t clock)
{
	I2C walker;

	for (walker = gI2C; walker; walker = walker->next) {
		if ((walker->data == data) && (walker->clock == clock))
			return 1;
	}

	return 0;
}

void *modI2CValidate(xsMachine *the, xsSlot *instance)
{
	return xsmcGetHostDataValidate(*instance, (xsHostHooks *)&xsI2CHooks);
}

uint8_t modI2CDeactivate(void *instanceData)
{
	return 1;
}

#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
uint8_t i2cActivate(I2C i2c)
{
	if (C_NULL == gI2CMutex)
		return 1;		// pins may call before any ECMA-419 I2C exists

	xSemaphoreTake(gI2CMutex, portMAX_DELAY);

	if (i2c) {
		if (i2c->device)
			return 1;	// already alive, mutex held

		i2c_master_bus_handle_t bus = C_NULL;
		I2C walker;
		for (walker = gI2C; walker; walker = walker->next) {
			if ((walker != i2c) && walker->bus &&
				(walker->data == i2c->data) && (walker->clock == i2c->clock) &&
				(walker->port == i2c->port) && (walker->pullup == i2c->pullup)) {
				bus = walker->bus;
				break;
			}
		}

		uint8_t busIsNew = 0;
		if (!bus) {
			if (modI2CUninit)
				modI2CUninit(C_NULL);		// make pins release these pins

			i2c_master_bus_config_t busC = {
				.i2c_port = i2c->port,
				.sda_io_num = i2c->data,
				.scl_io_num = i2c->clock,
				.clk_source = I2C_CLK_SRC_DEFAULT,
				.flags.enable_internal_pullup = i2c->pullup,
			};
			if (ESP_OK != i2c_new_master_bus(&busC, &bus)) {
				xSemaphoreGive(gI2CMutex);
				return 0;
			}
			busIsNew = 1;
		}

		i2c_device_config_t deviceC = {
			.dev_addr_length = I2C_ADDR_BIT_LEN_7,
			.device_address = i2c->address,
			.scl_speed_hz = i2c->hz,
		};
		i2c_master_dev_handle_t device = C_NULL;
		if (ESP_OK != i2c_master_bus_add_device(bus, &deviceC, &device)) {
			if (busIsNew)
				i2c_del_master_bus(bus);
			xSemaphoreGive(gI2CMutex);
			return 0;
		}

		i2c->bus = bus;
		i2c->device = device;
		return 1;		// mutex held
	}

	// pins asked us to step aside: tear down all persistent buses+devices
	I2C walker;
	for (walker = gI2C; walker; walker = walker->next) {
		if (walker->device) {
			i2c_master_bus_rm_device(walker->device);
			walker->device = C_NULL;
		}
	}
	for (walker = gI2C; walker; walker = walker->next) {
		if (walker->bus) {
			i2c_master_bus_handle_t bus = walker->bus;
			I2C w2;
			for (w2 = walker; w2; w2 = w2->next) {
				if (w2->bus == bus)
					w2->bus = C_NULL;
			}
			i2c_del_master_bus(bus);
		}
	}

	xSemaphoreGive(gI2CMutex);
	return 1;
}
#endif

/*
	async experiment
*/

typedef struct TransactionRecord TransactionRecord;
typedef TransactionRecord *Transaction;

struct TransactionRecord {
	struct TransactionRecord  *next;

	I2C					i2c;
	xsMachine			*the;
	xsSlot				callback;
	xsSlot				readBuffer;
	uint8_t				hasCallback;
	uint8_t				hasReadBuffer;
	uint8_t				operation;
	uint8_t				bufferLength;
	uint8_t				processing;		// 0 unstarted, 1 i2c transaction in progress, 2 i2c transaction done
	uint8_t				reg;			// smbus register
	int					err;

	uint8_t				buffer[];
};

enum {
	kOperationCancelled = 1,
	kOperationClose,
	kOperationWrite,
	kOperationRead,
	kOperationReadUint8,
	kOperationReadUint16,
	kOperationWriteUint8,
	kOperationWriteUint16,
	kOperationReadBuffer,
	kOperationWriteBuffer,
	kOperationSendByte,
	kOperationReceiveByte,
	kOperationWriteQuick,
	kOperationReadQuick,
};

static SemaphoreHandle_t gI2CTaskMutex;
static TaskHandle_t gI2CTask;
static Transaction gTransactions;

void i2cDeliver(void *theIn, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsMachine *the = theIn;
	Transaction transaction = refcon;
	uint8_t operation = transaction->operation;
	xsSlot obj = transaction->i2c->obj;

	xSemaphoreTake(gI2CTaskMutex, portMAX_DELAY);
	Transaction walker = gTransactions, prev = NULL;
	while (walker) {
		if (walker == transaction) {
			if (prev)
				prev->next = transaction->next;
			else
				gTransactions = transaction->next;
			break;
		}
		prev = walker;
		walker = walker->next;
	} 
	xSemaphoreGive(gI2CTaskMutex);

	if (kOperationClose == operation) {
		xsForget(transaction->i2c->obj);
		_xs_i2casync_destructor(transaction->i2c);	// all resources must be released before invoking callback ... so callback could call new again
	}

	if (transaction->hasCallback) {
		xsBeginHost(the);
		xsmcVars(2);
		if (transaction->err) {
			xsmcSetInteger(xsVar(0), transaction->err);
			xsVar(0) = xsNew1(xsGlobal, xsID_Error, xsVar(0));
		}
		else
			xsmcSetNull(xsVar(0));

		switch (operation) {
			case kOperationWrite:
			case kOperationWriteUint8:
			case kOperationWriteUint16:
			case kOperationWriteBuffer:
			case kOperationSendByte:
			case kOperationWriteQuick:
			case kOperationReadQuick:
			case kOperationClose:
			case kOperationCancelled:
				xsCallFunction1(transaction->callback, obj, xsVar(0));
				break;
			
			case kOperationRead:
			case kOperationReadBuffer:
				if (transaction->hasReadBuffer) {
					void *buffer;
					xsUnsignedValue length;

					xsmcSetInteger(xsVar(1), transaction->bufferLength);
					xsmcGetBufferWritable(transaction->readBuffer, &buffer, &length);
					if (transaction->bufferLength <= length)
						c_memmove(buffer, transaction->buffer, transaction->bufferLength);
					else {			// buffer now too small to hold result
						xsmcSetInteger(xsVar(0), -3);
						xsVar(0) = xsNew1(xsGlobal, xsID_Error, xsVar(0));
					}
				}
				else
					xsmcSetArrayBuffer(xsVar(1), transaction->buffer, transaction->bufferLength);

				xsCallFunction2(transaction->callback, obj, xsVar(0), xsVar(1));
				break;

			case kOperationReadUint8:
			case kOperationReceiveByte:
				xsmcSetInteger(xsVar(1), transaction->buffer[0]);
				xsCallFunction2(transaction->callback, obj, xsVar(0), xsVar(1));
				break;
			
			case kOperationReadUint16:
				xsmcSetInteger(xsVar(1), *(uint16_t *)transaction->buffer);
				xsCallFunction2(transaction->callback, obj, xsVar(0), xsVar(1));
				break;
		}
		xsEndHost(the);
	}

	if (transaction->hasCallback)
		xsForget(transaction->callback);
	if (transaction->hasReadBuffer)
		xsForget(transaction->readBuffer);
	c_free(transaction);
}

static void i2cTask(void *pvParameter)
{
	while (true) {
		uint32_t newState;

		xTaskNotifyWait(0, 0, &newState, portMAX_DELAY);

		while (gTransactions) {
			// find next unprocessed transaction
			xSemaphoreTake(gI2CTaskMutex, portMAX_DELAY);
			Transaction transaction = gTransactions;
			while (transaction && (0 != transaction->processing))
				transaction = transaction->next;
			if (!transaction) {
				xSemaphoreGive(gI2CTaskMutex);
				break;
			}
			transaction->processing = 1;

			xSemaphoreGive(gI2CTaskMutex);

			if (kOperationClose == transaction->operation)
				;
			else if (kOperationCancelled == transaction->operation)
				transaction->err = -2;
#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
			else if (!i2cActivate(transaction->i2c))
				transaction->err = -1;
#endif
			else {
				I2C i2c = transaction->i2c;
				switch (transaction->operation) {
					case kOperationRead:
					case kOperationReceiveByte:
					case kOperationReadQuick:
						transaction->err = i2c_master_receive(i2c->device, transaction->buffer, transaction->bufferLength, i2c->timeout);
						break;

					case kOperationReadUint8:
					case kOperationReadUint16:
					case kOperationReadBuffer:
						transaction->err = i2c_master_transmit_receive(i2c->device, &transaction->reg, 1, transaction->buffer, transaction->bufferLength, i2c->timeout);
						break;

					case kOperationWrite:
					case kOperationSendByte:
						if (transaction->bufferLength)
							transaction->err = i2c_master_transmit(i2c->device, transaction->buffer, transaction->bufferLength, i2c->timeout);
						else //@@ write quick unsupported thorugh transmit, but probe seems similar-ish
							transaction->err = i2c_master_probe(i2c->bus, i2c->address, 1000);
						break;

					case kOperationWriteUint8:
					case kOperationWriteUint16:
					case kOperationWriteBuffer: {
						i2c_master_transmit_multi_buffer_info_t buffers[2] = {
							{.write_buffer = (uint8_t*)&transaction->reg, .buffer_size = 1},
							{.write_buffer = (uint8_t*)transaction->buffer, .buffer_size = transaction->bufferLength},
						};
						transaction->err = i2c_master_multi_buffer_transmit(i2c->device, buffers, 2, i2c->timeout);
						} break;

					case kOperationWriteQuick:	//@@ write quick unsupported through transmit, but probe seems similar-ish
						transaction->err = i2c_master_probe(i2c->bus, i2c->address, 1000);
						break;
				}
#if MODDEF_ECMA419_I2C_PINS_COMPATIBLE
				xSemaphoreGive(gI2CMutex);
#endif
			}

			transaction->processing = 2;

			if (transaction->hasCallback || transaction->hasReadBuffer)
				modMessagePostToMachine(transaction->the, NULL, 0, i2cDeliver, transaction);
			else
				i2cDeliver(NULL, transaction, NULL, 0);		// short-circuit if no dependency on VM
		}
	}
}

static Transaction newI2CTransaction(xsMachine *the, I2C i2c, uint8_t operation, xsUnsignedValue bufferLength, xsIntegerValue callbackIndex)
{
	Transaction transaction;

	if (bufferLength > 255)
		xsUnknownError("invalid buffer");

	// set-up transaction record
	transaction = c_calloc(sizeof(TransactionRecord) + bufferLength, 1);
	if (!transaction)
		xsUnknownError("no memory");

	transaction->i2c = i2c;
	transaction->the = the;
	transaction->hasCallback = callbackIndex >= 0;
	if (transaction->hasCallback) {
		transaction->callback = xsArg(callbackIndex);
		xsRemember(transaction->callback);
	}
	transaction->operation = operation;
	transaction->bufferLength = (uint8_t)bufferLength;
	
	return transaction;
}

static Transaction newSMBusTransaction(xsMachine *the, I2C i2c, uint8_t operation, xsUnsignedValue bufferLength, xsIntegerValue callbackIndex, uint8_t reg)
{
	Transaction transaction = newI2CTransaction(the, i2c, operation, bufferLength, callbackIndex);

	transaction->reg = reg;

	return transaction;
}

static void queueTransaction(Transaction transaction)
{	
	xSemaphoreTake(gI2CTaskMutex, portMAX_DELAY);
		if (NULL == gTransactions)
			gTransactions = transaction;
		else {
			Transaction walker;
			for (walker = gTransactions; walker->next; walker = walker->next)
				;
			walker->next = transaction;
		}
	xSemaphoreGive(gI2CTaskMutex);
	
	// notify task transaction ready
	xTaskNotify(gI2CTask, 1, eSetValueWithoutOverwrite);
}

void _xs_i2casync_destructor(void *data)
{
	if (!data)
		return;

	// should only reach here on tear down of VM with open i2casync instance

	// cancel pending transactions on this instance
	xSemaphoreTake(gI2CTaskMutex, portMAX_DELAY);
	Transaction walker;
	uint8_t done = true;
	for (walker = gTransactions; walker; walker = walker->next) {
		if (walker->i2c == data) {
			walker->operation = kOperationCancelled;
			walker->hasCallback = false;
			walker->hasReadBuffer = false;

			if (1 == walker->processing)
				done = false;
		}
	}
	xSemaphoreGive(gI2CTaskMutex);

	// wait for transactions to finish processing (should be no more than one)
	while (!done) {
		modDelayMilliseconds(1);
		done = true;
		xSemaphoreTake(gI2CTaskMutex, portMAX_DELAY);
		for (walker = gTransactions; walker && done; walker = walker->next) {
			if ((walker->i2c == data) && (1 == walker->processing))
				done = false;
		}
		xSemaphoreGive(gI2CTaskMutex);
	}
	_xs_i2c_destructor(data);
}

void _xs_i2casync_constructor(xsMachine *the)
{
	_xs_i2c_constructor(the);

	if (NULL == gI2CTaskMutex) {
		gI2CTaskMutex = xSemaphoreCreateMutex();
		xTaskCreate(i2cTask, "i2c", 2048 + XT_STACK_EXTRA_CLIB, NULL, 10, &gI2CTask);
	}
}

void asyncclose(xsMachine *the, const xsHostHooks *hooks)
{
	I2C i2c = xsmcGetHostData(xsThis);
	if (!i2c) return;

	xsmcGetHostDataValidate(xsThis, (xsHostHooks *)hooks);

	Transaction walker, transaction = newI2CTransaction(the, i2c, kOperationClose, 0, (xsmcArgc > 0) ? 0 : -1);		// i2c transaction here is correct even for smbus instances

	xSemaphoreTake(gI2CTaskMutex, portMAX_DELAY);

	for (walker = gTransactions; walker; walker = walker->next) {
		if ((walker->i2c == i2c) && (0 == walker->processing))
			walker->operation = kOperationCancelled;
	}

	xSemaphoreGive(gI2CTaskMutex);
	
	queueTransaction(transaction);

	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

void _xs_i2casync_close(xsMachine *the)
{
	asyncclose(the, (xsHostHooks *)&xsI2CHooks);
}

void _xs_i2casync_read(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsI2CHooks);
	xsUnsignedValue length;
	uint8_t stop = true;
	uint8_t hasCallback = false, hasReadBuffer = false;
	int callbackIndex = -1;
	Transaction transaction;

	if (xsmcArgc > 1) {
		if (xsmcIsCallable(xsArg(1)))
			callbackIndex = 1;
		else {
			stop = xsmcToBoolean(xsArg(1));
			if (xsmcArgc > 2)
				callbackIndex = 2;
		}
	}

	if (xsReferenceType == xsmcTypeOf(xsArg(0))) {
		void *buffer;
		xsmcGetBufferWritable(xsArg(0), &buffer, &length);
		hasReadBuffer = true;
	}
	else
 		length = xsmcToInteger(xsArg(0));

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationRead, length, callbackIndex);
	transaction->hasReadBuffer = hasReadBuffer;
	if (hasReadBuffer) {
		transaction->readBuffer = xsArg(0);
		xsRemember(transaction->readBuffer);
	}

	queueTransaction(transaction);
}

void _xs_i2casync_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsI2CHooks);
	xsUnsignedValue length;
	uint8_t stop = true;
	void *buffer;
	Transaction transaction;
	int callbackIndex = -1;

	if (xsmcArgc > 1) {
		if (xsmcIsCallable(xsArg(1)))
			callbackIndex = 1;
		else {
			stop = xsmcToBoolean(xsArg(1));
			if (xsmcArgc > 2)
				callbackIndex = 2;
		}
	}

	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationWrite, length, callbackIndex);
	c_memmove(transaction->buffer, buffer, length);
	
	queueTransaction(transaction);
}

void _xs_smbusasync_destructor(void *data)
{
	_xs_i2casync_destructor(data);
}

void _xs_smbusasync_constructor(xsMachine *the)
{
	_xs_i2casync_constructor(the);
	
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsI2CHooks);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsSMBusHooks);
	i2c->stop = 0;
	if (xsmcHas(xsArg(0), xsID_stop)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_stop);
		i2c->stop = xsmcToBoolean(xsVar(0));
	}
}

void _xs_smbusasync_close(xsMachine *the)
{
	asyncclose(the, (xsHostHooks *)&xsSMBusHooks);
}

void _xs_smbusasync_readUint8(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationReadUint8, sizeof(uint8_t), (xsmcArgc > 1) ? 1 : -1, reg);

	queueTransaction(transaction);
}

void _xs_smbusasync_readUint16(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));
	uint8_t bigEndian = 0; 
	int callbackIndex = -1;

	if (xsmcArgc > 1) {
		if (xsmcIsCallable(xsArg(1)))
			callbackIndex = 1;
		else {
			bigEndian = xsmcToBoolean(xsArg(1));
			if (xsmcArgc > 2)
				callbackIndex = 2;
		}
	}

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationReadUint16, sizeof(uint16_t), callbackIndex, reg);

	queueTransaction(transaction);
}

void _xs_smbusasync_writeUint8(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));
	uint8_t byte = xsmcToInteger(xsArg(1));

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationWriteUint8, sizeof(uint8_t), (xsmcArgc > 2) ? 2 : -1, reg);
	c_memmove(transaction->buffer, &byte, sizeof(byte));

	queueTransaction(transaction);
}

void _xs_smbusasync_writeUint16(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));
	uint16_t word = (uint16_t)xsmcToInteger(xsArg(1));
	uint8_t bigEndian = 0; 
	int callbackIndex = -1;

	if (xsmcArgc > 1) {
		if (xsmcIsCallable(xsArg(1)))
			callbackIndex = 1;
		else {
			bigEndian = xsmcToBoolean(xsArg(1));
			if (xsmcArgc > 2)
				callbackIndex = 2;
		}
	}

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationWriteUint16, sizeof(uint16_t), callbackIndex, reg);
	c_memmove(transaction->buffer, &word, sizeof(word));

	queueTransaction(transaction);
}

void _xs_smbusasync_readBuffer(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));
	void *buffer = NULL;
	xsUnsignedValue length;

	if (xsReferenceType == xsmcTypeOf(xsArg(1)))
		xsmcGetBufferWritable(xsArg(1), &buffer, &length);
	else
		length = xsmcToInteger(xsArg(1));

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationReadBuffer, length, (xsmcArgc > 2) ? 2 : -1, reg);
	if (buffer) {
		transaction->hasReadBuffer = true;
		transaction->readBuffer = xsArg(1);
		xsRemember(transaction->readBuffer);
	}

	queueTransaction(transaction);
}

void _xs_smbusasync_writeBuffer(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));
	void *buffer;
	xsUnsignedValue length;

	xsmcGetBufferReadable(xsArg(1), &buffer, &length);

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationWriteBuffer, length, (xsmcArgc > 2) ? 2 : -1, reg);
	c_memmove(transaction->buffer, buffer, length);

	queueTransaction(transaction);
}

void _xs_smbusasync_sendByte(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	uint8_t command = xsmcToInteger(xsArg(0));
	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationSendByte, 1, (xsmcArgc > 1) ? 1 : -1);
	c_memmove(transaction->buffer, &command, sizeof(command));

	queueTransaction(transaction);
}

void _xs_smbusasync_receiveByte(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationReceiveByte, sizeof(uint8_t), (xsmcArgc > 1) ? 1 : -1);

	queueTransaction(transaction);
}

void _xs_smbusasync_readQuick(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationReadQuick, 0, xsmcArgc ? 0 : -1);

	queueTransaction(transaction);
}

void _xs_smbusasync_writeQuick(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, (xsHostHooks *)&xsSMBusHooks);
	Transaction transaction;

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationWriteQuick, 0, xsmcArgc ? 0 : -1);

	queueTransaction(transaction);
}
