/*
 * Copyright (c) 2024-2026 Moddable Tech, Inc.
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

import ChatWorker from "ChatWorker"
import JSONBase64Parser from "JSONBase64Parser";
import TextEncoder from "text/encoder";

const WebSocketClient = device.network.ws.io;

const text = Object.freeze({binary: false});

class ChatWebSocketWorker extends ChatWorker {
	#buffers = [];
	#state = 0;
	#writable = 0;
	#encoder = new TextEncoder;

	constructor(options) {
		super(options);
		this.ws = null;
		this.outputMinimum = (options.outputSampleRate ?? 24000) >> 1;
		this.silence = new ArrayBuffer(this.outputMinimum);
	}
	close() {
		this.ws.close();
		this.ws = null;
		this.#buffers = [];
		this.#state = 0;
		this.#writable = 0;
	}
	
	connect(message) {
		super.connect(message);
		this.parser = new JSONBase64Parser(this, this.outputBuffer, 2, this.outputMinimum);
		this.parser.barrier = message.barrier;
		this.ws = new WebSocketClient({
			...device.network.wss,
			host: this.host,
			path: this.path,
			port: 443,
			headers: this.headers,
			onClose: () => {
// 				trace(`onClose\n`);
			},
			onControl: (opcode, data) => {
				switch (opcode) {
				case WebSocketClient.close: 
					data = new Uint8Array(data);
					const code = (data[0] << 8) | data[1];
					const reason = String.fromArrayBuffer(data.buffer.slice(2));
					if (code != 1000)
						this.postMessage({ id:"failed", string:reason });
					else
						this.postMessage({ id:"disconnected" });
					this.close();
					break;
					
				case WebSocketClient.ping:
//					trace("PING!\n");
					break;
				case WebSocketClient.pong:
//					trace("PONG!\n");
					break;
				}
			},
			onError: () => {
				this.postMessage({ id:"failed", string:"network error" });
				this.close();
			},
			onReadable: (count, options) => {
				const buffer = this.ws.read(count);
				if (this.#state == 1) {
					this.read(buffer, options);
				}
			},
			onWritable: (count) => {
				if (!count)
					return;
				this.#writable = count;
				if (this.#state == 0) {
					this.onOpen();
					this.#state = 1;
					return;
				}
				let buffers = this.#buffers;
				while (buffers.length) {
					let buffer = buffers[0];
					let options = buffer.options;
					let data = buffer.data;
					const dataLength = data.byteLength;
					const writable = this.#writable;
					if (dataLength <= writable) {
						this.#writable = this.ws.write(data, options);
						buffers.shift();
					}
					else if (0 < writable) {
						let moreOptions = { ...options, more:true };
						this.#writable = this.ws.write(data.slice(0, writable), moreOptions);
						buffer.data = data.slice(writable);
						break;
					}
					else
						break;
				}
			}
		});
	}
	disconnect() {
		const code = 1000;
		const data = Uint8Array.of(code >> 8, code & 0xFF);
		this.write(data, { opcode: WebSocketClient.close });	
		this.#state = 2;
	}
	isBase64(/* result, current, name */) {
		debugger;
	}
	onBase64(offset, size) {
// 		trace(`receiveAudio ${ offset } ${ offset + size }\n`);
		this.postMessage({ id:"receiveAudio", offset, size });
	}
	onJSON(json) {
		const type = json.type;
		if (type in this)
			this[type](json);
	}
	onOpen() {
	}
	read(data, options) {
		this.parser.read(data);
		if (options.more)
			return;
// 		trace(`<= ${ JSON.stringify(this.parser.result) }\n`);
		this.onJSON(this.parser.result);
		this.parser.reset();
	}
	sendAudio(message) {
// 		trace(`=> sendAudio ${ message.offset } ${ message.size }\n`);
		const samples = new Uint8Array(this.inputBuffer, message.offset, message.size);
		const string = samples.toBase64();
		const data = new Uint8Array(this.audioPrefix.length + string.length + this.audioSuffix.length);
		data.set(this.audioPrefix); 
		this.#encoder.encodeInto(string, data.subarray(this.audioPrefix.length));
		data.set(this.audioSuffix, this.audioPrefix.length + string.length);
		this.write(data, text);
	}
	sendJSON(json) {
		const string = JSON.stringify(json);
// 		trace(`=> ${ string }\n`);
		const data = ArrayBuffer.fromString(string);
		this.write(data, text);
	}
	write(data, options) {
		let buffers = this.#buffers;
		if (buffers.length)
			buffers.push({ data, options });
		else {
			const dataLength = data.byteLength;
			const writable = this.#writable;
			if (dataLength <= writable) {
				this.#writable = this.ws.write(data, options);
			}
			else if (0 < writable) {
				let moreOptions = { ...options, more:true };
				this.#writable = this.ws.write(data.slice(0, writable), moreOptions);
				buffers.push({ data: data.slice(writable), options });
			}
			else {
				buffers.push({ data, options });
			}
		}
	}
}

export default ChatWebSocketWorker;

