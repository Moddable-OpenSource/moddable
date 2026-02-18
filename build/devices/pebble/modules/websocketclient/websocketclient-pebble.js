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

/*
	To do: request headers
*/

import Messages from "pebble/message"

const bufferSize = 512;
const bufferOverhead = 64;		// a guess
const none = Object.freeze({});
const state = {};

const BASE = 15050;

class WebSocketClient {
	#options;
	#state = "connecting";
	#nextState;
	#remain;
	#response = [];

	constructor(options) {
		state.id ??= 0;
		state.clients ??= [];
		state.clients.push(this);

		const {host, port, path, secure, protocol, onError, onClose, onReadable, onWritable, onControl} = options; 
		this.#options = {onError, onClose, onReadable, onWritable, onControl, id: ++state.id};

		state.messages ??= new Messages({
			input: bufferSize,
			output: bufferSize,
			onReadable: () => {
				const message = state.messages.read();
				const id = message.get("id");
				for (let i = 0, clients = state.clients; i < clients.length; i++) {
					if (clients[i].#options?.id === id)
						return clients[i].#read(message);
				}
			},
			onWritable: () => {
				state.writable = true;
				for (let i = 0, clients = state.clients; i < clients.length; i++) {
					this.log(`Writable id=${clients[i].#options.id}`);
					clients[i].#write();
					this.log(`  called write state.writable ${state.writable}`);
					if (!state.writable)
						return;
				}
			},
			onSuspend: () => delete state.writable,
			keys: new Map([
				["id", BASE + 1]
			]),
		});
		delete state.writable;

		this.#remain = ArrayBuffer.fromString(`${secure ? "wss" : "ws"}:${protocol ?? ""}:${host}:${port ?? ""}:${path ?? "/"}:${state.messages.input}`);

		this.#remain = new Uint8Array(this.#remain);
		this.#remain.part = 2;
		this.#remain.position = 0;
		this.#nextState = "waitHandshake";
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

		this.#state = this.#remain = this.#response = undefined;
	}
	read(count) {
		const response = this.#response;
		if (!response.length)
			return;

		const fragment = response[0], available = fragment.byteLength;
		let result, bytes;
		count ??= available;
		if ("number" === typeof count) {
			if (count > available)
				count = available;

			if ((count === fragment.byteLength) && (0 === fragment.position)) {
				delete fragment.part;
				delete fragment.position;
				result = fragment;
				response.shift();
				return this.#schedule(result);
			}

			bytes = new Uint8Array(count);
			result = bytes.buffer;
		}
		else
			throw new Error("BYOB unimplemented");

		result = fragment.slice(fragment.position, count);
		fragment.position += count;
		if (fragment.position === fragment.byteLength)
			response.shift();

		return this.#schedule(result);
	}
	write(buffer, options = none) {
		this.log("ws - write");
		if (!state.writable)
			throw new Error("not writable");

		if (buffer.byteLength > (state.messages.output - bufferOverhead))
			throw new Error("would overflow");

		if (undefined !== options.opcode) {
			if (WebSocketClient.close === options.opcode) {
				this.log("ws - close opcode");
				const m = new Map;
				m.set("id", this.#options.id);
				m.set(BASE + 8, buffer);
				state.messages.write(m);

				delete state.writable;
				return;
			}
			throw new Error("opcode unsupported");
		}

		const binary = options.binary ?? true, more = options.more ?? false;
		if (this.#options.more) {
			if (binary !== this.#options.binary)
				throw new Error("inconsistent binary");
			if (!more) {
				delete this.#options.more;
				delete this.#options.binary;
			}
		}
		else if (more) {
			this.#options.more = true;
			this.#options.binary = binary;
		}

		const m = new Map;
		m.set("id", this.#options.id);
		m.set(BASE + (binary ? 4 : 6) + (more ? 1 : 0), buffer);
		// 4 - binary - no more
		// 5 - binary - more
		// 6 - text - no more
		// 7 - text more
		this.log(`ws - write binary ${binary} more ${more} byteLength ${buffer.byteLength}`)
		state.messages.write(m);

		delete state.writable;
		return 0;
	}
	#write() {
		this.log(`ws - onWritable ${this.#state}`);
		while (true) {
			const remain = this.#remain;
			if (remain) {
				let use = remain.byteLength - remain.position;
				if (use > (state.messages.output - bufferOverhead))
					use = state.messages.output - bufferOverhead;
				this.log(`ws - write ${use}`);

				const m = new Map;
				m.set("id", this.#options.id);
				m.set(BASE + remain.part, remain.subarray(remain.position, remain.position + use))
				remain.position += use;
				if (remain.position === remain.byteLength) {
					this.#remain = undefined;
					this.#state = this.#nextState;
					this.log(`ws - advance to state ${this.#state}`);
				}
				state.messages.write(m);
				delete state.writable;
				return;
			}

			switch (this.#state) {
				case "connected":
					this.log(`ws - connected`);
					this.#options.onWritable?.call(this, state.messages.output - bufferOverhead);
					break;

				default:
					break;
			}
			return;
		}
	}
	#read(message) {
		this.log(`ws - onReadable`);
		switch (this.#state) {
				case "waitHandshake":
					if (0 === message.get(BASE + 2)) {
						this.log(`ws - onReadable - connected`);
						this.#state = "connected";
						if (state.writable)
							this.#options.onWritable?.call(this, state.messages.output - bufferOverhead);
					}
					else {
						this.log(`ws - onReadable - error`);
						this.#done(new Error("handshake failed " + message.get(BASE + 2)));
					}
					break;

				case "connected": {
					for (let part = 4; part < 8; part++) {
						const fragment = message.get(BASE + part);
						if (undefined !== fragment) {
							fragment.position = 0;
							fragment.part = part;
							this.#response.push(fragment);
							if (1 === this.#response.length)
								this.#schedule();
							return;
						}
					}

					try {
						message.forEach((value, key) => {
							this.log(`ws key ${key}`)
						});
					}
					catch (e) {
						trace.log(e);
					}

					const closed = message.get(BASE + 3);
					if (undefined === closed)
						this.#done("expected close part");
					else if (closed)
							this.#done(new Error("disconnected " + closed));
					else {
						this.#options.onControl?.call(this, WebSocketClient.close, closed);
						this.#done();
					}
					} break;

				default:
					trace("unexpected onReadable state " + this.#state + "\n");
					break;
		}			
	}
	#schedule(result) {
		this.log(`ws - #schedule`)
		if (this.#response.length && this.#options.onReadable) {
			const fragment = this.#response[0];
			const options = {
				more: 0 !== (fragment.part & 1),
				binary: fragment.part < 6
			};
			this.log(`ws - deliver onReadable more ${options.more} binary ${options.binary}`)
			this.#options.onReadable.call(this, fragment.byteLength - fragment.position, options);
		}
		return result;
	}
	#done(error) {
		if (this.#response) {
			this.log(`ws - #done with ${this.#response.length} unread fragments${(undefined === error) ? "" : ", error " + error}`);
			this.#response.length = 0;		//@@ drain this.#response before calling onError / onClose?
		}
		else
			this.log(`ws - #done with no unread fragments${(undefined === error) ? "" : ", error " + error}`);

		this.close();
		if (error) {
			this.log("WebSocket error: " + error + "");
			this.#state = "error";
			this.#options.onError?.call(this, error);
		}
		else {
			this.#state = "disconnected";
			this.#options.onClose?.call(this);
		}
	}
	log(msg) {
		// trace(msg, "\n");
	}

	static close = 8;
}

export default WebSocketClient;
