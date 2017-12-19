/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

/*
	Sakura IO LTE module

	 English docs: http://python-sakuraio.readthedocs.io/en/latest/index.html
*/

import I2C from "pins/i2c";
import Timer from "timer";

const CMD = {
	GET_CONNECTION_STATUS: 0x01,
	GET_SIGNAL_QUALITY:	0x02,
	GET_DATETIME: 0x03,
	ECHO_BACK: 0x0f,

	// IO
	READ_ADC: 0x10,

	// Transmit
	TX_ENQUEUE: 0x20,
	TX_SENDIMMED: 0x21,
	TX_LENGTH: 0x22,
	TX_CLEAR: 0x23,
	TX_SEND: 0x24,
	TX_STAT: 0x25,

	// Receive
	RX_DEQUEUE: 0x30,
	RX_PEEK: 0x31,
	RX_LENGTH: 0x32,
	RX_CLEAR: 0x33,

	// File Download
	START_FILE_DOWNLOAD: 0x40,
	GET_FILE_METADATA: 0x41,
	GET_FILE_DOWNLOAD_STATUS: 0x42,
	CANCEL_FILE_DOWNLOAD: 0x43,
	GET_FILE_DATA: 0x44,

	// Operation
	GET_PRODUCT_ID: 0xA0,
	GET_UNIQUE_ID: 0xA1,
	GET_FIRMWARE_VERSION: 0xA2,
	UNLOCK: 0xA8,
	UPDATE_FIRMWARE: 0xA9,
	GET_UPDATE_FIRMWARE_STATUS: 0xAA,
	SOFTWARE_RESET: 0xAF
};
Object.freeze(CMD);

const ERROR = {
	NONE: 0x01,
	PARITY: 0x02,
	MISSING: 0x03,
	INVALID_SYNTAX: 0x04,
	RUNTIME: 0x05,
	LOCKED: 0x06,
	BUSY: 0x07
};
Object.freeze(ERROR);

class SakuraIO extends I2C {
	constructor(dictionary) {
		super(Object.assign({address: 0x4F}, dictionary));
	}
	getConnectionStatus() {
		let result = this.execute(CMD.GET_CONNECTION_STATUS, null, 1);
		if ("number" == typeof result)
			return 0x7f;
		return result[0];
	}
	echoback(data) {
		return this.execute(CMD.ECHO_BACK, data, data.length);
	}
	getUnixTime() {
		let result = this.execute(CMD.GET_DATETIME, null, 8);	// 64 bit little-endian result. use low 6 bytes.
		let time = 0;
		for (let i = 0, mul = 1; i < 6; i++, mul *= 256)
			time += result[i] * mul;
		return new Date(time);
	}
	getProductID() {
		let result = this.execute(CMD.GET_PRODUCT_ID, null, 2);
		return result[0] | (result[1] << 8);
	}
	getUniqueID() {
		let result = this.execute(CMD.GET_UNIQUE_ID, null, 10);
		return result.reduce((a, b) => a += String.fromCharCode(b), "");
	}
	getFirmwareVersion() {
		let result = this.execute(CMD.GET_FIRMWARE_VERSION, null, 32);
		return result.reduce((a, b) => a += String.fromCharCode(b), "");
	}
	getSignalQuality() {
		return this.execute(CMD.GET_SIGNAL_QUALITY, null, 1)[0];
	}
	getTxQueueLength() {
		let result = this.execute(CMD.TX_LENGTH, null, 2);
		return {available: result[0], queued: result[1]};
	}
	clearTx() {
		this.execute(CMD.TX_CLEAR);
	}
	send() {
		this.execute(CMD.TX_SEND);
	}
	getTxStatus() {
		let result = this.execute(CMD.TX_STAT, null, 2);
		return {queue: result[0], immediate: result[1]};
	}
	getRxQueueLength() {
		let result = this.execute(CMD.RX_LENGTH, null, 2);
		return {available: result[0], queued: result[1]};
	}
	clearRx() {
		this.execute(CMD.RX_CLEAR);
	}
	enqueueInt32(channel, value) {
		let bytes = new Uint8Array(4);
		(new DataView(bytes.buffer)).setInt32(0, value, true);
		this.enqueueTxRaw(channel, 'i', bytes);
	}
	enqueueUint32(channel, value) {
		let bytes = new Uint8Array(4);
		(new DataView(bytes.buffer)).setUint32(0, value, true);
		this.enqueueTxRaw(channel, 'I', bytes);
	}
	enqueueFloat(channel, value) {
		let bytes = new Uint8Array(4);
		(new DataView(bytes.buffer)).setFloat32(0, value, true);
		this.enqueueTxRaw(channel, 'f', bytes);
	}
	enqueueDouble(channel, value) {
		let bytes = new Uint8Array(8);
		(new DataView(bytes.buffer)).setFloat64(0, value, true);
		this.enqueueTxRaw(channel, 'd', bytes);
	}
	enqueueUint8Array(channel, value) {
		if (8 != value.length) throw new Error("must be 8 bytes");
		this.enqueueTxRaw(channel, 'b', value);
	}
	dequeue() {
		let result = this.execute(CMD.RX_DEQUEUE, null, 18);
		let type = String.fromCharCode(result[1]), value;

		let view = new DataView(result.buffer, 2);
		switch (type) {
			case "i": value = view.getInt32(0, true); break;
			case "I": value = view.getUint32(0, true); break;
			case "f": value = view.getFloat32(0, true); break;
			case "d": value = view.getFloat64(0, true); break;
			case "b": value = new Uint8Array(result.buffer, 2, 8); break;
			default: throw new Error("unimplemented type");
		}

		return {
			channel: result[0],
			bytes: result.slice(2, 18),
			value,
			type
		};
	}

	// internal
	execute(cmd, request = null, responseLength = 0) {  // returns Array-like object (success) or number (error)
		let requestLength = request ? request.length : 0
		let parity = cmd ^ requestLength;

		if (requestLength) {
			request.forEach(value => parity ^= value);
			this.write(cmd, requestLength, request, parity);
		}
		else
			this.write(cmd, 0, parity);

		// write sets stop bit... could be a problem... but does not appear to be
		Timer.delay(10);

		let result = this.read(responseLength + 3);
		if (ERROR.NONE != result[0])
			return result[0];

		parity = result[0] ^ result[1];
		for (let i = 0; i < responseLength; i++)
			parity ^= result[2 + i];

		parity ^= result[responseLength + 2];
		if (parity)
			return ERROR.PARITY;

		return result.slice(2, 2 + responseLength);
	}
	// offset is unimplemented. what does it do? "Time offset in ms"
	enqueueTxRaw(channel, type, data) {
		let request = new Uint8Array(10);
		request[0] = channel;
		request[1] = type.charCodeAt(0);
		data.forEach((value, index) => request[2 + index] = value);
		this.execute(CMD.TX_ENQUEUE, request);
	}
}

export default SakuraIO;
