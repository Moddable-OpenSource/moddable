/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

class BLEScanner @ "BLEScanner_destructor" {
	constructor(options) @ "BLEScanner_constructor"
	close() @ "BLEScanner_close"
	read() @ "BLEScanner_read"
}

const features = Object.freeze({
	constructor(options) @ "BLEClient_constructor",
	close() @ "BLEClient_close",
	connect(request) @ "BLEClient_connect",
	disconnect(request) @ "BLEClient_disconnect",
	getPrimaryServices(request, uuids) @ "BLEClient_getPrimaryServices",
	getCharacteristics(request, service, uuids) @ "BLEClient_getCharacteristics",
	getDescriptors(request, characteristic, uuids) @ "BLEClient_getDescriptors",
	read(request, what, options) @ "BLEClient_read",
	readNotification() @ "BLEClient_readNotification",
	write(request, what, options) @ "BLEClient_write",
	enableNotifications(request, characteristic, enable) @ "BLEClient_enableNotifications",
});

class GATTClient @ "BLEClient_destructor" {
	#closing = false;
	#onError = null;
	#onReadable = null;
	#requests = [];
	static #Request = class {
		constructor(client, feature, callback, ...params) {
			if (client.#closing)
				throw new Error("closing");			
			this.client = client;
			this.feature = feature;
			this.callback = callback;
			this.params = params;
			const requests = client.#requests;
			requests.push(this);
			if (requests.length == 1)
				this.execute();
		}
		execute() {
			this.feature.call(this.client, this, ...this.params);
		}
		executed(error, result) {
			this.callback?.call(this.client, error, result);
			const requests = this.client.#requests;
			const check = requests.shift();
			if (check != this)
				debugger;
			if (this.client.#closing) {
				while (requests.length > 1)
					requests.shift().callback?.(new Error("canceled"));
			}
			if (requests.length > 0)
				requests.at(0).execute();
		}
	};
	constructor(options) {
		this.#onError = options.onError;
		this.#onReadable = options.onReadable;
		options.onError = (error) => {
			this.#closing = true;
			const requests = this.#requests;
			while (requests.length > 0)
				requests.shift().callback?.(new Error("canceled"));
			features.close.call(this);
			this.#onError?.call(this, error);
		}
		options.onReadable = (count) => {
			this.#onReadable?.call(this, count);
		}
		features.constructor.call(this, options);
		const onConnected = (error, result) => {
			if (error)
				this.#onError?.call(this, error);
			else
				options.onReady?.call(this);
		};
		new GATTClient.#Request(this, features.connect, onConnected);
	}
	close(callback) {
		if (this.#closing) return;
		const onDisconnected = (error, result) => {
			features.close.call(this);
			if (callback)
				callback(error, result);
		};
		new GATTClient.#Request(this, features.disconnect, onDisconnected);
		this.#closing = true;
	}
	getPrimaryServices(options, callback = options) {
		new GATTClient.#Request(this, features.getPrimaryServices, callback, options === callback ? null : options);
	}
	getCharacteristics(service, options, callback = options) {
		new GATTClient.#Request(this, features.getCharacteristics, callback, service, options === callback ? null : options);
	}
	getDescriptors(characteristic, options, callback = options) {
		new GATTClient.#Request(this, features.getDescriptors, callback, characteristic, options === callback ? null : options);
	}
	read(what, options, callback = options) {
		if (!what)
			return features.readNotification.call(this);
		new GATTClient.#Request(this, features.read, callback, what, options === callback ? null : options);
	}
	write(what, value, options, callback = options) {
		new GATTClient.#Request(this, features.write, callback, what, value, options === callback ? null : options);
	}
	enableNotifications(characteristic, enable, callback) {
		new GATTClient.#Request(this, features.enableNotifications, callback, characteristic, enable);
	}
	
	get maximumWrite() @ "BLEClient_get_maximumWrite"

	static properties = Object.freeze({
		authenticatedSignedWrites: (1 << 6),
		broadcast: (1 << 0),
		indicate: (1 << 5),
		notify: (1 << 4),
		read: (1 << 1),
		reliableWrite: (1 << 7),
		write: (1 << 3),
		writeWithOutResponse: (1 << 2),
	})
}

export { BLEScanner as GAPClient, GATTClient };

