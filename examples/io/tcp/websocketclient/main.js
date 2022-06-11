/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import Timer from "timer";

const WSPublic = {
	host: "websockets.chilkat.io",
	path: "/wsChilkatEcho.ashx",
};

const WSLocal = {
	address: "10.0.1.11",
	port: 8080,
};

const WebSocketClient = device.network.ws.io;
let counter = 0;
const ws = new WebSocketClient({
	...device.network.ws,
	...WSPublic,
	onReadable(count, options) {
//		Timer.set(() => {
			const data = this.read(count);
			trace(String.fromArrayBuffer(data));
			if (!options.more)
				trace("\n");
//		});
	},
	onWritable(count) {
		if (0 === counter) {
			this.write(ArrayBuffer.fromString("pong"), {opcode: WebSocketClient.pong});
			this.write(ArrayBuffer.fromString("ping!"), {opcode: WebSocketClient.ping});
			this.write(ArrayBuffer.fromString("this is a test 1"));
			this.write(ArrayBuffer.fromString("this is a test 2"), {binary: false, more: true});
			this.write(ArrayBuffer.fromString("this is a test 3"), {binary: false});
 			this.write(ArrayBuffer.fromString("abcdefghijklmnopqrstuvwyxz".repeat(8)), {binary: false});
			this.write(ArrayBuffer.fromString("this is a test 4"), {binary: true});
			this.write(ArrayBuffer.fromString("ping!!"), {opcode: WebSocketClient.ping});
//			this.write(Uint16Array.of(0).buffer, {opcode: WebSocketClient.close});
			counter = 1;

			this.timer = Timer.repeat(() => {
				this.write(ArrayBuffer.fromString("tick " + ++counter));
			}, 500);
		}
	},
	onControl(opcode, data) {
		switch (opcode) {
			case WebSocketClient.close: 
				trace("** Connection Closing **\n");
				data = new Uint8Array(data);
				const code = (data[0] << 8) | data[1];
				const reason = String.fromArrayBuffer(data.buffer.slice(2));
				trace(code, ": " + reason + "\n");

				Timer.clear(this.timer);
				delete this.timer;
				break;

			case WebSocketClient.ping:
				trace("PING!\n");
				break;

			case WebSocketClient.pong:
				trace("PONG!\n");
				break;
		}
	},
	onClose() {
		trace("** Connection Closed **\n");
		Timer.clear(this.timer);
		delete this.timer;
		
	},
	onError() {
		trace("** Connection Error **\n");
		Timer.clear(this.timer);
		delete this.timer;
	}
});
