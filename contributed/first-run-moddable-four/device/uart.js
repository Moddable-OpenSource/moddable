/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import BLEServer from "bleserver";
import GAP from "gap";
import {uuid} from "btutils";
import {IOCapability} from "sm";

export default class UARTServer extends BLEServer {
	constructor(target, buffer, name) {
		super();
		this.deviceName = name;
		this.target = target;
		this.buffer = new Uint8Array(buffer);
		this.offset = 0;
		this.receivingImage = false;

		this.securityParameters = {
			mitm: true,
			bonding: true,
			ioCapability: IOCapability.DisplayOnly
		};
	}
	onReady() {
		this.onDisconnected();
	}
	onAuthenticated() {
		this.stopAdvertising();
		this.target.defer("onAuthenticated");
	}
	onConnected() {
		this.stopAdvertising();
		this.target.defer("onConnected");
	}
	onDisconnected() {
		this.target.defer("onDisconnected");
		const SERVICE_UUID = uuid`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`;
		delete this.tx;
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID128List: [SERVICE_UUID]}
		});
	}
	onCharacteristicNotifyEnabled(characteristic) {
		this.tx = characteristic;
	}
	onCharacteristicNotifyDisabled(characteristic) {
		delete this.tx;
	}
	onCharacteristicWritten(characteristic, from) {
		if (this.receivingImage) {
			let fromOffset = 0;
			let fromSize = from.length;
			let to = this.buffer;
			let toOffset = this.offset;
			while (fromOffset < fromSize) {
				to[toOffset++] = from[fromOffset++];
			}
			if (toOffset == 2048) {
				this.target.defer("onImage");
				this.offset = 0;
				this.receivingImage = false
			}
			else
				this.offset = toOffset;
		}
		else {
			const message = JSON.parse(String.fromArrayBuffer(from.buffer));
			if (message.name == "image")
				this.receivingImage = true;
			else if (message.name == "name") {
// 				trace(`name ${message.value}\n`);
				let name = message.value;
				const length = Math.min(name.length, GAP.MAX_AD_LENGTH - 2 - 1 - 2 - 16 - 2);
				const codes = new Array(length).fill(95);
				for (let index = 0; index < length; index++) {
					let code = name.charCodeAt(index);
					if ((32 <= code) && (code <= 127))
						codes[index] = code;
				}
				name = String.fromCharCode(...codes);
				this.deviceName = name
				this.target.defer("onRename", name);
			}
			else if (message.name == "time") {
// 				trace(`time ${message.value}\n`);
				application.delegate("setTime", Math.floor(message.value / 1000));
			}
		}
	}
	transmit(value) {
		if (this.tx)
			this.notifyValue(this.tx, value);
	}



	onPasskeyConfirm(params) {
		let passkey = this.passkeyToString(params.passkey);
		trace(`server confirm passkey: ${passkey}\n`);
		this.passkeyReply(params.address, true);
	}
	onPasskeyDisplay(params) {
		let passkey = this.passkeyToString(params.passkey);
		this.target.defer("onPasskey", passkey);
		trace(`server display passkey: ${passkey}\n`);
	}
	onPasskeyInput(params) {
		trace(`server input passkey displayed by peer\n`);
		//let passkey = 0;
		//this.passkeyInput(params.address, passkey);
	}
	onPasskeyRequested(params) {
		let passkey = Math.round(Math.random() * 999999);
		trace(`server requested passkey: ${this.passkeyToString(passkey)}\n`);
		return passkey;
	}
	onBondingDeleted(params) {
		trace(`device ${params.address} bond deleted\n`);
	}
	passkeyToString(passkey) {
		return passkey.toString().padStart(6, "0");
	}
}

export {UARTServer};
