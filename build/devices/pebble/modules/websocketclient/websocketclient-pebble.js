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

/*
	To do: request headers
*/

import Messages from "pebble/message"
import Timer from "timer"

const bufferSize = 512;
const bufferOverhead = 64;		// a guess
const none = Object.freeze({});
let id = 0;

const BASE = 0;

class WebSocketClient {
	#options;
	#state = "connecting";
	#nextState;
	#messages;
	#remain;
	#response = [];

	constructor(options) {
		const {host, port, path, secure, protocol, onError, onClose, onReadable, onWritable, onControl} = options; 
		this.#options = {onError, onClose, onReadable, onWritable, onControl, id: ++id};

		this.#messages = new Messages({
			input: bufferSize,
			output: bufferSize,
			onReadable: () => this.#onReadable(),
			onWritable: () => this.#onWritable(),
		});
		this.#messages.writable = false;

		this.#remain = ArrayBuffer.fromString(`${secure ? "wss" : "ws"}:${protocol ?? ""}:${host}:${port ?? ""}:${path ?? "/"}:${bufferSize}`);

		this.#remain = new Uint8Array(this.#remain);
		this.#remain.part = 2;
		this.#remain.position = 0;
		this.#nextState = "waitHandshake";
	}
	close () {
		this.#messages?.close();
		this.#messages = this.#state = this.#remain = this.#response = undefined;
	}
	read(count) {
		const response = this.#response;
		if (!response.length)
			return;

		const fragment = response[0];
		let available = fragment.byteLength, result, bytes;
		if (undefined === count)
			count = available;
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
trace("ws - write\n");
		if (!this.#messages.writable)
			throw new Error("not writable");

		if (buffer.byteLength > (bufferSize - bufferOverhead))
			throw new Error("would overflow");

		if (undefined !== options.opcode) {
			if (WebSocketClient.close === options.opcode) {
				trace("ws - close opcode\n");
				const m = new Map;
				m.set(BASE + 1, this.#options.id);
				m.set(BASE + 8, buffer);
				this.#messages.write(m);

				this.#messages.writable = false;
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
		m.set(BASE + 1, this.#options.id);
		m.set(BASE + (binary ? 4 : 6) + (more ? 1 : 0), buffer);
		// 4 - binary - no more
		// 5 - binary - more
		// 6 - text - no more
		// 7 - text more
trace(`ws - write binary ${binary} more ${more} byteLength ${buffer.byteLength}`)
		this.#messages.write(m);

		this.#messages.writable = false;
		return 0;
	}
	#onWritable() {
trace(`ws - onWritable ${this.#state}`);
		this.#messages.writable = true;

		while (true) {
			const remain = this.#remain;
			if (remain) {
				let use = remain.byteLength - remain.position;
				if (use > (bufferSize - bufferOverhead))
					use = bufferSize - bufferOverhead;
trace(`ws - write ${use}`);

				const m = new Map;
				m.set(BASE + 1, this.#options.id);
				m.set(BASE + remain.part, remain.subarray(remain.position, remain.position + use))
				remain.position += use;
				if (remain.position === remain.byteLength) {
					this.#remain = undefined;
					this.#state = this.#nextState;
trace(`ws - advance to state ${this.#state}`);
				}
				this.#messages.write(m);
				this.#messages.writable = false;
				return;
			}

			switch (this.#state) {
				case "waitHandshake":
trace(`ws - waitHandshake`);
					break;			// just wait

				case "connected":
trace(`ws - connected`);
					this.#options.onWritable?.call(this, bufferSize - bufferOverhead);
					break;

				default:
					trace("unexpected onWritable state " + this.#state);
					break;
			}
			return;;
		}
	}
	#onReadable() {
trace(`ws - onReadable`);
		const message = this.#messages.read();
		const id = message.get(BASE + 1);
		if (id !== this.#options.id)
			this.#done("unexpected id " + id);

		switch (this.#state) {
				case "waitHandshake":
					if (0 === message.get(BASE + 2)) {
trace(`ws - onReadable - connected`);
						this.#state = "connected";
						Timer.set(() => {		// cannot safely write from read callback
							this.#options.onWritable?.call(this, bufferSize - bufferOverhead);
						});
					}
					else {
trace(`ws - onReadable - error`);
						this.#options.onError?.call(this, new Error("handshake failed " + message.get(BASE + 2)));
						this.#state = "error";
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
							trace(`key ${key}`)
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
					trace("unexpected onReadable state " + this.#state);
					break;
		}			
	}
	#schedule(result) {
trace(`ws - #schedule`)
		if (this.#response.length && this.#options.onReadable) {
			Timer.set(() => {
trace(`ws - #schedule fired`)
				if (!this.#response?.length)
					return;

				const fragment = this.#response[0];
				const options = {
					more: 0 !== (fragment.part & 1),
					binary: fragment.part < 6
				};
trace(`ws - deliver onReadable more ${options.more} binary ${options.binary}`)
				this.#options.onReadable.call(this, fragment.byteLength - fragment.position, options);
			});
		}
		return result;
	}
	#done(error) {
		trace(`ws - #done with ${this.#response.length} unread fragments${(undefined === error) ? "" : ", error " + error}\n`);
		this.#response.length = 0;		//@@ drain this.#response before calling onError / onClose?
		if (error) {
			trace("WebSocket error: " + error);
			this.#options.onError?.call(this, error);
		}
		else
			this.#options.onClose?.call(this);

		this.#state = "disconnected";
	}

	static close = 8;
}

export default WebSocketClient;
