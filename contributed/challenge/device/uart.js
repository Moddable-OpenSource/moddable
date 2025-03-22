/*
 * Copyright (c) 2016-2014  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
 /*
 	Nordic UART Service Server 
	The Nordic UART Service is a simple GATT-based service with TX and RX characteristics.
	Strings written to the UART Service server are echoed back to the client as characteristic notifications.

	https://devzone.nordicsemi.com/f/nordic-q-a/10567/what-is-nus-nordic-uart-service
 */

import BLEServer from "bleserver";
import {uuid} from "btutils";
import SLIP from "slip";

const SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;
const UART_SERVER = "UART Server";

export default class UARTServer extends BLEServer {
	#target;
	#mtu = 17;
	#received = new Uint8Array(new ArrayBuffer(0, {maxByteLength: 8192}));

	constructor(target) {
		super();
		this.#target = target;
	}
	onReady() {
		this.deviceName = "UART";
		this.onDisconnected();
// 		trace.left("ready", UART_SERVER);
	}
	onConnected() {
		this.#target.defer("onConnected");
// 		trace.left("connected", UART_SERVER);
		this.stopAdvertising();
		this.script = "";
	}
	onDisconnected() {
		this.#target.defer("onDisconnected");
// 		trace.left("disconnected", UART_SERVER);
		delete this.tx;
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID128List: [SERVICE_UUID]}
		});
	}
	onMTUExchanged(size) {
		this.#mtu = size;
	}
	onCharacteristicNotifyEnabled(characteristic) {
		this.tx = characteristic;
	}
	onCharacteristicNotifyDisabled(characteristic) {
		delete this.tx;
	}
	onCharacteristicWritten(characteristic, value) {
		trace.left(value, UART_SERVER);

		let byteLength = this.#received.byteLength;
		this.#received.buffer.resize(byteLength + value.byteLength);
		this.#received.set(new Uint8Array(value), byteLength);
		while (true) {
			const result = SLIP.unescape(this.#received);
			if (result.remainder) {
				this.#received.buffer.resize(result.remainder.byteLength);
				this.#received.set(new Uint8Array(result.remainder));
			}
			else
				this.#received.buffer.resize(0);
			if (result.packet)
				this.#target.defer("onReceived", String.fromArrayBuffer(result.packet));

			if (!result.packet || !this.#received.byteLength)
				break;
		}
	}
	transmit(value) {
		if (!this.tx)
			return;

		if ("string" === typeof value)
			value = ArrayBuffer.fromString(value);
		value = SLIP.escape(value);
		for (let i = 0; i < value.byteLength; i += this.#mtu)
			this.notifyValue(this.tx, value.slice(i, i + Math.min(this.#mtu, value.byteLength - i)))
	}
}
