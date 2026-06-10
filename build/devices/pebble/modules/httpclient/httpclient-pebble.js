/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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

import Messages from "pebble/message"

const bufferSize = 512;
const bufferOverhead = 32;		// a guess
const state = {};

const BASE = 15000;

class HTTPClient {
	static #Request = class {
		#client;

		constructor(client) {
			this.#client = client;
		}
		read(count) {
			const client = this.#client, current = client.#current, response = current.response;
			if (current?.request !== this)
				throw new Error("bad state");

			if (!response.length)
				return;

			const available = response.byteLength
			let result, bytes;
			count ??= available;
			if ("number" === typeof count) {
				if (count > available)
					count = available;

				if ((count === response[0].byteLength) && (0 === response[0].position)) {
					response.byteLength -= count;
					delete response[0].position;
					return response.shift();
				}

				bytes = new Uint8Array(count);
				result = bytes.buffer;
			}
			else
				throw new Error("BYOB unimplemented");

			response.byteLength -= count;
			let offset = 0;
			while (count > 0) {
				const fragment = response[0];
				let use = fragment.byteLength - fragment.position;
				if (use > count)
					use = count;
				bytes.set(new Uint8Array(fragment, fragment.position, use), offset);
				offset += use;
				fragment.position += use;
				if (fragment.position === fragment.byteLength)
					response.shift();
				count -= use;
			}

			return result;
		}
		write(buffer) {
			const client = this.#client, current = client.#current
			if ((this !== current?.request) || ("sendBody" !== client.#state))
				throw new Error("bad state");

			if (!state.writable)
				throw new Error("not writable");

			if (undefined === buffer) {
				if (true === current.sending) {
					client.#state = "endOfBody";
					return;
				}
				throw new Error("no buffer");
			}

			if (buffer.byteLength > (state.messages.output - bufferOverhead))
				throw new Error("would overflow");

			const m = new Map;
			m.set(BASE + 1, current.id);
			m.set(BASE + 4, buffer);
			state.messages.write(m);

			if (true !== current.sending) {
				current.sending -= buffer.byteLength;
				if (0 === current.sending)
					client.#state = "endOfBody";
			}

			delete state.writable;

			return 0;			// message buffer used up
		}
	}
	
	#options;
	#requests = [];
	#headers;
	#state = "connected";
	#nextState;
	#current;
	#remain;
	
	constructor(options) {
		const {host, port, onError, protocol} = options; 
		this.#options = {host, port, onError, protocol};

		state.id ??= 0;
		state.clients ??= [];
		state.clients.push(this);

		state.messages ??= new Messages({
			input: bufferSize,
			output: bufferSize,
			onReadable: () => {
				const message = state.messages.read();
				const id = message.get("id");
				for (let i = 0, clients = state.clients; i < clients.length; i++) {
					if (clients[i].#current?.id === id)
						return clients[i].#read(message);
				}
			},
			onWritable: () => {
				state.writable = true;
				for (let i = 0, clients = state.clients; i < clients.length; i++) {
					if (clients[i].#current) {
						// trace(`Writable id=${clients[i].#current.id}\n`);
						clients[i].#write();
						if (!state.writable)
							return;
					}
				}
			},
			onSuspend: () => delete state.writable,
			keys: new Map([
				["id", BASE + 1]
			]),
		});
	}
	close () {
		const i = state.clients.indexOf(this);
		if (i >= 0) {
			state.clients.splice(i, 1);
			if (0 === state.clients.length) {
				state.messages?.close();
				delete state.clients;
				delete state.messages;
			}
		}
		this.#current = this.#headers = this.#state = this.#options = undefined;
		this.#requests.length = 0;
	}
	request(options) {
		const {method, path, headers, onHeaders, onReadable, onWritable, onDone, headersMask} = options;
		options = {method, path, headers, onHeaders, onReadable, onWritable, onDone, headersMask, id: ++state.id};

		this.#requests.push(options);
		if (("connected" === this.#state) && (1 === this.#requests.length))
			this.#next();
		options.request = new HTTPClient.#Request(this); 
	}
	#next() {
		this.#current = this.#requests.shift();
		if (!this.#current)
			return;

		this.#headers = (this.#current.headers ?? new Map).entries();
		this.#state = "sendRequest";
		this.#nextState = "";

		if (state.writable)
			this.#write();
	}
	#write() {
		const current = this.#current;

		while (true) {
			let remain = this.#remain;
			if (remain) {
				let use = remain.byteLength - remain.position;
				if (use > (state.messages.output - bufferOverhead))
					use = state.messages.output - bufferOverhead;

				const m = new Map;
				m.set("id", current.id);
				m.set(BASE + remain.part, remain.subarray(remain.position, remain.position + use))
				remain.position += use;
				if (remain.position === remain.byteLength) {
					this.#remain = undefined;
					this.#state = this.#nextState;
				}
				state.messages.write(m);
				delete state.writable;
				return;
			}

			switch (this.#state) {
				case "sendRequest":
					this.#remain = ArrayBuffer.fromString(`${this.#options.protocol ?? "https"}:${current.method ?? "GET"}:${this.#options.host}:${this.#options.port ?? ""}:${current.path ?? "/"}:${state.messages.input}:${current.headersMask ? current.headersMask.join(",") : ""}`);
					this.#remain = new Uint8Array(this.#remain);
					this.#remain.part = 2;
					this.#remain.position = 0;
					this.#nextState = "sendHeaders";
					break;
				case "sendHeaders": {
					const item = this.#headers.next();
					if (item.done) {
						this.#state = "sendBody";
						this.#headers = undefined;
					}
					else {
						item.value[0] = item.value[0].toLowerCase();
						item.value[1] = item.value[1].toString();
						if ("content-length" === item.value[0])
							current.sending = parseInt(item.value[1]);
						else if (("transfer-encoding" === item.value[0]) && ("chunked" === item.value[1].toLowerCase()))
							current.sending = true;		// unknown length
						this.#remain = ArrayBuffer.fromString(`${current.sentHeader ? "\n" : ""}${item.value[0]}:${item.value[1]}`);
						current.sentHeader = true;
						this.#remain = new Uint8Array(this.#remain);
						this.#remain.part = 3;
						this.#remain.position = 0;
					}
					} break;
				case "sendBody":
					if (current.sending) {
						let use = state.messages.output - bufferOverhead;
						if ((true !== current.sending) && (current.sending < use))
							use = current.sending;
						current.onWritable?.call(current.request, use);
						return;
					}
					else
						this.#state = "endOfBody";
					break;
						
				case "endOfBody":
					this.#remain = new ArrayBuffer(1);
					this.#remain = new Uint8Array(this.#remain);
					this.#remain.part = 5;
					this.#remain.position = 0;
					this.#nextState = "receiveStatus";
					break;

				default:
					return;
			}
		}
	}
	#read(message) {
		const current = this.#current;

		switch (this.#state) {
				case "receiveStatus":
					current.status = message.get(BASE + 6);
					if (current.status < 0)
						this.#done("fail " + current.status);
					else {
						this.#state = "receiveHeaders";
						current.headers = [];
					}
					current.statusText = message.get(BASE + 11);
					break;

					case "receiveHeaders": {
						const fragment = message.get(BASE + 7);
						if (fragment) {
							current.headers.push(fragment);
							break;
						}

						let headers = current.headers.join("");
						delete current.headers;
						const map = new Map;
						headers.split("\n").forEach(header => {
							const [key, value] = header.split(":");
							map.set(key, value);
						});
						current.onHeaders?.call(current.request, current.status, map, current.statusText);
						this.#state = "receiveBody";
						current.response = [];
						current.response.byteLength = 0;
						// deliberate fallthrough
					}

					case "receiveBody": {
						const fragment = message.get(BASE + 8);
						if (fragment) {
							fragment.position = 0;
							current.response.push(fragment);
							current.response.byteLength += fragment.byteLength;

							current.onReadable?.call(current.request, current.response.byteLength);
							break;
						}

						if (0 !== message.get(BASE + 9))
								this.#done("bad state");
						else
							this.#done();
						} break;

				default:
					trace("unexpected onReadable state " + this.#state);
					break;
		}			
	}
	#done(error) {
		if (error) {
			trace("HTTP error: " + error);
			this.#current.onError?.call(this.#current.request, error);
		}
		else
			this.#current.onDone?.call(this.#current.request);

		this.#current = this.#headers = undefined;
		this.#state = "connected";
		this.#next();
	}
}

export default HTTPClient;
