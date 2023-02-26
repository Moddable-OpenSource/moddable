/*
 * Copyright (c) 2019-2023 Moddable Tech, Inc.
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

	to do:
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "mc.xs.h"			// for xsID_* values
#include "xsHost.h"			// esp platform support

#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "builtinCommon.h"

struct I2CRecord {
	uint32_t					hz;
	uint32_t					data;
	uint32_t					clock;
	uint32_t					timeout;
	uint8_t						address;		// 7-bit
	uint8_t						pullup;
	uint8_t						port;
	uint8_t						stop;			// SMBus
	xsSlot						obj;
	struct I2CRecord			*next;
};
typedef struct I2CRecord I2CRecord;
typedef struct I2CRecord *I2C;

static I2C gI2C;
static I2C gI2CActive;

static uint8_t i2cActivate(I2C i2c);
static uint8_t usingPins(uint32_t data, uint32_t clock);

static SemaphoreHandle_t gI2CMutex;

void _xs_i2c_constructor(xsMachine *the)
{
	I2C i2c;
	int data, clock, hz, address;
	int timeout = 32000;
	uint8_t pullup = GPIO_PULLUP_ENABLE;
	int port = I2C_NUM_0;

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

	xsmcGet(xsVar(0), xsArg(0), xsID_hz);
	hz = xsmcToInteger(xsVar(0));
	if ((hz <= 0) || (hz > 20000000))
		xsRangeError("invalid hz");

	if (xsmcHas(xsArg(0), xsID_timeout)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeout);
		timeout = xsmcToInteger(xsVar(0)) * (80000000 / 1000);
	}

	if (xsmcHas(xsArg(0), xsID_pullup)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_pullup);
		pullup = xsmcToBoolean(xsVar(0)) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
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

	i2c = c_malloc(sizeof(I2CRecord));
	if (!i2c)
		xsRangeError("no memory");

	xsmcSetHostData(xsThis, i2c);
	i2c->obj = xsThis;
	xsRemember(i2c->obj);
	i2c->clock = clock;
	i2c->data = data;
	i2c->hz = hz;
	i2c->address = address;
	i2c->timeout = timeout;
	i2c->pullup = pullup;
	i2c->port = (uint8_t)port;

	i2c->next = gI2C;
	gI2C = i2c;

	builtinUsePin(data);
	builtinUsePin(clock);
}

void _xs_i2c_destructor(void *data)
{
	I2C i2c = data;
	if (!i2c)
		return;

	if (i2c == gI2CActive) {
		gI2CActive = NULL;
		i2c_driver_delete(i2c->port);
	}

	if (gI2C == i2c)
		gI2C = i2c->next;
	else {
		I2C walker;
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

void _xs_i2c_close(xsMachine *the)
{
	I2C i2c = xsmcGetHostData(xsThis);
	if (i2c && xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor)) {
		xsForget(i2c->obj);
		_xs_i2c_destructor(i2c);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void _xs_i2c_read(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	xsUnsignedValue length;
	int err;
	uint8_t stop = true;
	i2c_cmd_handle_t cmd;
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

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	if (length > 0) {
		if (length > 1)
			i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
		i2c_master_read(cmd, ((uint8_t *)buffer) + length - 1, 1, I2C_MASTER_NACK);
	}
	if (stop)
		i2c_master_stop(cmd);
	err = i2c_master_cmd_begin(i2c->port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	xSemaphoreGive(gI2CMutex);

	if (ESP_OK != err)
		xsUnknownError("read failed");
}

void _xs_i2c_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2c_destructor);
	int err;
	xsUnsignedValue length;
	uint8_t stop = true;
	i2c_cmd_handle_t cmd;
	void *buffer;

	if (xsmcArgc > 1)
		stop = xsmcToBoolean(xsArg(1));

	xsmcGetBufferReadable(xsArg(0), &buffer, &length);

	if (!i2cActivate(i2c))
		xsUnknownError("activate failed");

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
	if (length)
		i2c_master_write(cmd, (uint8_t *)buffer, length, 1);

	if (stop)
		i2c_master_stop(cmd);
	err = i2c_master_cmd_begin(i2c->port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	xSemaphoreGive(gI2CMutex);

	if (length == 0) {
		if (ESP_OK != err)
			xsmcSetInteger(xsResult, 1);
	}
	else if (ESP_OK != err)
		xsUnknownError("write failed");
}

uint8_t i2cActivate(I2C i2c)
{
	i2c_config_t conf;

	xSemaphoreTake(gI2CMutex, portMAX_DELAY);

	if ((i2c == gI2CActive) ||
		(gI2CActive && (gI2CActive->data == i2c->data) && (gI2CActive->clock == i2c->clock) && (gI2CActive->hz == i2c->hz) && (gI2CActive->port == i2c->port) && (gI2CActive->pullup == i2c->pullup)))
		return 1;

	if (gI2CActive) {
		i2c_driver_delete(gI2CActive->port);
		gI2CActive = NULL;
	}

	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = i2c->data;
	conf.scl_io_num = i2c->clock;
	conf.master.clk_speed = i2c->hz;
	conf.sda_pullup_en = i2c->pullup;
	conf.scl_pullup_en = i2c->pullup;
	conf.clk_flags = 0;
	if (ESP_OK != i2c_param_config(i2c->port, &conf)) {
		xSemaphoreGive(gI2CMutex);
		return 0;
	}

	if (ESP_OK != i2c_driver_install(i2c->port, I2C_MODE_MASTER, 0, 0, 0)) {
		xSemaphoreGive(gI2CMutex);
		return 0;
	}

	gI2CActive = i2c;

	i2c_set_timeout(i2c->port, i2c->timeout);

	return 1;
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
	i2c_cmd_handle_t	cmd;
	uint8_t				hasCallback;
	uint8_t				hasReadBuffer;
	uint8_t				operation;
	uint8_t				bufferLength;
	uint8_t				processing;		// 0 unstarted, 1 i2c transaction in progress, 2 i2c transaction done
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

	if (kOperationClose == operation)
		_xs_i2casync_destructor(transaction->i2c);	// all resources must be released before invoking callback ... so callback could call new again

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
			else if (i2cActivate(transaction->i2c)) {
				transaction->err = i2c_master_cmd_begin(transaction->i2c->port, transaction->cmd, 1000 / portTICK_RATE_MS);
				xSemaphoreGive(gI2CMutex);
			}
			else
				transaction->err = -1;

			transaction->processing = 2;

			i2c_cmd_link_delete(transaction->cmd);

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
	transaction->cmd = i2c_cmd_link_create();
	transaction->operation = operation;
	transaction->bufferLength = (uint8_t)bufferLength;
	
	return transaction;
}

static Transaction newSMBusTransaction(xsMachine *the, I2C i2c, uint8_t operation, xsUnsignedValue bufferLength, xsIntegerValue callbackIndex, uint8_t reg)
{
	Transaction transaction = newI2CTransaction(the, i2c, operation, bufferLength, callbackIndex);

	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, reg, 1);
	if (i2c->stop)
		i2c_master_stop(cmd);

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

void asyncclose(xsMachine *the, xsDestructor destructor)
{
	I2C i2c = xsmcGetHostData(xsThis);
	if (!i2c) return;

	xsmcGetHostDataValidate(xsThis, destructor);

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
	asyncclose(the, _xs_i2casync_destructor);
}

void _xs_i2casync_read(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2casync_destructor);
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

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	if (length > 0) {
		if (length > 1)
			i2c_master_read(cmd, transaction->buffer, length - 1, I2C_MASTER_ACK);
		i2c_master_read(cmd, transaction->buffer + length - 1, 1, I2C_MASTER_NACK);
	}
	if (stop)
		i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_i2casync_write(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_i2casync_destructor);
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

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
	if (length)
		i2c_master_write(cmd, transaction->buffer, length, 1);
	if (stop)
		i2c_master_stop(cmd);
	
	queueTransaction(transaction);
}

void _xs_smbusasync_destructor(void *data)
{
	_xs_i2casync_destructor(data);
}

void _xs_smbusasync_constructor(xsMachine *the)
{
	I2C i2c;

	_xs_i2casync_constructor(the);
	
	i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	i2c->stop = 0;
	if (xsmcHas(xsArg(0), xsID_stop)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_stop);
		i2c->stop = xsmcToBoolean(xsVar(0));
	}
}

void _xs_smbusasync_close(xsMachine *the)
{
	asyncclose(the, _xs_smbusasync_destructor);
}

void _xs_smbusasync_readUint8(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationReadUint8, sizeof(uint8_t), (xsmcArgc > 1) ? 1 : -1, reg);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	i2c_master_read(cmd, transaction->buffer, 1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_readUint16(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
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

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	if (bigEndian) {
		i2c_master_read(cmd, transaction->buffer + 1, 1, I2C_MASTER_ACK);
		i2c_master_read(cmd, transaction->buffer + 0, 1, I2C_MASTER_NACK);
	}
	else {
		i2c_master_read(cmd, transaction->buffer + 0, 1, I2C_MASTER_ACK);
		i2c_master_read(cmd, transaction->buffer + 1, 1, I2C_MASTER_NACK);
	}
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_writeUint8(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));
	int byte = xsmcToInteger(xsArg(1));

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationWriteUint8, 0, (xsmcArgc > 2) ? 2 : -1, reg);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, byte, 1);
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_writeUint16(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
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
	transaction = newSMBusTransaction(the, i2c, kOperationWriteUint16, 0, callbackIndex, reg);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	if (bigEndian) {
		i2c_master_write_byte(cmd, word >> 8, 1);
		i2c_master_write_byte(cmd, word & 0xff, 1);
	}
	else {
		i2c_master_write_byte(cmd, word & 0xff, 1);
		i2c_master_write_byte(cmd, word >> 8, 1);
	}
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_readBuffer(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
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

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	if (length > 0) {
		if (length > 1)
			i2c_master_read(cmd, transaction->buffer, length - 1, I2C_MASTER_ACK);
		i2c_master_read(cmd, transaction->buffer + length - 1, 1, I2C_MASTER_NACK);
	}
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_writeBuffer(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));
	void *buffer;
	xsUnsignedValue length;

	xsmcGetBufferReadable(xsArg(1), &buffer, &length);

	// set-up transaction record
	transaction = newSMBusTransaction(the, i2c, kOperationWriteBuffer, length, (xsmcArgc > 2) ? 2 : -1, reg);
	c_memmove(transaction->buffer, buffer, length);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
	if (length)
		i2c_master_write(cmd, transaction->buffer, length, 1);
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_sendByte(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	Transaction transaction;
	uint8_t command = xsmcToInteger(xsArg(0));
	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationSendByte, 1, (xsmcArgc > 1) ? 1 : -1);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, command, 1);
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_receiveByte(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationReceiveByte, sizeof(uint8_t), (xsmcArgc > 1) ? 1 : -1);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 1);
	i2c_master_read(cmd, transaction->buffer, 1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_readQuick(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	Transaction transaction;
	int reg = xsmcToInteger(xsArg(0));

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationReadQuick, sizeof(uint8_t), xsmcArgc ? 0 : -1);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, 0);
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}

void _xs_smbusasync_writeQuick(xsMachine *the)
{
	I2C i2c = xsmcGetHostDataValidate(xsThis, _xs_smbusasync_destructor);
	Transaction transaction;

	// set-up transaction record
	transaction = newI2CTransaction(the, i2c, kOperationWriteQuick, 1, xsmcArgc ? 0 : -1);

	// build command
	i2c_cmd_handle_t cmd = transaction->cmd;
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, 0);
	i2c_master_stop(cmd);

	queueTransaction(transaction);
}
